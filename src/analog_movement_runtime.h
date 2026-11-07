#pragma once
#include "analog_movement.h"

static bool g_modern_analog;
static bool g_actor_collision_hook;
// Optional camera-relative ground proposal, applied before speed scaling and
// the native solver. The camera layer supplies it only after its hooks succeed.
static void (*g_first_person_movement)(void*,int32_t*,int32_t*);
static ULONGLONG (WINAPI *g_analog_clock)()=GetTickCount64;
struct AnalogRates {
    const void* table=nullptr;
    const void* walk_frames=nullptr;
    const void* run_frames=nullptr;
    double walk=0,run=0;
};
struct AnalogPlayer {
    ff_input::AnalogMotion motion;
    bool active=false;
    int stage=-1,character=-1;
    ULONGLONG time=0;
    AnalogRates rates;
    void ResetMotion(){active=false;motion={};time=0;}
};
static AnalogPlayer g_analog_players[2];
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
static bool ReadAnalogRates(unsigned player,AnalogPlayer& state) {
    const int character=InputField<int16_t>(0x86A534+player*0xF4),stage=InputField<int>(0x85A6B8);
    if(character<0 || character>3)return false;
    const auto table=InputField<const uint32_t**>(0x879780+character*4);
    if(!AnalogReadable(table,0x58*4))return false;
    const auto walk=table[0x52],run=table[0x57];
    auto& rates=state.rates;
    if(character!=state.character || stage!=state.stage || table!=rates.table ||
       walk!=rates.walk_frames || run!=rates.run_frames) {
        state.ResetMotion();state.character=character;state.stage=stage;
        rates={table,walk,run,AnalogAnimationSpeed(walk),AnalogAnimationSpeed(run)};
        char message[128];sprintf_s(message,"Analog character %d stage %d: walk %.3f, run %.3f units per update.\r\n",
            character,stage,rates.walk,rates.run);Log(message);
    }
    return rates.walk>0 && rates.run>=rates.walk;
}
static void ResetAnalogMotion() {for(auto& state:g_analog_players)state.ResetMotion();}
static bool AnalogPlayerInGame(unsigned player) {
    return (ModernLiveGameplay() || ModernCooperativeGameplay()) && InputField<int16_t>(0x86A542+player*0xF4)>0;
}
static uint32_t UpdateAnalogMovement(uint32_t native,const ff_input::Keyboard& keys,const ff_input::Pad& pad,
                                    unsigned player=0,bool firing=false) {
    auto& state=g_analog_players[player];
    const auto stick=ff_input::Stick(pad.lx,pad.ly,g_modern_left_deadzone);
    const double magnitude=hypot(stick.x,stick.y);
    if(!AnalogPlayerInGame(player) || !ModernHasFocus() || !pad.connected ||
       (native&15) || (ff_input::Gameplay(keys)&15) || (pad.buttons&15) || magnitude==0) {
        state.ResetMotion();return 0;
    }
    const bool automatic=!g_controls.players[player].pad[ff_controls::Run] && !firing;
    const unsigned offset=player*0xF4;
    const bool holding=InputField<int8_t>(0x86A57D+offset)>=0 || InputField<int8_t>(0x86A57E+offset)>=0;
    if(!g_modern_analog || holding || !ReadAnalogRates(player,state)) {
        state.active=false;state.time=0;state.motion.speed=0;
        state.motion.running=automatic && ff_input::AutomaticRun(magnitude,state.motion.running);
        return state.motion.running?ff_input::Run:0;
    }
    const ULONGLONG now=g_analog_clock();
    if(state.time && now-state.time>250)state.ResetMotion();
    const double seconds=state.time ? fmin(0.1,(now-state.time)/1000.0):1.0/30;
    state.time=now;state.active=true;
    state.motion.Update(magnitude,state.rates.walk,state.rates.run,
        (pad.buttons&ff_input::RightShoulder)!=0 || keys.run || (native&ff_input::Run),seconds,automatic);
    return state.motion.running?ff_input::Run:0;
}
static int __cdecl ModernAnalogCollision(void* actor,int32_t* x,int32_t* z) {
    if(g_first_person_movement && ModernHasFocus())g_first_person_movement(actor,x,z);
    const unsigned player=actor==reinterpret_cast<void*>(g_base+0x86A5F4)?1:0,offset=player*0xF4;
    const auto& state=g_analog_players[player];
    const int action=InputField<int>(0x86A50C+offset);
    if(state.active && g_modern_analog && ModernHasFocus() && AnalogPlayerInGame(player) &&
       actor==reinterpret_cast<void*>(g_base+0x86A500+offset) && x && z &&
       (action==0x52 || action==0x57 || action==0x5C) && InputField<int16_t>(0x86A550+offset)==0 &&
       InputField<int8_t>(0x86A57D+offset)<0 && InputField<int8_t>(0x86A57E+offset)<0) {
        const int32_t px=InputField<int32_t>(0x86A528+offset),pz=InputField<int32_t>(0x86A530+offset);
        const double dx=double(*x)-px,dz=double(*z)-pz,length=hypot(dx,dz);
        if(length>0 && length<256*65536.0) {
            const double scale=state.motion.speed*65536.0/length;
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
static bool InstallAnalogMovementHook() {
    if(g_actor_collision_hook)return true;
    const bool analog=g_modern_gamepad && GetPrivateProfileIntW(L"Input",L"AnalogMovement",1,g_config_path)!=0;
    const bool first_person=GetPrivateProfileIntW(L"Camera",L"FreeOrbit",1,g_config_path)!=0 &&
        GetPrivateProfileIntW(L"Camera",L"FirstPerson",1,g_config_path)!=0;
    if(!analog && !first_person)return false;
    auto code=reinterpret_cast<BYTE*>(g_base+0x38855);LONG offset;memcpy(&offset,code+1,4);
    if(code[0]!=0xE8 || reinterpret_cast<uintptr_t>(code+5)+offset!=g_base+0x37CE0) {
        Log("Actor movement signature differs; native movement retained.\r\n");return false;
    }
    DWORD old;if(!VirtualProtect(code,5,PAGE_EXECUTE_READWRITE,&old))return false;
    offset=static_cast<LONG>(reinterpret_cast<BYTE*>(ModernAnalogCollision)-(code+5));memcpy(code+1,&offset,4);
    DWORD ignored;VirtualProtect(code,5,old,&ignored);FlushInstructionCache(GetCurrentProcess(),code,5);
    g_actor_collision_hook=true;g_modern_analog=analog;
    Log("Actor movement proposal hook enabled before native collision.\r\n");return true;
}
