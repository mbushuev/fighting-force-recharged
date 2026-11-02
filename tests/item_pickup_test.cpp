#include "../src/item_pickup.h"
#include <stdio.h>
static int failures;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
int main() {
    using namespace ff_pickup;
    Actor actor;actor.cosine=32767;Item item;item.type=0x1F3;
    CHECK(Reachable(actor,item));item.x=96*65536;CHECK(!Reachable(actor,item));
    item.x=95*65536;CHECK(Reachable(actor,item));item.y=1;CHECK(!Reachable(actor,item));
    item.y=0;item.remaining=-1;CHECK(!Reachable(actor,item));item.remaining=0;
    item.type=0x24B;CHECK(!Reachable(actor,item));
    item.type=0x210;item.x=0;item.z=-96*65536;CHECK(Reachable(actor,item));
    item.type=0x215;CHECK(!Reachable(actor,item));actor.hero=2;CHECK(Reachable(actor,item));
    item.type=0x21B;actor.hero=1;CHECK(!Reachable(actor,item));actor.hero=0;CHECK(Reachable(actor,item));
    CHECK(Action(0x1F3)==0x76 && Action(0x1F8)==0x80 && Action(0x1FA)==0x7B && Action(0x203)==0x7E && Action(0x210)==0x79);
    CHECK(ThrowAction(0x210,-1)==0xA1 && ThrowAction(0x1FB,-1)==0x9F);
    CHECK(ThrowAction(-1,0x1F8)==0xA3 && ThrowAction(-1,0x203)==0x45 && ThrowAction(-1,-1)==-1);
    CHECK(WorldWeapon(0x273)==0x1ED && WorldWeapon(0x297)==0x1EE && WorldWeapon(0x32D)==0x1FF);
    CHECK(WorldWeapon(0x274)==-1 && WorldWeapon(0x298)==-1 && WorldWeapon(0x32E)==-1);
    CHECK(!ReadyAction(0x85) && !ReadyAction(0x79) && !ReadyAction(0x57) && ReadyAction(0x45) && ReadyAction(0x54));
    Press p;CHECK(!p.Read(true,true,0,1,0,100));CHECK(!p.Read(false,true,0,2,0,133));
    CHECK(p.Read(true,true,0,3,0,166));CHECK(!p.Read(true,true,0,4,0,199));
    CHECK(!p.Read(true,false,0,5,0,232));CHECK(!p.Read(true,true,0,6,0,265));
    p.Read(false,true,0,7,0,298);CHECK(p.Read(true,true,0,8,0,331));
    CHECK(!p.Read(true,true,1,9,0,364));p.Read(false,true,1,10,0,397);
    CHECK(!p.Read(true,true,1,11,1,430));p.Read(false,true,1,12,1,463);
    CHECK(!p.Read(true,true,1,13,1,1100));p.Read(false,true,1,14,1,1133);
    CHECK(p.Read(true,true,1,15,1,1166));
    puts(failures?"Item pickup policy: FAIL":"Item pickup policy: PASS");return failures?1:0;
}
