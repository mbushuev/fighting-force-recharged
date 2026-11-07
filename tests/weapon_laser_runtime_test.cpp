#include "../src/dinput_proxy.cpp"
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"ole32.lib")
static int failures;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
static bool Focus(){return true;}
static int aim_segments;
static HRESULT STDMETHODCALLTYPE GetState(void*,D3DRENDERSTATETYPE,DWORD* value){*value=0;return S_OK;}
static HRESULT STDMETHODCALLTYPE SetState(void*,D3DRENDERSTATETYPE,DWORD){return S_OK;}
static HRESULT STDMETHODCALLTYPE Draw(void*,D3DPRIMITIVETYPE,D3DVERTEXTYPE,void* vertices,DWORD count,DWORD) {
    if(count==6 && static_cast<D3DTLVERTEX*>(vertices)[0].color==ff_laser::Color(true))++aim_segments;
    return S_OK;
}
static float weapon_matrix[12]={1,0,0,0,0,1,0,-200,0,0,1,50};
static void Jump(DWORD rva,void* target) {
    BYTE* code=reinterpret_cast<BYTE*>(g_base+rva);code[0]=0xE9;
    const LONG delta=static_cast<LONG>(reinterpret_cast<uintptr_t>(target)-reinterpret_cast<uintptr_t>(code+5));memcpy(code+1,&delta,4);
}
static const float* __cdecl Matrix(){return weapon_matrix;}
static int __cdecl HeldNative(const FirstPersonPropModel*,int flags,int depth){CHECK(flags==7 && depth==0);return 91;}
int main() {
    auto memory=static_cast<BYTE*>(VirtualAlloc(nullptr,0x900000,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    CHECK(memory);if(!memory)return 1;g_base=reinterpret_cast<uintptr_t>(memory);g_modern_focus=Focus;
    CameraField<int>(0x85A6B8)=0;CameraField<int>(0x8745A0)=1;CameraField<int16_t>(0x86A542)=100;
    g_camera_geometry_stage=0;g_camera_geometry_stamp=GetTickCount64();g_camera_active=g_free_orbit=true;
    InstallWeaponLaser();CHECK(!g_weapon_laser_available);
    const BYTE actor_signature[]={0xBE,0xE2,0xFE,0xC6,0};
    const BYTE right[]={0x8B,0x04,0x9D,0xF8,0xA6,0xC5,0,0x6A,0,0x6A,0,0x50};
    memcpy(memory+0x43F88,actor_signature,sizeof(actor_signature));memcpy(memory+0x44263,right,sizeof(right));
    InstallWeaponLaser();CHECK(g_weapon_laser_available);
    Jump(0x1980,reinterpret_cast<void*>(Matrix));g_first_person_available=true;g_first_person.selected=true;
    CameraField<int8_t>(0x86FEFD)=-1;CameraField<int8_t>(0x86FEFE)=0;
    CameraField<int>(0x870620)=0x1F3;CameraField<int16_t>(0x870636)=4;
    const int16_t vertices[]={-4,13,41,0,4,13,41,0,4,34,41,0,-4,34,41,0,0,-10,-30,0};
    FirstPersonPropModel model={vertices,5,nullptr,0};
    CaptureWeaponLaser(&model,g_base+0x86FEE2,0x1F3,1);
    CHECK(g_weapon_laser_frame.active && g_weapon_laser_frame.loaded);
    CHECK(fabs(g_weapon_laser_frame.start.z-91)<0.01 && fabs(g_weapon_laser_frame.start.y+176.5)<0.01);
    const auto old=g_weapon_laser_frame;
    weapon_matrix[6]=0.8f;weapon_matrix[10]=0.6f;
    CaptureWeaponLaser(&model,g_base+0x86FEE2,0x1F3,1);
    CHECK(g_weapon_laser_frame.end.y>old.end.y+1000);
    CameraField<int16_t>(0x870636)=0;CaptureWeaponLaser(&model,g_base+0x86FEE2,0x1F3,1);
    CHECK(!g_weapon_laser_frame.loaded && ff_laser::Color(false)==0xFFFF4040 && ff_laser::Color(true)==0xFF4098FF);
    g_weapon_laser_frame={};CaptureWeaponLaser(&model,g_base+0x86FEE2+0xF4,0x1F3,1);
    CHECK(!g_weapon_laser_frame.active);
    CaptureWeaponLaser(&model,g_base+0x86FEE2,0x1F3,1);CHECK(g_weapon_laser_frame.active);
    Jump(0x26DE0,reinterpret_cast<void*>(HeldNative));
    const auto entry=FirstPersonHeldRight;const auto model_pointer=&model;const auto owner=g_base+0x86FEE2;int native_result;
    g_weapon_laser_frame={};
    __asm {
        push esi
        push ebx
        mov esi,owner
        mov ebx,1F3h
        push 0
        push 7
        push model_pointer
        call entry
        add esp,12
        mov native_result,eax
        pop ebx
        pop esi
    }
    CHECK(native_result==91 && g_weapon_laser_frame.active);
    g_combat_world_drawn=false;DrawCombatOverlay(nullptr);CHECK(!g_weapon_laser_frame.active);
    for(int type:{0x1F9,0x1FA,0x1FB,0x203}) {
        CameraField<int>(0x870620)=type;CameraField<int16_t>(0x870636)=4;
        CaptureWeaponLaser(&model,g_base+0x86FEE2,type,1);
        CHECK(g_weapon_laser_frame.active && g_weapon_laser_frame.loaded && g_weapon_laser_frame.launcher==(type!=0x203));
    }
    CameraField<int>(0x870620)=0x1FA;weapon_matrix[6]=0;weapon_matrix[10]=1;
    CameraField<float>(0x9D170)=320;CameraField<float>(0x9D174)=240;
    CameraField<float>(0x20C50C)=160;CameraField<float>(0x20C508)=120;
    CameraField<float>(0xAD214)=CameraField<float>(0xAD234)=160;
    RenderObserver record;void* original[33]={};record.original=original;
    original[23]=reinterpret_cast<void*>(SetState);original[29]=reinterpret_cast<void*>(Draw);
    record.table[22]=reinterpret_cast<void*>(GetState);void** device=record.table;
    CaptureWeaponLaser(&model,g_base+0x86FEE2,0x1FA,1);
    DrawWeaponLaser(&device,true);CHECK(aim_segments==4 && !g_weapon_laser_frame.active);
    const ff_camera::Point origin={0,-100,0},delta={0,0,1000};
    CHECK(fabs(ff_laser::Box(origin,delta,{-10,-200,300},{10,0,400},1)-0.3)<0.001);
    double ax=160,ay=120,bx=160,by=3000;
    CHECK(ff_laser::ClipLine(ax,ay,bx,by,320,240) && by==239 && ay==120);
    ff_camera::Point near_a={0,0,-50},near_b={0,0,500};
    CHECK(ff_laser::ClipBeam(near_a,near_b,{}) && fabs(near_a.z-33)<0.001);
    near_a={0,0,-100};near_b={0,0,-10};CHECK(!ff_laser::ClipBeam(near_a,near_b,{}));
    VirtualFree(memory,0,MEM_RELEASE);
    puts(failures?"Weapon laser native adapter: FAIL":"Weapon laser native adapter: PASS");return failures?1:0;
}
