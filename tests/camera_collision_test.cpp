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
    // Invert a non-unit native basis and recover a translated model vertex.
    const float view[]={0,0,2,0, 0,3,0,0, -4,0,0,0}; double inverse[9];
    CHECK(InverseBasis(view,inverse));
    const float model[]={0,0,2,20, 0,-3,0,-60, 4,0,0,-120};
    Point p=WorldVertex(model,inverse,{10,20,30},{1,2,3});
    CHECK(Near(p.x,39) && Near(p.y,-2) && Near(p.z,43));
    const float singular[12]={};CHECK(!InverseBasis(singular,inverse));
    printf("Camera sphere sweep and world geometry: %s\n",failures?"FAIL":"PASS");return failures?1:0;
}
