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
    CHECK(p.Tick(true,true,true,false,false)==Entry); // Re-press during lowering.
    CHECK(p.Tick(true,true,true,false,false)==Entry);
    CHECK(p.Tick(true,true,false,false,false)==None); // Non-recoverable native interruption.
    CHECK(p.Tick(true,true,false,false,true)==None); // Release after interruption.
    p.Tick(false,true,false,false,true);p.Tick(true,true,false,false,true);
    CHECK(p.Tick(true,false,true,false,false)==Exit); // Lost focus or script control.
    // A masked button during focus/script suspension is not a physical release.
    p.Tick(false,false,true,true,false);
    CHECK(p.Tick(true,true,false,false,true)==None);
    CHECK(p.Tick(false,true,false,false,true)==None);
    CHECK(p.Tick(true,true,false,false,true)==Entry);
    for(auto phase:{Entry,Hold,Impact,Exit}) {
        GuardState s;s.phase=phase;
        CHECK(s.Tick(false,true,true,false,false,true)==None && !s.Protects());
        s.phase=phase;CHECK(s.Tick(true,true,true,false,false,true)==(phase==Exit?Entry:phase)); // Block wins.
        s.phase=phase;CHECK(s.Tick(false,false,true,false,false,true)==Exit); // No script cancel.
    }
    for(auto phase:{Entry,Hold,Impact}) {
        GuardState s;s.phase=phase;
        CHECK(s.Tick(true,true,false,false,false,false,true)==None && !s.Protects());
        CHECK(s.Tick(true,true,false,false,false,false,true)==None);
        CHECK(s.Tick(true,true,false,false,true)==Entry); // Held request survives a standing hit.
        CHECK(s.Tick(true,false,false,false,false,false,true)==None);
        CHECK(s.Tick(true,true,false,false,true)==None); // Focus/script loss still disarms it.
    }
    for(int action=0;action<0x121;++action)
        CHECK(HitRecovery(action)==(action==0x85 || action==0x86 || action==0x87 || action==0x88 ||
            action==0x89 || action==0x98 || action==0x99));
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
    for(int action:{0x45,0x46,0x47,0x48,0x49,0x4C,0x4E,0xC6})CHECK(StartAction(action));
    for(int action:{0x70,0x9F,0x10E,0xC1,0xC3,0xC7})CHECK(!StartAction(action));
    for(int action=0x5C;action<=0x61;++action)CHECK(TurnAction(action) && !StartAction(action));
    // All native sectors, rotated camera bases, wrap and contradictory keys.
    const unsigned directions[]={1,9,8,10,2,6,4,5};
    const int relative[]={1024,768,512,256,0,1792,1536,1280};
    for(unsigned d=0;d<8;++d)for(int camera:{0,512*32,2047*32}) {
        int heading=123;const int target=((camera>>5)+relative[d])&2047;
        for(int tick=0;tick<8;++tick){const int next=Turn(heading,camera,directions[d]);
            const int change=((next-heading+1024)&2047)-1024;CHECK(abs(change)<=128);heading=next;}
        CHECK(heading==target && Turn(heading,camera,directions[d])==target);
    }
    CHECK(Turn(2040,0,2)==0 && Turn(4,2047*32,2)==2047);
    CHECK(Turn(500,0,0)==500 && Turn(500,0,15)==500);
    AttackWindow w;w.Observe(1,0,5,1);CHECK(w.First(0));CHECK(!w.First(0));CHECK(w.First(1));
    w.Observe(1,0,6,1);CHECK(!w.First(0));w.Observe(1,0,7,0);w.Observe(1,0,8,1);CHECK(w.First(0));
    w.Observe(1,0,2,1);CHECK(w.First(0));w.Observe(1,0,3,0x801);CHECK(w.First(0));
    w.Observe(1,0,4,0x801);CHECK(!w.First(0));w.Observe(2,0,4,1);CHECK(w.First(0));
    w.Observe(3,0,4,0x100);CHECK(w.First(0));w.Observe(3,0,5,0x100);CHECK(!w.First(0));
    puts(failures?"Player guard policy: FAIL":"Player guard policy: PASS");return failures?1:0;
}
