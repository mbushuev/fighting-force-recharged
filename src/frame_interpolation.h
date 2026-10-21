#pragma once
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <initializer_list>

namespace ff_frame {
template<typename T> inline T Read(const void* data,unsigned offset) {
    T value;memcpy(&value,static_cast<const uint8_t*>(data)+offset,sizeof(value));return value;
}
template<typename T> inline void Write(void* data,unsigned offset,T value) {
    memcpy(static_cast<uint8_t*>(data)+offset,&value,sizeof(value));
}
inline int32_t Midpoint(int32_t a,int32_t b) {return static_cast<int32_t>((int64_t(a)+b)/2);}
inline int Angle(int a,int b,unsigned period) {
    const int mask=int(period-1),half=int(period/2);
    const int delta=((b-a+half)&mask)-half;
    return (a+delta/2)&mask;
}
inline bool Nearby(const void* a,const void* b,unsigned offset) {
    // Do not sweep a respawn, teleport or reused slot across the level.
    for(unsigned axis=0;axis<3;++axis)
        if(llabs(int64_t(Read<int32_t>(a,offset+axis*4))-Read<int32_t>(b,offset+axis*4))>1024LL*65536)return false;
    return true;
}
inline void Position(void* output,const void* before,const void* now,unsigned offset) {
    for(unsigned axis=0;axis<3;++axis)Write<int32_t>(output,offset+axis*4,
        Midpoint(Read<int32_t>(before,offset+axis*4),Read<int32_t>(now,offset+axis*4)));
}
struct Snapshot {
    uint8_t actors[8][0xF4]={};
    uint8_t palettes[8][0x238]={};
    uint8_t objects[64][36]={};
    uint8_t effects[128][36]={};
    int stage=-1,players=0;
    uint32_t tick=0;
    bool eye[2]={};
    bool Consecutive(const Snapshot& next) const {
        return stage==next.stage && players==next.players && uint32_t(next.tick-tick)==1 &&
            eye[0]==next.eye[0] && eye[1]==next.eye[1];
    }
};
inline bool Actor(void* output,void* palette,const void* before,const void* now,
                  const void* old_palette,const void* new_palette) {
    const int model=Read<int16_t>(now,0x34);
    if(model<0 || model!=Read<int16_t>(before,0x34) || !Nearby(before,now,0x28))return false;
    Position(output,before,now,0x28);
    for(unsigned offset:{0x10u,0x20u})Write<int32_t>(output,offset,
        Midpoint(Read<int32_t>(before,offset),Read<int32_t>(now,offset)));
    Write<int16_t>(output,0x38,int16_t(Angle(Read<int16_t>(before,0x38),Read<int16_t>(now,0x38),2048)));
    // Only decoded joint angles are interpolated. Animation pointers, frame
    // indices and attachment matrices are neither averaged nor advanced.
    for(unsigned offset=0x118;offset<0x238;offset+=2)Write<int16_t>(palette,offset,
        int16_t(Angle(Read<int16_t>(old_palette,offset),Read<int16_t>(new_palette,offset),4096)));
    Write<int32_t>(palette,0,Angle(Read<int32_t>(old_palette,0),Read<int32_t>(new_palette,0),4096));
    return true;
}
inline bool Object(void* output,const void* before,const void* now,bool angles=true) {
    const int type=Read<int32_t>(now,0);
    if(type<0 || type!=Read<int32_t>(before,0) || !Nearby(before,now,4))return false;
    Position(output,before,now,4);
    if(angles)for(unsigned offset=0x10;offset<=0x14;offset+=2)Write<int16_t>(output,offset,
        int16_t(Angle(Read<int16_t>(before,offset),Read<int16_t>(now,offset),65536)));
    return true;
}
struct Cadence {
    int64_t origin=0;
    unsigned frame=0;
    bool ready=false;
    void Reset(){ready=false;frame=0;}
    int64_t Next(int64_t now,int64_t frequency) {
        const int64_t expected=origin+int64_t(frame)*frequency/60;
        // Focus loss, long stalls and transitions start a new clock epoch.
        // Never issue a burst of catch-up renders or advance simulation here.
        if(!ready || now-expected>frequency/4 || now<origin) {origin=now;frame=0;ready=true;}
        const int64_t deadline=origin+int64_t(++frame)*frequency/60;
        if(frame==60) {origin=deadline;frame=0;}
        return deadline;
    }
};
} // namespace ff_frame
