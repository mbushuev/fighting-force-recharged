#include "../src/first_person.h"
#include <stdio.h>
using namespace ff_camera;
static int failures;
#define CHECK(x) do {if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);++failures;}}while(0)
static void Node(std::vector<int32_t>& words,int special,int joint=0) {
    const int32_t node[]={3,special,0,0,0,0,0,0,joint,0,0,0};
    words.insert(words.end(),node,node+12);
}
int main() {
    // Neck/head/hair share a subtree; hands and torso must remain visible.
    std::vector<int32_t> stream={1};Node(stream,0);
    stream.push_back(1);Node(stream,1);Node(stream,0);Node(stream,0);
    stream.push_back(2);Node(stream,2);stream.push_back(2);stream.push_back(0);
    const unsigned counts[]={8,0,12,4,8};HeadSkeleton skeleton;
    CHECK(skeleton.Build(stream.data(),stream.size(),counts,5,32,4));
    CHECK(skeleton.eye_step==4 && skeleton.steps[4].first==8);
    int16_t head[]={8,9,10,11,0,3},hair[]={20,21,22,23,0,3},hand[]={24,25,26,27,0,3};
    CHECK(skeleton.HeadFace(head) && skeleton.HeadFace(hair) && !skeleton.HeadFace(hand));
    CHECK(skeleton.head_vertices[7]==0 && skeleton.head_vertices[8]==1 && skeleton.head_vertices[24]==0);
    CHECK(!skeleton.Build(stream.data(),stream.size()-1,counts,5,32,4));
    CHECK(skeleton.steps.empty() && skeleton.eye_step==-1);
    CHECK(!skeleton.Build(stream.data(),stream.size(),counts,4,32,4));
    CHECK(!skeleton.Build(stream.data(),stream.size(),counts,5,31,4));
    auto invalid=stream;invalid[0]=2;
    CHECK(!skeleton.Build(invalid.data(),invalid.size(),counts,5,32,4));
    FirstPersonView view;Settings settings;
    CHECK(!view.Toggle(true,true) && !view.selected);
    view.Toggle(false,true);CHECK(view.Toggle(true,true) && view.selected);
    CHECK(!view.Toggle(true,true) && view.selected);
    view.Toggle(false,false);CHECK(!view.Toggle(true,true) && view.selected);
    view.Toggle(false,true);CHECK(view.Toggle(true,true) && !view.selected);
    for(int heading=0;heading<2048;heading+=64) {
        view.Begin(int16_t(heading));CHECK(view.BodyHeading()==heading);
        const auto eye=view.Eye({100,-160,200},6,8);
        CHECK(fabs(eye.y+166)<1e-8);
        const double angle=heading*kTurn/2048;
        CHECK(fabs(eye.x-(100+sin(angle)*8))<1e-8);
        CHECK(fabs(eye.z-(200-cos(angle)*8))<1e-8);
    }
    view.Begin(0);auto a=view,b=view;
    a.Look(120,0,0,0,0.05,settings);
    for(int i=0;i<4;++i)b.Look(30,0,0,0,0.0125,settings);
    CHECK(fabs(a.yaw-b.yaw)<1e-8);
    view.Look(0,100000,0,0,1,settings);CHECK(fabs(view.pitch-80*kPi/180)<1e-8);
    view.Look(0,-100000,0,0,1,settings);CHECK(fabs(view.pitch+80*kPi/180)<1e-8);
    auto ground=view.Follow(view.Eye({20,-150,30},6,8));
    auto jump=view.Follow(view.Eye({25,-250,40},6,8));
    CHECK(jump.x-ground.x==Fixed(5) && jump.y-ground.y==Fixed(-100) && jump.z-ground.z==Fixed(10));
    printf("Immersive skeleton, look and toggle: %s\n",failures?"FAIL":"PASS");return failures?1:0;
}
