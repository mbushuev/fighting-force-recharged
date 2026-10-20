#pragma once
#include <stdint.h>
#include <math.h>

namespace ff_input {
enum Action : uint32_t {
    Up=1, Down=2, Left=4, Right=8, Kick=0x20, Punch=0x40,
    Jump=0x80, Backfist=0x100, NativeCamera=0x200, Run=0x400,
    Confirm=0x20, Cancel=0x40, Pause=0x1000
};
struct Keyboard {
    bool up=false, down=false, left=false, right=false;
    bool punch=false, kick=false, jump=false, backfist=false, run=false;
    bool pause=false, confirm=false, cancel=false;
};
inline uint32_t Directions(bool up, bool down, bool left, bool right) {
    return (up!=down ? (up ? Up : Down) : 0) |
           (left!=right ? (left ? Left : Right) : 0);
}
inline uint32_t Gameplay(const Keyboard& k) {
    return Directions(k.up,k.down,k.left,k.right) | (k.punch?Punch:0) |
        (k.kick?Kick:0) | (k.jump?Jump:0) | (k.backfist?Backfist:0) | (k.run?Run:0);
}
inline uint32_t Menu(const Keyboard& k, bool space_confirm=true) {
    return Directions(k.up,k.down,k.left,k.right) |
        ((k.confirm || (space_confirm && k.jump)) ? Confirm:0) | (k.cancel?Cancel:0);
}
enum Button : uint16_t {
    DpadUp=0x0001,DpadDown=0x0002,DpadLeft=0x0004,DpadRight=0x0008,
    Start=0x0010,View=0x0020,LeftThumb=0x0040,RightThumb=0x0080,
    LeftShoulder=0x0100,RightShoulder=0x0200,A=0x1000,B=0x2000,X=0x4000,Y=0x8000
};
struct Pad {
    bool connected=false;
    uint16_t buttons=0;
    int16_t lx=0,ly=0,rx=0,ry=0;
    uint8_t lt=0,rt=0;
};
struct Axes {double x=0,y=0;};
inline Axes Stick(int x,int y,int deadzone) {
    const double length=sqrt(static_cast<double>(x)*x+static_cast<double>(y)*y);
    if(length<=deadzone)return {};
    const double magnitude=fmin(1.0,(length-deadzone)/(32767.0-deadzone));
    return {x*magnitude/length,y*magnitude/length};
}
inline uint32_t PadDirections(const Pad& p,int deadzone) {
    const auto s=Stick(p.lx,p.ly,deadzone);
    // Native heading selection has eight equal sectors. Analog physical speed
    // is applied separately at the actor's collision boundary.
    const double diagonal=0.414213562373095;
    const bool vertical=fabs(s.y)>fabs(s.x)*diagonal;
    const bool horizontal=fabs(s.x)>fabs(s.y)*diagonal;
    return Directions((vertical&&s.y>0)||(p.buttons&DpadUp),
        (vertical&&s.y<0)||(p.buttons&DpadDown),
        (horizontal&&s.x<0)||(p.buttons&DpadLeft),(horizontal&&s.x>0)||(p.buttons&DpadRight));
}
inline uint32_t Gameplay(const Pad& p,int deadzone) {
    if(!p.connected)return 0;
    return PadDirections(p,deadzone) | (p.buttons&A?Jump:0) | (p.buttons&X?Punch:0) |
        (p.buttons&Y?Kick:0) | (p.buttons&B?Backfist:0) |
        (p.buttons&(RightShoulder|LeftThumb)?Run:0);
}
inline uint32_t Menu(const Pad& p,int deadzone,bool start_confirm=true,bool accept=true) {
    if(!p.connected)return 0;
    return PadDirections(p,deadzone) |
        ((accept&&(p.buttons&A)) || (start_confirm&&(p.buttons&Start)) ? Confirm:0) |
        (p.buttons&B?Cancel:0);
}
struct PadSession {
    int selected=-1,context=-1;
    uint64_t last_poll=0,retry_at=0;
    Pad raw;
    bool ready=false;
    template<typename Reader> Pad Read(uint64_t now,bool focus,int preferred,int tag,
                                      int left_deadzone,int right_deadzone,Reader reader) {
        if(!focus) {ready=false;last_poll=0;return {};}
        if(tag!=context) {context=tag;ready=false;last_poll=0;}
        if(!last_poll || now-last_poll>=4) {
            last_poll=now;
            if(selected>=0) {
                if(!reader(selected,raw)) {selected=-1;raw={};ready=false;retry_at=now+500;}
            } else if(now>=retry_at) {
                raw={};
                for(int i=0;i<4;++i) {
                    const int index=preferred>=0?preferred:i;
                    if(reader(index,raw)) {selected=index;break;}
                    raw={};if(preferred>=0)break;
                }
                if(selected<0)retry_at=now+500;
            }
        }
        if(!raw.connected)return {};
        if(!ready) {
            const auto l=Stick(raw.lx,raw.ly,left_deadzone),r=Stick(raw.rx,raw.ry,right_deadzone);
            ready=!raw.buttons && l.x==0 && l.y==0 && r.x==0 && r.y==0 && raw.lt<=30 && raw.rt<=30;
            Pad neutral;neutral.connected=true;return neutral;
        }
        return raw;
    }
};
// A held attack/confirm must not answer a newly opened modal prompt. Releasing
// an input arms its next press; focus loss starts a new neutral boundary.
struct ConfirmGate {
    bool accept=false, cancel=false;
    void Begin(bool a,bool c) { accept=a;cancel=c; }
    int Read(bool a,bool c) {
        const bool yes=a&&!accept, no=c&&!cancel;accept=a;cancel=c;
        return no?0x31:yes?0x15:-1;
    }
};
struct PauseGate {
    bool held=false;
    uint32_t Read(bool down,bool inside) {
        const bool fresh=down&&!held;held=down;
        return inside ? (fresh ? Left|Confirm:0) : (down ? Pause:0);
    }
};
} // namespace ff_input
