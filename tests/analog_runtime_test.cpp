#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
static uintptr_t g_base;
static wchar_t g_config_path[]=L"analog-runtime-test-missing.ini";
static void Log(const char*) {}
#include "../src/modern_input_runtime.h"
static int failures,collisions;
static int32_t collision_x,collision_z;
static bool focus=true;
static XINPUT_STATE controller;
static DWORD WINAPI ControllerRead(DWORD index,XINPUT_STATE* output) {
    if(index!=0)return ERROR_DEVICE_NOT_CONNECTED;*output=controller;return ERROR_SUCCESS;
}
static ULONGLONG ticks=1000;
static ULONGLONG WINAPI Clock() {return ticks;}
static bool Focus() {return focus;}
#define CHECK(x) do {if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);++failures;}}while(0)
static int __cdecl NativeCollision(void*,int32_t* x,int32_t* z) {
    ++collisions;collision_x=*x;collision_z=*z;*x-=32768;return 123;
}
static void Branch(DWORD rva,BYTE opcode,uintptr_t target) {
    auto p=reinterpret_cast<BYTE*>(g_base+rva);p[0]=opcode;
    LONG offset=static_cast<LONG>(target-(g_base+rva+5));memcpy(p+1,&offset,4);
}
static uint32_t walk_frames[]={2,8,20,0,0,8*65536,0,0,12*65536};
static uint32_t run_frames[]={2,8,20,0,0,28*65536,0,0,32*65536};
static const uint32_t* animations[0x58];
int main() {
    using namespace ff_input;
    g_base=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,0x900000,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    CHECK(g_base!=0);if(!g_base)return 1;
    InstallAnalogMovementHook();CHECK(!g_modern_analog); // Reject an unknown call site.
    Branch(0x38855,0xE8,g_base+0x37CE0);InstallAnalogMovementHook();CHECK(g_modern_analog);
    Branch(0x37CE0,0xE9,reinterpret_cast<uintptr_t>(NativeCollision));
    // Execute the patched cdecl call with the original argument convention.
    const BYTE push_args[]={0xFF,0x74,0x24,0x0C,0xFF,0x74,0x24,0x0C,0xFF,0x74,0x24,0x0C};
    memcpy(reinterpret_cast<void*>(g_base+0x38849),push_args,sizeof(push_args));
    const BYTE cleanup[]={0x83,0xC4,0x0C,0xC3};memcpy(reinterpret_cast<void*>(g_base+0x3885A),cleanup,4);
    auto collide=reinterpret_cast<int(__cdecl*)(void*,int32_t*,int32_t*)>(g_base+0x38849);
    g_modern_focus=Focus;g_analog_clock=Clock;
    g_controls.players[0].pad[ff_controls::Run]=ff_controls::RT;
    g_controls.players[0].pad[ff_controls::Fire]=0;
    InputField<int>(0x8745A0)=1;InputField<int16_t>(0x86A542)=100;
    InputField<int8_t>(0x86A57D)=InputField<int8_t>(0x86A57E)=-1;
    InputField<int>(0x86A50C)=0x52;
    animations[0x52]=walk_frames;animations[0x57]=run_frames;
    InputField<const uint32_t**>(0x879780)=animations;
    CHECK(AnalogAnimationSpeed(walk_frames)==10 && AnalogAnimationSpeed(run_frames)==30);
    CHECK(AnalogAnimationSpeed(nullptr)==0);
    auto actor=reinterpret_cast<void*>(g_base+0x86A500);
    Pad pad;pad.connected=true;pad.ly=32767;
    CHECK(UpdateAnalogMovement(0,{},pad)==0 && g_analog_players[0].active);
    int32_t x=0,z=10*65536;
    CHECK(collide(actor,&x,&z)==123 && collisions==1);
    CHECK(collision_x==0 && collision_z==5*65536 && x==-32768 && z==collision_z);
    for(int i=0;i<10;++i) {ticks+=33;UpdateAnalogMovement(0,{},pad);}
    x=10*65536;z=10*65536;collide(actor,&x,&z);
    CHECK(fabs(hypot(collision_x,collision_z)/65536.0-10)<0.0001);
    pad.ly=11000;for(int i=0;i<10;++i) {ticks+=33;UpdateAnalogMovement(0,{},pad);}
    x=0;z=10*65536;collide(actor,&x,&z);CHECK(collision_z>0 && collision_z<3*65536);
    // Combat, airborne poses, AI actors and carrying retain their proposal.
    const int protected_actions[]={0x62,0x6A,0x80};
    for(int action : protected_actions) {
        InputField<int>(0x86A50C)=action;x=0;z=10*65536;collide(actor,&x,&z);CHECK(collision_z==10*65536);
    }
    InputField<int>(0x86A50C)=0x52;
    x=0;z=10*65536;collide(reinterpret_cast<BYTE*>(actor)+0xF4,&x,&z);CHECK(collision_z==10*65536);
    InputField<int8_t>(0x86A57D)=0;CHECK(UpdateAnalogMovement(0,{},pad)==0 && !g_analog_players[0].active);
    InputField<int8_t>(0x86A57D)=-1;
    // A second movement source restores native speed instead of fighting it.
    Keyboard keys;keys.up=true;UpdateAnalogMovement(0,keys,pad);CHECK(!g_analog_players[0].active);
    UpdateAnalogMovement(Up,{},pad);CHECK(!g_analog_players[0].active);
    pad.buttons=DpadUp;UpdateAnalogMovement(0,{},pad);CHECK(!g_analog_players[0].active);pad.buttons=0;
    focus=false;UpdateAnalogMovement(0,{},pad);CHECK(!g_analog_players[0].active);focus=true;
    InputField<int>(0x85A668)=1;UpdateAnalogMovement(0,{},pad);CHECK(!g_analog_players[0].active);InputField<int>(0x85A668)=0;
    InputField<int>(0x8745A0)=2;UpdateAnalogMovement(0,{},pad);CHECK(!g_analog_players[0].active);InputField<int>(0x8745A0)=1;
    pad.ly=32767;UpdateAnalogMovement(0,{},pad);ticks+=1000;UpdateAnalogMovement(0,{},pad);
    CHECK(g_analog_players[0].motion.speed==5); // Restart the ramp after a paused time gap.
    ticks+=33;UpdateAnalogMovement(0,{},pad);CHECK(g_analog_players[0].motion.speed>5 && g_analog_players[0].motion.speed<11);
    // Unreadable animation data cannot enable invented movement speeds.
    animations[0x57]=nullptr;UpdateAnalogMovement(0,{},pad);CHECK(!g_analog_players[0].active);
    // A saved manual Run button keeps the existing walk and run behavior.
    animations[0x57]=run_frames;g_modern_keyboard=false;g_modern_xinput=ControllerRead;
    InputField<int>(0x6F040)=1;ReadModernPad();
    controller.Gamepad.sThumbLY=32767;g_modern_pad_session.last_poll=0;
    CHECK(ModernGameplayActions(Left|Kick)==Up && g_analog_players[0].active);
    const WORD attacks[]={XINPUT_GAMEPAD_X,XINPUT_GAMEPAD_Y};
    for(WORD attack:attacks) {
        controller.Gamepad.wButtons=attack;g_modern_pad_session.last_poll=0;
        CHECK(ModernGameplayActions(Left|Run)==(Up|(attack==XINPUT_GAMEPAD_X?Punch:Kick)));
    }
    controller.Gamepad.wButtons=XINPUT_GAMEPAD_LEFT_THUMB;g_modern_pad_session.last_poll=0;
    CHECK(ModernGameplayActions(0)==Up && !g_analog_players[0].motion.running);
    controller.Gamepad.wButtons=XINPUT_GAMEPAD_RIGHT_SHOULDER;g_modern_pad_session.last_poll=0;
    CHECK(ModernGameplayActions(0)==Up && !g_analog_players[0].motion.running); // Pickup cannot start running.
    controller.Gamepad.wButtons=0;controller.Gamepad.bRightTrigger=255;g_modern_pad_session.last_poll=0;
    CHECK(ModernGameplayActions(0)==(Up|Run));
    controller.Gamepad.bRightTrigger=0;g_modern_pad_session.last_poll=0;
    CHECK(ModernGameplayActions(0)==Up && !g_analog_players[0].motion.running && g_analog_players[0].motion.speed<=10);
    g_controls.players[0]=ff_controls::Defaults();
    controller.Gamepad.sThumbLY=31500;g_modern_pad_session.last_poll=0;
    CHECK(ModernGameplayActions(0)==(Up|Run));
    controller.Gamepad.sThumbLY=27500;g_modern_pad_session.last_poll=0;
    CHECK(ModernGameplayActions(0)==(Up|Run)); // Stay running near the outer edge.
    controller.Gamepad.sThumbLY=25000;g_modern_pad_session.last_poll=0;
    CHECK(ModernGameplayActions(0)==Up);
    // The second player has an independent ramp before the same native solver.
    InputField<int>(0x8745A0)=2;g_modern_coop_input=true;
    InputField<int16_t>(0x86A636)=100;
    InputField<int8_t>(0x86A671)=InputField<int8_t>(0x86A672)=-1;
    InputField<int>(0x86A600)=0x57;
    pad.ly=32767;CHECK(UpdateAnalogMovement(0,{},pad,1)==Run && g_analog_players[1].active);
    x=0;z=30*65536;collide(reinterpret_cast<BYTE*>(actor)+0xF4,&x,&z);
    CHECK(collision_z==5*65536 && g_analog_players[1].motion.speed==5);
    InputField<int>(0x8745A0)=1;g_modern_coop_input=false;
    InputField<int>(0x6F040)=0;g_modern_pad_session.last_poll=0;
    CHECK(ModernGameplayActions(Left)==(Left|Up) && !g_analog_players[0].active);
    // First-person keyboard movement needs this boundary even with pad and
    // analog speed disabled. Installing twice must not wrap the hook again.
    g_actor_collision_hook=false;g_modern_gamepad=false;g_modern_analog=false;
    Branch(0x38855,0xE8,g_base+0x37CE0);
    CHECK(InstallAnalogMovementHook() && g_actor_collision_hook && !g_modern_analog);
    LONG first_offset,second_offset;memcpy(&first_offset,reinterpret_cast<void*>(g_base+0x38856),4);
    CHECK(InstallAnalogMovementHook());memcpy(&second_offset,reinterpret_cast<void*>(g_base+0x38856),4);
    CHECK(first_offset==second_offset);
    VirtualFree(reinterpret_cast<void*>(g_base),0,MEM_RELEASE);
    printf("Analog native collision boundary: %s\n",failures?"FAIL":"PASS");return failures?1:0;
}
