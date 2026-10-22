#include "../src/guard_animation.h"
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
    if(argc==3) {
        auto read=[](const char* name){std::ifstream f(name,std::ios::binary);return std::vector<uint8_t>(std::istreambuf_iterator<char>(f),{});};
        auto hero=read(argv[1]),donor=read(argv[2]);SmasherClips clips;clips.Build(View(hero),View(donor));
        const unsigned lengths[]={12,11,15,12};
        for(unsigned i=0;i<4;++i) {
            auto frames=Decode(View(clips.clips[i]),27);CHECK(frames.size()==lengths[i]);
            for(const auto& frame:frames){CHECK(frame.angles.size()==81);CHECK(frame.root[1]==0 && frame.root[2]==0);}
        }
        puts("Original Smasher guard conversion: PASS (read-only).");
    }
    puts(failures?"Guard animation: FAIL":"Guard animation: PASS");return failures?1:0;
}
