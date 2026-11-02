#pragma once
#include "modern_input.h"
namespace ff_input {
enum class Device {Keyboard,Gamepad};
struct Activity {
    Device device=Device::Keyboard;
    uint64_t serial=0;
    uint32_t keys[2]={};
    uint8_t physical[256]={};
    Pad pads[4];
    void Use(Device next){device=next;++serial;}
    void Keyboard(unsigned player,const ff_input::Keyboard& k) {
        if(player>=2)return;
        const uint32_t mask=Gameplay(k)|(k.pause?1u<<16:0)|(k.confirm?1u<<17:0)|
            (k.cancel?1u<<18:0)|(k.quit?1u<<19:0)|(k.first_person?1u<<20:0)|
            (k.recenter?1u<<21:0)|(k.zoom_in?1u<<22:0)|(k.zoom_out?1u<<23:0)|(k.block?1u<<24:0)|
            (k.previous_target?1u<<25:0)|(k.next_target?1u<<26:0);
        if(mask&~keys[player])Use(Device::Keyboard);keys[player]=mask;
    }
    void Physical(const uint8_t* state) {
        bool fresh=false;
        for(unsigned i=0;i<256;++i){fresh|=(state[i]&128) && !(physical[i]&128);physical[i]=state[i];}
        if(fresh)Use(Device::Keyboard);
    }
    void Controller(unsigned slot,const Pad& p,int left_deadzone,int right_deadzone) {
        if(slot>=4)return;const auto old=pads[slot];pads[slot]=p;
        if(!p.connected)return;
        const auto moved=[](int x,int y,int ox,int oy,int deadzone) {
            const auto stick=Stick(x,y,deadzone);
            return (fabs(stick.x)>.16 || fabs(stick.y)>.16) && (fabs(double(x-ox))>2048 || fabs(double(y-oy))>2048);
        };
        if((p.buttons&~old.buttons) || (p.lt>38 && p.lt>old.lt+8) || (p.rt>38 && p.rt>old.rt+8) ||
           moved(p.lx,p.ly,old.lx,old.ly,left_deadzone) || moved(p.rx,p.ry,old.rx,old.ry,right_deadzone))Use(Device::Gamepad);
    }
    void Reset(){const auto previous=device;*this={};device=previous;}
};
}
