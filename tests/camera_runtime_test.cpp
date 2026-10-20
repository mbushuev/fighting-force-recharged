#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include "../src/keyboard_read.h"
static uintptr_t g_base;
static wchar_t g_config_path[] = L"camera-runtime-test-missing.ini";
static void Log(const char*) {}
static FARPROC SystemFunction(const char* name) {
    static HMODULE runtime = LoadLibraryExW(L"dinput.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    return runtime ? GetProcAddress(runtime, name) : nullptr;
}
#include "../src/camera_runtime.h"
static int failures, stage_calls, input_calls, begin_value;
#define CHECK(x) do { if (!(x)) { printf("FAIL line %d: %s\n", __LINE__, #x); ++failures; } } while (0)
static void __cdecl NativeBegin(int value) { begin_value = value; }
static void __cdecl NativeInput() { ++input_calls; }
static float test_matrix[]={1,0,0,0, 0,1,0,0, 0,0,1,0};
static const float* __cdecl NativeMatrix() { return test_matrix; }
static void __cdecl NativeBasis() {}
static int16_t model_vertices[]={-100,-100,400,0, 100,-100,400,0, 100,100,400,0, -100,100,400,0};
static int16_t model_faces[]={0,1,2,3,0,0};
static CameraRetailModel test_model={model_vertices,4,model_faces,1};
static int projector_result;
static int projected_face_flags;
static const CameraRetailModel* projected_model;
static int __cdecl NativeProjector(const CameraRetailModel* model,int,int) {
    projected_face_flags=model->faces[4];projected_model=model;return 1;
}
static bool controller_focus=true;
static XINPUT_STATE controller_state;
static bool ControllerFocus() {return controller_focus;}
static DWORD WINAPI ControllerRead(DWORD index,XINPUT_STATE* output) {
    if(index!=1)return ERROR_DEVICE_NOT_CONNECTED;*output=controller_state;return ERROR_SUCCESS;
}
static void __cdecl NativeEnvironmentModels() {
    projector_result=reinterpret_cast<int(__cdecl*)(const CameraRetailModel*,int,int)>(g_base+0x26DE0)(&test_model,0,0);
}
static bool object_visible=true;
static int object_draws;
static void __cdecl NativeObjectModel(void*) {++object_draws;if(object_visible)NativeEnvironmentModels();}
static void __cdecl NativeSolidObjects() {
    if(CameraField<int>(0x870620)!=-1)
        reinterpret_cast<void(__cdecl*)(void*)>(g_base+0x3C14F)(reinterpret_cast<void*>(g_base+0x870620));
}
static void __cdecl NativeStage() {
    ++stage_calls;
    CameraField<int32_t>(0x866220) = 123; CameraField<int16_t>(0x87888C) = 456;
    CameraField<int32_t>(0x8744A8) = 789;
}
static void Branch(DWORD rva, BYTE opcode, uintptr_t target) {
    BYTE* code = reinterpret_cast<BYTE*>(g_base + rva); code[0] = opcode;
    DWORD relative = static_cast<DWORD>(target - (g_base + rva + 5)); memcpy(code + 1, &relative, 4);
}
int main() {
    g_base = reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr, 0x900000, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE));
    CHECK(g_base != 0); if (!g_base) return 1;
    Branch(0x22BC0, 0xE9, reinterpret_cast<uintptr_t>(NativeBegin));
    Branch(0x484F0, 0xE9, reinterpret_cast<uintptr_t>(NativeInput));
    Branch(0x1980, 0xE9, reinterpret_cast<uintptr_t>(NativeMatrix));
    Branch(0x2330, 0xE9, reinterpret_cast<uintptr_t>(NativeBasis));
    Branch(0x3C100, 0xE9, reinterpret_cast<uintptr_t>(NativeSolidObjects));
    const BYTE object_dispatch[]={0x8B,0x04,0x85,0x00,0x89,0xC7,0x00,0x56,0x85,0xC0,0x74,0x04,
        0xFF,0xD0,0xEB,0x05,0xE8,0x07,0xD7,0x00,0x00,0x83,0xC4,0x04};
    memcpy(reinterpret_cast<void*>(g_base+0x3C154),object_dispatch,sizeof(object_dispatch));
    const BYTE object_thunk[]={0x56,0x8B,0x74,0x24,0x08};
    memcpy(reinterpret_cast<void*>(g_base+0x3C14F),object_thunk,sizeof(object_thunk));
    const BYTE object_return[]={0x5E,0xC3};memcpy(reinterpret_cast<void*>(g_base+0x3C16C),object_return,2);
    CameraField<void(__cdecl*)(void*)>(0x878900)=NativeObjectModel;
    for(unsigned actor=0;actor<8;++actor)for(unsigned hand=0;hand<2;++hand)
        CameraField<int8_t>(0x86A57D+actor*0xF4+hand)=-1;
    const BYTE model_body[]={0x83,0xEC,0x3C,0x53,0x55,0x56,0x5E,0x5D,0x5B,0x83,0xC4,0x3C,0xB8,1,0,0,0,0xC3};
    memcpy(reinterpret_cast<void*>(g_base+0x26DE0),model_body,sizeof(model_body));
    Branch(0x26DEC,0xE9,reinterpret_cast<uintptr_t>(NativeProjector));
    const BYTE world_prologue[]={0x53,0x56,0x57};memcpy(reinterpret_cast<void*>(g_base+0x43990),world_prologue,3);
    Branch(0x43993,0xE8,g_base+0x2330);
    Branch(0x43998,0xE8,reinterpret_cast<uintptr_t>(NativeEnvironmentModels));
    const BYTE world_return[]={0x5F,0x5E,0x5B,0xC3};memcpy(reinterpret_cast<void*>(g_base+0x4399D),world_return,4);
    const DWORD world_calls[]={0x2EBC9,0x2F0AA,0x40C47,0x416E7,0x41B44,0x4235A,0x4AB2D};
    for(DWORD rva:world_calls){Branch(rva,0xE8,g_base+0x43990);*reinterpret_cast<BYTE*>(g_base+rva+5)=0xC3;}
    const DWORD object_calls[]={0x2EBCE,0x2F0AF,0x40C51,0x416F1,0x41B4E,0x4235F,0x4AB32};
    for(DWORD rva:object_calls){Branch(rva,0xE8,g_base+0x3C100);*reinterpret_cast<BYTE*>(g_base+rva+5)=0xC3;}
    // At several native sites the two calls are consecutive, so execute the
    // standalone test boundary whose next byte is not another patched call.
    *reinterpret_cast<BYTE*>(g_base+0x40C4C)=0xC3;
    Branch(0x40D6C, 0xE8, g_base + 0x22BC0); Branch(0x41177, 0xE8, g_base + 0x484F0);
    const BYTE dispatch[] = {0xFF, 0x14, 0x95, 0xE0, 0x36, 0xC7, 0};
    BYTE* site = reinterpret_cast<BYTE*>(g_base + 0x4120F);
    memcpy(site, dispatch, 7); site[7] = 0xC3;
    // A failed signature must leave every other site untouched.
    site[6] = 1; InstallCameraHooks(); CHECK(*reinterpret_cast<BYTE*>(g_base + 0x40D6C) == 0xE8);
    LONG relative; memcpy(&relative, reinterpret_cast<void*>(g_base + 0x40D6D), 4);
    CHECK(g_base + 0x40D71 + relative == g_base + 0x22BC0);
    site[6] = 0; InstallCameraHooks(); CHECK(site[0] == 0xE8 && site[5] == 0x90 && site[6] == 0x90);
    CHECK(g_camera_collision_enabled);
    // Validate the real system mouse format/cooperative-level combination on
    // an offscreen test window; no input injection or cursor capture is used.
    HWND window = CreateWindowExW(0, L"STATIC", L"Camera input test", WS_POPUP, -10000, -10000, 16, 16, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    CHECK(window != nullptr); CameraField<HWND>(0x1EC4E4) = window;
    CHECK(PrepareCameraMouse()); ReleaseCameraMouse(); DestroyWindow(window); CameraField<HWND>(0x1EC4E4) = nullptr;
    CameraField<void(__cdecl*)()>(0x8736E0) = NativeStage;
    CameraField<int>(0x8745A0) = 1; CameraField<int16_t>(0x86A542) = 100;
    CameraField<int32_t>(0x86A528) = ff_camera::Fixed(100);
    CameraField<int32_t>(0x86A530) = ff_camera::Fixed(200);
    GameCameraBegin(17); CHECK(begin_value == 17 && !g_camera_active);
    GameCameraInput(); CHECK(input_calls == 1 && g_camera_active);
    const int16_t yaw = CameraField<int16_t>(0x87888C);
    const int32_t x = CameraField<int32_t>(0x866220);
    // Execute the actual patched seven-byte dispatch boundary. Native stage
    // side effects must run, while the orbit survives its anchored pose writes.
    reinterpret_cast<void(__cdecl*)()>(site)();
    CHECK(stage_calls == 1 && CameraField<int16_t>(0x87888C) == yaw);
    CHECK(CameraField<int32_t>(0x866220) == x && CameraField<int32_t>(0x8744A8) == 0);
    CHECK(CameraField<int16_t>(0x86D318) == yaw);
    CHECK(CameraField<int32_t>(0x8677B0) == 0); // native render publication owns this copy
    CameraField<int>(0x85A668) = 1; GameCameraInput();
    CHECK(!g_camera_active && input_calls == 2);
    reinterpret_cast<void(__cdecl*)()>(site)();
    CHECK(stage_calls == 2 && CameraField<int32_t>(0x866220) == 123);
    CameraField<int>(0x85A668) = 0; CameraField<int>(0x8745A0) = 2; GameCameraInput(); CHECK(!g_camera_active);
    CameraField<int>(0x8745A0) = 1; g_free_orbit = false;
    GameCameraInput(); CHECK(!g_camera_active && g_mouse_combat);
    reinterpret_cast<void(__cdecl*)()>(site)();
    CHECK(stage_calls == 3 && CameraField<int32_t>(0x866220) == 123);
    g_free_orbit=true;g_camera_active=true;
    // Execute the patched environment and model entries, including the copied
    // native projector prologue. The resulting mesh must contract the boom.
    reinterpret_cast<void(__cdecl*)()>(g_base+0x40C47)();
    CHECK(projector_result==1 && g_camera_triangles.empty());
    CHECK(projected_face_flags==1 && model_faces[4]==0);
    CHECK(CameraObstacleDistance({0,0,0},{0,0,720})==720);
    reinterpret_cast<void(__cdecl*)()>(g_base+0x40C51)();CHECK(g_camera_triangles.size()==4);
    CHECK(projected_face_flags==0 && projected_model==&test_model); // Props retain native sidedness.
    CHECK(object_draws==1);
    CHECK(fabs(CameraObstacleDistance({0,0,0},{0,0,720})-344)<0.0001);
    CameraField<BYTE>(0x451DBC)=1;
    reinterpret_cast<void(__cdecl*)()>(g_base+0x40C47)();
    CHECK(projected_face_flags==0 && model_faces[4]==0); // Cutouts retain native flags.
    // A reader between the environment and object passes still sees the
    // previous complete frame, including its vehicles. Never release the boom.
    CHECK(g_camera_triangles.size()==4);
    CHECK(fabs(CameraObstacleDistance({0,0,0},{0,0,720})-344)<0.0001);
    reinterpret_cast<void(__cdecl*)()>(g_base+0x40C51)();
    CHECK(g_camera_triangles.empty() && CameraObstacleDistance({0,0,0},{0,0,720})==720);
    // A hidden model still belongs to a live solid object. Keep its previous
    // geometry, then invalidate it on transform changes, removal or carrying.
    CameraField<BYTE>(0x451DBC)=0;
    reinterpret_cast<void(__cdecl*)()>(g_base+0x40C47)();
    reinterpret_cast<void(__cdecl*)()>(g_base+0x40C51)();CHECK(g_camera_triangles.size()==4);
    object_visible=false;
    reinterpret_cast<void(__cdecl*)()>(g_base+0x40C47)();
    reinterpret_cast<void(__cdecl*)()>(g_base+0x40C51)();CHECK(g_camera_triangles.size()==4);
    CameraField<int>(0x870624)=1;
    reinterpret_cast<void(__cdecl*)()>(g_base+0x40C47)();
    reinterpret_cast<void(__cdecl*)()>(g_base+0x40C51)();CHECK(g_camera_triangles.size()==2);
    object_visible=true;
    reinterpret_cast<void(__cdecl*)()>(g_base+0x40C47)();
    reinterpret_cast<void(__cdecl*)()>(g_base+0x40C51)();CHECK(g_camera_triangles.size()==4);
    object_visible=false;CameraField<int8_t>(0x86A57D)=0;
    reinterpret_cast<void(__cdecl*)()>(g_base+0x40C47)();
    reinterpret_cast<void(__cdecl*)()>(g_base+0x40C51)();CHECK(g_camera_triangles.size()==2);
    CameraField<int8_t>(0x86A57D)=-1;
    CameraField<int32_t>(0x86A528)=CameraField<int32_t>(0x86A530)=0;
    g_camera_triangles={{{-1000,-1000,-400},{1000,-1000,-400},{1000,1000,-400}},
                        {{-1000,-1000,-400},{1000,1000,-400},{-1000,1000,-400}}};
    g_camera_orbit.Begin(0,g_camera_settings);g_camera_boom=720;
    PublishOrbitPose(0.033);double contracted=g_camera_boom;
    CHECK(contracted>300 && contracted<400);
    g_camera_triangles.clear();PublishOrbitPose();CHECK(g_camera_boom==contracted);
    PublishOrbitPose(0.033);CHECK(g_camera_boom==contracted);
    for(int i=0;i<12;++i)PublishOrbitPose(0.033);
    CHECK(g_camera_boom>contracted && g_camera_boom<720);
    ClearCameraGeometry();CHECK(g_camera_geometry_stage==-1);
    CameraField<int>(0x85A668)=1;
    reinterpret_cast<void(__cdecl*)()>(g_base+0x40C47)();
    CHECK(projected_model==&test_model && projected_face_flags==0); // Attract scenes stay native.
    CameraField<int>(0x85A668)=0;
    // Movement and orbit consume the same selected XInput controller. Exercise
    // actual input publication, timed right-stick look, trigger zoom and R3.
    g_modern_keyboard=false;g_modern_gamepad=true;g_modern_focus=ControllerFocus;
    g_modern_xinput=ControllerRead;g_modern_pad_session={};ReadModernPad();
    CHECK(g_modern_pad_session.selected==1);
    g_camera_orbit.Begin(0,g_camera_settings);g_camera_active=true;
    controller_state.Gamepad.sThumbLX=32767;controller_state.Gamepad.sThumbRX=32767;
    controller_state.Gamepad.bRightTrigger=255;g_modern_pad_session.last_poll=0;
    QueryPerformanceCounter(&g_camera_time);g_camera_time.QuadPart-=g_camera_frequency.QuadPart/20;
    GameCameraInput();
    CHECK((CameraField<uint32_t>(0x5523E0)&15)==ff_input::Right);
    CHECK(g_camera_orbit.yaw< -0.05 && g_camera_orbit.yaw> -0.3);
    CHECK(g_camera_orbit.distance<695 && g_camera_orbit.distance>665);
    controller_state.Gamepad={};controller_state.Gamepad.wButtons=XINPUT_GAMEPAD_RIGHT_THUMB;
    CameraField<int16_t>(0x86A538)=256;g_modern_pad_session.last_poll=0;GameCameraInput();
    CHECK(CameraField<int16_t>(0x87888C)==-24576);
    CameraField<int16_t>(0x86A538)=512;GameCameraInput();
    CHECK(CameraField<int16_t>(0x87888C)==-24576); // Holding R3 does not repeatedly snap.
    controller_state.Gamepad.wButtons=0;g_modern_pad_session.last_poll=0;GameCameraInput();
    controller_state.Gamepad.wButtons=XINPUT_GAMEPAD_RIGHT_THUMB;
    g_modern_pad_session.last_poll=0;GameCameraInput();CHECK(CameraField<int16_t>(0x87888C)==-16384);
    controller_focus=false;GameCameraInput();CHECK((CameraField<uint32_t>(0x5523E0)&0xFFFF)==0);
    VirtualFree(g_camera_model_trampoline,0,MEM_RELEASE);
    VirtualFree(reinterpret_cast<void*>(g_base), 0, MEM_RELEASE);
    printf("Camera runtime ownership and dispatch: %s\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
