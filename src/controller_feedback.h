#pragma once
#include <stdint.h>
namespace ff_feedback {
enum Event { Hit, Hurt, Block, Shot, Count };
struct Motors {
    uint16_t low=0,high=0;
    bool operator==(const Motors& other)const{return low==other.low && high==other.high;}
    bool operator!=(const Motors& other)const{return !(*this==other);}
};
struct Pulse {uint64_t start=0,end=0;};
struct Mixer {
    Pulse pulses[4][Count]={};
    void Clear(){*this={};}
    void Clear(unsigned slot){if(slot<4)for(auto& p:pulses[slot])p={};}
    void Add(unsigned slot,Event event,uint64_t now) {
        if(slot>=4 || event>=Count)return;
        auto& p=pulses[slot][event];
        // Repeated contacts cannot extend an active pulse or add motor strength.
        if(now<p.end)return;
        const unsigned duration[]={120,190,100,110};p={now,now+duration[event]};
    }
    Motors Read(unsigned slot,uint64_t now)const {
        Motors out;if(slot>=4)return out;
        const Motors strength[]={{20000,36000},{46000,23000},{14000,42000},{32000,30000}};
        for(unsigned e=0;e<Count;++e) {
            const auto& p=pulses[slot][e];if(!p.end || now<p.start || now>=p.end)continue;
            const auto duration=p.end-p.start,remaining=p.end-now;
            const auto scale=remaining*2<duration?remaining*2:duration;
            const auto low=static_cast<uint16_t>(strength[e].low*scale/duration);
            const auto high=static_cast<uint16_t>(strength[e].high*scale/duration);
            if(low>out.low)out.low=low;if(high>out.high)out.high=high;
        }
        return out;
    }
};
}
