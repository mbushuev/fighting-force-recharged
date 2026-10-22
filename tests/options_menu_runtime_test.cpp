#include "../src/dinput_proxy.cpp"
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"ole32.lib")
static int failures,native_calls,errors;
static bool focus=true;
static unsigned buttons[2];
static uintptr_t next_callback;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
static bool Focus(){return focus;}
static int __cdecl Width(const char* text){return static_cast<int>(strlen(text))*10;}
static void __cdecl Draw(const char*,int x,int y,const BYTE*){CHECK(x>=0 && y==226);++errors;}
static void Jump(DWORD rva,void* target) {
    auto code=reinterpret_cast<BYTE*>(g_base+rva);code[0]=0xE9;
    const LONG offset=static_cast<LONG>(reinterpret_cast<uintptr_t>(target)-reinterpret_cast<uintptr_t>(code+5));memcpy(code+1,&offset,4);
}
static const char* NativeLabel() {
    const auto address=g_base+0x2FFCE;const char* result;
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
    // Original retail Options row map, rebuilt for every native callback.
    InputField<int>(0x6C4F8)=999;InputField<int>(0x6C50C)=6;InputField<int>(0x6C510)=7;
    reinterpret_cast<void(__cdecl*)()>(g_base+0x2FD6A)();
    CHECK(InputField<int>(0x6C50C)==7 && InputField<int>(0x6C510)==8);
    CHECK(strstr(NativeLabel(),"PLAYER BLOCK ")==NativeLabel());
    InputField<unsigned>(0x5522E8)=buttons[0];InputField<unsigned>(0x5522EC)=buttons[1];
    if(next_callback)InputField<uintptr_t>(0x6C47C)=next_callback;
}
static void Frame(unsigned first,unsigned second=0) {buttons[0]=first;buttons[1]=second;ControlFrontend();}
int main() {
    auto base=static_cast<BYTE*>(VirtualAlloc(nullptr,0x900000,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    CHECK(base);if(!base)return 1;g_base=reinterpret_cast<uintptr_t>(base);g_modern_focus=Focus;
    const BYTE setup[]={0xA1,0x44,0x23,0x95,0x00},label[]={0x8B,0x3D,0x90,0x22,0x95,0x00};
    const BYTE unused[]={0x3B,0x05,0xF8,0xC4,0x46,0x00};
    memcpy(base+0x2FD6A,setup,sizeof(setup));memcpy(base+0x2FFCE,label,sizeof(label));
    CHECK(!InstallPlayerGuardOption() && base[0x2FD6A]==0xA1 && base[0x2FFCE]==0x8B);
    memcpy(base+0x304E1,unused,sizeof(unused));CHECK(InstallPlayerGuardOption());
    base[0x2FD6F]=base[0x2FFD4]=0xC3;
    InputField<int>(0x6C50C)=6;InputField<int>(0x6C510)=7;InputField<int>(0x552344)=123;
    CHECK(reinterpret_cast<int(__cdecl*)()>(g_base+0x2FD6A)()==123);
    CHECK(g_guard_option_row==6 && InputField<int>(0x6C4F8)==6 && InputField<int>(0x6C50C)==7);
    CHECK(!strcmp(NativeLabel(),"PLAYER BLOCK OFF"));
    // The extended native debug row map keeps its Exit and total independent.
    InputField<int>(0x6C50C)=8;InputField<int>(0x6C510)=9;PreparePlayerGuardOption();
    CHECK(g_guard_option_row==8 && InputField<int>(0x6C50C)==9 && InputField<int>(0x6C510)==10);
    Jump(0x2FC30,reinterpret_cast<void*>(NativeOptions));
    Jump(0x4AEB0,reinterpret_cast<void*>(Width));Jump(0x44A20,reinterpret_cast<void*>(Draw));
    wchar_t folder[MAX_PATH];GetTempPathW(MAX_PATH,folder);GetTempFileNameW(folder,L"ffo",0,g_config_path);
    WritePrivateProfileStringW(L"Input",L"Gamepad",L"0",g_config_path);
    WritePrivateProfileStringW(L"Player1Bindings",L"KeyPunch",L"77",g_config_path);
    WritePrivateProfileStringW(L"Unrelated",L"Keep",L"73",g_config_path);
    InputField<uintptr_t>(0x6C47C)=g_base+0x2FC30;InputField<int>(0x552348)=6;
    Frame(ff_input::Right);CHECK(!g_controls.guard && !strcmp(g_guard_option_message,"PLAYER BLOCK UNAVAILABLE"));
    CHECK(!ReadControlSettings(g_config_path).guard);
    g_player_guard_available=true;SetFileAttributesW(g_config_path,FILE_ATTRIBUTE_READONLY);
    Frame(ff_input::Right);CHECK(!g_controls.guard && !strcmp(g_guard_option_message,"SAVE FAILED  TRY AGAIN"));
    CHECK(!ReadControlSettings(g_config_path).guard && errors==2);
    SetFileAttributesW(g_config_path,FILE_ATTRIBUTE_NORMAL);
    const auto controls=g_controls;const auto camera=g_modern_coop_camera;const auto scheme=g_modern_coop_scheme;
    g_modern_keyboard=g_modern_gamepad=false;
    Frame(ff_input::Right,ff_input::Right);CHECK(g_controls.guard && ReadControlSettings(g_config_path).guard);
    CHECK(!*g_guard_option_message && !g_modern_keyboard && !g_modern_gamepad);
    CHECK(g_controls.keyboard==controls.keyboard && g_controls.dynamic==controls.dynamic);
    CHECK(g_modern_coop_camera==camera && g_modern_coop_scheme==scheme);
    CHECK(GetPrivateProfileIntW(L"Input",L"Gamepad",1,g_config_path)==0);
    CHECK(GetPrivateProfileIntW(L"Player1Bindings",L"KeyPunch",0,g_config_path)==77);
    CHECK(GetPrivateProfileIntW(L"Unrelated",L"Keep",0,g_config_path)==73);
    CHECK(GetPrivateProfileIntW(L"Controls",L"BindingsVersion",-1,g_config_path)==UINT(-1));
    Frame(0);CHECK(g_controls.guard && !strcmp(NativeLabel(),"PLAYER BLOCK ON"));
    Frame(ff_input::Confirm,ff_input::Confirm);CHECK(!g_controls.guard && !ReadControlSettings(g_config_path).guard);
    Frame(0);CHECK(!g_controls.guard && !strcmp(NativeLabel(),"PLAYER BLOCK OFF"));
    Frame(0,ff_input::Confirm);CHECK(g_controls.guard); // Second controller can change shared Options.
    Frame(ff_input::Left);CHECK(!g_controls.guard);
    focus=false;Frame(ff_input::Right);CHECK(!g_controls.guard);focus=true;
    InputField<int>(0x552348)=3;Frame(ff_input::Right);CHECK(!g_controls.guard); // Controls row is unchanged.
    InputField<int>(0x552348)=6;next_callback=g_base+0x2F3B0;
    Frame(ff_input::Confirm|ff_input::Cancel);CHECK(!g_controls.guard); // Back wins, without toggling on exit.
    CHECK(InputField<uintptr_t>(0x6C47C)==next_callback && native_calls>=10);
    VirtualFree(base,0,MEM_RELEASE);
    CHECK(DeleteFileW(g_config_path));
    puts(failures?"Native Options guard setting: FAIL":"Native Options guard setting: PASS");return failures?1:0;
}
