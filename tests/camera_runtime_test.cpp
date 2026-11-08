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
static int native_origins;
static int __cdecl NativeOrigin(){++native_origins;return 0;}
static void __cdecl NativeInput() { ++input_calls; }
static float test_matrix[]={1,0,0,0, 0,1,0,0, 0,0,1,0};
static const float* __cdecl NativeMatrix() { return test_matrix; }
static void __cdecl NativeBasis() {}
static int16_t model_vertices[]={-100,-100,400,0, 100,-100,400,0, 100,100,400,0, -100,100,400,0};
static int16_t model_faces[]={0,1,2,3,0,0};
static CameraRetailModel test_model={model_vertices,4,model_faces,1};
static int projector_result;
static int projected_calls;
static int projected_face_flags,projected_depth;
static int projected_face_texture;
static unsigned projected_face_count;
static const CameraRetailModel* projected_model;
static const int16_t* projected_vertices;
static int __cdecl NativeProjector(const CameraRetailModel* model,int,int depth) {
    ++projected_calls;projected_face_flags=model->faces[4];projected_face_texture=model->faces[5];
    projected_face_count=model->face_count;
    projected_model=model;projected_vertices=model->vertices;projected_depth=depth;return 1;
}
static int clipped_scene_calls;
static void __cdecl SceneFragment(FirstPersonPropPolygon* polygon,const FirstPersonPropMaterial*,int) {
    ++clipped_scene_calls;CHECK(polygon->count>=3 && polygon->count<=8);
    for(unsigned i=0;i<polygon->count;++i) {
        const auto& v=polygon->vertices[i];
        CHECK(v.x>=0 && v.x<=CameraField<float>(0x9D170) && v.y>=0 && v.y<=CameraField<float>(0x9D174) && v.rhw>0);
    }
}
static bool controller_focus=true;
static ff_input::Keyboard camera_keys;
static ff_input::Keyboard CameraKeys() {return camera_keys;}
static int32_t collision_x,collision_z;
static int __cdecl CameraCollision(void*,int32_t* x,int32_t* z) {
    collision_x=*x;collision_z=*z;*x-=32768;return 123;
}
static XINPUT_STATE controller_state;
static bool ControllerFocus() {return controller_focus;}
static DWORD WINAPI ControllerRead(DWORD index,XINPUT_STATE* output) {
    if(index!=1)return ERROR_DEVICE_NOT_CONNECTED;*output=controller_state;return ERROR_SUCCESS;
}
static void __cdecl NativeEnvironmentModels() {
    projector_result=reinterpret_cast<int(__cdecl*)(const CameraRetailModel*,int,int)>(g_base+0x26DE0)(&test_model,0,0);
}
static bool world_visible=true;
static int world_draws;
static void __cdecl NativeWorldModel(void*) {++world_draws;if(world_visible)NativeEnvironmentModels();}
static int native_hall_calls,wall_pushes,wall_pops,wall_turn;
static int native_train_calls;
static bool native_train_sound;
static int train_sounds;
static void __cdecl NativeTrainSound(int sample,int volume,int pitch) {
    CHECK(sample==0x7D && volume==0xFF && pitch==0x1700);
    ++train_sounds;
}
static float projected_door_v;
static int __cdecl TrainDoorProjector(const CameraRetailModel* model,int flags,int depth) {
    const int result=NativeProjector(model,flags,depth);
    projected_door_v=CameraField<float>(0x451DB0+projected_face_texture*32);
    return result;
}
static void __cdecl NativeTrainDoors() {
    for(unsigned i=0;i<3;++i) {
        if(CameraField<int>(0x85A6C0)>static_cast<int>(i))
            CameraField<TrainDoorCollision*>(0x86F988+i*4)->flags=3;
        CameraField<TrainDoorCollision*>(0x86F9A0+i*4)->flags=
            CameraField<int>(TrainDoors[i].travel)==64?0:3;
    }
    if(native_train_sound)reinterpret_cast<void(__cdecl*)(int,int,int)>(g_base+0x3B42A)(0x7D,0xFF,0x1700);
}
static void __cdecl NativeTrain(void* record) {
    ++native_train_calls;
    // Retail 0x44BCE0 rejects the complete exterior/interior pair here.
    if(test_matrix[11]<=256 || test_matrix[11]>=4096)return;
    const int type=*static_cast<int*>(record);
    for(int offset:{0,1})GameCameraModel(CameraField<const CameraRetailModel*>(0x85A6F8+(type+offset)*4),0,0);
}
static int32_t wall_translation[3];
static void __cdecl NativeHallModel(void*) {
    ++native_hall_calls;
    const int yaw=CameraField<int16_t>(0x87888C);
    if(yaw>0x4000 || yaw< -0x4000)NativeEnvironmentModels();
}
static void __cdecl NativeParkCutaway(void*) {
    GameCameraModel(CameraField<const CameraRetailModel*>(0x85A6F8+57*4),0,0);
}
static void __cdecl NativeBiasedHall(void* record) {
    const int type=*static_cast<int*>(record);
    GameCameraModel(CameraField<const CameraRetailModel*>(0x85A6F8+type*4),0,256);
}
static void __cdecl NativeWallPush() {++wall_pushes;}
static void __cdecl NativeWallPop() {++wall_pops;}
static void __cdecl NativeWallTurn(int angle) {wall_turn=angle;}
static void __cdecl NativeWallTranslate(int32_t x,int32_t y,int32_t z) {
    wall_translation[0]=x;wall_translation[1]=y;wall_translation[2]=z;
}
static void __cdecl NativeEnvironmentRecords() {
    for(int i=0;i<CameraField<int>(0x86F9C0);++i) {
        auto record=reinterpret_cast<void*>(g_base+0x8677E0+i*36);
        if(*static_cast<int*>(record)<0x37)break;
        reinterpret_cast<void(__cdecl*)(void*)>(g_base+0x439F1)(record);
    }
}
static bool object_visible=true;
static bool object_origin_gate;
static const CameraRetailModel* object_model;
static int object_draws;
static void __cdecl NativeObjectModel(void*) {
    ++object_draws;
    if(!object_visible || (object_origin_gate && !reinterpret_cast<int(__cdecl*)()>(g_base+0x48ACB)()))return;
    if(object_model)projector_result=reinterpret_cast<int(__cdecl*)(const CameraRetailModel*,int,int)>(g_base+0x26DE0)(object_model,0,0);
    else NativeEnvironmentModels();
}
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
__declspec(naked) static uintptr_t InvokePartitionCleanup(void*,int) {
    __asm {
        mov eax,[esp+4]
        mov ecx,[esp+8]
        mov edx,g_base
        add edx,0x3B224
        jmp edx
    }
}
__declspec(naked) static int InvokeProjectedModelGate(unsigned,unsigned,unsigned) {
    __asm {
        push esi
        mov eax,[esp+8]
        mov edx,[esp+12]
        mov esi,[esp+16]
        mov ecx,g_base
        add ecx,0x27128
        jmp ecx
    }
}
int main() {
    g_base = reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr, 0x900000, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE));
    CHECK(g_base != 0); if (!g_base) return 1;
    Branch(0x22BC0, 0xE9, reinterpret_cast<uintptr_t>(NativeBegin));
    Branch(0x484F0, 0xE9, reinterpret_cast<uintptr_t>(NativeInput));
    Branch(0x1980, 0xE9, reinterpret_cast<uintptr_t>(NativeMatrix));
    Branch(0x20E0,0xE9,uintptr_t(NativeOrigin));Branch(0x16A0,0xE9,uintptr_t(NativeOrigin));
    const DWORD object_origins[]={0xFBF2,0x45D70,0x48ACB,0x48EB0,0x49870,0x498A0};
    for(DWORD rva:object_origins) {Branch(rva,0xE8,g_base+0x20E0);CameraField<BYTE>(rva+5)=0xC3;}
    Branch(0x43B8F,0xE8,g_base+0x20E0);CameraField<BYTE>(0x43B94)=0xC3;
    Branch(0x2330, 0xE9, reinterpret_cast<uintptr_t>(NativeBasis));
    Branch(0x1B20, 0xE9, reinterpret_cast<uintptr_t>(NativeWallPush));
    Branch(0x1BB0, 0xE9, reinterpret_cast<uintptr_t>(NativeWallPop));
    Branch(0x1F00, 0xE9, reinterpret_cast<uintptr_t>(NativeWallTurn));
    Branch(0x2010, 0xE9, reinterpret_cast<uintptr_t>(NativeWallTranslate));
    const BYTE hall_wall_selector[]={0x66,0xA1,0x8C,0x88,0xC7,0x00,0x66,0x3D,0x00,0x40,
        0x7F,0x06,0x66,0x3D,0x00,0xC0,0x7D,0x1A};
    memcpy(reinterpret_cast<void*>(g_base+0xFC20),hall_wall_selector,sizeof(hall_wall_selector));
    const BYTE partition_cleanup[]={0x83,0x38,0x7C,0x75,0x06,0xC7,0x00,0x37,0x00,0x00,0x00,
        0x83,0xC0,0x24,0x49,0x75,0xEF,0xC3};
    memcpy(reinterpret_cast<void*>(g_base+0x3B224),partition_cleanup,sizeof(partition_cleanup));
    Branch(0x3C100, 0xE9, reinterpret_cast<uintptr_t>(NativeSolidObjects));
    const BYTE object_dispatch[]={0x8B,0x04,0x85,0x00,0x89,0xC7,0x00,0x56,0x85,0xC0,0x74,0x04,
        0xFF,0xD0,0xEB,0x05,0xE8,0x07,0xD7,0x00,0x00,0x83,0xC4,0x04};
    memcpy(reinterpret_cast<void*>(g_base+0x3C154),object_dispatch,sizeof(object_dispatch));
    const BYTE object_thunk[]={0x56,0x8B,0x74,0x24,0x08};
    memcpy(reinterpret_cast<void*>(g_base+0x3C14F),object_thunk,sizeof(object_thunk));
    const BYTE object_return[]={0x5E,0xC3};memcpy(reinterpret_cast<void*>(g_base+0x3C16C),object_return,2);
    CameraField<void(__cdecl*)(void*)>(0x878900)=NativeObjectModel;
    CameraField<int>(0x86F9C0)=1;CameraField<int>(0x8677E0)=56;
    CameraField<void(__cdecl*)(void*)>(0x872780+56*4)=NativeWorldModel;
    const BYTE world_thunk[]={0x56,0x8B,0x74,0x24,0x08,0x56};
    memcpy(reinterpret_cast<void*>(g_base+0x439F1),world_thunk,sizeof(world_thunk));
    const BYTE world_dispatch[]={0xFF,0x14,0xBD,0x80,0x27,0xC7,0};
    memcpy(reinterpret_cast<void*>(g_base+0x439F7),world_dispatch,sizeof(world_dispatch));
    const BYTE world_cleanup[]={0x83,0xC4,0x04,0x5E,0xC3};
    memcpy(reinterpret_cast<void*>(g_base+0x439FE),world_cleanup,sizeof(world_cleanup));
    for(unsigned actor=0;actor<8;++actor)for(unsigned hand=0;hand<2;++hand)
        CameraField<int8_t>(0x86A57D+actor*0xF4+hand)=-1;
    const BYTE model_body[]={0x83,0xEC,0x3C,0x53,0x55,0x56,0x5E,0x5D,0x5B,0x83,0xC4,0x3C,0xB8,1,0,0,0,0xC3};
    memcpy(reinterpret_cast<void*>(g_base+0x26DE0),model_body,sizeof(model_body));
    const BYTE model_screen_gate[]={0x3B,0xD0,0x74,0x47,0x3B,0xF0,0x74,0x43,0x33,0xC0,0x5E,0xC3};
    const BYTE model_reject[]={0xB8,1,0,0,0,0x5E,0xC3};
    memcpy(reinterpret_cast<void*>(g_base+0x27128),model_screen_gate,sizeof(model_screen_gate));
    memcpy(reinterpret_cast<void*>(g_base+0x27173),model_reject,sizeof(model_reject));
    Branch(0x26DEC,0xE9,reinterpret_cast<uintptr_t>(NativeProjector));
    const BYTE world_prologue[]={0x53,0x56,0x57};memcpy(reinterpret_cast<void*>(g_base+0x43990),world_prologue,3);
    Branch(0x43993,0xE8,g_base+0x2330);
    Branch(0x24236,0xE8,g_base+0x25C00);
    Branch(0x43998,0xE8,reinterpret_cast<uintptr_t>(NativeEnvironmentRecords));
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
    // An unknown wall selector must reject geometry hooks before any code write.
    g_free_orbit=true;CameraField<BYTE>(0xFC20)^=1;
    CHECK(!InstallCameraGeometryHooks() && CameraField<BYTE>(0x26DE0)==0x83);
    CHECK(CameraField<BYTE>(0x439F7)==0xFF);
    CameraField<BYTE>(0xFC20)^=1;CameraField<BYTE>(0x3B22B)^=1;
    CHECK(!InstallCameraGeometryHooks() && CameraField<BYTE>(0x26DE0)==0x83);
    CHECK(CameraField<BYTE>(0x3B229)==0xC7);
    CameraField<BYTE>(0x3B22B)^=1;
    CameraField<BYTE>(0x27128)^=1;
    CHECK(!InstallCameraGeometryHooks() && CameraField<BYTE>(0x26DE0)==0x83);
    CameraField<BYTE>(0x27128)^=1;
    CameraField<BYTE>(0x24236)=0x90;
    CHECK(!InstallCameraGeometryHooks() && CameraField<BYTE>(0x26DE0)==0x83);
    CHECK(CameraField<BYTE>(0x439F7)==0xFF && CameraField<BYTE>(0x3B229)==0xC7);
    CameraField<BYTE>(0x24236)=0xE8;
    for(DWORD rva:object_origins) {
        Branch(rva,0xE8,g_base+0x16A0);
        CHECK(!InstallCameraGeometryHooks() && CameraField<BYTE>(0x26DE0)==0x83);
        CHECK(CameraField<BYTE>(0x439F7)==0xFF && CameraField<BYTE>(0x3B229)==0xC7);
        Branch(rva,0xE8,g_base+0x20E0);
    }
    g_free_orbit=false;
    // A failed signature must leave every other site untouched.
    site[6] = 1; InstallCameraHooks(); CHECK(*reinterpret_cast<BYTE*>(g_base + 0x40D6C) == 0xE8);
    LONG relative; memcpy(&relative, reinterpret_cast<void*>(g_base + 0x40D6D), 4);
    CHECK(g_base + 0x40D71 + relative == g_base + 0x22BC0);
    site[6] = 0; InstallCameraHooks(); CHECK(site[0] == 0xE8 && site[5] == 0x90 && site[6] == 0x90);
    CHECK(g_camera_collision_enabled);
    CHECK(g_camera_scene_projector==CameraSceneModel && !g_first_person_available);
    memcpy(&relative,reinterpret_cast<void*>(g_base+0x24237),4);
    CHECK(g_base+0x2423B+relative==reinterpret_cast<uintptr_t>(CameraFloorFaces));
    Branch(0xFC20,0xE9,reinterpret_cast<uintptr_t>(NativeHallModel));
    CameraField<uintptr_t>(0x872780+123*4)=g_base+0xFC20;
    CameraField<const CameraRetailModel*>(0x85A6F8+123*4)=&test_model;
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
    CHECK(world_draws==1);
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
    // Environment vehicles have their own coarse visibility gate. Their last
    // geometry must survive a missing model independently of the object pass.
    object_visible=false;world_visible=false;
    reinterpret_cast<void(__cdecl*)()>(g_base+0x40C47)();
    reinterpret_cast<void(__cdecl*)()>(g_base+0x40C51)();CHECK(g_camera_triangles.size()==2);
    CameraField<int>(0x8677E4)=1;
    reinterpret_cast<void(__cdecl*)()>(g_base+0x40C47)();
    reinterpret_cast<void(__cdecl*)()>(g_base+0x40C51)();CHECK(g_camera_triangles.empty());
    world_visible=true;
    reinterpret_cast<void(__cdecl*)()>(g_base+0x40C47)();
    reinterpret_cast<void(__cdecl*)()>(g_base+0x40C51)();CHECK(g_camera_triangles.size()==2);
    CameraField<int>(0x86F9C0)=0;
    reinterpret_cast<void(__cdecl*)()>(g_base+0x40C47)();
    reinterpret_cast<void(__cdecl*)()>(g_base+0x40C51)();CHECK(g_camera_triangles.empty());
    CameraField<int>(0x86F9C0)=1;
    // The indoor hall appends a doorway wall, then removes only its final
    // record from the count on section 1 -> 2. Reproduce that publication at
    // the patched render boundaries; preserve the simulation and gate state.
    CameraField<int>(0x85A6B8)=1;CameraField<int>(0x85A6C0)=1;world_visible=false;
    CameraField<int>(0x86F9C0)=2;CameraField<int16_t>(0x87888C)=0;
    auto hall=reinterpret_cast<int32_t*>(g_base+0x8677E0+36);
    hall[0]=123;hall[1]=ff_camera::Fixed(100);hall[2]=ff_camera::Fixed(200);hall[3]=ff_camera::Fixed(24);
    BYTE hall_before[36];memcpy(hall_before,hall,sizeof(hall_before));
    const int before_hall=projected_calls;
    reinterpret_cast<void(__cdecl*)()>(g_base+0x40C47)();
    reinterpret_cast<void(__cdecl*)()>(g_base+0x40C51)();
    CHECK(projected_calls==before_hall+1 && native_hall_calls==0 && projected_face_flags==1);
    CHECK(g_camera_triangles.size()==2 && wall_pushes==0);
    CameraField<int>(0x85A6C0)=2; // Still counted: do not draw it twice.
    reinterpret_cast<void(__cdecl*)()>(g_base+0x40C47)();
    reinterpret_cast<void(__cdecl*)()>(g_base+0x40C51)();
    CHECK(projected_calls==before_hall+2 && wall_pushes==0);
    CameraField<int>(0x86F9C0)=1;world_visible=false;
    CameraField<int32_t>(0x8677B0)=ff_camera::Fixed(10);
    CameraField<int32_t>(0x8677B4)=ff_camera::Fixed(20);
    CameraField<int32_t>(0x8677B8)=ff_camera::Fixed(30);
    const int before_boundary=projected_calls;
    for(int i=0;i<20;++i) {
        CameraField<int16_t>(0x87888C)=static_cast<int16_t>(i*4096);
        reinterpret_cast<void(__cdecl*)()>(g_base+0x40C47)();
        reinterpret_cast<void(__cdecl*)()>(g_base+0x40C51)();
        CHECK(g_camera_triangles.size()==2);
        CHECK(fabs(CameraObstacleDistance({10,20,30},{10,20,750})-344)<0.0001);
    }
    CHECK(projected_calls==before_boundary+20 && native_hall_calls==0);
    CHECK(wall_pushes==20 && wall_pops==20 && wall_turn==0x8000);
    CHECK(wall_translation[0]==ff_camera::Fixed(90) && wall_translation[1]==ff_camera::Fixed(180));
    CHECK(wall_translation[2]==ff_camera::Fixed(-6));
    CHECK(CameraField<int>(0x86F9C0)==1 && CameraField<int>(0x85A6C0)==2);
    CHECK(!memcmp(hall_before,hall,sizeof(hall_before)) && model_faces[4]==0);
    hall[0]=124; // The supplemental fixed-wall draw never resurrects partitions.
    reinterpret_cast<void(__cdecl*)()>(g_base+0x40C47)();
    reinterpret_cast<void(__cdecl*)()>(g_base+0x40C51)();CHECK(g_camera_triangles.empty());
    hall[0]=123;
    for(int excluded=0;excluded<4;++excluded) {
        CameraField<int>(0x85A668)=excluded==0?1:0; // Attract demo.
        CameraField<int>(0x8745A0)=excluded==1?2:1; // Cooperative play.
        CameraField<int>(0x85A6B8)=excluded==2?2:1; // Different stage.
        g_free_orbit=excluded!=3;
        const int before=projected_calls;
        reinterpret_cast<void(__cdecl*)()>(g_base+0x40C47)();
        reinterpret_cast<void(__cdecl*)()>(g_base+0x40C51)();
        CHECK(projected_calls==before && wall_pushes==20);
    }
    // Execute the retail cleanup loop with one broken partition, an intact
    // counterpart and an unrelated gate. Keep native loop registers/stack and
    // all live state: retaining a model after destruction would be incorrect.
    auto partitions=reinterpret_cast<BYTE*>(g_base+0x86BF20);
    memset(partitions,0,3*36);
    *reinterpret_cast<int32_t*>(partitions)=55;
    *reinterpret_cast<int32_t*>(partitions+36)=124;
    *reinterpret_cast<int16_t*>(partitions+36+22)=7;
    *reinterpret_cast<int32_t*>(partitions+72)=127;
    BYTE partitions_before[3*36];memcpy(partitions_before,partitions,sizeof(partitions_before));
    g_free_orbit=g_camera_active=true;
    CHECK(InvokePartitionCleanup(partitions,3)==reinterpret_cast<uintptr_t>(partitions+3*36));
    CHECK(!memcmp(partitions,partitions_before,sizeof(partitions_before)));
    *reinterpret_cast<int32_t*>(partitions+36)=55; // Subsequent native breakage.
    InvokePartitionCleanup(partitions,3);
    CHECK(*reinterpret_cast<int32_t*>(partitions+36)==55);
    for(int native=0;native<5;++native) {
        *reinterpret_cast<int32_t*>(partitions+36)=124;
        g_free_orbit=native!=0;g_camera_active=native!=1;
        CameraField<int>(0x8745A0)=native==2?2:1;
        CameraField<int>(0x85A668)=native==3?1:0;
        CameraField<int>(0x85A6B8)=native==4?2:1;
        CHECK(InvokePartitionCleanup(partitions,3)==reinterpret_cast<uintptr_t>(partitions+3*36));
        CHECK(*reinterpret_cast<int32_t*>(partitions)==55);
        CHECK(*reinterpret_cast<int32_t*>(partitions+36)==55);
        CHECK(*reinterpret_cast<int16_t*>(partitions+36+22)==7);
        CHECK(*reinterpret_cast<int32_t*>(partitions+72)==127);
    }
    CameraField<int>(0x85A6B8)=1;
    // Native framing keeps both the original yaw gate and dropped count.
    g_free_orbit=true;g_camera_active=false;CameraField<int16_t>(0x87888C)=0;
    CameraField<int>(0x86F9C0)=2;
    reinterpret_cast<void(__cdecl*)()>(g_base+0x40C47)();CHECK(native_hall_calls==1);
    CameraField<int>(0x86F9C0)=1;CameraField<int>(0x85A6B8)=0;g_camera_active=true;
    CameraField<int32_t>(0x8677B0)=CameraField<int32_t>(0x8677B4)=CameraField<int32_t>(0x8677B8)=0;
    world_visible=true;
    reinterpret_cast<void(__cdecl*)()>(g_base+0x40C47)();
    reinterpret_cast<void(__cdecl*)()>(g_base+0x40C51)();
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
    // actual input publication, timed right-stick look, D-pad zoom and R3.
    g_modern_keyboard=false;g_modern_gamepad=true;g_modern_focus=ControllerFocus;
    g_modern_xinput=ControllerRead;g_modern_pad_session={};ReadModernPad();
    CHECK(g_modern_pad_session.selected==1);
    g_camera_orbit.Begin(0,g_camera_settings);g_camera_active=true;
    controller_state.Gamepad.sThumbLX=32767;controller_state.Gamepad.sThumbRX=32767;
    controller_state.Gamepad.wButtons=XINPUT_GAMEPAD_DPAD_UP;g_modern_pad_session.last_poll=0;
    QueryPerformanceCounter(&g_camera_time);g_camera_time.QuadPart-=g_camera_frequency.QuadPart/20;
    GameCameraInput();
    CHECK((CameraField<uint32_t>(0x5523E0)&15)==ff_input::Right);
    CHECK(g_camera_orbit.yaw< -0.05 && g_camera_orbit.yaw> -0.3);
    CHECK(g_camera_orbit.distance<695 && g_camera_orbit.distance>665);
    const WORD turns[]={XINPUT_GAMEPAD_DPAD_LEFT,XINPUT_GAMEPAD_DPAD_RIGHT};
    for(WORD turn:turns) {
        controller_state.Gamepad={};controller_state.Gamepad.wButtons=turn;g_modern_pad_session.last_poll=0;
        CameraField<uint32_t>(0x5523E0)=0;
        const double previous_yaw=g_camera_orbit.yaw,distance=g_camera_orbit.distance;
        QueryPerformanceCounter(&g_camera_time);g_camera_time.QuadPart-=g_camera_frequency.QuadPart/20;
        GameCameraInput();const double delta=ff_camera::Wrap(g_camera_orbit.yaw-previous_yaw);
        CHECK(turn==XINPUT_GAMEPAD_DPAD_LEFT?delta>0:delta<0);
        CHECK(!(CameraField<uint32_t>(0x5523E0)&15) && g_camera_orbit.distance==distance);
    }
    controller_state.Gamepad={};controller_state.Gamepad.wButtons=XINPUT_GAMEPAD_RIGHT_THUMB;
    const auto focus_yaw=CameraField<int16_t>(0x87888C);
    CameraField<int16_t>(0x86A538)=256;g_modern_pad_session.last_poll=0;GameCameraInput();
    CHECK(CameraField<int16_t>(0x87888C)==focus_yaw);
    CameraField<int16_t>(0x86A538)=512;GameCameraInput();
    CHECK(CameraField<int16_t>(0x87888C)==focus_yaw);
    controller_state.Gamepad.wButtons=0;g_modern_pad_session.last_poll=0;GameCameraInput();
    controller_state.Gamepad.wButtons=XINPUT_GAMEPAD_RIGHT_THUMB;
    g_modern_pad_session.last_poll=0;GameCameraInput();CHECK(CameraField<int16_t>(0x87888C)==focus_yaw);
    controller_focus=false;GameCameraInput();CHECK((CameraField<uint32_t>(0x5523E0)&0xFFFF)==0);
    controller_focus=true;g_modern_pad_session.ready=true;g_modern_pad_session.last_poll=1;
    g_camera_time.QuadPart=123;
    GameFocusLost();CHECK(!g_modern_pad_session.ready && !g_modern_pad_session.last_poll);
    CHECK(!g_camera_time.QuadPart);
    CHECK(!ReadModernPad().buttons); // A held R3 cannot acquire a target on focus return.
    controller_state.Gamepad={};g_modern_pad_session.last_poll=0;ReadModernPad();
    CHECK(g_modern_pad_session.ready);
    // Exercise both view bindings through the complete camera input boundary.
    // Skeletal math and temporary face scope are covered by their native fixture.
    struct ViewModel {
        FirstPersonModel model;unsigned counts[1];int32_t commands[13];int16_t vertices[4],faces[6];
    } view_model={};
    const int32_t head_node[]={3,1,0,0,0,0,0,0,0,0,0,0,0};
    memcpy(view_model.commands,head_node,sizeof(head_node));view_model.counts[0]=1;
    view_model.model={view_model.vertices,1,view_model.faces,1,nullptr,1,view_model.commands,1};
    CameraField<int16_t>(0x86A534)=0;
    CameraField<const FirstPersonModel*>(0x85A6F8)=&view_model.model;
    Branch(0x1000,0xE9,reinterpret_cast<uintptr_t>(NativeMatrix));
    const DWORD matrix_calls[]={0x10F0,0x1170,0x11F0,0x12F0,0x13F0,0x14F0,0x15F0};
    for(DWORD rva:matrix_calls)Branch(rva,0xE9,reinterpret_cast<uintptr_t>(NativeBasis));
    g_first_person_available=true;g_first_person={};g_modern_keyboard=true;g_modern_keyboard_reader=CameraKeys;
    CameraField<uint32_t>(0x5523E0)=0;GameCameraInput();
    camera_keys.first_person=true;GameCameraInput();CHECK(FirstPersonActive());
    GameCameraInput();CHECK(FirstPersonActive()); // Holding Tab cannot toggle twice.
    camera_keys.first_person=false;GameCameraInput();
    controller_state.Gamepad.wButtons=XINPUT_GAMEPAD_DPAD_LEFT;g_modern_pad_session.last_poll=0;
    const double eye_yaw=g_first_person.yaw,eye_pitch=g_first_person.pitch;
    QueryPerformanceCounter(&g_camera_time);g_camera_time.QuadPart-=g_camera_frequency.QuadPart/20;
    GameCameraInput();CHECK(ff_camera::Wrap(g_first_person.yaw-eye_yaw)>0 && g_first_person.pitch==eye_pitch);
    CHECK(!(CameraField<uint32_t>(0x5523E0)&15));
    g_first_person.pitch=-80*ff_camera::kPi/180;
    controller_state.Gamepad.wButtons=XINPUT_GAMEPAD_LEFT_THUMB;g_modern_pad_session.last_poll=0;
    const double saved_distance=g_camera_orbit.distance;
    GameCameraInput();CHECK(!g_first_person.selected && g_camera_orbit.distance==saved_distance);
    CHECK(g_camera_orbit.pitch>=ff_camera::MinimumPitch(saved_distance,g_camera_settings));
    CHECK(!(CameraField<uint32_t>(0x5523E0)&ff_input::Run));
    controller_state.Gamepad={};g_modern_pad_session.last_poll=0;GameCameraInput();
    camera_keys.first_person=true;GameFocusLost();GameCameraInput();CHECK(!g_first_person.selected);
    camera_keys.first_person=false;GameCameraInput();camera_keys.first_person=true;
    GameCameraInput();CHECK(FirstPersonActive());
    // Keyboard and pad retreat reach the native solver in view space. Preserve
    // the native solver's correction, including when analog speed is disabled.
    Branch(0x37CE0,0xE9,reinterpret_cast<uintptr_t>(CameraCollision));
    g_first_person_movement=FirstPersonMovement;g_modern_analog=false;
    g_first_person.yaw=0;CameraField<int>(0x86A50C)=0x52;
    camera_keys={};camera_keys.down=true;CameraField<uint32_t>(0x5523E0)=0;GameCameraInput();
    auto actor=reinterpret_cast<void*>(g_base+0x86A500);
    int32_t step_x=0,step_z=10*65536;
    CHECK(ModernAnalogCollision(actor,&step_x,&step_z)==123);
    CHECK(collision_x==0 && collision_z==-10*65536 && step_x==-32768 && step_z==collision_z);
    camera_keys={};controller_state.Gamepad.sThumbLY=-32768;g_modern_pad_session.last_poll=0;
    CameraField<uint32_t>(0x5523E0)=0;GameCameraInput();step_x=0;step_z=10*65536;
    ModernAnalogCollision(actor,&step_x,&step_z);CHECK(collision_x==0 && collision_z==-10*65536);
    g_first_person.selected=false;step_x=0;step_z=10*65536;
    ModernAnalogCollision(actor,&step_x,&step_z);CHECK(collision_x==0 && collision_z==10*65536);
    // Armed keyboard and pad diagonals use the native weapon step length,
    // despite analog speed being enabled for unarmed movement.
    g_first_person.selected=true;g_modern_analog=true;controller_state.Gamepad={};
    CameraField<int8_t>(0x86A57E)=5;CameraField<int>(0x870620+5*36)=0x1F3;
    CameraField<int>(0x86A50C)=0x48;CameraField<int16_t>(0x86A538)=123;
    camera_keys={};CameraField<uint32_t>(0x5523E0)=0;g_modern_pad_session.last_poll=0;
    GameCameraInput();CHECK(CameraField<int16_t>(0x86A538)==1024);
    CameraField<int>(0x86A50C)=0x52;camera_keys.up=camera_keys.right=true;
    CameraField<uint32_t>(0x5523E0)=0;GameCameraInput();CHECK(!g_analog_active);
    step_x=0;step_z=10*65536;ModernAnalogCollision(actor,&step_x,&step_z);
    CHECK(collision_x>0 && collision_z>0 && collision_x==collision_z);
    CHECK(fabs(hypot(collision_x,collision_z)/65536.0-10)<0.00002);
    CHECK(step_x==collision_x-32768 && step_z==collision_z);
    camera_keys={};controller_state.Gamepad.sThumbLX=-32768;controller_state.Gamepad.sThumbLY=32767;
    CameraField<uint32_t>(0x5523E0)=0;g_modern_pad_session.last_poll=0;GameCameraInput();
    step_x=0;step_z=10*65536;ModernAnalogCollision(actor,&step_x,&step_z);
    CHECK(collision_x<0 && collision_z>0 && collision_x==-collision_z && !g_analog_active);
    // Exercise scene near clipping through the installed model entry and its
    // original trampoline. Capture still sees the original world geometry.
    Branch(0x265B0,0xE9,uintptr_t(SceneFragment));
    g_camera_scene_projector=CameraSceneModel;
    CameraField<float>(0xAD214)=CameraField<float>(0xAD234)=100;
    CameraField<float>(0x20C50C)=160;CameraField<float>(0x20C508)=120;
    CameraField<float>(0x9D170)=320;CameraField<float>(0x9D174)=240;CameraField<float>(0x5E8A4)=0.0001f;
    CameraField<FirstPersonPropMaterial>(0x451DA8)={0,1,0,1,0,0,3,{0,0}};
    const int16_t floor_vertices[]={-64,8,-16,0,64,8,-16,0,64,8,128,0,-64,8,128,0};
    const int16_t floor_faces[]={0,3,2,1,0,0};
    const CameraRetailModel floor={floor_vertices,4,floor_faces,1};
    g_camera_geometry_frame.active=false;g_camera_geometry_frame.environment=true;
    // Captured hall partition: bottom and top rows both lie outside the
    // viewport, on opposite sides. Preserve the native per-face clipping.
    CHECK(FirstPersonActive());
    const BYTE gate_clips[]={5,5,4,6,6,9,9,8,10,10};
    for(unsigned i=0;i<_countof(gate_clips);++i)CameraField<BYTE>(0x9D224+i*0x20)=gate_clips[i];
    CHECK(InvokeProjectedModelGate(10,8,10)==0);
    g_first_person.selected=false;CHECK(InvokeProjectedModelGate(10,8,10)==1);
    g_first_person.selected=true;g_camera_geometry_frame.environment=false;
    CHECK(InvokeProjectedModelGate(10,8,10)==1);
    g_camera_geometry_frame.environment=true;
    CameraField<BYTE>(0x9D224)=5;CameraField<BYTE>(0x9D244)=9;
    CHECK(InvokeProjectedModelGate(2,2,2)==1); // both right, despite different Y flags
    CameraField<BYTE>(0x9D244)=6;
    CHECK(InvokeProjectedModelGate(2,2,2)==1); // both below, despite different X flags
    CameraField<BYTE>(0x9D224)=1;CameraField<BYTE>(0x9D244)=2;
    CHECK(InvokeProjectedModelGate(2,2,0)==0);
    CHECK(InvokeProjectedModelGate(2,1,1)==0); // neither native axis gate fires
    auto project=reinterpret_cast<int(__cdecl*)(const CameraRetailModel*,int,int)>(g_base+0x26DE0);
    CHECK(project(&floor,0,0)==1 && clipped_scene_calls>0);
    CHECK(projected_face_texture==-1 && projected_face_flags==1 && floor_faces[5]==0 && floor_faces[4]==0);
    g_camera_geometry_frame.environment=false;const int clipped_floor=clipped_scene_calls;
    project(&floor,0,0);CHECK(clipped_scene_calls>clipped_floor && projected_face_flags==0);
    const int clipped_eye=clipped_scene_calls;g_first_person.selected=false;
    project(&floor,0,0);CHECK(clipped_scene_calls>clipped_eye && projected_face_texture==-1);
    const int clipped_live=clipped_scene_calls;
    g_first_person.selected=true;CameraField<int>(0x85A668)=1;
    project(&floor,0,0);CHECK(clipped_scene_calls==clipped_live && projected_model==&floor);
    // All six object gates now belong to custom scene visibility, independently
    // of eye-view availability. Menus, demos and original cameras stay native.
    CameraField<int>(0x85A668)=0;g_first_person.selected=g_first_person_available=false;
    g_free_orbit=g_camera_active=true;
    auto origins=[&](int expected,bool forwarded) {
        for(DWORD rva:object_origins) {
            const int before=native_origins;
            CHECK(reinterpret_cast<int(__cdecl*)()>(g_base+rva)()==expected);
            CHECK(native_origins==before+int(forwarded));
        }
    };
    origins(1,false);g_first_person.selected=true;origins(1,false);g_first_person.selected=false;
    for(int excluded=0;excluded<5;++excluded) {
        g_free_orbit=excluded!=0;g_camera_active=excluded!=1;
        CameraField<int>(0x85A668)=excluded==2?1:0;
        CameraField<int>(0x85A6B8)=excluded==3?26:0;
        CameraField<int>(0x8745A0)=excluded==4?2:1;
        origins(0,true);const int before=clipped_scene_calls;
        project(&floor,0,0);CHECK(clipped_scene_calls==before && projected_model==&floor);
    }
    g_free_orbit=g_camera_active=true;CameraField<int>(0x8745A0)=1;
    CameraField<int>(0x85A6B8)=0;CameraField<int>(0x85A668)=0;
    CHECK(reinterpret_cast<int(__cdecl*)()>(g_base+0x43B8F)()==0);
    CHECK(reinterpret_cast<int(__cdecl*)()>(g_base+0x16A0)()==0);
    // Exercise the installed vehicle gate and model projector together while
    // a visible face crosses the camera plane and its origin is already behind.
    object_origin_gate=object_visible=true;object_model=&floor;CameraField<int>(0x870620)=0;
    for(int origin:{-16,-64,-120}) {
        test_matrix[11]=float(origin);const int before=clipped_scene_calls;
        reinterpret_cast<void(__cdecl*)(void*)>(g_base+0x3C14F)(reinterpret_cast<void*>(g_base+0x870620));
        CHECK(clipped_scene_calls>before && projected_face_texture==-1 && projected_face_flags==0);
        CHECK(floor_faces[4]==0 && floor_faces[5]==0 && CameraField<int>(0x870620)==0);
    }
    const int visible=clipped_scene_calls;object_visible=false;GameCameraSolidObjects();
    CHECK(clipped_scene_calls==visible);object_visible=true;CameraField<int>(0x870620)=-1;
    GameCameraSolidObjects();CHECK(clipped_scene_calls==visible);
    object_origin_gate=false;object_model=nullptr;
    // Shared and separate cooperative views use each view's own projection;
    // original native multiplayer rendering was checked in the exclusions above.
    g_camera_cooperative=true;CameraField<int>(0x8745A0)=2;
    CameraField<int16_t>(0x86A542)=CameraField<int16_t>(0x86A636)=100;
    const float projections[][3]={{320,160,100},{400,200,100},{640,0,200},{640,640,200}};
    for(int owner:{-1,0,1})for(const auto& projection:projections) {
        CameraPlayerScope scope(owner);origins(1,false);
        CameraField<float>(0x9D170)=projection[0];CameraField<float>(0x20C50C)=projection[1];
        CameraField<float>(0xAD214)=projection[2];const int before=clipped_scene_calls;
        project(&floor,0,0);CHECK(clipped_scene_calls>before && floor_faces[5]==0 && floor_faces[4]==0);
    }
    g_camera_cooperative=false;
    // The first-person world dispatch replaces the Car Park cutaway and the
    // Hall's biased draw, retaining the original paths in the orbit camera.
    Branch(0x9600,0xE9,uintptr_t(NativeParkCutaway));
    Branch(0x9670,0xE9,uintptr_t(NativeBiasedHall));
    g_camera_scene_projector=NativeProjector;
    g_first_person_available=true;
    CameraField<int>(0x8745A0)=1;CameraField<int>(0x85A6B8)=0;
    CameraField<int>(0x85A6C0)=3;CameraField<int16_t>(0x87888C)=0;
    CameraField<uintptr_t>(0x872780+56*4)=g_base+0x9600;
    CameraField<const CameraRetailModel*>(0x85A6F8+56*4)=&test_model;
    CameraField<const CameraRetailModel*>(0x85A6F8+57*4)=&floor;
    CameraField<const CameraRetailModel*>(0x85A6F8+59*4)=&floor;
    int architecture=56;
    g_first_person.selected=false;GameCameraWorldObject(&architecture);CHECK(projected_vertices==floor.vertices);
    g_first_person.selected=true;GameCameraWorldObject(&architecture);CHECK(projected_vertices==test_model.vertices);
    CameraField<int16_t>(0x87888C)=0x4000;
    const int before_bush=projected_calls;
    GameCameraWorldObject(&architecture);CHECK(projected_calls==before_bush+2 && projected_vertices==test_model.vertices);
    for(int type:{119,122}) {
        CameraField<int>(0x85A6B8)=1;architecture=type;
        CameraField<uintptr_t>(0x872780+type*4)=g_base+0x9670;
        CameraField<const CameraRetailModel*>(0x85A6F8+type*4)=&test_model;
        g_first_person.selected=false;GameCameraWorldObject(&architecture);CHECK(projected_depth==256);
        g_first_person.selected=true;GameCameraWorldObject(&architecture);CHECK(projected_depth==0);
    }
    g_first_person.selected=false;
    // Train dispatch must reach both meshes even with the origin behind the
    // view. Test all pairs, both camera modes and every cooperative owner.
    Branch(0x4BCE0,0xE9,uintptr_t(NativeTrain));
    g_camera_scene_projector=NativeProjector;
    CameraField<int>(0x85A6B8)=14;CameraField<int>(0x8745A0)=1;
    g_camera_geometry_frame.environment=true;g_controls.complete_rooms=false;
    for(int type:{307,309,311,313}) {
        CameraField<uintptr_t>(0x872780+type*4)=g_base+0x4BCE0;
        CameraField<const CameraRetailModel*>(0x85A6F8+type*4)=&test_model;
        CameraField<const CameraRetailModel*>(0x85A6F8+(type+1)*4)=&floor;
        int record[9]={type};const int before_native=native_train_calls;
        for(bool eye:{false,true})for(float depth:{-2000.f,0.f,256.f,1000.f,5000.f}) {
            g_first_person.selected=eye;test_matrix[11]=depth;
            const int before=projected_calls;GameCameraWorldObject(record);
            CHECK(projected_calls==before+2 && native_train_calls==before_native);
        }
        for(int owner:{-1,0,1}) {
            g_camera_cooperative=true;CameraField<int>(0x8745A0)=2;CameraPlayerScope scope(owner);
            const int before=projected_calls;GameCameraWorldObject(record);CHECK(projected_calls==before+2);
        }
        g_camera_cooperative=false;CameraField<int>(0x8745A0)=1;test_matrix[11]=-100;
        for(int excluded=0;excluded<5;++excluded) {
            g_free_orbit=excluded!=0;g_camera_active=excluded!=1;CameraField<int>(0x85A668)=excluded==2;
            CameraField<int>(0x85A6B8)=excluded==3?13:14;
            CameraField<uintptr_t>(0x872780+type*4)=excluded==4?uintptr_t(NativeTrain):g_base+0x4BCE0;
            const int before=projected_calls,calls=native_train_calls;GameCameraWorldObject(record);
            CHECK(projected_calls==before && native_train_calls==calls+1);
        }
        g_free_orbit=g_camera_active=true;CameraField<int>(0x85A668)=0;CameraField<int>(0x85A6B8)=14;
        CameraField<uintptr_t>(0x872780+type*4)=g_base+0x4BCE0;
    }
    CHECK(model_faces[4]==0 && floor_faces[4]==0);
    // Each passable join has two nearly coincident door panels. Only one
    // panel's atlas row follows the native phase for that join.
    CameraField<int16_t>(0x86D168)=534;
    const struct {int type,z,material,phase;bool duplicate;} doors[]={
        {307,-4752,288,0x863E80,false}, {313,-3040,294,0x8788A4,true},
        {311,-1328,294,0x8788A4,false}, {309,384,288,0x863E80,true},
        {307,2096,288,0x863E80,false}, {313,3808,294,0x8788A4,true},
        {309,-6464,288,0,false}, {311,5520,294,0,false},
    };
    int16_t train_faces[]={0,1,2,3,0,0, 0,1,2,3,0,821};
    const CameraRetailModel train_interior={model_vertices,4,train_faces,2};
    for(bool eye:{false,true})for(const auto& door:doors) {
        g_first_person.selected=eye;
        CameraField<const CameraRetailModel*>(0x85A6F8+(door.type+1)*4)=&train_interior;
        const int16_t material=static_cast<int16_t>(534+door.material);
        train_faces[5]=material;
        int32_t record[9]={door.type,0,0,door.z*65536};
        for(int phase:{0,32,64}) {
            CameraField<int>(0x863E80)=CameraField<int>(0x8788A4)=0;
            if(door.phase)CameraField<int>(door.phase)=phase;
            GameCameraWorldObject(record);
            const bool hide=door.duplicate || (door.phase && phase==64);
            CHECK(projected_face_count==(hide?1u:2u));
            CHECK(projected_face_texture==(hide?821:material));
            CHECK(train_faces[5]==material && train_faces[4]==0);
        }
        record[1]=65536;
        GameCameraWorldObject(record);
        CHECK(projected_face_count==2 && projected_face_texture==material);
    }
    // The rear rectangle turns on before the walker has reached the leaf.
    // Move the blocker to the leaf and close only after the doorway clears.
    Branch(0x3BFF6,0xE8,g_base+0x3B260);
    Branch(0x3B260,0xE9,uintptr_t(NativeTrainDoors));
    Branch(0x3B42A,0xE8,g_base+0x42600);
    Branch(0x42600,0xE9,uintptr_t(NativeTrainSound));
    CameraField<BYTE>(0x3B42F)=0xC3;
    InstallTrainDoorHook();
    TrainDoorCollision leaves[3]={},rears[3]={};
    for(unsigned i=0;i<3;++i) {
        const auto& door=TrainDoors[i];
        leaves[i].min_z=door.leaf_min_z;leaves[i].max_z=door.leaf_max_z;
        rears[i].min_z=door.rear_min_z;rears[i].max_z=door.rear_max_z;
        CameraField<TrainDoorCollision*>(0x86F9A0+i*4)=leaves+i;
        CameraField<TrainDoorCollision*>(0x86F988+i*4)=rears+i;
    }
    for(unsigned i=0;i<8;++i)CameraField<int16_t>(0x86A542+i*0xF4)=0;
    auto tick_train=reinterpret_cast<void(__cdecl*)()>(g_base+0x3BFF6);
    CameraField<BYTE>(0x3BFFB)=0xC3;
    CameraField<int>(0x85A6C0)=0;
    tick_train();
    CameraField<int16_t>(0x86A542)=100;
    CameraField<int32_t>(0x86A530)=-4860*65536;
    CameraField<int>(0x863E80)=2;
    native_train_sound=true;tick_train();native_train_sound=false;
    CHECK(train_sounds==1);
    CameraField<int>(0x85A6C0)=1;
    CameraField<int>(0x863E80)=64;CameraField<int>(0x8788A4)=64;
    const int plane=-4752;
    CameraField<int16_t>(0x86A542)=100;
    CameraField<int32_t>(0x86A530)=-4860*65536;
    tick_train();
    CHECK(leaves[0].min_z==plane && leaves[0].max_z==plane);
    CHECK(rears[0].flags==0 && leaves[0].flags==0 && g_train_door_travel[0]==-1);
    CameraField<int32_t>(0x86A530)=(plane+33)*65536;
    CameraField<int>(0x8745A0)=2;
    CameraField<int>(0x878894)=1;
    CameraField<int32_t>(0x86A624)=(plane-100)*65536;
    tick_train();CHECK(leaves[0].flags==0 && g_train_door_travel[0]==-1);
    CameraField<int16_t>(0x86A636)=100;
    CameraField<int32_t>(0x86A624)=(plane+32)*65536;
    tick_train();CHECK(leaves[0].flags==0 && g_train_door_travel[0]==-1);
    CameraField<int32_t>(0x86A624)=(plane+33)*65536;
    CameraField<int16_t>(0x86A72A)=100; // Enemy still in the previous wagon.
    CameraField<int32_t>(0x86A718)=(plane-100)*65536;
    tick_train();CHECK(leaves[0].flags==0 && g_train_door_travel[0]==-1);
    CameraField<int32_t>(0x86A718)=(plane+33)*65536;
    tick_train();
    CHECK(leaves[0].flags==3 && g_train_door_travel[0]==62);
    CHECK(train_sounds==2);
    CHECK(leaves[1].flags==0 && leaves[2].flags==0);
    CameraField<int>(0x8745A0)=1;CameraField<int>(0x878894)=0;
    train_faces[5]=822;
    CameraField<const CameraRetailModel*>(0x85A6F8+308*4)=&train_interior;
    g_camera_scene_projector=TrainDoorProjector;
    int32_t near_door[9]={307,0,0,plane*65536},far_door[9]={307,0,0,2096*65536};
    auto& door_v=CameraField<float>(0x451DB0+822*32);
    CameraField<int>(0x20C4F8)=0;door_v=0.25f;
    GameCameraWorldObject(near_door);
    CHECK(projected_face_count==2 && projected_door_v==62/256.0f && door_v==0.25f);
    GameCameraWorldObject(far_door);
    CHECK(projected_face_count==1 && projected_face_texture==821);
    for(int i=0;i<31;++i)tick_train();
    CHECK(g_train_door_travel[0]==0 && leaves[0].flags==3);
    GameCameraWorldObject(near_door);
    CHECK(projected_face_count==2 && projected_door_v==0 && door_v==0.25f);
    CameraField<int>(0x20C4F8)=1;door_v=0.125f;
    GameCameraWorldObject(near_door);
    CHECK(projected_face_count==2 && projected_door_v==0 && door_v==0.125f);
    // Later waves spawn behind a door that has already closed. The leaf
    // reopens for them even before native proximity animation starts.
    CameraField<int>(0x20C4F8)=0;
    CameraField<int>(0x863E80)=0;door_v=0;
    CameraField<int32_t>(0x86A718)=(plane-400)*65536;
    tick_train();CHECK(g_train_door_travel[0]==8 && leaves[0].flags==3);
    CHECK(train_sounds==3);
    GameCameraWorldObject(near_door);
    CHECK(projected_door_v==8/256.0f && door_v==0);
    for(int i=0;i<7;++i)tick_train();
    CHECK(g_train_door_travel[0]==64 && leaves[0].flags==0);
    CameraField<int>(0x863E80)=2;
    CameraField<int32_t>(0x86A718)=(plane-40)*65536;
    native_train_sound=true;tick_train();native_train_sound=false;
    CHECK(train_sounds==3);
    CameraField<int32_t>(0x86A718)=(plane+32)*65536;
    tick_train();CHECK(g_train_door_travel[0]==64 && leaves[0].flags==0);
    CameraField<int32_t>(0x86A718)=(plane+33)*65536;
    tick_train();CHECK(g_train_door_travel[0]==62 && leaves[0].flags==3);
    CHECK(train_sounds==4);
    CameraField<int>(0x863E80)=64;
    CameraField<int>(0x85A6C0)=0;tick_train();
    CHECK(g_train_door_travel[0]==-1 && leaves[0].flags==0);
    // Empty enemy slots in a live Train run retain positive health at Z=0.
    // They must not keep the third door open once the actual actors cross.
    CameraField<int>(0x85A6C0)=3;
    CameraField<int32_t>(0x86A530)=2200*65536;
    CameraField<int16_t>(0x86A636)=0;
    CameraField<int16_t>(0x86A72A)=0;
    CameraField<int16_t>(0x86A534+6*0xF4)=-1;
    CameraField<int16_t>(0x86A542+6*0xF4)=380;
    CameraField<int32_t>(0x86A530+6*0xF4)=0;
    tick_train();
    CHECK(leaves[2].flags==3 && g_train_door_travel[2]==62);
    CameraField<int16_t>(0x86A542+6*0xF4)=0;
    CameraField<int>(0x20C4F8)=0;
    g_camera_scene_projector=NativeProjector;
    // The recessed Subway track sides must not paint their backs over the
    // painter-ordered platform floor. Exercise the normal model entry in both
    // stages and camera modes while retaining the native depth offset.
    CameraField<const CameraRetailModel*>(0x85A6F8+281*4)=&test_model;
    CameraField<uintptr_t>(0x872780+281*4)=g_base+0x100E0;
    for(int stage:{12,13})for(bool eye:{false,true}) {
        CameraField<int>(0x85A6B8)=stage;g_first_person.selected=eye;
        GameCameraModel(&test_model,0,1024);
        CHECK(projected_face_flags==0 && projected_model==&test_model && projected_depth==1024);
        GameCameraModel(&floor,0,0);CHECK(projected_face_flags==1);
        for(int owner:{-1,0,1}) {
            g_camera_cooperative=true;CameraField<int>(0x8745A0)=2;CameraPlayerScope scope(owner);
            GameCameraModel(&test_model,0,1024);CHECK(projected_face_flags==0 && projected_depth==1024);
        }
        g_camera_cooperative=false;CameraField<int>(0x8745A0)=1;
    }
    CameraField<int>(0x85A6B8)=11;GameCameraModel(&test_model,0,1024);CHECK(projected_face_flags==1);
    CameraField<int>(0x85A6B8)=12;CameraField<uintptr_t>(0x872780+281*4)=uintptr_t(NativeWorldModel);
    GameCameraModel(&test_model,0,1024);CHECK(projected_face_flags==1);
    CameraField<uintptr_t>(0x872780+281*4)=g_base+0x100E0;model_faces[4]=1;
    GameCameraModel(&test_model,0,1024);CHECK(projected_face_flags==1 && model_faces[4]==1);
    model_faces[4]=0;
    // The front-facing far wall also needs the platform lip as an occluder.
    // Exercise that path through the installed entry, not only the helper.
    const int16_t rail_vertices[]={471,0,0,0,471,0,2048,0,471,-198,2048,0,471,-198,0,0};
    const int16_t rail_faces[]={0,1,2,3,0,0};
    const CameraRetailModel rail={rail_vertices,4,rail_faces,1};
    CameraField<const CameraRetailModel*>(0x85A6F8+281*4)=&rail;
    const float rail_matrix[]={0,0,1,-1024,0,-1,0,160,1,0,0,1000};
    memcpy(test_matrix,rail_matrix,sizeof(test_matrix));g_camera_scene_projector=CameraSceneModel;
    CameraField<float>(0x6BC64)=65535;CameraField<float>(0x9D170)=320;
    CameraField<float>(0x20C50C)=160;CameraField<float>(0xAD214)=100;
    for(int stage:{12,13})for(bool eye:{false,true})for(int owner:{-1,0,1}) {
        CameraField<int>(0x85A6B8)=stage;g_first_person.selected=eye;
        g_camera_cooperative=owner>=0;CameraField<int>(0x8745A0)=owner>=0?2:1;CameraPlayerScope scope(owner);
        const int before=clipped_scene_calls;project(&rail,0,1024);
        CHECK(clipped_scene_calls>before && projected_face_texture==-1 && projected_depth==1024 && projected_face_flags==0);
        CHECK(rail_faces[4]==0 && rail_faces[5]==0);
    }
    g_camera_cooperative=false;CameraField<int>(0x8745A0)=1;
    const int repaired=clipped_scene_calls;
    for(int excluded=0;excluded<5;++excluded) {
        g_camera_active=excluded!=0;g_free_orbit=excluded!=1;
        CameraField<int>(0x85A668)=excluded==2;CameraField<int>(0x85A6B8)=excluded==3?11:12;
        project(&rail,0,excluded==4?512:1024);CHECK(clipped_scene_calls==repaired && projected_face_texture==0);
    }
    VirtualFree(g_camera_model_trampoline,0,MEM_RELEASE);
    VirtualFree(reinterpret_cast<void*>(g_base), 0, MEM_RELEASE);
    printf("Camera runtime ownership and dispatch: %s\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
