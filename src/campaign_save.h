#pragma once
#include <stdint.h>
#include <string.h>

namespace ff_campaign {
struct Player {
    int32_t character=-1,health=0,maximum=384,lives=0,score=0,next_life=100000;
};
struct Checkpoint {
    int32_t stage=0,players=1,mode=0,difficulty=1;
    Player player[2];
};
inline bool Valid(const Checkpoint& c) {
    if(c.stage<0 || c.stage>=25 || c.players<1 || c.players>2 ||
       c.mode!=c.players-1 || c.difficulty<0 || c.difficulty>2)return false;
    bool alive=false;
    for(int i=0;i<2;++i) {
        const auto& p=c.player[i];
        if(p.character< -1 || p.character>3 || p.maximum<1 || p.maximum>4096 ||
           p.health<0 || p.health>p.maximum || p.lives<0 || p.lives>9999 ||
           p.score<0 || p.next_life<0)return false;
        if(i>=c.players && (p.character!=-1 || p.health || p.lives || p.score))return false;
        if(p.character==-1 && (p.health || p.lives))return false;
        if(p.character>=0 && p.health>0 && p.lives>0 && i<c.players)alive=true;
    }
    return alive;
}
// Fixed-width, little-endian records contain values only, never game memory,
// pointers, device assignments or executable-specific resource addresses.
constexpr unsigned Words=16,Bytes=16+Words*4;
inline void Put(uint8_t* out,uint32_t value) {
    for(unsigned i=0;i<4;++i)out[i]=uint8_t(value>>(i*8));
}
inline uint32_t Get(const uint8_t* in) {
    uint32_t value=0;for(unsigned i=0;i<4;++i)value|=uint32_t(in[i])<<(i*8);return value;
}
inline uint32_t Checksum(const uint8_t* data,unsigned size) {
    uint32_t crc=~0u;
    for(unsigned i=0;i<size;++i) {
        crc^=data[i];for(unsigned bit=0;bit<8;++bit)crc=(crc>>1)^(0xEDB88320u&uint32_t(-int32_t(crc&1)));
    }
    return ~crc;
}
inline bool Encode(const Checkpoint& c,uint8_t (&out)[Bytes]) {
    if(!Valid(c))return false;
    memcpy(out,"FFCP",4);Put(out+4,1);Put(out+8,Bytes);
    int32_t values[Words]={c.stage,c.players,c.mode,c.difficulty};
    for(unsigned i=0;i<2;++i) {
        const auto& p=c.player[i];const int32_t fields[]={p.character,p.health,p.maximum,p.lives,p.score,p.next_life};
        memcpy(values+4+i*6,fields,sizeof(fields));
    }
    for(unsigned i=0;i<Words;++i)Put(out+16+i*4,uint32_t(values[i]));
    Put(out+12,Checksum(out+16,Words*4));return true;
}
inline bool Decode(const uint8_t* data,unsigned size,Checkpoint& output) {
    if(size!=Bytes || memcmp(data,"FFCP",4) || Get(data+4)!=1 || Get(data+8)!=Bytes ||
       Get(data+12)!=Checksum(data+16,Words*4))return false;
    Checkpoint c;c.stage=int32_t(Get(data+16));c.players=int32_t(Get(data+20));
    c.mode=int32_t(Get(data+24));c.difficulty=int32_t(Get(data+28));
    for(unsigned i=0;i<2;++i) {
        const auto fields=data+32+i*24;auto& p=c.player[i];
        p.character=int32_t(Get(fields));p.health=int32_t(Get(fields+4));p.maximum=int32_t(Get(fields+8));
        p.lives=int32_t(Get(fields+12));p.score=int32_t(Get(fields+16));p.next_life=int32_t(Get(fields+20));
    }
    if(!Valid(c))return false;output=c;return true;
}
}
