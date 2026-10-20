#include "../src/camera_collision.h"
#include <stdio.h>
using namespace ff_camera;
static int failures;
#define CHECK(x) do {if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);++failures;}}while(0)
static bool Near(double a,double b){return fabs(a-b)<0.00001;}
int main() {
    Triangle wall={{-100,-100,50},{100,-100,50},{0,100,50}};
    CHECK(Near(SweepSphere({0,0,0},{0,0,100},5,wall),0.45));
    CHECK(Near(SweepSphere({0,0,100},{0,0,0},5,wall),0.45)); // two-sided
    CHECK(SweepSphere({200,0,0},{200,0,100},5,wall)==1);
    CHECK(SweepSphere({0,0,48},{0,0,100},5,wall)==0);
    CHECK(SweepSphere({0,0,48},{0,0,0},5,wall)==1); // leave an initial contact
    Triangle edge={{0,0,50},{100,0,50},{0,100,50}};
    CHECK(Near(SweepSphere({50,-3,0},{50,-3,100},5,edge),0.46)); // edge capsule, ray misses face
    CHECK(Near(SweepSphere({-3,-4,0},{-3,-4,100},5,edge),0.5)); // vertex sphere
    Triangle floor={{-500,0,-500},{500,0,-500},{0,0,500}};
    CHECK(Near(SweepSphere({0,-100,0},{0,100,0},10,floor),0.45));
    CHECK(SweepSphere({0,-100,0},{0,-20,0},10,floor)==1);
    CHECK(SweepSphere({0,0,0},{0,0,0},1,wall)==1);
    CHECK(Near(SweepSphere({0,0,0},{0,0,1000},5,wall),0.045)); // no discrete-step tunneling
    Triangle far=wall;far.a.z=far.b.z=far.c.z=80;
    double first=SweepSphere({0,0,0},{0,0,100},5,far);
    CHECK(Near(SweepSphere({0,0,0},{0,0,100},5,wall,first),0.45));
    Triangle degenerate={{0,0,50},{0,0,50},{0,0,50}};
    CHECK(Near(SweepSphere({0,0,0},{0,0,100},5,degenerate),0.45));
    Triangle line={{0,0,50},{100,0,50},{50,0,50}};
    CHECK(SweepSphere({50,1,50},{50,0,50},5,line)==0);
    CHECK(SweepSphere({50,1,50},{50,20,50},5,line)==1);
    // Standing beside a car puts the old full-size pivot sphere inside its
    // hood corner, even though this camera path climbs clear of the body.
    Triangle hood_a={{-256,-128,0},{256,-128,0},{256,-128,512}};
    Triangle hood_b={{-256,-128,0},{256,-128,512},{-256,-128,512}};
    Point pivot={280,-160,256},above_car={-396,-406,256};
    double old_hit=SweepSphere(pivot,above_car,48,hood_a);
    old_hit=SweepSphere(pivot,above_car,48,hood_b,old_hit);CHECK(old_hit==0);
    double hit=SweepCameraProbe(pivot,above_car,48,hood_a);
    hit=SweepCameraProbe(pivot,above_car,48,hood_b,hit);CHECK(hit==1);
    // Full radius is retained away from the pivot, and even a very near wall
    // blocks the tapered prefix instead of being skipped as an initial overlap.
    CHECK(Near(SweepCameraProbe({0,0,0},{0,0,720},48,wall),50.0/936.0));
    Triangle distant=wall;distant.a.z=distant.b.z=distant.c.z=400;
    CHECK(Near(SweepCameraProbe({0,0,0},{0,0,720},48,distant),352.0/720));
    CHECK(Near(SweepCameraProbe({0,0,0},{0,0,80},48,wall),50.0/104));
    Triangle away=wall;away.a.x+=1000;away.b.x+=1000;away.c.x+=1000;
    CHECK(SweepCameraProbe({0,0,0},{0,0,720},48,away)==1);
    CHECK(Near(FirstRoot(-4,0,1),0.5)); // growing radius can overtake a parallel edge
    CHECK(Near(FirstRoot(0,-1,1),0.5));
    // Invert a non-unit native basis and recover a translated model vertex.
    const float view[]={0,0,2,0, 0,3,0,0, -4,0,0,0}; double inverse[9];
    CHECK(InverseBasis(view,inverse));
    const float model[]={0,0,2,20, 0,-3,0,-60, 4,0,0,-120};
    Point p=WorldVertex(model,inverse,{10,20,30},{1,2,3});
    CHECK(Near(p.x,39) && Near(p.y,-2) && Near(p.z,43));
    const float singular[12]={};CHECK(!InverseBasis(singular,inverse));
    printf("Camera sphere sweep and world geometry: %s\n",failures?"FAIL":"PASS");return failures?1:0;
}
