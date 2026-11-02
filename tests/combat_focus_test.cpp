#include "../src/combat_focus.h"
#include <stdio.h>
static int failures;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
int main() {
    using namespace ff_focus;
    Target targets[Actors];
    targets[0]=targets[1]={{0,0,200},0,100,0x45,true,true};
    targets[2]={{0,0,500},4,100,0x45,true,true};
    targets[3]={{300,0,600},5,100,0x45,true,true};
    targets[4]={{-300,0,600},6,100,0x45,true,true};
    targets[5]={{0,0,-500},7,100,0x45,true,true};
    View view;Point player;
    CHECK(Choose(targets,player,view)==2);
    CHECK(Choose(targets,player,view,2,1)==3 && Choose(targets,player,view,2,-1)==4);
    targets[2].visible=false;CHECK(Choose(targets,player,view)!=2);
    targets[2].visible=true;targets[2].health=0;CHECK(Choose(targets,player,view)!=2);
    targets[2].health=100;targets[2].type=0x11;CHECK(Choose(targets,player,view)!=2);targets[2].type=4;
    State state;
    auto update=[&](bool down=false,double axis=0,bool prev=false,bool next=false,double dt=1.0/30) {
        return state.Update(targets,player,view,0,true,down,axis,prev,next,dt);
    };
    update(true);CHECK(!state.Active());
    update();CHECK(!update(true) && state.target==2);
    for(int i=0;i<30;++i)update(true);CHECK(state.target==2);
    update();update(false,1);CHECK(state.target==3);
    update(false,-1);CHECK(state.target==3);
    update();update(false,-1);CHECK(state.target==2);
    update(true);CHECK(!state.Active());update();update(true);CHECK(state.target==2);
    targets[2].visible=false;
    for(int i=0;i<10;++i)update();CHECK(state.Active());
    for(int i=0;i<5;++i)update();CHECK(!state.Active());
    targets[2].visible=true;update(true);CHECK(state.Active());
    targets[2].health=0;update();CHECK(!state.Active());
    targets[2].health=100;update(true);CHECK(state.Active());
    state.Update(targets,player,view,1,true,false,0,false,false,0.033);CHECK(!state.Active());
    update();update();update(true);CHECK(state.Active());
    update(false,0,false,false,0.4);CHECK(!state.Active());
    for(auto& t:targets)t.health=0;
    update();CHECK(update(true) && !state.Active());
    double yaw=ff_camera::kPi-0.01,pitch=0;
    Track({0,0,0},{10,0,-500},1.0/30,false,-1,1,yaw,pitch);
    CHECK(fabs(ff_camera::Wrap(yaw-ff_camera::kPi))<0.1 && pitch==0);
    Track({0,0,0},{0,-100,500},1.0/30,true,-1,1,yaw,pitch);CHECK(pitch<0 && fabs(pitch)<0.1);
    ScreenPoint p;CHECK(!Project({0,0,0},view.pose,160,160,160,120,p));
    CHECK(Project({100,0,500},view.pose,160,160,160,120,p) && p.x==192);
    puts(failures?"Combat focus policy: FAIL":"Combat focus policy: PASS");return failures?1:0;
}
