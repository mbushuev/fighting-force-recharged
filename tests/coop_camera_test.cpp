#include <initializer_list>
#include <stdio.h>
#include "../src/coop_camera.h"
static int failures;
#define CHECK(x) do {if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
int main() {
    using namespace ff_camera;
    Settings settings;settings.height=160;Orbit orbit;orbit.Begin(0,settings);
    const auto shared=orbit.Follow({0,0,0},settings);
    CHECK(ActorOccupancy(shared,{-100,0,0},400,400,1280,800)<0.70);
    CHECK(ActorOccupancy(shared,{1500,0,0},400,400,1280,800)>1.05);
    CooperativeSplit split;
    for(int i=0;i<100;++i)split.Update(0.3,false,1.0/30);
    CHECK(!split.separate && split.amount==0);
    split.Update(1.1,false,1.0/30);CHECK(split.separate && split.amount>0);
    for(int i=0;i<30;++i)split.Update(0.8,false,1.0/30);
    CHECK(split.amount==1 && split.separate); // Hysteresis holds at the boundary.
    for(int i=0;i<20;++i)split.Update(0.3,false,1.0/30);
    CHECK(split.separate); // A brief crossing cannot repeatedly merge views.
    for(int i=0;i<60;++i)split.Update(0.3,false,1.0/30);
    CHECK(!split.separate && split.amount==0);
    split.Update(0.1,true,0.1);CHECK(split.separate);
    for(int i=0;i<100;++i)split.Update(0.1,true,0.1);
    CHECK(split.separate && split.amount==1);
    for(double x:{-100.0,0.0,100.0}) {
        auto left=SplitFrustum(0,0,400,1280),right=SplitFrustum(1,0,400,1280);
        CHECK(fabs((left.center_x+left.focal_x*x/1000)*0.5-(640+400*x/1000))<1e-9);
        CHECK(fabs(640+(right.center_x+right.focal_x*x/1000)*0.5-(640+400*x/1000))<1e-9);
    }
    CHECK(SplitFrustum(0,1,400,1280).center_x==640);
    const auto blended=BlendPose({0,0,0,NativeAngle(3.1),0},{65536,0,0,NativeAngle(-3.1),0},0.5);
    CHECK(blended.x==32768 && fabs(Radians(blended.yaw))>3.13);
    printf("Cooperative framing: %s\n",failures?"FAILED":"passed");return failures?1:0;
}
