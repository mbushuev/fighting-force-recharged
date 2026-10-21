#pragma once
#include <Xinput.h>
#include "modern_input.h"

static bool g_modern_keyboard=true,g_modern_gamepad=true;
static int g_modern_controller=-1,g_modern_left_deadzone=7849,g_modern_right_deadzone=8689;
static ff_input::PadSession g_modern_pad_session;
static ff_input::PadSession g_modern_second_session;
static bool g_modern_coop_input;
static int g_modern_second_controller=-1;
using ModernXInput=DWORD(WINAPI*)(DWORD,XINPUT_STATE*);
static ModernXInput g_modern_xinput;
static bool g_modern_quit_button,g_modern_pause_accept_blocked,g_modern_main_back_button;
static ff_input::ConfirmGate g_modern_confirm;
static ff_input::PauseGate g_modern_pause;
static bool DefaultModernFocus() {
    DWORD process=0;GetWindowThreadProcessId(GetForegroundWindow(),&process);
    return process==GetCurrentProcessId();
}
static bool (*g_modern_focus)()=DefaultModernFocus;
static bool ModernHasFocus() {return g_modern_focus();}
template<typename T> static T& InputField(DWORD rva) { return *reinterpret_cast<T*>(g_base+rva); }
static bool ModernLiveGameplay() {
    return InputField<int>(0x85A6B8)>=0 && InputField<int>(0x85A6B8)<25 &&
        InputField<int>(0x8745A0)==1 && InputField<int>(0x85A668)!=1 && InputField<int16_t>(0x86A542)>0;
}
static bool ModernKey(unsigned scan) {
    // Query physical positions, independent of the active text-entry layout.
    // No OS input events are generated and no native binding table is changed.
    const UINT key=MapVirtualKeyW(scan,MAPVK_VSC_TO_VK_EX);
    return key && (GetAsyncKeyState(static_cast<int>(key))&0x8000)!=0;
}
static ff_input::Keyboard PollModernKeyboard() {
    ff_input::Keyboard k;
    if(!g_modern_keyboard || !ModernHasFocus())return k;
    k.up=ModernKey(0x11);k.down=ModernKey(0x1F);k.left=ModernKey(0x1E);k.right=ModernKey(0x20);
    k.punch=ModernKey(0x24);k.kick=ModernKey(0x25);k.jump=ModernKey(0x39);
    k.backfist=ModernKey(0x12);k.run=ModernKey(0x2A);
    k.pause=ModernKey(0x01)||ModernKey(0x19);k.confirm=ModernKey(0x1C);k.cancel=ModernKey(0x01);
    k.quit=ModernKey(0x44); // F10 stays outside the movement/action cluster.
    k.first_person=ModernKey(0x0F); // Tab switches the wrapper camera only.
    return k;
}
static ff_input::Keyboard (*g_modern_keyboard_reader)()=PollModernKeyboard;
static ff_input::Keyboard ReadModernKeyboard() {return g_modern_keyboard?g_modern_keyboard_reader():ff_input::Keyboard{};}
static ff_input::Pad ReadModernPad(int context=-1) {
    if(!g_modern_gamepad)return {};
    // Resolve optional controller support outside the loader lock. A missing
    // runtime or disconnected controller leaves keyboard/mouse available.
    static bool initialized=false;
    if(!initialized && !g_modern_xinput) {
        initialized=true;
        const wchar_t* names[]={L"xinput1_4.dll",L"xinput1_3.dll",L"xinput9_1_0.dll"};
        for(const auto name:names) {
            HMODULE runtime=LoadLibraryExW(name,nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
            if(!runtime)continue;
            g_modern_xinput=reinterpret_cast<ModernXInput>(GetProcAddress(runtime,"XInputGetState"));
            if(g_modern_xinput)break;
        }
    }
    const int tag=context>=0?context:InputField<int>(0x85A6B8)*16+InputField<int>(0x8745A0)*4+(InputField<int>(0x85A668)==1?1:0);
    const auto pad=g_modern_pad_session.Read(GetTickCount64(),ModernHasFocus(),g_modern_controller,tag,
        g_modern_left_deadzone,g_modern_right_deadzone,[](int index,ff_input::Pad& output) {
            XINPUT_STATE state={};
            if((g_modern_coop_input && index==g_modern_second_session.selected) ||
               !g_modern_xinput || g_modern_xinput(index,&state)!=ERROR_SUCCESS)return false;
            const auto& p=state.Gamepad;
            output={true,p.wButtons,p.sThumbLX,p.sThumbLY,p.sThumbRX,p.sThumbRY,p.bLeftTrigger,p.bRightTrigger};return true;
        });
    static int reported=-1;
    if(g_modern_pad_session.selected!=reported) {
        reported=g_modern_pad_session.selected;
        char message[96];sprintf_s(message,"Modern controller slot: %d (-1 disconnected).\r\n",reported);Log(message);
    }
    return pad;
}
static ff_input::Pad ReadModernSecondPad(int context=-1) {
    if(!g_modern_coop_input || !g_modern_gamepad)return {};
    ReadModernPad(context); // Resolve XInput and reserve player one's slot first.
    const int tag=context>=0?context:InputField<int>(0x85A6B8)*16+InputField<int>(0x8745A0)*4+
        (InputField<int>(0x85A668)==1?1:0);
    return g_modern_second_session.Read(GetTickCount64(),ModernHasFocus(),g_modern_second_controller,tag,
        g_modern_left_deadzone,g_modern_right_deadzone,[](int index,ff_input::Pad& output) {
            XINPUT_STATE state={};
            if(index==g_modern_pad_session.selected || !g_modern_xinput ||
               g_modern_xinput(index,&state)!=ERROR_SUCCESS)return false;
            const auto& p=state.Gamepad;
            output={true,p.wButtons,p.sThumbLX,p.sThumbLY,p.sThumbRX,p.sThumbRY,p.bLeftTrigger,p.bRightTrigger};return true;
        });
}
static ff_input::Pad ReadModernFrontendPad() {
    auto pad=ReadModernPad();
    if(g_modern_coop_input && InputField<int>(0x8745A0)==2) {
        const auto second=ReadModernSecondPad();
        if(!pad.connected)return second;
        pad.buttons|=second.buttons;
        if(!pad.lx && !pad.ly) {pad.lx=second.lx;pad.ly=second.ly;}
    }
    return pad;
}
static bool ModernCooperativeGameplay() {
    return g_modern_coop_input && InputField<int>(0x8745A0)==2 &&
        InputField<int>(0x85A6B8)>=0 && InputField<int>(0x85A6B8)<25 && InputField<int>(0x85A668)!=1;
}
static uint32_t ModernCooperativeActions(unsigned player) {
    uint32_t native=InputField<uint32_t>(0x5523E0+player*4);
    const auto pad=player?ReadModernSecondPad():ReadModernPad();
    if(!ModernHasFocus())return native&0xFFFF0000;
    if(pad.connected)native&=0xFCFF0000;
    const auto keys=player?ff_input::Keyboard{}:ReadModernKeyboard();
    return native|ff_input::Gameplay(keys)|ff_input::Gameplay(pad,g_modern_left_deadzone);
}
static void ModernMovieInput(bool begin) {
    static bool held=true;
    if(begin) {held=true;g_modern_pad_session.ready=false;return;}
    if(!g_modern_gamepad || !InputField<int>(0x55207C))return;
    const auto pad=ReadModernPad(0x10000); // Movie playback has its own neutral boundary.
    if(!ModernHasFocus()) {held=true;return;}
    const bool down=(pad.buttons&(ff_input::A|ff_input::B|ff_input::Start))!=0;
    if(down && !held) {
        // Use the same abort flag as the native movie window's key handler.
        // Its playback loop still stops sound/timer and destroys every handle.
        InputField<int>(0x20D544)=1;Log("Controller requested native movie skip.\r\n");
    }
    held=down;
}
#include "analog_movement_runtime.h"
static uint32_t ModernGameplayActions(uint32_t native) {
    const auto pad=ReadModernPad();
    if(!ModernLiveGameplay()) {ResetAnalogMotion();return native;}
    if(!ModernHasFocus()) {ResetAnalogMotion();return (g_modern_keyboard||g_modern_gamepad)?native&0xFFFF0000:native;}
    // An XInput pad may also appear as a legacy joystick. Discard that device's
    // duplicate axes/buttons while preserving native command flags.
    if(pad.connected && (InputField<int>(0x6F040)&1))native&=0xFCFF0000;
    const auto keys=ReadModernKeyboard();
    return native|ff_input::Gameplay(keys)|ff_input::Gameplay(pad,g_modern_left_deadzone)|
        UpdateAnalogMovement(native,keys,pad);
}
template<bool Joystick> static uint32_t NativeModernRead(int player) {
    return reinterpret_cast<uint32_t(__cdecl*)(int)>(g_base+(Joystick?0x22D70:0x22BE0))(player);
}
static bool ModernMainMenu() {
    return InputField<int>(0x85A6B8)==26 && InputField<uintptr_t>(0x6C47C)==g_base+0x2F3B0;
}
template<bool Joystick> static uint32_t __cdecl ModernMenuRead(int player) {
    uint32_t native=NativeModernRead<Joystick>(player);
    if(player || InputField<int>(0x93D20)!=0)return native;
    const auto pad=ReadModernPad();
    if(!ModernHasFocus())return Joystick&&g_modern_gamepad?0:native;
    if(pad.connected && Joystick)native=0;
    const auto keys=ReadModernKeyboard();
    const bool main=ModernMainMenu(),back=keys.cancel||(pad.buttons&ff_input::B);
    // Returning from a submenu while Back is held must not open main-menu quit.
    if(!main || !back)g_modern_main_back_button=back;
    uint32_t mapped=ff_input::Menu(pad,g_modern_left_deadzone);
    uint32_t keyboard=ff_input::Menu(keys);
    if(main) {
        mapped&=~ff_input::Cancel;keyboard&=~ff_input::Cancel;
        if(keys.cancel)native&=~ff_input::Cancel;
    }
    return native|keyboard|mapped;
}
template<bool Joystick> static uint32_t __cdecl ModernSecondMenuRead(int player) {
    if(!g_modern_coop_input)return ModernMenuRead<Joystick>(player);
    uint32_t native=NativeModernRead<Joystick>(player);
    if(InputField<int>(0x93D20)!=0)return native;
    const auto pad=ReadModernSecondPad();
    if(!ModernHasFocus())return 0;
    if(pad.connected)native=0;
    return native|ff_input::Menu(pad,g_modern_left_deadzone);
}
template<bool Joystick,bool Inside> static uint32_t __cdecl ModernPauseRead(int player) {
    uint32_t native=NativeModernRead<Joystick>(player);
    if((!g_modern_keyboard&&!g_modern_gamepad) || (!ModernLiveGameplay() && !ModernCooperativeGameplay()))return native;
    const auto pad=ReadModernFrontendPad();
    if(!ModernHasFocus()) {g_modern_pause.held=true;g_modern_pause_accept_blocked=true;return 0;}
    const auto keys=ReadModernKeyboard();
    if(pad.connected && Joystick)native=0;
    // The native pause reader switches to frontend mode and treats Space as
    // pause. Reserve Space for jumping; Esc/P pause instead. The paused menu
    // uses horizontal choice: Left selects Continue before confirming it.
    const bool open=keys.pause||(pad.buttons&ff_input::Start);
    uint32_t pause=0;
    if(!Inside) {
        g_modern_pause.held=open||(pad.buttons&ff_input::B);
        g_modern_pause_accept_blocked=(pad.buttons&ff_input::A)!=0;
        pause=open?ff_input::Pause:0;
    } else {
        pause=g_modern_pause.Read(open||(pad.buttons&ff_input::B),true);
        if(!(pad.buttons&ff_input::A))g_modern_pause_accept_blocked=false;
    }
    if(pause)return pause;
    if(g_modern_keyboard)native&=~ff_input::Pause;
    return native|(Inside ? (ff_input::Menu(keys,false)|ff_input::Menu(pad,g_modern_left_deadzone,false,
        !g_modern_pause_accept_blocked))&~ff_input::Cancel : 0);
}
static int __cdecl ModernQuitKey() {
    int native=reinterpret_cast<int(__cdecl*)()>(g_base+0x22F70)();
    const auto pad=ReadModernFrontendPad();
    const auto k=ReadModernKeyboard();
    const bool main=ModernMainMenu();
    const bool escape=g_modern_keyboard && main && (k.cancel || native==0x01);
    if(g_modern_keyboard && native==0x10)native=-1; // Q is no longer a quit shortcut.
    const bool keyboard=g_modern_keyboard && (k.quit || native==0x44);
    if(g_modern_keyboard && (native==0x44 || (main && native==0x01)))native=-1;
    const bool quit=ModernHasFocus() && (keyboard || (pad.buttons&ff_input::View) ||
        (main && (escape || (pad.buttons&ff_input::B)) && !g_modern_main_back_button));
    g_modern_main_back_button=k.cancel || (pad.buttons&ff_input::B) || escape;
    if(quit&&!g_modern_quit_button)native=0x10;
    g_modern_quit_button=quit;
    if(native==0x10) {
        g_modern_confirm.Begin(k.confirm||(pad.buttons&ff_input::A),k.cancel||(pad.buttons&(ff_input::B|ff_input::View)));
    }
    return native;
}
static int __cdecl ModernQuitAnswer() {
    const int native=reinterpret_cast<int(__cdecl*)()>(g_base+0x22F70)();
    if((!g_modern_keyboard&&!g_modern_gamepad) || native==0x15 || native==0x31)return native;
    const auto pad=ReadModernFrontendPad();
    if(!ModernHasFocus()) { g_modern_confirm.Begin(true,true);return -1; }
    const auto k=ReadModernKeyboard();
    const int answer=g_modern_confirm.Read(k.confirm||(pad.buttons&ff_input::A),
        k.cancel||(pad.buttons&(ff_input::B|ff_input::View)));
    return answer>=0?answer:native;
}
static int __cdecl ModernAnyKey() {
    const int native=reinterpret_cast<int(__cdecl*)()>(g_base+0x22F70)();
    return native<0 && ReadModernPad().buttons ? 0x1C:native;
}
static void InstallModernInputHooks() {
    g_modern_keyboard=GetPrivateProfileIntW(L"Input",L"ModernKeyboard",1,g_config_path)!=0;
    g_modern_gamepad=GetPrivateProfileIntW(L"Input",L"Gamepad",1,g_config_path)!=0;
    g_modern_controller=static_cast<int>(GetPrivateProfileIntW(L"Input",L"ControllerIndex",-1,g_config_path));
    if(g_modern_controller< -1 || g_modern_controller>3)g_modern_controller=-1;
    g_modern_second_controller=static_cast<int>(GetPrivateProfileIntW(L"Input",L"Controller2Index",-1,g_config_path));
    if(g_modern_second_controller< -1 || g_modern_second_controller>3)g_modern_second_controller=-1;
    g_modern_left_deadzone=static_cast<int>(GetPrivateProfileIntW(L"Input",L"LeftStickDeadzone",7849,g_config_path));
    g_modern_right_deadzone=static_cast<int>(GetPrivateProfileIntW(L"Input",L"RightStickDeadzone",8689,g_config_path));
    if(g_modern_left_deadzone<1000 || g_modern_left_deadzone>30000)g_modern_left_deadzone=7849;
    if(g_modern_right_deadzone<1000 || g_modern_right_deadzone>30000)g_modern_right_deadzone=8689;
    if(!g_modern_keyboard&&!g_modern_gamepad)return;
    struct Call { DWORD rva,target;void* replacement; };
    const Call calls[]={
        {0x4884E,0x22D70,reinterpret_cast<void*>(ModernMenuRead<true>)},
        {0x48857,0x22BE0,reinterpret_cast<void*>(ModernMenuRead<false>)},
        {0x48895,0x22D70,reinterpret_cast<void*>(ModernSecondMenuRead<true>)},
        {0x488A0,0x22BE0,reinterpret_cast<void*>(ModernSecondMenuRead<false>)},
        {0x4CD96,0x22D70,reinterpret_cast<void*>(ModernMenuRead<true>)},
        {0x4CD9D,0x22BE0,reinterpret_cast<void*>(ModernMenuRead<false>)},
        {0x4CE3C,0x22D70,reinterpret_cast<void*>(ModernSecondMenuRead<true>)},
        {0x4CE43,0x22BE0,reinterpret_cast<void*>(ModernSecondMenuRead<false>)},
        {0x4AA6A,0x22D70,reinterpret_cast<void*>(ModernPauseRead<true,false>)},
        {0x4AA75,0x22BE0,reinterpret_cast<void*>(ModernPauseRead<false,false>)},
        {0x4AB00,0x22D70,reinterpret_cast<void*>(ModernPauseRead<true,true>)},
        {0x4AB0C,0x22BE0,reinterpret_cast<void*>(ModernPauseRead<false,true>)},
        {0x42319,0x22F70,reinterpret_cast<void*>(ModernQuitKey)},
        {0x4245D,0x22F70,reinterpret_cast<void*>(ModernQuitAnswer)},
        {0x41BC5,0x22F70,reinterpret_cast<void*>(ModernAnyKey)},
        {0x486DC,0x22F70,reinterpret_cast<void*>(ModernAnyKey)}
    };
    for(const auto& call:calls) {
        auto code=reinterpret_cast<BYTE*>(g_base+call.rva);LONG offset;memcpy(&offset,code+1,4);
        if(code[0]!=0xE8 || reinterpret_cast<uintptr_t>(code+5)+offset!=g_base+call.target) {
            g_modern_keyboard=g_modern_gamepad=false;Log("Modern input signatures differ; native controls retained.\r\n");return;
        }
    }
    // Validate the complete set before making any call-site changes.
    auto start=reinterpret_cast<BYTE*>(g_base+0x41BC5);const SIZE_T length=0x4CE48-0x41BC5;
    DWORD old;if(!VirtualProtect(start,length,PAGE_EXECUTE_READWRITE,&old)) {g_modern_keyboard=g_modern_gamepad=false;return;}
    for(const auto& call:calls) {
        auto code=reinterpret_cast<BYTE*>(g_base+call.rva);
        LONG offset=static_cast<LONG>(reinterpret_cast<BYTE*>(call.replacement)-(code+5));memcpy(code+1,&offset,4);
    }
    DWORD ignored;VirtualProtect(start,length,old,&ignored);FlushInstructionCache(GetCurrentProcess(),start,length);
    Log("Modern gameplay, menu and pause controls enabled in process memory.\r\n");
    InstallAnalogMovementHook();
}
