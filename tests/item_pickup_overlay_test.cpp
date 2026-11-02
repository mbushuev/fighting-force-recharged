#include "../src/dinput_proxy.cpp"
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"ole32.lib")
static int failures,draws;
static bool focused=true;
static DWORD states[256];
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
static bool Focus(){return focused;}
static HRESULT STDMETHODCALLTYPE GetState(void*,D3DRENDERSTATETYPE state,DWORD* value){*value=states[state];return S_OK;}
static HRESULT STDMETHODCALLTYPE SetState(void*,D3DRENDERSTATETYPE state,DWORD value){states[state]=value;return S_OK;}
static HRESULT STDMETHODCALLTYPE Draw(void*,D3DPRIMITIVETYPE kind,D3DVERTEXTYPE type,void* data,DWORD count,DWORD) {
    CHECK(kind==D3DPT_TRIANGLELIST && type==D3DVT_TLVERTEX && count>12 && count%6==0);
    for(unsigned i=0;i<count;++i){const auto& v=static_cast<D3DTLVERTEX*>(data)[i];CHECK(isfinite(v.sx) && isfinite(v.sy) && v.sx>=0 && v.sx<=320 && v.sy>=0 && v.sy<=240);}
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
    for(unsigned i=0;i<256;++i)CHECK(states[i]==i+120);
    VirtualFree(base,0,MEM_RELEASE);puts(failures?"Item pickup overlay: FAIL":"Item pickup overlay: PASS");return failures?1:0;
}
