#include "../src/player_guard.h"
#include "../src/first_person.h"
#include <stdio.h>
static int failures;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
int main() {
    using namespace ff_guard;
    GuardState p,q;
    CHECK(p.Tick(true,true,false,false,true)==None); // Neutral boundary.
    CHECK(p.Tick(false,true,false,false,true)==None);
    CHECK(p.Tick(true,true,false,false,false)==None); // Cannot cancel an attack.
    CHECK(p.Tick(true,true,false,false,true)==Entry);
    CHECK(q.phase==None);CHECK(p.Protects());
    CHECK(p.Tick(true,true,true,true,false)==Hold);
    p.phase=Impact;CHECK(p.Tick(true,true,true,true,false)==Hold);
    CHECK(p.Tick(false,true,true,false,false)==Exit);CHECK(!p.Protects());
    CHECK(p.Tick(true,true,true,true,true)==None);
    CHECK(p.Tick(true,true,false,false,true)==Entry);
    CHECK(p.Tick(true,true,false,false,false)==None); // Native hit interruption.
    CHECK(p.Tick(true,true,false,false,true)==None); // Release after interruption.
    p.Tick(false,true,false,false,true);p.Tick(true,true,false,false,true);
    CHECK(p.Tick(true,false,true,false,false)==Exit); // Lost focus or script control.
    CHECK(Frontal(0,0,-100));CHECK(!Frontal(0,0,100));CHECK(!Frontal(0,100,0));
    CHECK(Frontal(512,100,0));CHECK(!Frontal(512,-100,0));CHECK(!Frontal(0,0,0));
    CHECK(Frontal(0,90,-100));CHECK(!Frontal(0,300,-100));
    // The protected direction must agree with the existing, separately tested
    // body-to-camera conversion, including diagonal headings and angle wrap.
    for(int h=0;h<2048;h+=64) {
        ff_camera::FirstPersonView view;view.Begin(static_cast<int16_t>(h));
        const auto ahead=view.Eye({0,0,0},0,100);
        CHECK(Frontal(h,ahead.x,ahead.z));CHECK(Frontal(h+2048,ahead.x,ahead.z));
        CHECK(!Frontal(h,-ahead.x,-ahead.z));CHECK(!Frontal(h,-ahead.z,ahead.x));
    }
    CHECK(OrdinaryMelee(0x1A));CHECK(!OrdinaryMelee(0x41));CHECK(!OrdinaryMelee(0xE6));
    CHECK(!StartAction(0x9E) && !StartAction(0x85) && StartAction(0x45));
    AttackWindow w;w.Observe(1,0,5,1);CHECK(w.First(0));CHECK(!w.First(0));CHECK(w.First(1));
    w.Observe(1,0,6,1);CHECK(!w.First(0));w.Observe(1,0,7,0);w.Observe(1,0,8,1);CHECK(w.First(0));
    w.Observe(1,0,2,1);CHECK(w.First(0));w.Observe(1,0,3,0x801);CHECK(w.First(0));
    w.Observe(1,0,4,0x801);CHECK(!w.First(0));w.Observe(2,0,4,1);CHECK(w.First(0));
    w.Observe(3,0,4,0x100);CHECK(w.First(0));w.Observe(3,0,5,0x100);CHECK(!w.First(0));
    puts(failures?"Player guard policy: FAIL":"Player guard policy: PASS");return failures?1:0;
}
