#include "../src/guard_retarget.h"
#include <stdio.h>
#include <fstream>
#include <iterator>
static int failures;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
template<typename F> static bool Rejects(F f){try{f();return false;}catch(const std::exception&){return true;}}
static ff_guard::Bytes View(const std::vector<uint8_t>& v){return {v.data(),v.size()};}
int main(int argc,char** argv) {
    using namespace ff_guard;
    Poses poses(3);for(auto& p:poses)p.angles.resize(81);
    poses[0].root[0]=123456;poses[0].angles[0]=4094;poses[1].angles[0]=2;poses[2].angles[0]=4092;
    poses[1].angles[80]=2048;poses[2].angles[80]=6;poses[2].root[2]=-54321;
    auto encoded=Encode(poses);auto decoded=Decode(View(encoded),27);
    for(unsigned i=0;i<3;++i){CHECK(decoded[i].angles==poses[i].angles);CHECK(!memcmp(decoded[i].root,poses[i].root,12));}
    CHECK(Rejects([&](){Decode({encoded.data(),10},27);}));
    auto corrupt=encoded;Put32(corrupt,4,0);CHECK(Rejects([&](){Decode(View(corrupt),27);}));
    corrupt=encoded;Put32(corrupt,8,View(corrupt).U32(4));CHECK(Rejects([&](){Decode(View(corrupt),27);}));
    corrupt=encoded;corrupt[4+View(corrupt).U32(4)+12]=0;CHECK(Rejects([&](){Decode(View(corrupt),27);}));
    poses[0].angles[0]=1;CHECK(Rejects([&](){Encode(poses);}));
    CHECK(AngleDelta(4094,2)==4 && AngleDelta(2,4094)==-4);
    Poses entry(13);for(unsigned i=0;i<entry.size();++i){entry[i].angles={int16_t((4090+i*4)&4094),0,0};entry[i].root[0]=100+i*6;}
    const auto quick=QuickEntry(entry);CHECK(quick.size()==4);
    CHECK(quick.front().angles==entry.front().angles && quick.back().angles==entry.back().angles);
    CHECK(quick.front().root[0]==100 && quick.back().root[0]==172);
    CHECK(quick[1].angles==entry[4].angles && quick[2].angles==entry[8].angles);
    CHECK(QuickEntry(quick).size()==4 && QuickEntry({}).empty());
    entry.resize(12);const auto fractional=QuickEntry(entry);
    CHECK(fractional[1].angles==Blend(entry[3],entry[4],2,3).angles);
    // The inverse must preserve orientation across Euler branches and both
    // singularities, including the destination decoder's two-unit quantization.
    for(int a:{0,500,1024,2048,4094})for(int b:{0,1024,1026,2048,3072,4094})for(int c:{0,800,2048,4094}) {
        const int16_t input[]={int16_t(a),int16_t(b),int16_t(c)},nearby[]={100,200,300};int16_t output[3];
        const auto rotation=Rotation::Euler(input);rotation.Angles(output,nearby);const auto decoded_rotation=Rotation::Euler(output);
        for(unsigned r=0;r<3;++r)for(unsigned k=0;k<3;++k)CHECK(fabs(rotation.m[r][k]-decoded_rotation.m[r][k])<0.006);
        const auto identity=rotation*rotation.Inverse();
        for(unsigned r=0;r<3;++r)for(unsigned k=0;k<3;++k)CHECK(fabs(identity.m[r][k]-(r==k?1:0))<1e-9);
    }
    if(argc==2) {
        auto read=[&](const char* name){std::ifstream f(std::string(argv[1])+"/"+name+".WAD",std::ios::binary);return std::vector<uint8_t>(std::istreambuf_iterator<char>(f),{});};
        const char* heroes[]={"HAWK","MACE","ALANA"};const unsigned joints[]={20,30,32},upper[]={8,12,12};
        for(unsigned h=0;h<3;++h) {
            auto hero=read(heroes[h]),donor=read(h?"CHARS15":"CHARS11");UpperBodyClips clips;
            const std::string ach=std::string(heroes[h])+".ACH",adt=std::string(heroes[h])+".ADT";
            clips.Build(View(hero),View(donor),ach.c_str(),adt.c_str(),h?"VAMP.ACH":"SUIT.ACH",h?"GENFMALE.ADT":"GENMALE.ADT",joints[h],upper[h]);
            const auto idle=Decode(Clip(Member(View(hero),adt.c_str()),"STBY1"),joints[h])[0];
            const unsigned counts[2][4]={{4,14,15,14},{4,11,13,10}};
            for(unsigned i=0;i<4;++i) {
                auto frames=Decode(View(clips.clips[i]),joints[h]);CHECK(frames.size()==counts[h?1:0][i]);
                for(const auto& frame:frames) {
                    CHECK(frame.angles.size()==joints[h]*3 && frame.root[0]==idle.root[0] && frame.root[1]==0 && frame.root[2]==0);
                    CHECK(!memcmp(frame.angles.data(),idle.angles.data(),6));
                    for(unsigned c=(upper[h]+1)*3;c<joints[h]*3;++c)CHECK(frame.angles[c]==idle.angles[c]);
                }
            }
            CHECK(Rejects([&](){clips.Build(View(hero),View(donor),ach.c_str(),adt.c_str(),h?"VAMP.ACH":"SUIT.ACH",h?"GENFMALE.ADT":"GENMALE.ADT",joints[h]+1,upper[h]);}));
        }
        puts("Original upper-body guard conversion: PASS (read-only).");
    }
    if(argc==3) {
        auto read=[](const char* name){std::ifstream f(name,std::ios::binary);return std::vector<uint8_t>(std::istreambuf_iterator<char>(f),{});};
        auto hero=read(argv[1]),donor=read(argv[2]);SmasherClips clips;clips.Build(View(hero),View(donor));
        const unsigned lengths[]={4,11,15,12};
        for(unsigned i=0;i<4;++i) {
            auto frames=Decode(View(clips.clips[i]),27);CHECK(frames.size()==lengths[i]);
            for(const auto& frame:frames){CHECK(frame.angles.size()==81);CHECK(frame.root[1]==0 && frame.root[2]==0);}
        }
        puts("Original Smasher guard conversion: PASS (read-only).");
    }
    puts(failures?"Guard animation: FAIL":"Guard animation: PASS");return failures?1:0;
}
