#pragma once
#include <stdint.h>
#include <math.h>
namespace ff_guard {
enum Phase {None=0,Entry=0x39,Exit=0x3A,Impact=0x3B,Hold=0x3C};
inline bool GuardAction(int action){return action>=Entry && action<=Hold;}
inline bool StartAction(int action) {
    return (action>=0x45 && action<=0x49) || action==0x4C || action==0x4E || action==0xC6 ||
        (action>=0x52 && action<=0x5A) ||
        (action>=0x5C && action<=0x61);
}
inline int Turn(int heading,int camera_heading,unsigned directions) {
    // Match the native eight movement sectors and 16-bit camera yaw, but
    // turn in place instead of selecting a locomotion or backstep animation.
    const int forward=((directions&1)!=0)-((directions&2)!=0);
    const int right=((directions&8)!=0)-((directions&4)!=0);
    if(!forward && !right)return heading&2047;
    const int relative=forward>0?(right>0?0x6000:right<0?0xA000:0x8000):
        forward<0?(right>0?0x2000:right<0?0xE000:0):right>0?0x4000:0xC000;
    const int target=((static_cast<unsigned>(camera_heading)+relative)&65535)>>5;
    const int delta=((target-heading+1024)&2047)-1024;
    const int step=delta>128?128:delta<-128?-128:delta;
    return (heading+step)&2047;
}
inline bool OrdinaryMelee(int action) {
    return (action>=0 && action<=0x12) || action==0x18 || action==0x19 ||
        (action>=0x1A && action<=0x1F) || action==0x27 || (action>=0x36 && action<=0x38);
}
inline bool Frontal(int heading,double dx,double dz) {
    const double length=hypot(dx,dz);if(length<1)return false;
    const double angle=(heading&2047)*(6.283185307179586/2048);
    // Actor forward is +X at heading 512 and -Z at heading zero. Camera
    // yaw has the opposite zero direction; do not use its forward vector here.
    return (sin(angle)*dx-cos(angle)*dz)/length>=0.38268343236509; // +/-67.5 degrees.
}
struct GuardState {
    Phase phase=None;bool released=false;
    // Return the desired action, or None when native control owns the actor.
    Phase Tick(bool down,bool allowed,bool owned,bool finished,bool can_start) {
        // A disabled input reader supplies neutral values. Only observe a
        // release while control is available, never while focus is suspended.
        if(phase!=None && !owned){phase=None;released=!down;}
        if(!allowed)released=false;
        else if(!down)released=true;
        if(!allowed)down=false;
        if(phase==None) {
            if(down && allowed && released && can_start){phase=Entry;released=false;}
        } else if(phase==Exit) {
            if(finished)phase=None;
        } else if(!down)phase=Exit;
        else if(finished)phase=Hold;
        return phase;
    }
    bool Protects() const {return phase==Entry || phase==Hold || phase==Impact;}
};
// One response per continuous attack window and defender, independently for
// each attacker. Frame rewind/new clip and native reset events start a new window.
struct AttackWindow {
    uintptr_t clip=0;int action=-1,frame=-1;unsigned flags=0,blocked=0;
    void Observe(uintptr_t next_clip,int next_action,int next_frame,unsigned next_flags) {
        const bool active=(next_flags&0x1FF)!=0; // Native contacts use nine limb bits.
        if(next_clip!=clip || next_action!=action || next_frame<frame || !active ||
           (!(flags&0x1FF) && active) || ((next_flags&0x800) && !(flags&0x800)))blocked=0;
        clip=next_clip;action=next_action;frame=next_frame;flags=next_flags;
    }
    bool First(unsigned player) {
        const unsigned bit=1u<<player;const bool first=(blocked&bit)==0;blocked|=bit;return first;
    }
};
}
