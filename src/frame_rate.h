#pragma once
#include <stdint.h>

namespace ff_frame {
struct RateSamples {
    unsigned count=0;
    int64_t began=0,last_time=0;
    uint32_t first_tick=0,last_tick=0;
    int last_stage=-1;
    bool ready=false;
    bool Add(int64_t now,int64_t frequency,uint32_t tick,int stage,double& frames,double& updates) {
        if(frequency<=0) {count=0;ready=false;return false;}
        // Each completed image sees the same native tick or its successor.
        // A stage restart is an epoch change, not billions of wrapped updates.
        if(ready && (stage!=last_stage || uint32_t(tick-last_tick)>1 || now<last_time ||
                     now-last_time>frequency/4))count=0;
        last_time=now;last_tick=tick;last_stage=stage;ready=true;
        if(!count) {began=now;first_tick=tick;}
        if(++count<121)return false;
        count=0;
        if(now<=began)return false;
        const double seconds=double(now-began)/frequency;
        frames=120/seconds;updates=uint32_t(tick-first_tick)/seconds;return true;
    }
};
} // namespace ff_frame
