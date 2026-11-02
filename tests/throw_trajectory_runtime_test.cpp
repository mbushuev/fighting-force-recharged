#include "../src/dinput_proxy.cpp"
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"ole32.lib")
static int failures,draws;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
static bool Focus(){return true;}
static float matrix[12]={1,0,0,20,0,1,0,-160,0,0,1,50};
static const float* __cdecl Matrix(){return matrix;}
static int __cdecl HeldNative(const FirstPersonPropModel*,int,int){return 73;}
static void Jump(DWORD rva,void* target) {
    auto code=reinterpret_cast<BYTE*>(g_base+rva);code[0]=0xE9;
    const LONG delta=static_cast<LONG>(reinterpret_cast<uintptr_t>(target)-reinterpret_cast<uintptr_t>(code+5));memcpy(code+1,&delta,4);
}
static DWORD states[256];
static HRESULT STDMETHODCALLTYPE GetState(void*,D3DRENDERSTATETYPE state,DWORD* value){*value=states[state];return S_OK;}
static HRESULT STDMETHODCALLTYPE SetState(void*,D3DRENDERSTATETYPE state,DWORD value){states[state]=value;return S_OK;}
static HRESULT STDMETHODCALLTYPE Draw(void*,D3DPRIMITIVETYPE,D3DVERTEXTYPE,void* data,DWORD count,DWORD) {
    for(unsigned i=0;i<count;++i){const auto v=static_cast<D3DTLVERTEX*>(data)[i];CHECK(isfinite(v.sx) && isfinite(v.sy));}
    ++draws;return S_OK;
}
int main() {
    auto memory=static_cast<BYTE*>(VirtualAlloc(nullptr,0x900000,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    CHECK(memory);if(!memory)return 1;g_base=reinterpret_cast<uintptr_t>(memory);g_modern_focus=Focus;
    g_weapon_laser_available=true;InstallThrowTrajectory();CHECK(!g_throw_trajectory_available);
    const BYTE gravity[]={0x66,0x83,0x46,0x20,0xF0,0x66,0x8B,0x7E,0x1A,0x0F,0xBF,0x56,0x20};
    const BYTE drag[]={0x66,0x83,0x46,0x18,0xFD};
    const BYTE heavy[]={0x66,0xC7,0x87,0x40,0x06,0xC7,0,0x20,0},light[]={0x66,0xC7,0x87,0x40,0x06,0xC7,0,0x40,0};
    memcpy(memory+0xA7AC,gravity,sizeof(gravity));memcpy(memory+0xAA22,drag,sizeof(drag));
    memcpy(memory+0x51694,heavy,sizeof(heavy));memcpy(memory+0x515FD,light,sizeof(light));
    InstallThrowTrajectory();CHECK(g_throw_trajectory_available && g_held_weapon_pose==CaptureHeldAim);
    Jump(0x1980,reinterpret_cast<void*>(Matrix));Jump(0x26DE0,reinterpret_cast<void*>(HeldNative));
    g_camera_active=g_free_orbit=true;CameraField<int>(0x8745A0)=1;CameraField<int16_t>(0x86A542)=100;
    CameraField<float>(0xAD214)=CameraField<float>(0xAD234)=160;
    CameraField<float>(0x20C50C)=160;CameraField<float>(0x20C508)=120;
    CameraField<float>(0x9D170)=320;CameraField<float>(0x9D174)=240;
    for(unsigned p=0;p<8;++p)CameraField<int16_t>(0x86FEB4+p*0xF4)=-1;
    CameraField<int16_t>(0x86FEB4)=0;CameraField<int16_t>(0x86FEC2)=100;
    CameraField<int8_t>(0x86FEFE)=3;CameraField<int8_t>(0x86FEFD)=-1;
    CameraField<int>(0x870620+3*36)=0x210;CameraField<int16_t>(0x68350+1024*2)=-32767;
    CameraField<int16_t>(0x86FEB8)=1024;
    FirstPersonPropModel model={};const uintptr_t actor=g_base+0x86FEE2;
    CaptureHeldAim(&model,actor,0x210,1);auto path=g_throw_frames[0].path;
    CHECK(path.count>2 && path.hit && path.points[0].x==20 && path.points[0].y==-160 && path.points[1].z>113);
    CHECK(path.points[1].y==-168 && CameraField<int8_t>(0x86FEFE)==3 && CameraField<int>(0x870620+3*36)==0x210);
    g_throw_frames[0]={};g_controls.trajectory=false;CaptureHeldAim(&model,actor,0x210,1);CHECK(!g_throw_frames[0].path.count);
    g_controls.trajectory=true;CaptureHeldAim(&model,actor,0x1F3,1);CHECK(!g_throw_frames[0].path.count);
    {CameraPlayerScope owner(1);CaptureHeldAim(&model,actor,0x210,1);CHECK(!g_throw_frames[0].path.count);}
    // Current render matrix and owner, independent of camera orientation.
    matrix[3]=70;CaptureHeldAim(&model,actor,0x210,1);CHECK(g_throw_frames[0].path.points[0].x==70);
    g_camera_geometry_stage=0;g_camera_geometry_stamp=GetTickCount64();
    g_camera_triangles={{{-500,-500,150},{500,-500,150},{0,500,150}}};
    CaptureHeldAim(&model,actor,0x210,1);CHECK(fabs(g_throw_frames[0].path.points[g_throw_frames[0].path.count-1].z-150)<0.001);
    g_camera_triangles.clear();
    // A grenade uses the higher native throw, and owner 1 retains its own preview.
    CameraField<int>(0x870620+3*36)=0x1F0;CaptureHeldAim(&model,actor,0x1F0,1);
    CHECK(g_throw_frames[0].path.points[1].y==-184);CameraField<int>(0x870620+3*36)=0x210;
    CameraField<int16_t>(0x86FEB4+0xF4)=1;CameraField<int16_t>(0x86FEC2+0xF4)=100;
    CameraField<int8_t>(0x86FEFE+0xF4)=4;CameraField<int>(0x870620+4*36)=0x210;
    {CameraPlayerScope owner(1);CaptureHeldAim(&model,actor+0xF4,0x210,1);CHECK(g_throw_frames[1].path.count>2);}
    CameraField<int16_t>(0x86FEB4+0xF4)=-1;g_throw_frames[1]={};
    RenderObserver record;void* original[33]={};record.original=original;
    original[23]=reinterpret_cast<void*>(SetState);original[29]=reinterpret_cast<void*>(Draw);
    record.table[22]=reinterpret_cast<void*>(GetState);void** table=record.table;void* object=&table;
    for(unsigned i=0;i<256;++i)states[i]=i+90;
    g_combat_world_drawn=true;DrawCombatOverlay(object);CHECK(draws>0 && !g_throw_frames[0].path.count);
    for(unsigned i=0;i<256;++i)CHECK(states[i]==i+90);
    const int previous=draws;g_combat_world_drawn=true;DrawCombatOverlay(object);CHECK(draws==previous);
    CaptureHeldAim(&model,actor,0x210,1);DrawCombatOverlay(object);CHECK(draws==previous && !g_throw_frames[0].path.count);
    // Exercise the actual reviewed right-hand thunk with a heavy prop.
    const auto entry=FirstPersonHeldRight;const auto pointer=&model;int result;
    __asm {
        push esi
        push ebx
        mov esi,actor
        mov ebx,210h
        push 0
        push 0
        push pointer
        call entry
        add esp,12
        mov result,eax
        pop ebx
        pop esi
    }
    CHECK(result==73 && g_throw_frames[0].path.count>2);
    VirtualFree(memory,0,MEM_RELEASE);puts(failures?"Throw trajectory native adapter: FAIL":"Throw trajectory native adapter: PASS");return failures?1:0;
}
