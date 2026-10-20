#pragma once
#include <Xinput.h>
#include "modern_input.h"

static bool g_modern_keyboard=true,g_modern_gamepad=true;
static int g_modern_controller=-1,g_modern_left_deadzone=7849,g_modern_right_deadzone=8689;
static ff_input::PadSession g_modern_pad_session;
using ModernXInput=DWORD(WINAPI*)(DWORD,XINPUT_STATE*);
static ModernXInput g_modern_xinput;
static bool g_modern_quit_button,g_modern_pause_accept_blocked;
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
    return k;
}
static ff_input::Keyboard (*g_modern_keyboard_reader)()=PollModernKeyboard;
static ff_input::Keyboard ReadModernKeyboard() {return g_modern_keyboard?g_modern_keyboard_reader():ff_input::Keyboard{};}
static ff_input::Pad ReadModernPad() {
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
    const int tag=InputField<int>(0x85A6B8)*16+InputField<int>(0x8745A0)*4+(InputField<int>(0x85A668)==1?1:0);
    const auto pad=g_modern_pad_session.Read(GetTickCount64(),ModernHasFocus(),g_modern_controller,tag,
        g_modern_left_deadzone,g_modern_right_deadzone,[](int index,ff_input::Pad& output) {
            XINPUT_STATE state={};
            if(!g_modern_xinput || g_modern_xinput(index,&state)!=ERROR_SUCCESS)return false;
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
template<bool Joystick> static uint32_t __cdecl ModernMenuRead(int player) {
    uint32_t native=NativeModernRead<Joystick>(player);
    if(player || InputField<int>(0x93D20)!=0)return native;
    const auto pad=ReadModernPad();
    if(!ModernHasFocus())return Joystick&&g_modern_gamepad?0:native;
    if(pad.connected && Joystick)native=0;
    return native|ff_input::Menu(ReadModernKeyboard())|ff_input::Menu(pad,g_modern_left_deadzone);
}
template<bool Joystick,bool Inside> static uint32_t __cdecl ModernPauseRead(int player) {
    uint32_t native=NativeModernRead<Joystick>(player);
    if((!g_modern_keyboard&&!g_modern_gamepad) || !ModernLiveGameplay())return native;
    const auto pad=ReadModernPad();
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
    const auto pad=ReadModernPad();
    const bool quit=ModernHasFocus()&&(pad.buttons&ff_input::View)!=0;
    if(quit&&!g_modern_quit_button)native=0x10;
    g_modern_quit_button=quit;
    if(native==0x10) {
        const auto k=ReadModernKeyboard();
        g_modern_confirm.Begin(k.confirm||(pad.buttons&ff_input::A),k.cancel||(pad.buttons&(ff_input::B|ff_input::View)));
    }
    return native;
}
static int __cdecl ModernQuitAnswer() {
    const int native=reinterpret_cast<int(__cdecl*)()>(g_base+0x22F70)();
    if((!g_modern_keyboard&&!g_modern_gamepad) || native==0x15 || native==0x31)return native;
    const auto pad=ReadModernPad();
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
    g_modern_left_deadzone=static_cast<int>(GetPrivateProfileIntW(L"Input",L"LeftStickDeadzone",7849,g_config_path));
    g_modern_right_deadzone=static_cast<int>(GetPrivateProfileIntW(L"Input",L"RightStickDeadzone",8689,g_config_path));
    if(g_modern_left_deadzone<1000 || g_modern_left_deadzone>30000)g_modern_left_deadzone=7849;
    if(g_modern_right_deadzone<1000 || g_modern_right_deadzone>30000)g_modern_right_deadzone=8689;
    if(!g_modern_keyboard&&!g_modern_gamepad)return;
    struct Call { DWORD rva,target;void* replacement; };
    const Call calls[]={
        {0x4884E,0x22D70,reinterpret_cast<void*>(ModernMenuRead<true>)},
        {0x48857,0x22BE0,reinterpret_cast<void*>(ModernMenuRead<false>)},
        {0x48895,0x22D70,reinterpret_cast<void*>(ModernMenuRead<true>)},
        {0x488A0,0x22BE0,reinterpret_cast<void*>(ModernMenuRead<false>)},
        {0x4CD96,0x22D70,reinterpret_cast<void*>(ModernMenuRead<true>)},
        {0x4CD9D,0x22BE0,reinterpret_cast<void*>(ModernMenuRead<false>)},
        {0x4CE3C,0x22D70,reinterpret_cast<void*>(ModernMenuRead<true>)},
        {0x4CE43,0x22BE0,reinterpret_cast<void*>(ModernMenuRead<false>)},
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
