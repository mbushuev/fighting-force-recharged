#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static SHORT WINAPI NoPhysicalKeys(int){return 0;}
#define GetAsyncKeyState NoPhysicalKeys
#include "../src/dinput_proxy.cpp"
#undef GetAsyncKeyState
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"ole32.lib")
static int failures;
static bool focused=true;
static unsigned raw;
struct Line {std::string text;int y;BYTE color;};
static std::vector<Line> lines;
static DWORD states[256];static int panels,overlays;
static bool frontend_frame,world_flushed;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
static bool Focus(){return focused;}
static DWORD WINAPI NoPads(DWORD,XINPUT_STATE*){return ERROR_DEVICE_NOT_CONNECTED;}
static unsigned Read(bool gameplay){CHECK(gameplay==g_move_list.gameplay);return raw;}
static HRESULT STDMETHODCALLTYPE GetState(void*,D3DRENDERSTATETYPE state,DWORD* value){*value=states[state];return S_OK;}
static HRESULT STDMETHODCALLTYPE SetState(void*,D3DRENDERSTATETYPE state,DWORD value){states[state]=value;return S_OK;}
static HRESULT STDMETHODCALLTYPE Panel(void*,D3DPRIMITIVETYPE type,D3DVERTEXTYPE format,void* data,DWORD count,DWORD) {
    CHECK(type==D3DPT_TRIANGLELIST && format==D3DVT_TLVERTEX && count==6);++panels;
    const auto vertices=static_cast<D3DTLVERTEX*>(data);
    const DWORD color=frontend_frame?0xE0101820:0xFF101820;
    if(frontend_frame)CHECK(world_flushed && InputField<int>(0x1EC4DC)==1);
    for(unsigned i=0;i<count;++i)CHECK(vertices[i].sx>=0 && vertices[i].sx<=320 && vertices[i].sy>=0 && vertices[i].sy<=240 && vertices[i].color==color);
    CHECK(!states[D3DRENDERSTATE_ZENABLE] && !states[D3DRENDERSTATE_TEXTUREHANDLE]);return S_OK;
}
static void Overlay(void*,bool){++overlays;}
static int __cdecl Width(const char* text){int n=0;for(;*text;++text)n+=*text==' '?6:11;return n;}
static void __cdecl Draw(const char* text,int x,int y,const BYTE* rgb) {
    if(frontend_frame)CHECK(world_flushed && InputField<int>(0x1EC4DC)==1);
    if(x<0 || x+Width(text)>320)printf("Overflow: %s\n",text);
    CHECK(x>=0 && x+Width(text)<=320 && y>=0 && y<=226);
    CHECK(rgb[0]==248 || rgb[0]==15 || rgb[0]==252 || rgb[0]==6);
    CHECK(strchr(text,'+')==nullptr);
    for(int i=3;i<12;++i)CHECK(rgb[i]==rgb[i%3]);
    lines.push_back({text,y,rgb[0]});
}
static void __cdecl FlushWorld(){world_flushed=true;}
static void Branch(DWORD rva,void* target) {
    auto code=reinterpret_cast<BYTE*>(g_base+rva);code[0]=0xE9;
    const LONG delta=static_cast<LONG>(reinterpret_cast<BYTE*>(target)-(code+5));memcpy(code+1,&delta,4);
}
static bool Has(const char* text){for(const auto& line:lines)if(line.text==text)return true;return false;}
static void DrawList(){DrawMoveList();CHECK(g_combat_hide_overlays);}
static void Press(unsigned button){raw=0;PollMoveList();raw=button;PollMoveList();}
int main() {
    using namespace ff_moves;using namespace ff_controls;
    auto base=static_cast<BYTE*>(VirtualAlloc(nullptr,0x900000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));
    CHECK(base);if(!base)return 1;g_base=reinterpret_cast<uintptr_t>(base);
    Branch(0x4AEB0,reinterpret_cast<void*>(Width));Branch(0x44A20,reinterpret_cast<void*>(Draw));
    g_modern_focus=Focus;g_modern_xinput=NoPads;g_move_list_reader=Read;
    RenderObserver record;void* original[33]={};record.original=original;
    original[23]=reinterpret_cast<void*>(SetState);original[29]=reinterpret_cast<void*>(Panel);
    record.table[11]=reinterpret_cast<void*>(RenderEndScene);record.table[22]=reinterpret_cast<void*>(GetState);
    void** table=record.table;InputField<void*>(0xAD210)=&table;
    CameraField<float>(0x9D170)=320;CameraField<float>(0x9D174)=240;
    for(unsigned i=0;i<256;++i)states[i]=i+73;
    DrawMenuBackdrop(0xFF101820);CHECK(panels==1 && g_combat_hide_overlays);
    for(unsigned i=0;i<256;++i)CHECK(states[i]==i+73);
    g_combat_weapon_overlay=g_combat_throw_overlay=g_combat_pickup_overlay=Overlay;g_combat_world_drawn=true;
    DrawCombatOverlay(&table);CHECK(overlays==0 && !g_combat_hide_overlays && !g_combat_world_drawn);
    DrawCombatOverlay(&table);CHECK(overlays==3);
    g_combat_weapon_overlay=g_combat_throw_overlay=g_combat_pickup_overlay=nullptr;
    const BYTE frontend[]={0xFF,0x15,0x7C,0xC4,0x46,0};
    const BYTE flush[]={0xE8,0x93,0xF5,0xFF,0xFF};
    memcpy(base+0x2EBAF,frontend,sizeof(frontend));memcpy(base+0x2F091,frontend,sizeof(frontend));
    memcpy(base+0x258E8,flush,sizeof(flush));base[0x258ED]=0xC3;
    Branch(0x24E80,reinterpret_cast<void*>(FlushWorld));InstallControlMenu();
    OpenControlMenu(g_base+0x2FC30);g_control_menu.page=-3;OpenMoveList(false);raw=0;
    frontend_frame=true;ControlMenuFrame();CHECK(lines.empty() && panels==1 && !world_flushed);
    reinterpret_cast<void(__cdecl*)()>(g_base+0x258E8)();
    CHECK(world_flushed && panels==2 && Has("MOVE LIST") && InputField<int>(0x1EC4DC)==0);
    frontend_frame=false;CloseControlMenu();g_controls_wait_release=false;
    InputField<void*>(0xAD210)=nullptr;
    InputField<int>(0x8745A0)=1;GuardField<int16_t>(GuardActor(0),0x34)=3;
    GuardField<int16_t>(GuardActor(1),0x34)=2;
    GuardField<int16_t>(GuardActor(0),0x42)=GuardField<int16_t>(GuardActor(1),0x42)=100;
    OpenMoveList(true);CHECK(g_move_list.page==Page::Moves && g_move_list.hero[0]==3);
    // A connected idle pad does not change the solo player's current device.
    g_input_activity.pads[0].connected=true;g_input_activity.Use(ff_input::Device::Keyboard);
    CHECK(!MoveListUsePad(0));lines.clear();DrawList();CHECK(Has("MOUSE 1 REPEATEDLY"));
    g_input_activity.Use(ff_input::Device::Gamepad);CHECK(MoveListUsePad(0));
    lines.clear();DrawList();CHECK(Has("X REPEATEDLY"));
    g_input_activity.pads[0].connected=false;CHECK(!MoveListUsePad(0));
    // Cooperative pages retain their owner's layout regardless of who navigates.
    InputField<int>(0x8745A0)=2;OpenMoveList(true);CHECK(g_move_list.page==Page::Players);
    g_modern_coop_scheme=1;CHECK(!MoveListUsePad(0) && MoveListUsePad(1));
    g_modern_coop_scheme=3;CHECK(MoveListUsePad(0) && !MoveListUsePad(1));
    g_modern_coop_scheme=2;CHECK(MoveListUsePad(0) && MoveListUsePad(1));
    g_controls.players[1].pad[Hand]=ff_input::LeftShoulder;
    Press(ff_input::Down);Press(ff_input::Confirm);CHECK(g_move_list.page==Page::Moves && g_move_list.player==1);
    lines.clear();DrawList();CHECK(Has("LB REPEATEDLY") && Has("P2  SMASHER  GAMEPAD"));
    CHECK(!Has("X REPEATEDLY"));
    // The surviving second player keeps the correct character and bindings.
    GuardField<int16_t>(GuardActor(0),0x42)=0;OpenMoveList(true);
    CHECK(g_move_list.player==1 && g_move_list.hero[1]==2 && g_move_list.page==Page::Moves);
    lines.clear();DrawList();CHECK(Has("LB REPEATEDLY") && Has("P2  SMASHER  GAMEPAD"));
    Press(ff_input::Cancel);CHECK(g_move_list.page==Page::Closed);
    GuardField<int16_t>(GuardActor(0),0x42)=100;GuardField<int16_t>(GuardActor(1),0x42)=0;
    OpenMoveList(true);CHECK(g_move_list.player==0 && g_move_list.hero[0]==3 && g_move_list.page==Page::Moves);
    Press(ff_input::Cancel);CHECK(g_move_list.page==Page::Closed);
    OpenMoveList(false);CHECK(g_move_list.page==Page::Players);
    g_input_activity.Use(ff_input::Device::Keyboard);CHECK(!MoveListUsePad(1));
    g_input_activity.pads[0].connected=true;g_input_activity.Use(ff_input::Device::Gamepad);CHECK(MoveListUsePad(1));
    Press(ff_input::Down);Press(ff_input::Confirm);CHECK(g_move_list.page==Page::Heroes);
    Press(ff_input::Up);Press(ff_input::Up);Press(ff_input::Confirm);
    CHECK(g_move_list.page==Page::Moves && g_move_list.hero[1]==3);
    // Measure every selectable row with a conservative native-font width fixture.
    for(bool pad:{false,true})for(int hero=0;hero<4;++hero)for(int settings=0;settings<3;++settings) {
        g_controls=DefaultsSettings();g_controls.guard=settings!=0;g_controls.separate_pickup=settings!=1;
        g_input_activity.pads[0].connected=true;g_input_activity.Use(pad?ff_input::Device::Gamepad:ff_input::Device::Keyboard);g_move_list.hero[1]=hero;
        if(settings==2) {
            for(auto& binding:g_controls.players[1].keys)binding=Mouse5;
            g_controls.players[1].pad[Hand]=ff_input::LeftShoulder|ff_input::X;
            g_controls.players[1].pad[Jump]=ff_input::RightShoulder|ff_input::A;
            g_controls.players[1].pad[Foot]=0;
        }
        for(int group=0;group<int(Group::Count);++group) {
            g_move_list.group=Group(group);
            for(int row=0;row<Rows(Group(group));++row) {
                g_move_list.row=row;lines.clear();DrawList();
                CHECK(MoveListWrap(At(Group(group),row).note).size()<=2);
                int selected=0;for(const auto& line:lines)if(line.y>=61 && line.y<196 && line.color==15)++selected;
                CHECK(selected==1);
            }
        }
    }
    // Returning from a nested viewer never activates its parent with a held key.
    OpenControlMenu(g_base+0x2FC30);g_control_menu.page=-3;OpenMoveList(false);
    raw=0;PollControlMenu();raw=ff_input::Cancel;PollControlMenu();
    CHECK(!g_controls_menu_active && InputField<uintptr_t>(0x6C47C)==g_base+0x2FC30 && g_controls_wait_release);
    CHECK(!g_control_menu.save && !g_control_menu.close);
    OpenMoveList(false);raw=ff_input::Confirm;CHECK(PollMoveList() && g_move_list.page==Page::Players);
    raw=0;PollMoveList();raw=ff_input::Confirm;PollMoveList();CHECK(g_move_list.page==Page::Heroes);
    focused=false;raw=0;PollMoveList();focused=true;raw=ff_input::Cancel;
    PollMoveList();CHECK(g_move_list.page==Page::Heroes); // Require neutral after focus recovery.
    Press(ff_input::Cancel);CHECK(g_move_list.page==Page::Players);
    InputField<int>(0x85A6B8)=26;const int native_order[]={3,0,1,2};
    for(int i=0;i<4;++i)GuardField<int16_t>(GuardActor(i),0x34)=int16_t(native_order[i]);
    OpenMoveList(false);g_move_list.Accept();lines.clear();DrawList();
    for(int i=0;i<4;++i){CHECK(g_move_list.order[i]==native_order[i]);CHECK(lines[2+i].text==HeroName(native_order[i]));}
    g_move_list.row=2;g_move_list.Accept();CHECK(g_move_list.hero[0]==1);
    g_move_list.Back();CHECK(g_move_list.row==2);
    VirtualFree(base,0,MEM_RELEASE);
    puts(failures?"Move list runtime: FAIL":"Move list runtime: PASS");return failures?1:0;
}
