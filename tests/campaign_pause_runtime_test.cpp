#include "../src/dinput_proxy.cpp"
#include <fstream>
#include <iterator>
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"ole32.lib")
static int failures,frame,native_hud;
static bool world_flushed,panel_drawn,saw_list,saw_picker,saw_pause;
static std::vector<unsigned> script;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
static bool Focus(){return true;}
static unsigned Raw(bool pause){CHECK(pause);return frame<int(script.size())?script[frame]:0;}
static int __cdecl Open(int){return ff_input::Pause;}
static int __cdecl Zero(int){return 0;}
static void __cdecl Noop(){}
static int __cdecl Width(const char* text){int n=0;for(;*text;++text)n+=*text==' '?6:11;return n;}
static void __cdecl Text(const char* text,int x,int y,const BYTE*) {
    CHECK(world_flushed && panel_drawn && x>=0 && x+Width(text)<=320 && y>=0 && y<=226);
    CHECK(!strstr(text,"P1  PUNCH") && !strstr(text,"P2  PUNCH") && !strstr(text,"LOAD GAME USES"));
    saw_pause|=!strcmp(text,"PAUSED");saw_list|=!strcmp(text,"MOVE LIST") && g_move_list.page!=ff_moves::Page::Closed;
    saw_picker|=!strcmp(text,"CHOOSE PLAYER LAYOUT");
}
static HRESULT STDMETHODCALLTYPE GetState(void*,D3DRENDERSTATETYPE,DWORD* value){*value=0;return S_OK;}
static HRESULT STDMETHODCALLTYPE SetState(void*,D3DRENDERSTATETYPE,DWORD){return S_OK;}
static HRESULT STDMETHODCALLTYPE Panel(void*,D3DPRIMITIVETYPE type,D3DVERTEXTYPE format,void* vertices,DWORD count,DWORD) {
    CHECK(world_flushed && !panel_drawn && InputField<int>(0x1EC4DC)==1);
    CHECK(type==D3DPT_TRIANGLELIST && format==D3DVT_TLVERTEX && count==6);
    const DWORD color=g_move_list.page==ff_moves::Page::Closed?0x99000000:0xE0101820;
    CHECK(static_cast<D3DTLVERTEX*>(vertices)[0].color==color);
    panel_drawn=true;return S_OK;
}
static void __cdecl FlushWorld(){CHECK(!world_flushed);world_flushed=true;}
static void __cdecl NativeHud(){CHECK(world_flushed && !g_campaign_pause_present);++native_hud;}
static void __cdecl FinishPresent() {
    CHECK(world_flushed && InputField<int>(0x1EC4DC)==0);
    if(g_campaign_pause_present){CHECK(panel_drawn && g_combat_hide_overlays);DrawCombatOverlay(nullptr);}
    world_flushed=panel_drawn=false;
    if(++frame>80){puts("Native pause did not finish");ExitProcess(1);}
}
static void Branch(BYTE* base,DWORD rva,void* target,BYTE opcode=0xE9) {
    auto code=base+rva;code[0]=opcode;LONG delta=LONG(reinterpret_cast<BYTE*>(target)-code-5);memcpy(code+1,&delta,4);
}
static LONG CALLBACK Exception(EXCEPTION_POINTERS* e) {
    printf("Native pause exception %08lx at %08lx\n",e->ExceptionRecord->ExceptionCode,e->ContextRecord->Eip-DWORD(g_base));fflush(stdout);ExitProcess(1);
}
static void OriginalPause(BYTE* base,const char* executable) {
    std::ifstream file(executable,std::ios::binary);std::vector<BYTE> bytes((std::istreambuf_iterator<char>(file)),{});
    CHECK(bytes.size()>4096);if(bytes.size()<4096)return;
    const auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(bytes.data()+reinterpret_cast<const IMAGE_DOS_HEADER*>(bytes.data())->e_lfanew);
    const auto sections=IMAGE_FIRST_SECTION(nt);bool found=false;
    for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i)if(0x4AA50>=sections[i].VirtualAddress &&
        0x4AEAD<=sections[i].VirtualAddress+sections[i].SizeOfRawData) {
        memcpy(base+0x4AA50,bytes.data()+sections[i].PointerToRawData+0x4AA50-sections[i].VirtualAddress,0x45D);found=true;
    }
    CHECK(found);if(!found)return;
    // Replay the original presentation through the world flush and HUD call.
    found=false;
    for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i)if(0x25880>=sections[i].VirtualAddress &&
        0x2591E<=sections[i].VirtualAddress+sections[i].SizeOfRawData) {
        memcpy(base+0x25880,bytes.data()+sections[i].PointerToRawData+0x25880-sections[i].VirtualAddress,0x2591E-0x25880);found=true;
    }
    CHECK(found);if(!found)return;
    const DWORD addresses[]={0x4aa7c,0x4aa8b,0x4aa93,0x4aaa4,0x4aaab,0x4aab1,0x4aabb,0x4aac3,0x4aacb,
        0x4aad2,0x4aae0,0x4aaeb,0x4aaf5,0x4ab13,0x4ab22,0x4ab39,0x4ab42,0x4ab5f,0x4ab95,0x4aba5,
        0x4abd9,0x4abed,0x4ac00,0x4ac1c,0x4ac23,0x4ac29,0x4ac5d,0x4ac6d,0x4acab,0x4acec,0x4acfe,
        0x4ad15,0x4ad23,0x4ad34,0x4ad4e,0x4ad5a,0x4ad60,0x4ad6f,0x4ada2,0x4adb2,0x4adb8,0x4adc2,
        0x4adc8,0x4add0,0x4add6,0x4adde,0x4adf6,0x4ae0f,0x4ae29,0x4ae34,0x4ae3a,0x4ae49,0x4ae7c,
        0x4ae8c,0x4ae92,0x4ae9c,
        0x25881,0x2589a,0x2589f,0x258a8,0x258ba,0x258e2,0x258f3,0x25900,0x25914,0x2591a};
    for(DWORD rva:addresses){DWORD address;memcpy(&address,base+rva,4);address=DWORD(g_base)+address-0x400000;memcpy(base+rva,&address,4);}
    for(DWORD rva:{0x280E0u,0x28170u,0x47A10u,0x43990u,0x3C100u,0x43A20u,0x3C180u})
        Branch(base,rva,reinterpret_cast<void*>(Noop));
    Branch(base,0x22BC0,reinterpret_cast<void*>(Zero));
    Branch(base,0x24BC0,reinterpret_cast<void*>(Noop));Branch(base,0x24E80,reinterpret_cast<void*>(FlushWorld));
    Branch(base,0x49AD0,reinterpret_cast<void*>(NativeHud));
    Branch(base,0x2590E,reinterpret_cast<void*>(CampaignPauseHud),0xE8);
    Branch(base,0x2591E,reinterpret_cast<void*>(FinishPresent),0xE8);
    const BYTE present_return[]={0x5F,0x5E,0x5D,0x5B,0x83,0xC4,0x20,0xC3};memcpy(base+0x25923,present_return,sizeof(present_return));
    Branch(base,0x4AEB0,reinterpret_cast<void*>(Width));Branch(base,0x44A20,reinterpret_cast<void*>(Text));
    Branch(base,0x4AA6A,reinterpret_cast<void*>(Open),0xE8);Branch(base,0x4AA75,reinterpret_cast<void*>(Zero),0xE8);
    Branch(base,0x4AB00,reinterpret_cast<void*>(ModernPauseRead<true,true>),0xE8);
    Branch(base,0x4AB0C,reinterpret_cast<void*>(ModernPauseRead<false,true>),0xE8);
    Branch(base,0x4AC0C,reinterpret_cast<void*>(CampaignPauseChoice),0xE8);
    Branch(base,0x4AC7D,reinterpret_cast<void*>(CampaignPauseChoice),0xE8);
    Branch(base,0x4ACC5,reinterpret_cast<void*>(PauseInputHints),0xE8);
    Branch(base,0x4ACCE,reinterpret_cast<void*>(CampaignPausePresent),0xE8);
    RenderObserver record;void* original[33]={};record.original=original;
    original[23]=reinterpret_cast<void*>(SetState);original[29]=reinterpret_cast<void*>(Panel);
    record.table[11]=reinterpret_cast<void*>(RenderEndScene);record.table[22]=reinterpret_cast<void*>(GetState);
    void** device=record.table;InputField<void*>(0xAD210)=&device;
    CameraField<float>(0x9D170)=320;CameraField<float>(0x9D174)=240;
    InputField<const char*>(0x55225C)="CONTINUE";InputField<const char*>(0x552260)="QUIT";
    InputField<const char*>(0x552250)="PAUSED";
    using namespace ff_input;
    const struct {std::vector<unsigned> input;int result;bool restart;} cases[]={
        {{0,Confirm},0,false},{{0,Down,0,Down,0,Cancel},0,false},{{0,Up,0,Confirm},1,false},
        {{0,Down,0,Down,0,Confirm,0,Down,0,Confirm},1,true},{{0,Down,0,Down,0,Confirm,0,Cancel,0,Cancel},0,false},
        {{Confirm,Confirm,0,Confirm},0,false}};
    for(const auto& c:cases)for(int players:{1,2}) {
        script=c.input;frame=0;InputField<int>(0x5523A8)=0;g_campaign_restart=false;
        InputField<int>(0x8745A0)=players;InputField<int>(0x85A6BC)=players-1;
        CHECK(CampaignPause()==c.result && g_campaign_restart==c.restart);
        CHECK(!g_campaign_pause_reader && !g_campaign_pause_draw && frame>=2);
    }
    // Browsing a move list and returning through its player selector keeps the
    // original pause loop alive until a separate resume command is pressed.
    for(int players:{1,2}) {
        script={0,Down,0,Confirm,0};
        if(players==2)script.insert(script.end(),{Confirm,0});
        script.insert(script.end(),{Right,0,Cancel,0});
        if(players==2)script.insert(script.end(),{Cancel,0});
        script.push_back(Cancel);frame=0;g_campaign_restart=false;
        InputField<int>(0x5523A8)=0;InputField<int>(0x8745A0)=players;InputField<int>(0x85A6BC)=players-1;
        CHECK(CampaignPause()==0 && !g_campaign_restart && frame>=int(script.size()));
        CHECK(g_move_list.page==ff_moves::Page::Closed && !g_campaign_pause_reader && !g_campaign_pause_draw);
    }
    for(unsigned survivor:{0u,1u}) {
        CameraField<int16_t>(0x86A542)=survivor==0?100:0;
        CameraField<int16_t>(0x86A636)=survivor==1?100:0;
        script={0,Down,0,Confirm,0,Cancel,0,Cancel};frame=0;
        InputField<int>(0x5523A8)=0;g_campaign_restart=false;
        saw_list=saw_picker=saw_pause=false;
        CHECK(CampaignPause()==0 && frame==int(script.size()));
        CHECK(saw_list && !saw_picker && saw_pause && !g_campaign_pause_present && native_hud==0);
    }
    FriendlyFireSetting()=1;
    script={0,Up,0,Up,0,Confirm,Confirm,0,Cancel};frame=0;
    InputField<int>(0x5523A8)=0;
    CHECK(CampaignPause()==0 && frame==int(script.size()) && FriendlyFireSetting()==0);
    reinterpret_cast<void(__cdecl*)(int)>(g_base+0x25880)(1);CHECK(native_hud==1);
    InputField<void*>(0xAD210)=nullptr;
}
int main(int argc,char** argv) {
    auto base=static_cast<BYTE*>(VirtualAlloc(nullptr,0x900000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));
    CHECK(base);if(!base)return 1;g_base=reinterpret_cast<uintptr_t>(base);g_modern_focus=Focus;g_campaign_raw_reader=Raw;g_move_list_reader=Raw;
    g_campaign_entry_valid=true;g_campaign_session_slot=0;g_campaign_entry.stage=1;
    InputField<int>(0x85A6B8)=1;InputField<int>(0x6F000)=-1;
    if(argc>1){AddVectoredExceptionHandler(1,Exception);OriginalPause(base,argv[1]);}
    else {
        g_campaign_pause={};g_campaign_pause.row=3;g_campaign_pause_nav.Reset();script={0,ff_input::Cancel};frame=0;
        CampaignPauseRead();frame=1;CHECK(CampaignPauseRead()==(ff_input::Left|ff_input::Confirm));
        CHECK(CampaignPauseRead()==(ff_input::Left|ff_input::Confirm)); // Both native device reads agree.
    }
    VirtualFree(base,0,MEM_RELEASE);puts(failures?"Campaign native pause: FAIL":"Campaign native pause: PASS");return failures?1:0;
}
