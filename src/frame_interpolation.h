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
inline int AngleDelta(int a,int b,unsigned period) {
    const int mask=int(period-1),half=int(period/2);
    return ((b-a+half)&mask)-half;
}
inline int Angle(int a,int b,unsigned period) {
    return (a+AngleDelta(a,b,period)/2)&int(period-1);
}
inline void JointRotation(void* output,const void* before,const void* now,unsigned offset) {
    int a[3],b[3],equivalent[3];
    for(unsigned i=0;i<3;++i) {
        a[i]=Read<int16_t>(before,offset+i*2);b[i]=Read<int16_t>(now,offset+i*2);
    }
    // Retail Euler decomposition can flip both outer axes by half a turn
    // while crossing the middle axis's singularity. These triples describe
    // the same rotation; choose the branch continuous with the previous pose.
    equivalent[0]=(b[0]+2048)&4095;
    equivalent[1]=(2048-b[1])&4095;
    equivalent[2]=(b[2]+2048)&4095;
    int direct_distance=0,equivalent_distance=0;
    for(unsigned i=0;i<3;++i) {
        const int direct=AngleDelta(a[i],b[i],4096),other=AngleDelta(a[i],equivalent[i],4096);
        direct_distance+=direct*direct;equivalent_distance+=other*other;
    }
    const auto target=equivalent_distance<direct_distance?equivalent:b;
    for(unsigned i=0;i<3;++i)Write<int16_t>(output,offset+i*2,int16_t(Angle(a[i],target[i],4096)));
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
    const int old_heading=Read<int16_t>(before,0x38),new_heading=Read<int16_t>(now,0x38);
    const bool half_turn=abs(AngleDelta(old_heading,new_heading,2048))==1024;
    Write<int16_t>(output,0x38,int16_t(half_turn?new_heading:Angle(old_heading,new_heading,2048)));
    // Only decoded joint angles are interpolated. Animation pointers, frame
    // indices and attachment matrices are neither averaged nor advanced.
    // A native backstep can transfer an exact half-turn from the root to the
    // actor heading. Keep that pair coherent: their independent shortest arcs
    // can otherwise take opposite paths and introduce an extra whole-body turn.
    if(half_turn)memcpy(static_cast<uint8_t*>(palette)+0x118,static_cast<const uint8_t*>(new_palette)+0x118,6);
    for(unsigned offset=half_turn?0x11E:0x118;offset<0x238;offset+=6)JointRotation(palette,old_palette,new_palette,offset);
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
