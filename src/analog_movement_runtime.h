#pragma once
#include "analog_movement.h"

static bool g_modern_analog;
static ff_input::AnalogMotion g_analog_motion;
static bool g_analog_active;
static int g_analog_stage=-1,g_analog_character=-1;
static ULONGLONG g_analog_time;
static ULONGLONG (WINAPI *g_analog_clock)()=GetTickCount64;
struct AnalogRates {
    const void* table=nullptr;
    const void* walk_frames=nullptr;
    const void* run_frames=nullptr;
    double walk=0,run=0;
};
static AnalogRates g_analog_rates;
static bool AnalogReadable(const void* pointer,SIZE_T bytes) {
    uintptr_t cursor=reinterpret_cast<uintptr_t>(pointer),end=cursor+bytes;
    if(!cursor || end<cursor)return false;
    while(cursor<end) {
        MEMORY_BASIC_INFORMATION page={};
        if(!VirtualQuery(reinterpret_cast<void*>(cursor),&page,sizeof(page)) || page.State!=MEM_COMMIT ||
           (page.Protect&(PAGE_NOACCESS|PAGE_GUARD)))return false;
        const uintptr_t next=reinterpret_cast<uintptr_t>(page.BaseAddress)+page.RegionSize;
        if(next<=cursor)return false;cursor=next;
    }
    return true;
}
static double AnalogAnimationSpeed(const uint32_t* animation) {
    if(!AnalogReadable(animation,4) || !*animation || *animation>1024)return 0;
    const uint32_t count=*animation;
    if(!AnalogReadable(animation+1,count*4))return 0;
    double total=0;
    for(unsigned i=0;i<count;++i) {
        const auto root=reinterpret_cast<const BYTE*>(animation+1)+(animation[i+1]&0xFFFF);
        if(!AnalogReadable(root,12))return 0;
        int32_t lateral,forward;memcpy(&lateral,root+4,4);memcpy(&forward,root+8,4);
        total+=hypot(lateral/65536.0,forward/65536.0);
    }
    const double mean=total/count;
    return mean>=1 && mean<=128?mean:0;
}
static bool ReadAnalogRates() {
    const int character=InputField<int16_t>(0x86A534),stage=InputField<int>(0x85A6B8);
    if(character<0 || character>3)return false;
    const auto table=InputField<const uint32_t**>(0x879780+character*4);
    if(!AnalogReadable(table,0x58*4))return false;
    const auto walk=table[0x52],run=table[0x57];
    if(character!=g_analog_character || stage!=g_analog_stage || table!=g_analog_rates.table ||
       walk!=g_analog_rates.walk_frames || run!=g_analog_rates.run_frames) {
        g_analog_motion={};g_analog_time=0;g_analog_character=character;g_analog_stage=stage;
        g_analog_rates={table,walk,run,AnalogAnimationSpeed(walk),AnalogAnimationSpeed(run)};
        char message[128];sprintf_s(message,"Analog character %d stage %d: walk %.3f, run %.3f units per update.\r\n",
            character,stage,g_analog_rates.walk,g_analog_rates.run);Log(message);
    }
    return g_analog_rates.walk>0 && g_analog_rates.run>=g_analog_rates.walk;
}
static void ResetAnalogMotion() {g_analog_active=false;g_analog_motion={};g_analog_time=0;}
static uint32_t UpdateAnalogMovement(uint32_t native,const ff_input::Keyboard& keys,const ff_input::Pad& pad) {
    const auto stick=ff_input::Stick(pad.lx,pad.ly,g_modern_left_deadzone);
    const double magnitude=hypot(stick.x,stick.y);
    const bool holding=InputField<int8_t>(0x86A57D)>=0 || InputField<int8_t>(0x86A57E)>=0;
    if(!g_modern_analog || !ModernLiveGameplay() || !ModernHasFocus() || !pad.connected || holding ||
       (native&15) || (ff_input::Gameplay(keys)&15) || (pad.buttons&15) || magnitude==0 || !ReadAnalogRates()) {
        ResetAnalogMotion();return 0;
    }
    const ULONGLONG now=g_analog_clock();
    if(g_analog_time && now-g_analog_time>250) {g_analog_motion={};g_analog_time=0;}
    const double seconds=g_analog_time ? fmin(0.1,(now-g_analog_time)/1000.0):1.0/30;
    g_analog_time=now;g_analog_active=true;
    g_analog_motion.Update(magnitude,g_analog_rates.walk,g_analog_rates.run,
        (pad.buttons&ff_input::RightShoulder)!=0 || keys.run || (native&ff_input::Run),seconds);
    return g_analog_motion.running?ff_input::Run:0;
}
static int __cdecl ModernAnalogCollision(void* actor,int32_t* x,int32_t* z) {
    const int action=InputField<int>(0x86A50C);
    if(g_analog_active && g_modern_analog && ModernHasFocus() && ModernLiveGameplay() &&
       actor==reinterpret_cast<void*>(g_base+0x86A500) && x && z &&
       (action==0x52 || action==0x57 || action==0x5C) && InputField<int16_t>(0x86A550)==0 &&
       InputField<int8_t>(0x86A57D)<0 && InputField<int8_t>(0x86A57E)<0) {
        const int32_t px=InputField<int32_t>(0x86A528),pz=InputField<int32_t>(0x86A530);
        const double dx=double(*x)-px,dz=double(*z)-pz,length=hypot(dx,dz);
        if(length>0 && length<256*65536.0) {
            const double scale=g_analog_motion.speed*65536.0/length;
            const double next_x=px+round(dx*scale),next_z=pz+round(dz*scale);
            if(next_x>=INT32_MIN && next_x<=INT32_MAX && next_z>=INT32_MIN && next_z<=INT32_MAX) {
                *x=static_cast<int32_t>(next_x);*z=static_cast<int32_t>(next_z);
            }
        }
    }
    // The native solver receives the new proposal and keeps authority over
    // walls, objects and sliding. Never move the actor after collision checks.
    return reinterpret_cast<int(__cdecl*)(void*,int32_t*,int32_t*)>(g_base+0x37CE0)(actor,x,z);
}
static void InstallAnalogMovementHook() {
    if(!g_modern_gamepad || !GetPrivateProfileIntW(L"Input",L"AnalogMovement",1,g_config_path))return;
    auto code=reinterpret_cast<BYTE*>(g_base+0x38855);LONG offset;memcpy(&offset,code+1,4);
    if(code[0]!=0xE8 || reinterpret_cast<uintptr_t>(code+5)+offset!=g_base+0x37CE0) {
        Log("Analog movement signature differs; native movement retained.\r\n");return;
    }
    DWORD old;if(!VirtualProtect(code,5,PAGE_EXECUTE_READWRITE,&old))return;
    offset=static_cast<LONG>(reinterpret_cast<BYTE*>(ModernAnalogCollision)-(code+5));memcpy(code+1,&offset,4);
    DWORD ignored;VirtualProtect(code,5,old,&ignored);FlushInstructionCache(GetCurrentProcess(),code,5);
    g_modern_analog=true;Log("Continuous stick speed enabled before native actor collision.\r\n");
}
