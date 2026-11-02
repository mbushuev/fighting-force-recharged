#include "../src/throw_trajectory.h"
#include <stdio.h>
#include <limits>
static int failures;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
int main() {
    using namespace ff_throw;
    CHECK(Eligible(0x1F0) && Eligible(0x1F1) && Eligible(0x210) && Eligible(0x232));
    CHECK(!Eligible(0x1EF) && !Eligible(0x1F3) && !Eligible(0x209) && !Eligible(0x233));
    CHECK(Lift(0x1F0,1,0x45)==64 && Lift(0x210,1,0x46)==32 && Lift(0x1F0,0,0x45)==32);
    auto grenade=Flight({0,-160,0},0,32767,64);
    // Samples from the retail fixed-point update: upward 24,16,8,0 then
    // downward 8,16,24. Horizontal displacement loses 1.5 units per tick.
    const double heights[]={-160,-184,-200,-208,-208,-200,-184,-160};
    const double distances[]={0,64,126.5,187.5,247,305,361.5,416.5};
    for(unsigned i=0;i<8;++i) {
        CHECK(grenade.points[i].y==heights[i]);CHECK(fabs(grenade.points[i].z+distances[i]*32767/32768)<0.001);
    }
    CHECK(grenade.hit && grenade.points[grenade.count-1].y==0);
    auto heavy=Flight({0,-160,0},32767,0,32);CHECK(heavy.count<grenade.count && heavy.points[1].y==-168);
    CHECK(heavy.points[1].x>63.99 && heavy.points[1].z==0);
    auto diagonal=Flight({0,-160,0},-23170,-23170,64);
    CHECK(diagonal.points[1].x<0 && diagonal.points[1].z>0 && fabs(diagonal.points[1].x+diagonal.points[1].z)<0.001);
    CHECK(!Flight({0,0,0},0,32767,64).count);
    CHECK(!Flight({0,std::numeric_limits<double>::quiet_NaN(),0},0,32767,64).count);
    CHECK(!Flight({0,-160,0},0,40000,64).count);
    // Unordered geometry must pick the earliest segment, including a reverse face.
    ff_camera::Triangle walls[]={{{-500,-500,-300},{500,-500,-300},{0,500,-300}},
        {{0,500,-100},{500,-500,-100},{-500,-500,-100}}};
    Obstruct(grenade,walls,2);CHECK(grenade.hit && grenade.count==3 && fabs(grenade.points[2].z+100)<1e-6);
    auto parallel=Flight({1000,-160,0},0,32767,64);const auto original=parallel;
    Obstruct(parallel,walls,2);CHECK(parallel.count==original.count && parallel.points[parallel.count-1].y==0);
    puts(failures?"Throw trajectory: FAIL":"Throw trajectory: PASS");return failures?1:0;
}
