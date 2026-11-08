#define FF_PATCH_VERSION "1.2.3-rc.4"
#include "../src/dinput_proxy.cpp"
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"ole32.lib")
static int failures,draws;
static bool focused=true;
static DWORD states[256];
static float label_right,label_bottom;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
static bool Focus(){return focused;}
static HRESULT STDMETHODCALLTYPE GetState(void*,D3DRENDERSTATETYPE state,DWORD* value){*value=states[state];return S_OK;}
static HRESULT STDMETHODCALLTYPE SetState(void*,D3DRENDERSTATETYPE state,DWORD value){states[state]=value;return S_OK;}
static HRESULT STDMETHODCALLTYPE Draw(void*,D3DPRIMITIVETYPE kind,D3DVERTEXTYPE type,void* data,DWORD count,DWORD) {
    CHECK(kind==D3DPT_TRIANGLELIST && type==D3DVT_TLVERTEX && count>12 && count%6==0);
    const auto vertices=static_cast<D3DTLVERTEX*>(data);
    label_right=vertices[5].sx;label_bottom=vertices[5].sy;
    for(unsigned i=0;i<count;++i) {
        const auto& v=vertices[i];
        CHECK(isfinite(v.sx) && isfinite(v.sy) && v.sx>=0 && v.sx<=CameraField<float>(0x9D170) &&
            v.sy>=0 && v.sy<=CameraField<float>(0x9D174));
    }
    ++draws;return E_FAIL; // State restoration must also survive a draw failure.
}
int main() {
    auto base=static_cast<BYTE*>(VirtualAlloc(nullptr,0x900000,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    CHECK(base);if(!base)return 1;g_base=reinterpret_cast<uintptr_t>(base);g_modern_focus=Focus;
    g_item_pickup_available=true;g_modern_keyboard=true;InputField<int>(0x8745A0)=1;
    InputField<int>(0x85A6B8)=0;InputField<int16_t>(0x86A542)=100;
    CameraField<float>(0xAD214)=CameraField<float>(0xAD234)=160;
    CameraField<float>(0x20C50C)=160;CameraField<float>(0x20C508)=120;
    CameraField<float>(0x9D170)=320;CameraField<float>(0x9D174)=240;
    InputField<int>(0x870620)=0x1F3;InputField<int32_t>(0x87062C)=100*65536;PublishItemPickup(0,0);
    RenderObserver record;void* original[33]={};record.original=original;
    original[23]=reinterpret_cast<void*>(SetState);original[29]=reinterpret_cast<void*>(Draw);
    record.table[22]=reinterpret_cast<void*>(GetState);void** table=record.table;void* object=&table;
    for(unsigned i=0;i<256;++i)states[i]=i+120;
    DrawItemPickupHint(object,true);CHECK(draws==1);
    for(unsigned i=0;i<256;++i)CHECK(states[i]==i+120);
    DrawItemPickupHint(object,false);CHECK(draws==1);
    focused=false;DrawItemPickupHint(object,true);CHECK(draws==1);focused=true;
    g_controls.separate_pickup=false;DrawItemPickupHint(object,true);CHECK(draws==1);g_controls.separate_pickup=true;
    InputField<int>(0x870620)=0x210;DrawItemPickupHint(object,true);CHECK(draws==1);
    PublishItemPickup(0,0);g_controls.players[0].keys[ff_controls::Pickup]=ff_controls::Mouse5;
    DrawItemPickupHint(object,true);CHECK(draws==2);
    // Near-plane/behind-camera items remain available at the lower screen edge.
    InputField<int32_t>(0x87062C)=0;DrawItemPickupHint(object,true);CHECK(draws==3);
    InputField<int32_t>(0x87062C)=-100*65536;DrawItemPickupHint(object,true);CHECK(draws==4);
    InputField<int32_t>(0x87062C)=100*65536;InputField<int32_t>(0x870624)=300*65536;
    DrawItemPickupHint(object,true);CHECK(draws==5);InputField<int32_t>(0x870624)=0;
    InputField<int>(0x8745A0)=2;InputField<int16_t>(0x86A542+0xF4)=100;PublishItemPickup(1,0);
    g_modern_coop_scheme=1;g_camera_owner=1;DrawItemPickupHint(object,true);CHECK(draws==6);
    g_camera_owner=0;DrawItemPickupHint(object,true);CHECK(draws==7);
    g_camera_owner=-1;DrawItemPickupHint(object,true);CHECK(draws==9);
    InputField<int16_t>(0x86A542)=0;DrawItemPickupHint(object,true);CHECK(draws==10);
    InputField<int>(0x85A6B8)=1;DrawItemPickupHint(object,true);CHECK(draws==10);
    InputField<int>(0x8745A0)=1;InputField<int16_t>(0x86A542)=100;g_camera_owner=-1;
    auto fixture=base+0x86DD20,source=base+0x80000;InputField<int>(0x8689E8)=1;
    GuardField<int16_t>(fixture,8)=9;GuardField<void*>(fixture,0x14)=source;
    GuardField<int32_t>(source,8)=-160*65536;GuardField<int32_t>(source,12)=100*65536;
    for(int type:{0x273,0x297,0x32D}) {
        GuardField<int>(source,0)=type;PublishItemPickup(0,-2,fixture);
        const int before=draws;DrawItemPickupHint(object,true);CHECK(draws==before+1);
        GuardField<int>(source,0)=type+1;DrawItemPickupHint(object,true);CHECK(draws==before+1);
    }
    GuardField<int>(source,0)=0x273;PublishItemPickup(0,-2,fixture);
    GuardField<int16_t>(fixture,8)=17;const int before=draws;DrawItemPickupHint(object,true);CHECK(draws==before);
    GuardField<int16_t>(fixture,8)=9;GuardField<void*>(fixture,0x14)=base+0x80040;
    GuardField<int>(base+0x80040,0)=0x273;DrawItemPickupHint(object,true);CHECK(draws==before);
    // The version badge is anchored to the actual render target and is only
    // visible on the main menu, even when other front-end pages share it.
    DrawGameOverlays(object);CHECK(draws==before);
    InputField<int>(0x85A6B8)=26;InputField<uintptr_t>(0x6C47C)=g_base+0x2F3B0;
    DrawGameOverlays(object);CHECK(draws==before+1 && label_right==312 && label_bottom==232);
    CameraField<float>(0x9D170)=2880;CameraField<float>(0x9D174)=1800;
    DrawGameOverlays(object);CHECK(draws==before+2 && label_right==2856 && label_bottom==1776);
    g_campaign_menu_active=true;DrawGameOverlays(object);CHECK(draws==before+2);g_campaign_menu_active=false;
    g_controls_menu_active=true;DrawGameOverlays(object);CHECK(draws==before+2);g_controls_menu_active=false;
    InputField<uintptr_t>(0x6C47C)=g_base+0x2FC30;DrawGameOverlays(object);CHECK(draws==before+2);
    for(char c:"v" FF_PATCH_VERSION)if(c)CHECK(OverlayGlyph(c)!=nullptr);
    for(unsigned i=0;i<256;++i)CHECK(states[i]==i+120);
    VirtualFree(base,0,MEM_RELEASE);puts(failures?"Item pickup overlay: FAIL":"Item pickup overlay: PASS");return failures?1:0;
}
