#pragma once
#include "modern_input.h"

static bool g_modern_keyboard=true;
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
static uint32_t ModernGameplayActions(uint32_t native) {
    if(!ModernLiveGameplay() || !ModernHasFocus())return native;
    return native|ff_input::Gameplay(ReadModernKeyboard());
}
template<bool Joystick> static uint32_t NativeModernRead(int player) {
    return reinterpret_cast<uint32_t(__cdecl*)(int)>(g_base+(Joystick?0x22D70:0x22BE0))(player);
}
template<bool Joystick> static uint32_t __cdecl ModernMenuRead(int player) {
    const uint32_t native=NativeModernRead<Joystick>(player);
    if(player || !g_modern_keyboard || !ModernHasFocus() || InputField<int>(0x93D20)!=0)return native;
    return native|ff_input::Menu(ReadModernKeyboard());
}
template<bool Joystick,bool Inside> static uint32_t __cdecl ModernPauseRead(int player) {
    uint32_t native=NativeModernRead<Joystick>(player);
    if(!g_modern_keyboard || !ModernHasFocus() || !ModernLiveGameplay())return native;
    const auto keys=ReadModernKeyboard();
    // The native pause reader switches to frontend mode and treats Space as
    // pause. Reserve Space for jumping; Esc/P pause instead. The paused menu
    // uses horizontal choice: Left selects Continue before confirming it.
    const uint32_t pause=g_modern_pause.Read(keys.pause,Inside);
    if(pause)return pause;
    native&=~ff_input::Pause;
    return native|(Inside ? ff_input::Menu(keys,false)&~ff_input::Cancel : 0);
}
static int __cdecl ModernQuitKey() {
    const int native=reinterpret_cast<int(__cdecl*)()>(g_base+0x22F70)();
    if(native==0x10) {
        const auto k=ReadModernKeyboard();g_modern_confirm.Begin(k.confirm,k.cancel);
    }
    return native;
}
static int __cdecl ModernQuitAnswer() {
    const int native=reinterpret_cast<int(__cdecl*)()>(g_base+0x22F70)();
    if(!g_modern_keyboard || native==0x15 || native==0x31)return native;
    if(!ModernHasFocus()) { g_modern_confirm.Begin(true,true);return -1; }
    const auto k=ReadModernKeyboard();const int answer=g_modern_confirm.Read(k.confirm,k.cancel);
    return answer>=0?answer:native;
}
static void InstallModernInputHooks() {
    g_modern_keyboard=GetPrivateProfileIntW(L"Input",L"ModernKeyboard",1,g_config_path)!=0;
    if(!g_modern_keyboard)return;
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
        {0x4245D,0x22F70,reinterpret_cast<void*>(ModernQuitAnswer)}
    };
    for(const auto& call:calls) {
        auto code=reinterpret_cast<BYTE*>(g_base+call.rva);LONG offset;memcpy(&offset,code+1,4);
        if(code[0]!=0xE8 || reinterpret_cast<uintptr_t>(code+5)+offset!=g_base+call.target) {
            g_modern_keyboard=false;Log("Modern input signatures differ; native controls retained.\r\n");return;
        }
    }
    // Validate the complete set before making any call-site changes.
    auto start=reinterpret_cast<BYTE*>(g_base+0x42319);const SIZE_T length=0x4CE48-0x42319;
    DWORD old;if(!VirtualProtect(start,length,PAGE_EXECUTE_READWRITE,&old)) {g_modern_keyboard=false;return;}
    for(const auto& call:calls) {
        auto code=reinterpret_cast<BYTE*>(g_base+call.rva);
        LONG offset=static_cast<LONG>(reinterpret_cast<BYTE*>(call.replacement)-(code+5));memcpy(code+1,&offset,4);
    }
    DWORD ignored;VirtualProtect(start,length,old,&ignored);FlushInstructionCache(GetCurrentProcess(),start,length);
    Log("Modern keyboard, menu and pause controls enabled in process memory.\r\n");
}
