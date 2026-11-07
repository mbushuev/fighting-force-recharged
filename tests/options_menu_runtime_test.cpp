#include "../src/dinput_proxy.cpp"
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"ole32.lib")
static int failures,native_calls;
static bool focus=true;
static unsigned buttons[2];
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
static bool Focus(){return focus;}
static void Jump(DWORD rva,void* target) {
    auto code=reinterpret_cast<BYTE*>(g_base+rva);code[0]=0xE9;
    const LONG offset=static_cast<LONG>(reinterpret_cast<uintptr_t>(target)-reinterpret_cast<uintptr_t>(code+5));memcpy(code+1,&offset,4);
}
static const char* NativeLabel(DWORD rva=0x2FFCE) {
    const auto address=g_base+rva;const char* result;
    __asm {
        push edi
        call address
        mov result,edi
        pop edi
    }
    return result;
}
static void __cdecl NativeOptions() {
    ++native_calls;
    InputField<int>(0x6C4F8)=999;InputField<int>(0x6C50C)=6;InputField<int>(0x6C510)=7;
    reinterpret_cast<void(__cdecl*)()>(g_base+0x2FD6A)();
    CHECK(InputField<int>(0x6C50C)==8 && InputField<int>(0x6C510)==9);
    CHECK(!strcmp(NativeLabel(),"GAMEPLAY SETTINGS"));
    CHECK(!strcmp(NativeLabel(0x2FFE1),"MOVE LIST"));
    CHECK(!strcmp(NativeLabel(0x30031),"FRIENDLY FIRE"));
    InputField<unsigned>(0x5522E8)=buttons[0];InputField<unsigned>(0x5522EC)=buttons[1];
}
static void Frame(unsigned first,unsigned second=0){buttons[0]=first;buttons[1]=second;ControlFrontend();}
int main() {
    auto base=static_cast<BYTE*>(VirtualAlloc(nullptr,0x900000,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    CHECK(base);if(!base)return 1;g_base=reinterpret_cast<uintptr_t>(base);g_modern_focus=Focus;
    const BYTE setup[]={0xA1,0x44,0x23,0x95,0},label[]={0x8B,0x3D,0x90,0x22,0x95,0};
    const BYTE unused[]={0x3B,0x05,0xF8,0xC4,0x46,0};
    memcpy(base+0x2FD6A,setup,sizeof(setup));memcpy(base+0x2FFCE,label,sizeof(label));
    CHECK(!InstallPlayerGuardOption());
    memcpy(base+0x304E1,unused,sizeof(unused));CHECK(!InstallPlayerGuardOption());
    const BYTE moves_label[]={0x8B,0x3D,0x38,0x22,0x95,0},moves_unused[]={0x3B,0x05,0xFC,0xC4,0x46,0};
    memcpy(base+0x2FFE1,moves_label,sizeof(moves_label));memcpy(base+0x304FB,moves_unused,sizeof(moves_unused));
    const BYTE friendly[]={0x8B,0x3D,0xB0,0x22,0x95,0};
    const BYTE title[]={0x6A,0x14},anchor[]={0xBB,0x80,0,0,0},rows[]={0x8D,0x0C,0x40,0xC1,0xE1,0x02};
    memcpy(base+0x30031,friendly,sizeof(friendly));memcpy(base+0x2FE34,title,sizeof(title));
    memcpy(base+0x2FE41,anchor,sizeof(anchor));memcpy(base+0x2FE4B,rows,sizeof(rows));
    CHECK(InstallPlayerGuardOption());base[0x2FD6F]=base[0x2FFD4]=base[0x2FFE7]=base[0x30037]=0xC3;
    InputField<int>(0x6C50C)=8;InputField<int>(0x6C510)=9;PreparePlayerGuardOption();
    CHECK(g_guard_option_row==8 && g_move_option_row==9 && InputField<int>(0x6C50C)==10 && InputField<int>(0x6C510)==11);
    Jump(0x2FC30,reinterpret_cast<void*>(NativeOptions));
    InputField<uintptr_t>(0x6C47C)=g_base+0x2FC30;InputField<int>(0x552348)=6;
    g_modern_keyboard=g_modern_gamepad=false;
    Frame(ff_input::Right);CHECK(!g_controls_menu_active && !g_controls.guard);
    focus=false;Frame(ff_input::Confirm);CHECK(!g_controls_menu_active);focus=true;
    Frame(ff_input::Confirm,ff_input::Confirm);
    CHECK(g_controls_menu_active && g_control_menu.GameplayPage() && !g_controls.guard);
    CHECK(InputField<uintptr_t>(0x6C47C)==reinterpret_cast<uintptr_t>(ControlMenuFrame));
    g_control_menu.row=0;g_control_menu.Accept();CHECK(g_control_menu.draft.guard && !g_controls.guard);
    CloseControlMenu();CHECK(InputField<uintptr_t>(0x6C47C)==g_base+0x2FC30);
    CHECK(!g_controls.guard && !g_modern_keyboard && !g_modern_gamepad);
    InputField<int>(0x552348)=7;Frame(ff_input::Confirm);
    CHECK(g_controls_menu_active && g_control_menu.MoveListPage() && g_move_list.page==ff_moves::Page::Players);
    CloseControlMenu();
    InputField<int>(0x552348)=3;Frame(ff_input::Confirm);CHECK(!g_controls_menu_active);
    VirtualFree(base,0,MEM_RELEASE);
    puts(failures?"Native Gameplay Settings entry: FAIL":"Native Gameplay Settings entry: PASS");return failures?1:0;
}
