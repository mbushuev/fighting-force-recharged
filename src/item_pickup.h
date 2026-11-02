#pragma once
#include <stdint.h>
namespace ff_pickup {
inline bool ReadyAction(int action) {
    return action==0x45 || (action>=0x46 && action<=0x49) || action==0x4C ||
        action==0x4E || action==0xC6 || (action>=0x52 && action<=0x56);
}
inline int Action(int type) {
    if(type<=0x1DC || type>=0x233)return -1;
    return type>0x209?0x79:type==0x1FB || type==0x1FA?0x7B:
        type==0x203?0x7E:type==0x1F8?0x80:0x76;
}
inline int ThrowAction(int right,int left) {
    // Native right-hand throwing precedes left-hand throwing/dropping.
    if(Action(right)>=0)return right>0x209?0xA1:0x9F;
    if(Action(left)>=0)return left==0x1F8?0xA3:0x45;
    return -1;
}
struct Actor {int32_t x=0,z=0;int hero=0,sine=0,cosine=0;};
struct Item {int type=-1;int32_t x=0,y=0,z=0;int16_t remaining=0;};
inline int64_t Distance(int64_t x,int64_t z) {
    if(x<0)x=-x;if(z<0)z=-z;return x>z?x+(z>>1):z+(x>>1);
}
inline bool Reachable(const Actor& actor,const Item& item) {
    if(Action(item.type)<0 || item.y || item.remaining<0)return false;
    int radius=96;int64_t x=actor.x,z=actor.z;
    if(item.type>=0x209) {
        x+=int64_t(actor.sine)*192;z-=int64_t(actor.cosine)*192;
        if(item.type==0x215 || item.type==0x216)radius=actor.hero==2?144:0;
        if(item.type==0x21B && actor.hero!=0 && actor.hero!=2)radius=0;
    }
    return Distance((item.x>>16)-(x>>16),(item.z>>16)-(z>>16))<radius;
}
struct Press {
    bool held=true;int stage=-1,tick=-1;unsigned epoch=0;uint64_t time=0;
    bool Read(bool down,bool active,int scene,int frame,unsigned focus,uint64_t now) {
        if(!active || scene!=stage || frame<=tick || focus!=epoch || (time && now-time>500))held=true;
        stage=scene;tick=frame;epoch=focus;time=now;
        const bool fresh=active && down && !held;held=down || !active;return fresh;
    }
};
}
