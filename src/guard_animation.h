#pragma once
#include <stdint.h>
#include <string.h>
#include <vector>
#include <stdexcept>

// Original archives are read only. Converted clips live in wrapper memory.
namespace ff_guard {
struct Bytes {
    const uint8_t* data=nullptr;size_t size=0;
    Bytes Slice(size_t offset,size_t count) const {
        if(offset>size || count>size-offset)throw std::runtime_error("Truncated guard asset");
        return {data+offset,count};
    }
    uint32_t U32(size_t offset,bool big=false) const {
        auto p=Slice(offset,4).data;
        return big?(uint32_t(p[0])<<24)|(uint32_t(p[1])<<16)|(uint32_t(p[2])<<8)|p[3]:
            p[0]|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);
    }
};
inline bool Name(Bytes bytes,const char* name) {
    const size_t length=strlen(name);if(length>bytes.size)return false;
    return !memcmp(bytes.data,name,length) && (length==bytes.size || bytes.data[length]==0);
}
inline Bytes Member(Bytes wad,const char* name) {
    const auto count=wad.U32(0,true);if(count>4096)throw std::runtime_error("Invalid WAD count");
    const size_t end=4+size_t(count)*20;wad.Slice(0,end);Bytes result;
    for(unsigned i=0;i<count;++i) {
        const size_t record=4+i*20;const auto size=wad.U32(record+12,true),offset=wad.U32(record+16,true);
        if(offset<end)throw std::runtime_error("Invalid WAD range");
        auto member=wad.Slice(offset,size);
        if(Name(wad.Slice(record,12),name)) {
            if(result.data)throw std::runtime_error("Duplicate WAD member");result=member;
        }
    }
    if(!result.data)throw std::runtime_error("Missing guard member");return result;
}
inline Bytes Clip(Bytes adt,const char* name) {
    Bytes result;size_t cursor=0;
    for(;cursor<adt.size && adt.data[cursor];) {
        const auto size=adt.U32(cursor+8);if(size<4)throw std::runtime_error("Invalid ADT size");
        auto record=adt.Slice(cursor+8,size);
        if(Name(adt.Slice(cursor,8),name))result=record.Slice(4,size-4);
        cursor+=8+size;
    }
    if(cursor==adt.size || !result.data)throw std::runtime_error("Missing guard clip or terminator");
    return result;
}
struct Pose {int32_t root[3]={};std::vector<int16_t> angles;};
using Poses=std::vector<Pose>;
inline Poses Decode(Bytes clip,unsigned joints) {
    if(!joints || joints>48)throw std::runtime_error("Invalid guard joints");
    const unsigned count=clip.U32(0);if(!count || count>1024)throw std::runtime_error("Invalid guard frames");
    clip.Slice(4,count*4);Poses result;Pose pose;pose.angles.resize(joints*3);
    size_t previous=4+count*4;
    for(unsigned f=0;f<count;++f) {
        const auto descriptor=clip.U32(4+f*4);
        if(descriptor>>16)throw std::runtime_error("Unexpected guard frame event");
        const size_t start=4+(descriptor&65535),end=f+1<count?4+(clip.U32(8+f*4)&65535):clip.size;
        if(start<previous || end<=start || end>clip.size)throw std::runtime_error("Invalid guard offsets");
        auto frame=clip.Slice(start,end-start);previous=end;
        for(unsigned i=0;i<3;++i)pose.root[i]=static_cast<int32_t>(frame.U32(i*4));
        const auto mask_size=frame.Slice(12,1).data[0];
        if(!mask_size || mask_size>127)throw std::runtime_error("Invalid guard mask");
        auto mask=frame.Slice(13,mask_size);size_t bit=0,cursor=13+mask_size;
        auto take=[&](){if(bit>=mask.size*8)throw std::runtime_error("Short guard mask");
            const unsigned value=(mask.data[bit/8]>>(bit%8))&1;++bit;return value;};
        for(auto& angle:pose.angles) {
            if(take()) {
                const bool wide=!f || take();auto value=frame.Slice(cursor,wide?2:1);cursor+=wide?2:1;
                const int delta=wide?static_cast<int16_t>(value.data[0]|(value.data[1]<<8)):static_cast<int8_t>(value.data[0]);
                angle=static_cast<int16_t>(f?(angle+2*delta)&4094:delta);
            } else if(!f)angle=0;
        }
        result.push_back(pose);
    }
    return result;
}
inline int AngleDelta(int a,int b) {return ((b-a+2048)&4095)-2048;}
inline void Put32(std::vector<uint8_t>& out,size_t offset,uint32_t value) {
    for(unsigned i=0;i<4;++i)out[offset+i]=static_cast<uint8_t>(value>>(i*8));
}
inline std::vector<uint8_t> Encode(const Poses& frames) {
    if(frames.empty() || frames.size()>1024 || frames[0].angles.empty() || frames[0].angles.size()>144)
        throw std::runtime_error("Invalid converted guard dimensions");
    const size_t components=frames[0].angles.size();std::vector<uint8_t> out(4+frames.size()*4);
    Put32(out,0,static_cast<uint32_t>(frames.size()));
    for(size_t f=0;f<frames.size();++f) {
        if(frames[f].angles.size()!=components || out.size()-4>65535)throw std::runtime_error("Invalid converted guard frame");
        Put32(out,4+f*4,static_cast<uint32_t>(out.size()-4));const size_t start=out.size();
        const size_t mask=(components*(f?2:1)+7)/8;out.resize(start+13+mask+components*2,255);
        for(unsigned i=0;i<3;++i)Put32(out,start+i*4,static_cast<uint32_t>(frames[f].root[i]));
        out[start+12]=static_cast<uint8_t>(mask);
        for(size_t c=0;c<components;++c) {
            if(frames[f].angles[c]&1)throw std::runtime_error("Unrepresentable odd guard angle");
            const int value=f?AngleDelta(frames[f-1].angles[c],frames[f].angles[c])/2:frames[f].angles[c];
            const size_t offset=start+13+mask+c*2;out[offset]=static_cast<uint8_t>(value);out[offset+1]=static_cast<uint8_t>(value>>8);
        }
    }
    if(out.size()>65539)throw std::runtime_error("Converted guard exceeds native offset range");return out;
}
inline Pose Blend(const Pose& a,const Pose& b,unsigned step,unsigned length) {
    Pose out=a;
    for(unsigned i=0;i<3;++i)out.root[i]=static_cast<int32_t>(a.root[i]+(int64_t(b.root[i])-a.root[i])*step/length);
    for(size_t c=0;c<out.angles.size();++c)
        out.angles[c]=static_cast<int16_t>((a.angles[c]+AngleDelta(a.angles[c],b.angles[c])*int(step)/int(length))&4094);
    return out;
}
struct SmasherClips {
    std::vector<uint8_t> clips[4]; // entry, hold, impact, exit
    int32_t height[4]={};
    void Build(Bytes hero,Bytes donor) {
        const auto idle=Decode(Clip(Member(hero,"SMASHER.ADT"),"STBY1"),27)[0];
        const auto source=Member(donor,"GENBIGMA.ADT");
        const char* names[]={"BLOCKIN1","BLOCKSB1","BLOCKHT1","BLOCKOT1"};
        for(unsigned clip=0;clip<4;++clip) {
            auto poses=Decode(Clip(source,names[clip]),21);Poses converted;
            for(const auto& pose:poses) {
                Pose target=idle;target.root[0]=pose.root[0];target.root[1]=target.root[2]=0;
                for(unsigned joint=0;joint<21;++joint) {
                    const unsigned mapped=joint<7?joint:joint<10?joint+3:joint+6;
                    for(unsigned c=0;c<3;++c)target.angles[mapped*3+c]=pose.angles[joint*3+c];
                }
                converted.push_back(target);
            }
            Pose rest=idle;rest.root[1]=rest.root[2]=0;
            if(clip==0) {
                Poses prefix;for(unsigned i=0;i<4;++i)prefix.push_back(Blend(rest,converted.front(),i,4));
                converted.insert(converted.begin(),prefix.begin(),prefix.end());
            } else if(clip==3) {
                const auto last=converted.back();for(unsigned i=1;i<=4;++i)converted.push_back(Blend(last,rest,i,4));
            }
            height[clip]=converted.front().root[0];clips[clip]=Encode(converted);
        }
    }
};
}
