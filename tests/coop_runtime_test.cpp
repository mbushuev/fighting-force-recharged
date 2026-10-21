#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <new>
#include <vector>
#include "../src/keyboard_read.h"
static uintptr_t g_base;
static wchar_t g_config_path[]=L"cooperative-runtime-test-missing.ini";
static void Log(const char*) {}
static FARPROC SystemFunction(const char*) {return nullptr;}
#include "../src/render_observer.h"
#include "../src/widescreen.h"
#include "../src/camera_runtime.h"
#include "../src/coop_runtime.h"
static int failures,draws,inputs,stages,last_owner=-2;
static bool last_eye;
static int clock_stops,clock_resets,clock_starts,clock_reads;
static void __cdecl StopClock() {++clock_stops;}
static void __cdecl ResetClock() {++clock_resets;}
static void __cdecl StartClock() {++clock_starts;}
static int __cdecl ReadClock() {++clock_reads;return 40;}
#define CHECK(x) do {if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
static bool Focus() {return true;}
static XINPUT_STATE states[2];
static DWORD WINAPI Pads(DWORD slot,XINPUT_STATE* output) {
    if(slot>1)return ERROR_DEVICE_NOT_CONNECTED;*output=states[slot];return ERROR_SUCCESS;
}
static void Branch(DWORD rva,void* target) {
    CameraField<BYTE>(rva)=0xE9;const LONG relative=static_cast<LONG>(reinterpret_cast<uintptr_t>(target)-(g_base+rva+5));
    memcpy(reinterpret_cast<void*>(g_base+rva+1),&relative,4);
}
static void __cdecl Input() {++inputs;}
static void __cdecl Stage() {++stages;CameraField<int32_t>(0x866220)=1;}
static ff_camera::Pose observed[2];
static float centers[2];
static void __cdecl Draw() {
    ++draws;last_owner=g_camera_owner;last_eye=FirstPersonActive();
    if(g_coop_render_pass>=0) {
        const int player=g_coop_render_pass;
        CHECK(g_camera_owner==player);
        CHECK(CooperativeSkipPresent()==(player==0));
        CHECK(CameraField<float>(0xAD214)==800);
        observed[player]={CameraField<int32_t>(0x866220),CameraField<int32_t>(0x866224),CameraField<int32_t>(0x866228),
            CameraField<int16_t>(0x87888C),CameraField<int16_t>(0x87888E)};
        centers[player]=CameraField<float>(0x20C50C);
    }
    CameraField<int16_t>(0x86A544)+=4;CameraField<DWORD>(0x9D200)-=8;
}
static int Heading(void* actor) {
    auto code=reinterpret_cast<void*>(g_base+0x4F197);int result;
    __asm {
        push esi
        mov esi,actor
        call code
        mov result,eax
        pop esi
    }
    return result;
}
static const float eye_matrix[]={1,0,0,0,0,1,0,-160,0,0,1,0};
static const float* __cdecl EyeMatrix(){return eye_matrix;}
static void __cdecl EyeBasis(){}
static void InstallEyeFixture() {
    static struct Model {
        FirstPersonModel model;unsigned counts[2];int32_t commands[29];int16_t vertices[48];int16_t faces[12];
    } model={};
    const int32_t commands[]={1,3,0,0,0,0,0,0,0,0,0,0,0,1,3,1,0,-160*65536,0,0,0,0,1,0,0,0,2,2};
    memcpy(model.commands,commands,sizeof(commands));model.counts[0]=8;model.counts[1]=4;
    model.model={model.vertices,12,model.faces,2,nullptr,2,model.commands,2};
    for(int i=0;i<4;++i)CameraField<const FirstPersonModel*>(0x85A6F8+i*4)=&model.model;
    Branch(0x1000,reinterpret_cast<void*>(EyeMatrix));
    for(DWORD rva:{0x10F0,0x1170,0x11F0,0x12F0,0x13F0,0x14F0,0x15F0})Branch(rva,reinterpret_cast<void*>(EyeBasis));
}
int main() {
    g_base=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,0x900000,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    CHECK(g_base!=0);if(!g_base)return 1;
    g_free_orbit=g_first_person_available=g_presentation_installed=true;
    g_camera_settings.height=160;g_modern_focus=Focus;g_modern_xinput=Pads;
    QueryPerformanceFrequency(&g_camera_frequency);
    const BYTE entry[]={0x8B,0x0D,0x24,0x62,0xC6,0x00},heading[]={0x8B,0x0D,0x8C,0xA6,0xC5,0x00};
    const DWORD clock_calls[][2]={{0x25978,0x23060},{0x25985,0x23090},{0x259CB,0x23030},{0x259D0,0x23040}};
    for(const auto& call:clock_calls) {
        CameraField<BYTE>(call[0])=0xE8;CameraField<LONG>(call[0]+1)=call[1]-call[0]-5;
        CameraField<BYTE>(call[0]+5)=0xC3;
    }
    memcpy(reinterpret_cast<void*>(g_base+0x40BC0),entry,6);
    memcpy(reinterpret_cast<void*>(g_base+0x4F197),heading,6);
    CameraField<BYTE>(0x25A5A)=0xE8;CameraField<LONG>(0x25A5B)=0x23B00-0x25A5F;
    CameraField<BYTE>(0x4F197)=0;InstallCooperativeHooks();CHECK(!g_coop_enabled && CameraField<BYTE>(0x40BC0)==0x8B);
    memcpy(reinterpret_cast<void*>(g_base+0x4F197),heading,6);
    CameraField<BYTE>(0x259CB)=0;InstallCooperativeHooks();CHECK(!g_coop_enabled && CameraField<BYTE>(0x40BC0)==0x8B);
    CameraField<BYTE>(0x259CB)=0xE8;InstallCooperativeHooks();
    CHECK(g_coop_enabled && g_modern_coop_input && CameraField<BYTE>(0x40BC0)==0xE9);
    g_modern_coop_scheme=2;
    Branch(0x23060,reinterpret_cast<void*>(StopClock));Branch(0x23030,reinterpret_cast<void*>(ResetClock));
    Branch(0x23040,reinterpret_cast<void*>(StartClock));Branch(0x23090,reinterpret_cast<void*>(ReadClock));
    for(int pass:{-1,0,1}) {
        g_coop_render_pass=pass;
        reinterpret_cast<void(__cdecl*)()>(g_base+0x25978)();
        const int elapsed=reinterpret_cast<int(__cdecl*)()>(g_base+0x25985)();
        CHECK(elapsed==(pass==0?INT_MAX:40));
        reinterpret_cast<void(__cdecl*)()>(g_base+0x259CB)();
    }
    CHECK(clock_stops==2 && clock_reads==2 && clock_resets==2 && clock_starts==2);
    g_coop_render_pass=-1;
    const BYTE heading_return[]={0x8B,0xC1,0xC3};memcpy(reinterpret_cast<void*>(g_base+0x4F19D),heading_return,3);
    g_coop_frame_trampoline=reinterpret_cast<void*>(Draw);
    Branch(0x484F0,reinterpret_cast<void*>(Input));
    CameraField<void(__cdecl*)()>(0x8736E0)=Stage;
    CameraField<int>(0x8745A0)=2;CameraField<int16_t>(0x86A542)=CameraField<int16_t>(0x86A636)=100;
    CameraField<float>(0x9D170)=1280;CameraField<float>(0x9D174)=800;
    CameraField<float>(0xAD214)=CameraField<float>(0xAD234)=400;
    CameraField<float>(0x9D178)=800.0f/240;CameraField<float>(0x20C50C)=640;
    CameraField<int32_t>(0x86A528)=-100*65536;CameraField<int32_t>(0x86A61C)=100*65536;
    GameCameraInput();CHECK(inputs==1 && g_camera_cooperative && g_camera_active && g_coop_ready);
    CHECK(g_modern_pad_session.selected==0 && g_modern_second_session.selected==1);
    CHECK(g_coop_split.amount==0);
    CooperativeFrame();CHECK(draws==1);
    // Gameplay actions and angular heading belong to each player's own camera.
    states[0].Gamepad.wButtons=XINPUT_GAMEPAD_X;states[1].Gamepad.wButtons=XINPUT_GAMEPAD_Y;
    g_modern_pad_session.last_poll=g_modern_second_session.last_poll=0;
    GameCameraInput();CHECK(inputs==2);
    CHECK((CameraField<uint32_t>(0x5523E0)&0x60)==ff_input::Punch);
    CHECK((CameraField<uint32_t>(0x5523E4)&0x60)==ff_input::Kick);
    g_coop_players[0].orbit.yaw=0.5;g_coop_players[1].orbit.yaw=-0.5;
    CHECK(Heading(reinterpret_cast<void*>(g_base+0x86A500))==ff_camera::NativeAngle(0.5));
    CHECK(Heading(reinterpret_cast<void*>(g_base+0x86A5F4))==ff_camera::NativeAngle(-0.5));
    CameraField<int>(0x85A68C)=765;CHECK(Heading(reinterpret_cast<void*>(g_base+0x86A6E8))==765);
    // Moving apart produces two views without advancing the native game tick.
    CameraField<int32_t>(0x86A528)=-1500*65536;CameraField<int32_t>(0x86A61C)=1500*65536;
    CooperativeUpdatePoses(0.1,true);CHECK(g_coop_split.separate && g_coop_split.amount>0);
    GameCameraStage();CHECK(stages==1 && CameraField<int32_t>(0x866220)!=1);
    for(int i=0;i<10;++i)CooperativeUpdatePoses(0.1,true);
    CHECK(g_coop_split.amount==1);
    const auto shared=g_coop_shared_pose;CameraField<unsigned>(0x85B560)=123;
    CameraField<int16_t>(0x86A544)=0;CameraField<DWORD>(0x9D200)=128;
    CooperativeFrame();CHECK(draws==3 && CameraField<unsigned>(0x85B560)==123);
    CHECK(observed[0].x!=observed[1].x && centers[0]==640 && centers[1]==640);
    CHECK(CameraField<int16_t>(0x86A544)==4 && CameraField<DWORD>(0x9D200)==120);
    CHECK(g_camera_owner==-1 && g_coop_render_pass==-1 && CameraField<float>(0xAD214)==400);
    CHECK(CameraField<float>(0x20C50C)==640 && CameraField<int32_t>(0x866220)==shared.x);
    RetailScreenPolygon polygon={},mapped;polygon.kind=5;polygon.flags=1;polygon.count=4;
    g_coop_render_pass=0;CHECK(CooperativeScreenPolygon(&polygon,&polygon,mapped));
    const float first=mapped.vertices[0].sx;
    g_coop_render_pass=1;CHECK(CooperativeScreenPolygon(&polygon,&polygon,mapped));
    CHECK(mapped.vertices[0].sx==first-1280);
    polygon.kind=0;CHECK(!CooperativeScreenPolygon(&polygon,&polygon,mapped));g_coop_render_pass=-1;
    // Original shared camera keeps modern routing but uses one native view.
    g_modern_coop_camera=false;GameCameraInput();
    CHECK(!g_camera_cooperative && !g_camera_active && !g_coop_ready);
    CHECK((CameraField<uint32_t>(0x5523E0)&0x60)==ff_input::Punch);
    CHECK((CameraField<uint32_t>(0x5523E4)&0x60)==ff_input::Kick);
    const int stage_before=stages,draw_before=draws;
    GameCameraStage();CHECK(stages==stage_before+1 && CameraField<int32_t>(0x866220)==1);
    CooperativeFrame();CHECK(draws==draw_before+1);
    g_modern_coop_camera=true;GameCameraInput();CHECK(g_camera_cooperative && g_coop_ready);
    InstallEyeFixture();
    // Losing either actor must retain the survivor's look, eye toggle and full view.
    for(int survivor=0;survivor<2;++survivor) {
        CameraField<int16_t>(0x86A542)=survivor==0?100:0;
        CameraField<int16_t>(0x86A636)=survivor==1?100:0;
        states[0]={};states[1]={};g_modern_pad_session.last_poll=g_modern_second_session.last_poll=0;
        GameCameraInput();CHECK(g_camera_active && g_camera_cooperative && CameraMayOwnPose());
        CHECK(CooperativeSoloPlayer()==survivor && g_coop_split.amount==0);
        const double yaw=g_coop_players[survivor].orbit.yaw;
        states[survivor].Gamepad.sThumbRX=30000;
        g_modern_pad_session.last_poll=g_modern_second_session.last_poll=0;
        g_camera_time.QuadPart-=g_camera_frequency.QuadPart/30;
        GameCameraInput();CHECK(g_coop_players[survivor].orbit.yaw!=yaw);
        states[survivor].Gamepad.wButtons=XINPUT_GAMEPAD_LEFT_THUMB;
        g_modern_pad_session.last_poll=g_modern_second_session.last_poll=0;
        GameCameraInput();CHECK(g_coop_players[survivor].eye.selected);
        const int before_solo=draws;CooperativeFrame();
        CHECK(draws==before_solo+1 && last_owner==survivor && last_eye && g_camera_owner==-1);
        CHECK(g_coop_shared_pose.x==g_coop_players[survivor].pose.x);
        CameraField<int16_t>(0x86A542)=CameraField<int16_t>(0x86A636)=100;
        states[0]={};states[1]={};g_modern_pad_session.last_poll=g_modern_second_session.last_poll=0;
        GameCameraInput();CHECK(CooperativeSoloPlayer()==-1 && g_camera_active);
        g_coop_players[survivor].eye.selected=false;
    }
    CameraField<int16_t>(0x86A542)=CameraField<int16_t>(0x86A636)=0;
    GameCameraInput();CHECK(!g_camera_active && !CameraMayOwnPose());
    CameraField<int16_t>(0x86A542)=100;
    CameraField<int>(0x8745A0)=1;GameCameraInput();CHECK(!g_camera_cooperative && !g_coop_ready);
    g_camera_active=false; // Single-player path retains ownership on its own terms.
    const int before=draws;CooperativeFrame();CHECK(draws==before+1);
    printf("Cooperative native camera, input and render scope: %s\n",failures?"FAILED":"passed");
    return failures?1:0;
}
