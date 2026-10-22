#pragma once
#include "campaign_save.h"

namespace ff_campaign {
constexpr int SlotCount=3;
struct Slot {
    uint64_t order=0;
    Checkpoint current,previous;
    bool has_previous=false;
};
enum class SlotStatus { Missing,Ready,Recovered,Invalid,Unavailable };
inline bool Usable(SlotStatus status){return status==SlotStatus::Ready || status==SlotStatus::Recovered;}
inline bool Valid(const Slot& s) {
    return s.order>0 && s.order<INT64_MAX && Valid(s.current) &&
        (!s.has_previous || (Valid(s.previous) && s.previous.stage!=s.current.stage &&
                            s.previous.players==s.current.players && s.previous.mode==s.current.mode));
}
constexpr unsigned SlotBytes=28+Bytes*2;
inline bool Encode(const Slot& s,uint8_t (&data)[SlotBytes]) {
    if(!Valid(s))return false;
    memset(data,0,sizeof(data));memcpy(data,"FFCS",4);Put(data+4,2);Put(data+8,SlotBytes);
    Put(data+16,uint32_t(s.order));Put(data+20,uint32_t(s.order>>32));Put(data+24,s.has_previous?1:0);
    uint8_t point[Bytes];Encode(s.current,point);memcpy(data+28,point,Bytes);
    if(s.has_previous){Encode(s.previous,point);memcpy(data+28+Bytes,point,Bytes);}
    Put(data+12,Checksum(data+16,SlotBytes-16));return true;
}
inline bool Decode(const uint8_t* data,unsigned size,Slot& output) {
    if(size!=SlotBytes || memcmp(data,"FFCS",4) || Get(data+4)!=2 || Get(data+8)!=SlotBytes ||
       Get(data+12)!=Checksum(data+16,SlotBytes-16) || Get(data+24)>1)return false;
    Slot s;s.order=Get(data+16)|(uint64_t(Get(data+20))<<32);s.has_previous=Get(data+24)!=0;
    if(!Decode(data+28,Bytes,s.current) || (s.has_previous && !Decode(data+28+Bytes,Bytes,s.previous)) || !Valid(s))return false;
    output=s;return true;
}
struct Library {
    Slot slots[SlotCount];SlotStatus status[SlotCount]={};
    int Latest()const {
        int chosen=-1;for(int i=0;i<SlotCount;++i)if(Usable(status[i]) &&
            (chosen<0 || slots[i].order>slots[chosen].order))chosen=i;return chosen;
    }
    int Empty()const{for(int i=0;i<SlotCount;++i)if(status[i]==SlotStatus::Missing)return i;return -1;}
    uint64_t NextOrder()const {
        uint64_t order=0;for(int i=0;i<SlotCount;++i)if(Usable(status[i]) && slots[i].order>order)order=slots[i].order;
        return order+1;
    }
    Slot Updated(int index,const Checkpoint& point,bool replace=false,bool rollback=false)const {
        Slot s;if(!replace && Usable(status[index]))s=slots[index];
        if(rollback)s.has_previous=false;
        else if(!replace && Usable(status[index]) && s.current.stage!=point.stage) {
            s.previous=s.current;s.has_previous=true;
        }
        s.current=point;s.order=NextOrder();return s;
    }
};
}
