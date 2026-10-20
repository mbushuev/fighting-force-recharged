#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
static uintptr_t g_base;
static wchar_t g_config_path[]=L"modern-runtime-test-missing.ini";
static void Log(const char*) {}
#include "../src/modern_input_runtime.h"
static int failures,native_calls,scan=-1;
static uint32_t native_actions;
static ff_input::Keyboard keys;
static bool focus=true;
#define CHECK(x) do {if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);++failures;}}while(0)
static bool Focus() {return focus;}
static ff_input::Keyboard KeyboardState() {return keys;}
static uint32_t __cdecl NativeRead(int player) {++native_calls;return native_actions+player;}
static int __cdecl NativeScan() {return scan;}
static void Branch(DWORD rva,uintptr_t target) {
    auto code=reinterpret_cast<BYTE*>(g_base+rva);code[0]=0xE8;
    LONG offset=static_cast<LONG>(target-(g_base+rva+5));memcpy(code+1,&offset,4);
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
    keys.up=keys.jump=true;native_actions=0x800000;
    CHECK(reinterpret_cast<uint32_t(__cdecl*)(int)>(g_base+0x48853)(0)==(native_actions|Up|Confirm));
    CHECK(native_calls==1);CHECK(ModernMenuRead<false>(1)==native_actions+1);
    InputField<int>(0x93D20)=1;CHECK(ModernMenuRead<false>(0)==native_actions);
    InputField<int>(0x8745A0)=1;InputField<int16_t>(0x86A542)=100;
    CHECK(ModernGameplayActions(Kick)==(Kick|Up|Jump));
    focus=false;CHECK(ModernGameplayActions(Kick)==Kick);focus=true;
    InputField<int>(0x85A668)=1;CHECK(ModernGameplayActions(Kick)==Kick);
    InputField<int>(0x85A668)=0;InputField<int>(0x8745A0)=2;CHECK(ModernGameplayActions(Kick)==Kick);
    InputField<int>(0x8745A0)=1;native_actions=Pause;
    CHECK((ModernPauseRead<false,false>(0)&Pause)==0); // Space jumps without pausing.
    keys={};keys.pause=true;CHECK((ModernPauseRead<false,false>(0))==Pause);
    CHECK((ModernPauseRead<true,true>(0))==0); // Holding Esc cannot immediately resume.
    keys.pause=false;CHECK((ModernPauseRead<false,true>(0))==0);
    keys.pause=true;CHECK((ModernPauseRead<false,true>(0))==(Left|Confirm));
    scan=0x10;keys={};keys.confirm=true;CHECK(ModernQuitKey()==0x10);
    scan=-1;CHECK(ModernQuitAnswer()==-1);keys.confirm=false;CHECK(ModernQuitAnswer()==-1);
    keys.confirm=true;CHECK(ModernQuitAnswer()==0x15);
    scan=0x31;CHECK(ModernQuitAnswer()==0x31);
    VirtualFree(reinterpret_cast<void*>(g_base),0,MEM_RELEASE);
    printf("Modern native input boundaries: %s\n",failures?"FAIL":"PASS");return failures?1:0;
}
