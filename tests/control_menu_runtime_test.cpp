#define WIN32_LEAN_AND_MEAN
#include <windows.h>
// Isolate capture from real desktop mouse presses while the fake keyboard is neutral.
static SHORT WINAPI TestAsyncKeyState(int){return 0;}
#define GetAsyncKeyState TestAsyncKeyState
#include "../src/dinput_proxy.cpp"
#undef GetAsyncKeyState
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"ole32.lib")
static int failures,native_calls,text_calls;
static bool highlighted[240],physical_focus;
static BYTE physical_keys[256];
static bool device_acquired;
static int acquisitions;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
static bool Unfocused(){return physical_focus;}
static DWORD WINAPI NoPads(DWORD,XINPUT_STATE*){return ERROR_DEVICE_NOT_CONNECTED;}
static unsigned capture_buttons;
static DWORD WINAPI CapturePad(DWORD slot,XINPUT_STATE* state) {
    if(slot)return ERROR_DEVICE_NOT_CONNECTED;*state={};state->Gamepad.wButtons=static_cast<WORD>(capture_buttons);return ERROR_SUCCESS;
}
static HRESULT WINAPI DeviceRead(void*,DWORD size,void* output) {
    CHECK(size==sizeof(physical_keys));if(!device_acquired)return DIERR_NOTACQUIRED;
    memcpy(output,physical_keys,size);return DI_OK;
}
static HRESULT WINAPI DeviceAcquire(void*){device_acquired=true;++acquisitions;return DI_OK;}
static uint32_t __cdecl NativeRead(int){return 0;}
static void __cdecl NativeMenu(){++native_calls;}
static int __cdecl TextWidth(const char* text) {
    int width=0;for(const char* c=text;*c;++c)width+=*c==' '?6:11;return width;
}
static void __cdecl TextDraw(const char* text,int x,int y,const BYTE* rgb) {
    ++text_calls;CHECK(x>=0 && y>=0 && y<=226);CHECK(x+TextWidth(text)<=320);
    CHECK(rgb[0]==248 || rgb[0]==15 || rgb[0]==252);highlighted[y]=rgb[0]==15;
    if(rgb[0]==252)CHECK(rgb[1]==6 && rgb[2]==10);
    for(int i=3;i<12;++i)CHECK(rgb[i]==rgb[i%3]);
    for(const char* c=text;*c;++c)CHECK((*c>='A' && *c<='Z') || (*c>='0' && *c<='9') || *c==' ' || *c=='-' || *c=='+');
}
static void JumpStub(BYTE* code,void* target) {
    code[0]=0xE9;LONG offset=static_cast<LONG>(reinterpret_cast<BYTE*>(target)-(code+5));memcpy(code+1,&offset,4);
}
static uint32_t AssignedRead(int owner,int device) {
    uint32_t result;
    __asm {
        push edi
        mov edi,owner
        push device
        call ModernAssignedKeyboardThunk
        add esp,4
        mov result,eax
        pop edi
    }
    return result;
}
int main() {
    auto base=static_cast<BYTE*>(VirtualAlloc(nullptr,0x900000,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    CHECK(base);if(!base)return 1;g_base=reinterpret_cast<uintptr_t>(base);g_modern_focus=Unfocused;
    const BYTE call[]={0xFF,0x15,0x7C,0xC4,0x46,0};
    memcpy(base+0x2EBAF,call,6);InstallControlMenu();CHECK(base[0x2EBAF]==0xFF);
    memcpy(base+0x2F091,call,6);InstallControlMenu();CHECK(base[0x2EBAF]==0xE8 && base[0x2F096]==0x90);
    JumpStub(base+0x4AEB0,reinterpret_cast<void*>(TextWidth));JumpStub(base+0x44A20,reinterpret_cast<void*>(TextDraw));
    InputField<uintptr_t>(0x6C47C)=reinterpret_cast<uintptr_t>(NativeMenu);base[0x2EBB5]=0xC3;
    reinterpret_cast<void(__cdecl*)()>(base+0x2EBAF)();CHECK(native_calls==1 && !g_controls_menu_active);
    InputField<uintptr_t>(0x6C47C)=g_base+0x30800;
    reinterpret_cast<void(__cdecl*)()>(base+0x2EBAF)();CHECK(g_controls_menu_active && text_calls>=12);
    CHECK(g_control_return==g_base+0x2FC30 && InputField<uintptr_t>(0x6C47C)==reinterpret_cast<uintptr_t>(ControlMenuFrame));
    CHECK(ModernMenuRead<false>(0)==0 && ModernSecondMenuRead<true>(0)==0 && ModernQuitKey()==-1);
    for(int selected=0;selected<10;++selected) {
        g_control_menu.row=selected;DrawControlMenu();
        for(int row=0;row<10;++row)CHECK(highlighted[36+row*15]==(row==selected));
    }
    g_control_menu.page=-2;
    for(int selected=0;selected<9;++selected) {
        g_control_menu.row=selected;DrawControlMenu();
        for(int row=0;row<9;++row)CHECK(highlighted[38+row*17]==(row==selected));
    }
    for(int player=0;player<2;++player)for(int keyboard=-1;keyboard<2;++keyboard) {
        g_control_menu.draft.keyboard=keyboard;g_control_menu.page=player;
        for(int row=0;row<g_control_menu.Rows();++row) {
            g_control_menu.row=row;DrawControlMenu();const int first=row/7*7;
            for(int item=first;item<g_control_menu.Rows() && item<first+7;++item)CHECK(highlighted[54+(item-first)*18]==(item==row));
        }
        g_control_menu.capture=true;DrawControlMenu();g_control_menu.capture=false;
    }
    for(const char* message:{"BUTTON ALREADY USED","DYNAMIC CAMERA UNAVAILABLE","SAVE FAILED  TRY AGAIN"}) {
        g_control_message=message;DrawControlMenu();
    }
    g_control_message="";
    // The game's existing exclusive DirectInput keyboard remains available
    // even when Windows asynchronous key polling cannot supply a menu key.
    void* table[18]={};table[7]=reinterpret_cast<void*>(DeviceAcquire);table[9]=reinterpret_cast<void*>(DeviceRead);
    struct Device {void** table;} device={table};auto keyboard=reinterpret_cast<IDirectInputDeviceA*>(&device);
    InputField<IDirectInputDeviceA**>(0x9D180)=&keyboard;physical_focus=true;g_modern_xinput=NoPads;
    BYTE keys[256];CHECK(ReadModernPhysicalKeyboard(keys) && acquisitions==1);
    physical_keys[0xC8]=128;CHECK(ReadModernPhysicalKeyboard(keys) && PhysicalKeyboardKey(keys,0x148));
    physical_focus=false;CHECK(!ReadModernPhysicalKeyboard(keys) && !PhysicalKeyboardKey(keys,0x148));
    physical_focus=true;ZeroMemory(physical_keys,sizeof(physical_keys));
    g_control_menu.Open(g_controls);g_control_nav_ready=false;PollControlMenu();CHECK(g_control_nav_ready);
    physical_keys[0xD0]=128;PollControlMenu();CHECK(g_control_menu.row==1);
    physical_keys[0xD0]=0;physical_keys[0xC8]=128;PollControlMenu();CHECK(g_control_menu.row==0);
    ZeroMemory(physical_keys,sizeof(physical_keys));PollControlMenu();
    g_control_menu.row=2;physical_keys[0x1C]=128;PollControlMenu();CHECK(g_control_menu.page==0);
    physical_keys[0x1C]=0;PollControlMenu();physical_keys[0x1C]=128;PollControlMenu();CHECK(g_control_menu.capture);
    physical_keys[0x1C]=0;PollControlMenu();physical_keys[0x14]=128;PollControlMenu();
    CHECK(!g_control_menu.capture && g_control_menu.draft.players[0].keys[ff_controls::Forward]==0x14);
    ZeroMemory(physical_keys,sizeof(physical_keys));
    g_control_menu.page=1;g_control_menu.draft.keyboard=0;
    g_control_menu.row=ff_controls::Backfist-4;g_control_menu.capture=true;
    g_control_capture_ready=false;g_modern_xinput=CapturePad;PollControlMenu();
    capture_buttons=ff_input::RightShoulder;PollControlMenu();CHECK(g_control_menu.capture);
    capture_buttons|=ff_input::B;PollControlMenu();CHECK(!g_control_menu.capture);
    CHECK(g_control_menu.draft.players[1].pad[ff_controls::Backfist]==(ff_input::RightShoulder|ff_input::B));
    CHECK(!strcmp(ControlPadName(ff_input::RightShoulder|ff_input::B),"RB AND B"));
    CHECK(!strcmp(ControlPadName(ff_input::DpadUp),"D UP") && !strcmp(ControlPadName(ff_input::DpadDown),"D DOWN"));
    DrawControlMenu();capture_buttons=0;g_modern_xinput=NoPads;
    CloseControlMenu();CHECK(!g_controls_menu_active && g_controls_wait_release && InputField<uintptr_t>(0x6C47C)==g_base+0x2FC30);
    CHECK(ControlInputBlocked());CHECK(!ControlInputBlocked());
    JumpStub(base+0x22D70,reinterpret_cast<void*>(NativeRead));JumpStub(base+0x22BE0,reinterpret_cast<void*>(NativeRead));
    InputField<int>(0x85A6B8)=26;InputField<int>(0x8745A0)=2;g_modern_coop_input=true;g_modern_coop_scheme=2;
    physical_keys[0xC8]=128;CHECK(ModernMenuRead<true>(0)==ff_input::Up);
    CHECK(ModernSecondMenuRead<true>(1)==ff_input::Up);
    CHECK(AssignedRead(0,1)==ff_input::Up && AssignedRead(1,0)==ff_input::Up);
    g_modern_coop_scheme=3;InputField<uintptr_t>(0x6C47C)=g_base+0x2F730;InputField<int>(0x86E9C8)=2;
    CHECK(ModernMenuRead<false>(0)==0 && ModernSecondMenuRead<false>(0)==ff_input::Up);
    CHECK(AssignedRead(0,1)==0 && AssignedRead(1,0)==ff_input::Up);
    g_modern_coop_scheme=1;
    CHECK(AssignedRead(0,1)==ff_input::Up && AssignedRead(1,0)==0);
    physical_focus=false;ZeroMemory(physical_keys,sizeof(physical_keys));
    auto settings=ff_controls::DefaultsSettings();settings.keyboard=1;settings.dynamic=false;settings.players[0].slot=3;
    ApplyControlSettings(settings);CHECK(g_modern_coop_scheme==3 && !g_modern_coop_camera && g_modern_controller==3);
    CHECK(!g_modern_pad_session.ready && !g_coop_ready);
    VirtualFree(base,0,MEM_RELEASE);
    puts(failures?"Native control menu: FAIL":"Native control menu: PASS; call signatures, entry/return, input isolation and menu pages.");return failures?1:0;
}
