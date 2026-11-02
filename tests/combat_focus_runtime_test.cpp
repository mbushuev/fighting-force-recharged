#include "../src/dinput_proxy.cpp"
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"ole32.lib")
static int failures;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
static bool Focus(){return true;}
static int draws,fail_state=-1;
static DWORD render_states[256];
static HRESULT STDMETHODCALLTYPE GetState(void*,D3DRENDERSTATETYPE state,DWORD* value) {
    if(int(state)==fail_state)return E_FAIL;*value=render_states[state];return S_OK;
}
static HRESULT STDMETHODCALLTYPE SetState(void*,D3DRENDERSTATETYPE state,DWORD value){render_states[state]=value;return S_OK;}
static HRESULT STDMETHODCALLTYPE Draw(void*,D3DPRIMITIVETYPE kind,D3DVERTEXTYPE type,void* data,DWORD count,DWORD) {
    CHECK(kind==D3DPT_TRIANGLELIST && type==D3DVT_TLVERTEX && count==6);
    for(unsigned i=0;i<count;++i){const auto v=static_cast<D3DTLVERTEX*>(data)[i];CHECK(isfinite(v.sx) && isfinite(v.sy));}
    ++draws;return E_FAIL;
}
int main() {
    auto memory=static_cast<BYTE*>(VirtualAlloc(nullptr,0x900000,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    CHECK(memory);if(!memory)return 1;g_base=reinterpret_cast<uintptr_t>(memory);g_modern_focus=Focus;
    CameraField<int>(0x85A6B8)=0;CameraField<int>(0x8745A0)=1;CameraField<int16_t>(0x86A542)=100;
    CameraField<float>(0xAD214)=CameraField<float>(0xAD234)=160;
    CameraField<float>(0x9D170)=320;CameraField<float>(0x9D174)=240;
    g_camera_geometry_stage=0;g_camera_geometry_stamp=GetTickCount64();
    g_camera_active=g_free_orbit=true;g_camera_settings.height=160;g_camera_orbit.Begin(0,g_camera_settings);
    auto enemy=[&](int slot,int x,int z) {
        const unsigned o=slot*0xF4;CameraField<uintptr_t>(0x86A500+o)=1;
        CameraField<int16_t>(0x86A534+o)=4;CameraField<int16_t>(0x86A542+o)=100;
        CameraField<int32_t>(0x86A528+o)=x*65536;CameraField<int32_t>(0x86A530+o)=z*65536;
    };
    enemy(2,0,600);enemy(3,300,600);
    auto input=[&](bool down=false,double axis=0) {
        CombatFocusInput(0,g_camera_orbit,g_first_person,down,down,axis,false,false,1.0/30,true);
    };
    input();input(true);CHECK(g_focus_states[0].target==2 && CombatFocusActive());
    input();input(false,1);CHECK(g_focus_states[0].target==3);
    CameraField<int>(0x86A50C)=0x45;FirstPersonAlignBody();CHECK(CameraField<int16_t>(0x86A538)!=0);
    CameraField<int>(0x86A50C)=0x62;CameraField<int16_t>(0x86A538)=123;FirstPersonAlignBody();
    CHECK(CameraField<int16_t>(0x86A538)==123);
    {CameraPlayerScope owner(1);CHECK(!CombatFocusActive());}
    g_controls.marker=false;CHECK(CombatFocusActive());g_controls.marker=true;
    CameraField<int16_t>(0x86A542+3*0xF4)=0;input();CHECK(!CombatFocusActive());
    input(true);CHECK(CombatFocusActive());GameFocusLost();CHECK(!CombatFocusActive() && g_focus_visual.slot[0]<0);
    FirstPersonModel model={};model.vertex_count=5;
    g_focus_visual.slot[0]=2;
    const float lying[5][2]={{20,80},{140,80},{20,105},{140,105},{10000,10000}};
    for(unsigned i=0;i<5;++i) {
        memcpy(memory+0x9D208+i*32,lying[i],8);CameraField<DWORD>(0x9D208+i*32+0x1C)=i==4?0x10:0;
    }
    CaptureCombatBounds(&model,memory+0x8623E0+2*0x238);
    CHECK(g_combat_actor_bounds.active && g_combat_actor_bounds.left==20 && g_combat_actor_bounds.right==140 &&
        g_combat_actor_bounds.top==80 && g_combat_actor_bounds.bottom==105);
    RenderObserver record;void* original[33]={};record.original=original;
    original[23]=reinterpret_cast<void*>(SetState);original[29]=reinterpret_cast<void*>(Draw);
    record.table[22]=reinterpret_cast<void*>(GetState);void** table=record.table;void* object=&table;
    for(unsigned i=0;i<256;++i)render_states[i]=i+100;
    {CombatOverlayDraw draw(object);CHECK(draw.ready);draw.Frame(20,30,140,210,320,240,1);}
    CHECK(draws==16);for(unsigned i=0;i<256;++i)CHECK(render_states[i]==i+100);
    fail_state=D3DRENDERSTATE_CULLMODE;
    {CombatOverlayDraw draw(object);CHECK(!draw.ready);draw.Frame(20,30,140,210,320,240,1);}
    CHECK(draws==16);for(unsigned i=0;i<256;++i)CHECK(render_states[i]==i+100);
    VirtualFree(memory,0,MEM_RELEASE);
    puts(failures?"Combat focus native adapters: FAIL":"Combat focus native adapters: PASS");return failures?1:0;
}
