#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <initializer_list>
static uintptr_t g_base;
static wchar_t g_config_path[]=L"modern-runtime-test-missing.ini";
static void Log(const char*) {}
#include "../src/modern_input_runtime.h"
static int failures,native_calls,native_device,scan=-1;
static uint32_t native_actions;
static ff_input::Keyboard test_keys;
static bool focus=true;
static XINPUT_STATE pad_state;
static XINPUT_STATE second_state;
static DWORD WINAPI TwoPads(DWORD index,XINPUT_STATE* out) {
    if(index==1){*out=pad_state;return ERROR_SUCCESS;}
    if(index==3){*out=second_state;return ERROR_SUCCESS;}
    return ERROR_DEVICE_NOT_CONNECTED;
}
static DWORD WINAPI PadState(DWORD index,XINPUT_STATE* out) {
    if(index!=2)return ERROR_DEVICE_NOT_CONNECTED;*out=pad_state;return ERROR_SUCCESS;
}
#define CHECK(x) do {if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);++failures;}}while(0)
static bool Focus() {return focus;}
static ff_input::Keyboard KeyboardState() {return test_keys;}
static uint32_t __cdecl NativeRead(int player) {++native_calls;native_device=player;return native_actions+player;}
static int __cdecl NativeScan() {return scan;}
static void Branch(DWORD rva,uintptr_t target) {
    auto code=reinterpret_cast<BYTE*>(g_base+rva);code[0]=0xE8;
    LONG offset=static_cast<LONG>(target-(g_base+rva+5));memcpy(code+1,&offset,4);
}
static uint32_t AssignedMenu(bool joystick,int device,int owner) {
    const auto boundary=reinterpret_cast<void*>(g_base+(joystick?0x4CE3C:0x4CD9D)-4);
    uint32_t result;
    __asm {
        push edi
        mov edi,owner
        push device
        call boundary
        add esp,4
        mov result,eax
        pop edi
    }
    return result;
}
int main() {
    using namespace ff_input;
    g_base=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,0x900000,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    CHECK(g_base!=0);if(!g_base)return 1;
    const DWORD joystick[]={0x4884E,0x48895,0x4CD96,0x4CE3C,0x4AA6A,0x4AB00};
    const DWORD keyboard[]={0x48857,0x488A0,0x4CD9D,0x4CE43,0x4AA75,0x4AB0C};
    for(DWORD rva:joystick)Branch(rva,g_base+0x22D70);
    for(DWORD rva:keyboard)Branch(rva,g_base+0x22BE0);
    Branch(0x42319,g_base+0x22F70);Branch(0x4245D,g_base+0x22F70);
    Branch(0x41BC5,g_base+0x22F70);Branch(0x486DC,g_base+0x22F70);
    *reinterpret_cast<BYTE*>(g_base+0x4245D)=0;InstallModernInputHooks();CHECK(!g_modern_keyboard);
    LONG offset;memcpy(&offset,reinterpret_cast<void*>(g_base+0x48858),4);
    CHECK(g_base+0x4885C+offset==g_base+0x22BE0);
    Branch(0x4245D,g_base+0x22F70);InstallModernInputHooks();CHECK(g_modern_keyboard);
    Branch(0x22BE0,reinterpret_cast<uintptr_t>(NativeRead));*reinterpret_cast<BYTE*>(g_base+0x22BE0)=0xE9;
    Branch(0x22D70,reinterpret_cast<uintptr_t>(NativeRead));*reinterpret_cast<BYTE*>(g_base+0x22D70)=0xE9;
    Branch(0x22F70,reinterpret_cast<uintptr_t>(NativeScan));*reinterpret_cast<BYTE*>(g_base+0x22F70)=0xE9;
    // Execute a patched native menu call, not just the pure mapper.
    const BYTE push_argument[]={0xFF,0x74,0x24,0x04};
    const BYTE return_argument[]={0x83,0xC4,0x04,0xC3};
    memcpy(reinterpret_cast<void*>(g_base+0x48853),push_argument,4);
    memcpy(reinterpret_cast<void*>(g_base+0x4885C),return_argument,4);
    g_modern_focus=Focus;g_modern_keyboard_reader=KeyboardState;
    g_modern_gamepad=false;
    test_keys.up=test_keys.jump=true;native_actions=0x800000;
    CHECK(reinterpret_cast<uint32_t(__cdecl*)(int)>(g_base+0x48853)(0)==(native_actions|Up|Confirm));
    CHECK(native_calls==1);CHECK(ModernMenuRead<false>(1)==native_actions+1);
    InputField<int>(0x93D20)=1;CHECK(ModernMenuRead<false>(0)==native_actions);
    InputField<int>(0x8745A0)=1;InputField<int16_t>(0x86A542)=100;
    CHECK(ModernGameplayActions(Kick)==(Kick|Up|Jump));
    focus=false;CHECK(ModernGameplayActions(Kick)==0);focus=true;
    InputField<int>(0x85A668)=1;CHECK(ModernGameplayActions(Kick)==Kick);
    InputField<int>(0x85A668)=0;InputField<int>(0x8745A0)=2;CHECK(ModernGameplayActions(Kick)==Kick);
    InputField<int>(0x8745A0)=1;native_actions=Pause;
    CHECK((ModernPauseRead<false,false>(0)&Pause)==0); // Space jumps without pausing.
    test_keys={};test_keys.pause=true;CHECK((ModernPauseRead<false,false>(0))==Pause);
    CHECK((ModernPauseRead<true,true>(0))==0); // Holding Esc cannot immediately resume.
    test_keys.pause=false;CHECK((ModernPauseRead<false,true>(0))==0);
    test_keys.pause=true;CHECK((ModernPauseRead<false,true>(0))==(Left|Confirm));
    scan=0x10;test_keys={};CHECK(ModernQuitKey()==-1);
    scan=0x44;test_keys.confirm=true;CHECK(ModernQuitKey()==0x10);
    CHECK(ModernQuitKey()==-1); // Holding F10 cannot repeatedly reopen the prompt.
    scan=-1;CHECK(ModernQuitAnswer()==-1);test_keys.confirm=false;CHECK(ModernQuitAnswer()==-1);
    test_keys.confirm=true;CHECK(ModernQuitAnswer()==0x15);
    scan=0x31;CHECK(ModernQuitAnswer()==0x31);
    scan=-1;test_keys={};ModernQuitKey();
    // Feed XInput through its runtime adapter and the same native action/menu
    // boundaries. A duplicate legacy joystick must not leak old bindings.
    test_keys={};scan=-1;g_modern_gamepad=true;g_modern_xinput=PadState;g_modern_pad_session={};
    InputField<int>(0x6F040)=1;ReadModernPad();CHECK(g_modern_pad_session.selected==2);
    pad_state.Gamepad.wButtons=XINPUT_GAMEPAD_X|XINPUT_GAMEPAD_B;
    pad_state.Gamepad.sThumbLX=30000;g_modern_pad_session.last_poll=0;
    CHECK(ModernGameplayActions(0x01AB0221)==(0xAB0000|Right|Punch|Backfist));
    InputField<int>(0x93D20)=0;native_actions=0x01000008;
    CHECK(ModernMenuRead<true>(0)==(Right|Cancel));
    pad_state.Gamepad={};pad_state.Gamepad.wButtons=XINPUT_GAMEPAD_START|XINPUT_GAMEPAD_A;
    g_modern_pad_session.last_poll=0;native_actions=0;
    CHECK((ModernPauseRead<true,false>(0))==Pause);
    CHECK((ModernPauseRead<true,true>(0))==0); // A held while jumping cannot answer pause.
    pad_state.Gamepad={};g_modern_pad_session.last_poll=0;CHECK((ModernPauseRead<true,true>(0))==0);
    pad_state.Gamepad.wButtons=XINPUT_GAMEPAD_B;g_modern_pad_session.last_poll=0;
    CHECK((ModernPauseRead<true,true>(0))==(Left|Confirm));
    pad_state.Gamepad.wButtons=XINPUT_GAMEPAD_BACK|XINPUT_GAMEPAD_A;g_modern_pad_session.last_poll=0;
    CHECK(ModernQuitKey()==0x10);CHECK(ModernQuitAnswer()==-1);
    pad_state.Gamepad={};g_modern_pad_session.last_poll=0;CHECK(ModernQuitAnswer()==-1);
    pad_state.Gamepad.wButtons=XINPUT_GAMEPAD_A;g_modern_pad_session.last_poll=0;
    CHECK(ModernQuitAnswer()==0x15);
    focus=false;CHECK(ModernGameplayActions(0xAB0020)==0xAB0000);focus=true;
    CHECK(ModernGameplayActions(0xAB0000)==0xAB0000); // Reacquire while A held waits for neutral.
    pad_state.Gamepad={};g_modern_pad_session.last_poll=0;ReadModernPad();
    InputField<int>(0x85A6B8)=26;InputField<uintptr_t>(0x6C47C)=g_base+0x2F3B0;
    g_modern_pad_session.last_poll=0;ReadModernPad();ModernQuitKey();
    pad_state.Gamepad.wButtons=XINPUT_GAMEPAD_B;g_modern_pad_session.last_poll=0;native_actions=0;
    CHECK(ModernMenuRead<true>(0)==0 && ModernQuitKey()==0x10);
    CHECK(ModernQuitAnswer()==-1); // The B that opens the prompt cannot cancel it immediately.
    pad_state.Gamepad={};g_modern_pad_session.last_poll=0;CHECK(ModernQuitAnswer()==-1);
    pad_state.Gamepad.wButtons=XINPUT_GAMEPAD_A;g_modern_pad_session.last_poll=0;CHECK(ModernQuitAnswer()==0x15);
    InputField<uintptr_t>(0x6C47C)=g_base+0x2F780;CHECK(!ModernMainMenu());
    pad_state.Gamepad={};g_modern_pad_session.last_poll=0;ModernQuitKey();
    pad_state.Gamepad.wButtons=XINPUT_GAMEPAD_B;g_modern_pad_session.last_poll=0;
    CHECK(ModernMenuRead<true>(0)==Cancel && ModernQuitKey()==-1);
    InputField<uintptr_t>(0x6C47C)=g_base+0x2F3B0;
    CHECK(ModernMenuRead<true>(0)==0 && ModernQuitKey()==-1); // B held after Back needs release too.
    InputField<uintptr_t>(0x6C47C)=g_base+0x2F780;
    // Esc goes back in a submenu, then requires release before main-menu quit.
    pad_state.Gamepad={};g_modern_pad_session.last_poll=0;
    g_modern_keyboard=true;test_keys={};ModernQuitKey();
    native_actions=Cancel;scan=0x01;test_keys.cancel=true;
    CHECK(ModernMenuRead<false>(0)==Cancel && ModernQuitKey()==0x01);
    InputField<uintptr_t>(0x6C47C)=g_base+0x2F3B0;
    CHECK(ModernMenuRead<false>(0)==0 && ModernQuitKey()==-1);
    test_keys.cancel=false;scan=-1;native_actions=0;ModernMenuRead<false>(0);ModernQuitKey();
    test_keys.cancel=true;scan=0x01;
    CHECK(ModernMenuRead<false>(0)==0 && ModernQuitKey()==0x10);
    scan=-1;CHECK(ModernQuitAnswer()==-1); // Opening Esc cannot also cancel its prompt.
    test_keys.cancel=false;CHECK(ModernQuitAnswer()==-1);
    test_keys.cancel=true;CHECK(ModernQuitAnswer()==0x31);
    CHECK(ModernQuitKey()==-1); // Held cancel cannot reopen quit.
    test_keys.cancel=false;ModernMenuRead<false>(0);ModernQuitKey();
    test_keys.cancel=true;scan=0x01;CHECK(ModernQuitKey()==0x10);
    scan=-1;test_keys.cancel=false;test_keys.confirm=true;CHECK(ModernQuitAnswer()==0x15);
    g_modern_keyboard=false;scan=0x01;CHECK(ModernQuitKey()==0x01);g_modern_keyboard=true;
    InputField<int>(0x85A6B8)=0;test_keys={};test_keys.cancel=test_keys.pause=true;scan=0x01;
    CHECK((ModernQuitKey()==0x01 && ModernPauseRead<false,false>(0)==Pause));
    g_modern_keyboard=false;test_keys={};scan=-1;
    // Intro playback does not call native gameplay/menu readers. The movie
    // callback polls the same selected pad, with a separate release gate.
    InputField<int>(0x55207C)=1;pad_state.Gamepad={};g_modern_pad_session.last_poll=0;
    ModernMovieInput(true);ModernMovieInput(false);CHECK(InputField<int>(0x20D544)==0);
    pad_state.Gamepad.wButtons=XINPUT_GAMEPAD_START;g_modern_pad_session.last_poll=0;
    ModernMovieInput(false);CHECK(InputField<int>(0x20D544)==1);
    InputField<int>(0x20D544)=0;ModernMovieInput(false);CHECK(InputField<int>(0x20D544)==0);
    ModernMovieInput(true);g_modern_pad_session.last_poll=0;ModernMovieInput(false);
    CHECK(InputField<int>(0x20D544)==0); // A held skip cannot skip the following movie.
    pad_state.Gamepad={};g_modern_pad_session.last_poll=0;ModernMovieInput(false);
    pad_state.Gamepad.wButtons=XINPUT_GAMEPAD_A;g_modern_pad_session.last_poll=0;
    ModernMovieInput(false);CHECK(InputField<int>(0x20D544)==1);
    InputField<int>(0x20D544)=0;focus=false;ModernMovieInput(false);CHECK(InputField<int>(0x20D544)==0);
    focus=true;InputField<int>(0x55207C)=0;ModernMovieInput(false);CHECK(InputField<int>(0x20D544)==0);
    // Cooperative slots remain distinct and feed different native action words.
    g_modern_coop_input=true;g_modern_coop_scheme=2;g_modern_xinput=TwoPads;g_modern_pad_session={};g_modern_second_session={};
    g_modern_controller=g_modern_second_controller=-1;pad_state={};second_state={};
    InputField<int>(0x8745A0)=2;InputField<int>(0x85A6B8)=0;InputField<int>(0x85A668)=0;
    CHECK(ReadModernPad().connected && ReadModernSecondPad().connected);
    CHECK(g_modern_pad_session.selected==1 && g_modern_second_session.selected==3);
    pad_state.Gamepad.wButtons=XINPUT_GAMEPAD_X;second_state.Gamepad.wButtons=XINPUT_GAMEPAD_Y;
    second_state.Gamepad.sThumbLY=30000;
    g_modern_pad_session.last_poll=g_modern_second_session.last_poll=0;
    InputField<uint32_t>(0x5523E0)=0xAB0020;InputField<uint32_t>(0x5523E4)=0xCD0040;
    CHECK(ModernCooperativeActions(0)==(0xAB0000|Punch));
    CHECK(ModernCooperativeActions(1)==(0xCD0000|Kick|Up));
    CHECK(ModernSecondMenuRead<true>(0)==Up);
    second_state.Gamepad={};second_state.Gamepad.wButtons=XINPUT_GAMEPAD_START;
    g_modern_second_session.last_poll=0;CHECK((ModernPauseRead<true,false>(0)==Pause));
    focus=false;CHECK(ModernCooperativeActions(1)==0xCD0000);focus=true;
    CHECK(ModernCooperativeActions(1)==0xCD0000); // Held input after focus needs neutral.
    g_modern_coop_scheme=1;g_modern_keyboard=true;test_keys={};test_keys.up=true;
    g_modern_pad_session={};g_modern_second_session={};pad_state={};ReadModernPad();
    InputField<int>(0x6F040)=1;InputField<uintptr_t>(0x6C47C)=0;
    CHECK(ModernCooperativeActions(0)==(0xAB0000|Up));
    pad_state.Gamepad.wButtons=XINPUT_GAMEPAD_X;g_modern_pad_session.last_poll=0;
    CHECK(ModernCooperativeActions(0)==(0xAB0000|Up));
    CHECK(ModernCooperativeActions(1)==(0xCD0000|Punch));
    CHECK(ModernSecondMenuRead<true>(0)==0);
    // Keyboard ownership can move to player two without leaking to player one.
    g_modern_coop_scheme=3;
    CHECK(ModernCooperativeActions(0)==(0xAB0000|Punch));
    CHECK(ModernCooperativeActions(1)==(0xCD0000|Up));
    CHECK(ModernSecondMenuRead<true>(0)==Up);
    test_keys={};g_modern_coop_scheme=2;
    g_modern_controller=-1;g_modern_second_controller=1;
    g_modern_pad_session={};g_modern_second_session={};pad_state={};second_state={};
    CHECK(ReadModernPad().connected && ReadModernSecondPad().connected);
    CHECK(g_modern_pad_session.selected==3 && g_modern_second_session.selected==1);
    // Reserve the explicit second slot before automatically selecting the first.
    second_state.Gamepad.wButtons=XINPUT_GAMEPAD_A;
    g_controls.players[0].pad[ff_controls::Jump]=0;
    CHECK(ff_controls::Bind(g_controls.players[0],ff_controls::Hand,ff_input::A,true));
    g_modern_pad_session.last_poll=0;
    CHECK(ModernCooperativeActions(0)==(0xAB0000|Punch));
    CHECK(ModernCooperativeActions(1)==0xCD0000);
    // The selector's displayed actor count can differ from its requested game
    // mode. Solo must combine keyboard and pad even with cooperative layouts;
    // only a requested two-player game may enforce separate device ownership.
    for(DWORD rva:{0x4CD9D,0x4CE3C}) {
        memcpy(reinterpret_cast<void*>(g_base+rva-4),push_argument,sizeof(push_argument));
        memcpy(reinterpret_cast<void*>(g_base+rva+5),return_argument,sizeof(return_argument));
    }
    InputField<int>(0x85A6B8)=26;InputField<uintptr_t>(0x6C47C)=g_base+0x2F730;
    InputField<int>(0x93D20)=0;native_actions=0;g_modern_keyboard=true;
    g_modern_controller=g_modern_second_controller=-1;
    for(int scheme:{1,2,3})for(int displayed:{0,1,2,4})for(int requested:{1,2}) {
        g_modern_coop_scheme=scheme;InputField<int>(0x8745A0)=displayed;
        InputField<int>(0x86E9C8)=requested;
        CHECK(ModernCooperativeMenu()==(requested==2));
        g_modern_pad_session={};g_modern_second_session={};pad_state={};second_state={};
        ReadModernFrontendPad(); // Arm both assigned slots at a neutral boundary.
        test_keys={};test_keys.down=true;
        pad_state.Gamepad.wButtons=XINPUT_GAMEPAD_DPAD_RIGHT|XINPUT_GAMEPAD_A;
        second_state.Gamepad.wButtons=XINPUT_GAMEPAD_DPAD_LEFT|XINPUT_GAMEPAD_B;
        g_modern_pad_session.last_poll=g_modern_second_session.last_poll=0;
        for(bool use_joystick:{false,true})for(int device:{0,1})for(int owner:{0,1}) {
            uint32_t expected;
            if(requested==1)expected=owner==1?0:Down|Confirm|(scheme==2?Cancel:Right)|(use_joystick?0:device);
            else expected=ModernKeyboardPlayer()==owner?Down:scheme==2 && owner==1?Left|Cancel:Right|Confirm;
            const uint32_t actual=AssignedMenu(use_joystick,device,owner);
            if(actual!=expected)printf("Selection scheme=%d display=%d requested=%d joystick=%d device=%d owner=%d: %x != %x\n",
                scheme,displayed,requested,int(use_joystick),device,owner,actual,expected);
            CHECK(actual==expected);
            if(requested!=1 || owner==0)CHECK(native_device==device);
        }
        if(requested==1) {
            test_keys={};g_modern_pad_session.last_poll=g_modern_second_session.last_poll=0;
            CHECK(AssignedMenu(false,1,0)==(Confirm|(scheme==2?Cancel:Right)|1));
            pad_state={};second_state={};test_keys.confirm=true;
            g_modern_pad_session.last_poll=g_modern_second_session.last_poll=0;
            CHECK(AssignedMenu(true,1,0)==Confirm);
        }
    }
    VirtualFree(reinterpret_cast<void*>(g_base),0,MEM_RELEASE);
    printf("Modern native input boundaries: %s\n",failures?"FAIL":"PASS");return failures?1:0;
}
