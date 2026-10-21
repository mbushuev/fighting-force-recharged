#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <new>
#include <vector>
#include "../src/keyboard_read.h"
static uintptr_t g_base;
static wchar_t g_config_path[]=L"frame-runtime-test-missing.ini";
static void Log(const char*) {}
static FARPROC SystemFunction(const char*) {return nullptr;}
#include "../src/render_observer.h"
#include "../src/widescreen.h"
#include "../src/camera_runtime.h"
#include "../src/coop_runtime.h"
#include "../src/frame_runtime.h"
static int failures,draws,stops,resets,starts,reads,backgrounds,waits;
static int64_t counter;
static bool focus=true;
#define CHECK(x) do {if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
static bool Focus(){return focus;}
static int64_t Counter(){return counter;}
static void Delay(int64_t remaining){counter+=remaining;++waits;}
static void __cdecl Stop(){++stops;}
static void __cdecl Reset(){++resets;}
static void __cdecl Start(){++starts;}
static int __cdecl Elapsed(){++reads;return 12;}
static int32_t render_x[8],object_x[8],camera_x[8];
static int16_t joint[8];
static void __cdecl Background() {
    const int i=backgrounds++%8;
    render_x[i]=CameraField<int32_t>(0x86FEA8);object_x[i]=CameraField<int32_t>(0x870624);
    camera_x[i]=CameraField<int32_t>(0x8677B0);joint[i]=CameraField<int16_t>(0x8624F8);
    if(g_render_intermediate) {
        CHECK(!CameraMayCaptureGeometry());
        CHECK(CameraField<int32_t>(0x866220)==camera_x[i]);
        CHECK(CameraField<int16_t>(0x87888C)==CameraField<int16_t>(0x873CE0));
    }
}
static void __cdecl Draw() {
    ++draws;
    memcpy(reinterpret_cast<void*>(g_base+0x86FE80),reinterpret_cast<void*>(g_base+0x86A500),8*0xF4);
    memcpy(reinterpret_cast<void*>(g_base+0x8623E0),reinterpret_cast<void*>(g_base+0x868A40),8*0x238);
    CameraField<int32_t>(0x8677B0)=CameraField<int32_t>(0x866220);
    CameraField<int32_t>(0x8677B4)=CameraField<int32_t>(0x866224);
    CameraField<int32_t>(0x8677B8)=CameraField<int32_t>(0x866228);
    CameraField<int16_t>(0x873CE0)=CameraField<int16_t>(0x87888C);
    CameraField<int16_t>(0x87459C)=CameraField<int16_t>(0x87888E);
    reinterpret_cast<void(__cdecl*)()>(g_base+0x40C42)();
    CameraField<int16_t>(0x86A544)+=4;CameraField<DWORD>(0x9D200)-=8;
    ++CameraField<int>(0x870638); // Simulate a legacy render-side mutation.
    ++CameraField<DWORD>(0x6F5D8);
    ++CameraField<DWORD>(0x868A44);++CameraField<DWORD>(0x86ACB8);++CameraField<DWORD>(0x86BF38);
    reinterpret_cast<void(__cdecl*)()>(g_base+0x25978)();
    const int elapsed=reinterpret_cast<int(__cdecl*)()>(g_base+0x25985)();
    CHECK(elapsed==(g_smooth_frame_active?INT_MAX:12));
    reinterpret_cast<void(__cdecl*)()>(g_base+0x259CB)();
}
static void Jump(DWORD rva,void* target) {
    CameraField<BYTE>(rva)=0xE9;
    CameraField<LONG>(rva+1)=static_cast<LONG>(reinterpret_cast<uintptr_t>(target)-(g_base+rva+5));
}
int main() {
    g_base=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,0x900000,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    CHECK(g_base);if(!g_base)return 1;
    const DWORD sites[]={0x41157,0x40C42,0x25978,0x25985,0x259CB,0x259D0};
    const DWORD targets[]={0x40BC0,0x47A10,0x23060,0x23090,0x23030,0x23040};
    for(unsigned i=0;i<6;++i) {
        CameraField<BYTE>(sites[i])=0xE8;CameraField<LONG>(sites[i]+1)=targets[i]-sites[i]-5;
        CameraField<BYTE>(sites[i]+5)=0xC3;
    }
    g_presentation_installed=true;g_modern_focus=Focus;
    CameraField<BYTE>(0x25985)=0;InstallSmoothFrameHooks();
    CHECK(!g_smooth_frames_enabled && CameraField<LONG>(0x41158)==0x40BC0-0x4115C);
    CameraField<BYTE>(0x25985)=0xE8;InstallSmoothFrameHooks();CHECK(g_smooth_frames_enabled);
    Jump(0x40BC0,reinterpret_cast<void*>(Draw));Jump(0x47A10,reinterpret_cast<void*>(Background));
    Jump(0x23060,reinterpret_cast<void*>(Stop));Jump(0x23030,reinterpret_cast<void*>(Reset));
    Jump(0x23040,reinterpret_cast<void*>(Start));Jump(0x23090,reinterpret_cast<void*>(Elapsed));
    g_smooth_counter=Counter;g_smooth_delay=Delay;g_smooth_frequency.QuadPart=6000;
    CameraField<int>(0x8745A0)=1;CameraField<int16_t>(0x86A534)=2;
    CameraField<int32_t>(0x870620)=500;CameraField<DWORD>(0x9D200)=128;
    SmoothGameplayFrame();CHECK(draws==2 && waits==2 && counter==200);
    CHECK(CameraField<int16_t>(0x86A544)==4 && CameraField<DWORD>(0x9D200)==120);
    CHECK(CameraField<int>(0x870638)==1 && CameraField<uint32_t>(0x85B560)==0);
    CHECK(CameraField<DWORD>(0x6F5D8)==1);
    CHECK(CameraField<DWORD>(0x868A44)==1 && CameraField<DWORD>(0x86ACB8)==1 && CameraField<DWORD>(0x86BF38)==1);
    CHECK(stops==0 && reads==0 && resets==1 && starts==1);
    ++CameraField<uint32_t>(0x85B560);
    CameraField<int32_t>(0x86A528)=100*65536;CameraField<int32_t>(0x870624)=80*65536;
    CameraField<int32_t>(0x866220)=20*65536;CameraField<int16_t>(0x868B58)=100;
    SmoothGameplayFrame();CHECK(draws==4 && counter==400);
    CHECK(render_x[2]==50*65536 && render_x[3]==100*65536 && joint[2]==50 && joint[3]==100);
    CHECK(object_x[2]==40*65536 && object_x[3]==80*65536);
    CHECK(camera_x[2]==10*65536 && camera_x[3]==20*65536);
    CHECK(CameraField<int32_t>(0x866220)==20*65536);
    CHECK(CameraField<int32_t>(0x86A528)==100*65536 && CameraField<int32_t>(0x870624)==80*65536);
    CHECK(CameraField<int16_t>(0x868B58)==100 && CameraField<int>(0x870638)==2);
    CHECK(CameraField<DWORD>(0x6F5D8)==2);
    CHECK(CameraField<DWORD>(0x868A44)==2 && CameraField<DWORD>(0x86ACB8)==2 && CameraField<DWORD>(0x86BF38)==2);
    CHECK(CameraField<int16_t>(0x86A544)==8 && CameraField<DWORD>(0x9D200)==112);
    CHECK(CameraField<uint32_t>(0x85B560)==1);
    // A menu/demo/focus transition uses exactly one untouched native draw.
    CameraField<int>(0x85A6B8)=26;SmoothGameplayFrame();CHECK(draws==5 && !g_smooth_frame_history);
    CHECK(stops==1 && reads==1 && resets==3 && starts==3);
    CameraField<int>(0x85A6B8)=0;CameraField<int>(0x85A668)=1;SmoothGameplayFrame();CHECK(draws==6);
    CameraField<int>(0x85A668)=0;focus=false;SmoothGameplayFrame();CHECK(draws==7);
    focus=true;CameraField<int>(0x861F68)=1;SmoothGameplayFrame();CHECK(draws==8);
    CameraField<int>(0x861F68)=0;
    // Split subviews do not create an extra pacing boundary.
    g_smooth_frame_active=true;g_coop_render_pass=0;const int before=waits;
    CHECK(SmoothClockElapsed()==INT_MAX && waits==before);
    g_coop_render_pass=1;CHECK(SmoothClockElapsed()==INT_MAX && waits==before+1);
    g_coop_render_pass=-1;g_smooth_frame_active=false;
    // The real cooperative dispatcher draws four views but only two completed
    // images, preserves native ticks, and advances HUD/fade counters once.
    Jump(0x40BC0,reinterpret_cast<void*>(CooperativeFrame));g_coop_frame_trampoline=reinterpret_cast<void*>(Draw);
    g_coop_enabled=g_coop_ready=g_camera_cooperative=g_camera_active=true;g_coop_stage=0;
    g_coop_split.amount=1;g_coop_split.separate=true;CameraField<int>(0x8745A0)=2;
    CameraField<int16_t>(0x86A542)=CameraField<int16_t>(0x86A636)=100;
    CameraField<float>(0xAD214)=400;CameraField<float>(0x9D178)=4;CameraField<float>(0x9D170)=1280;
    g_coop_shared_pose={0,0,0,0,0};g_coop_players[0].pose={100,0,0,0,0};g_coop_players[1].pose={-100,0,0,0,0};
    const int split_draws=draws,split_waits=waits;
    const auto health=CameraField<int16_t>(0x86A544);const auto fade=CameraField<DWORD>(0x9D200);
    SmoothGameplayFrame();
    CHECK(draws==split_draws+4 && waits==split_waits+2);
    CHECK(CameraField<int16_t>(0x86A544)==health+4 && CameraField<DWORD>(0x9D200)==fade-8);
    CHECK(CameraField<uint32_t>(0x85B560)==1 && g_coop_render_pass==-1 && !g_render_intermediate);
    printf("60 Hz native rendering transactions and hook validation: %s\n",failures?"FAILED":"passed");
    return failures?1:0;
}
