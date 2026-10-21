#include <stdio.h>
#include <limits.h>
#include "../src/frame_interpolation.h"
static int failures;
#define CHECK(x) do {if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
int main() {
    using namespace ff_frame;
    CHECK(Midpoint(INT_MIN,INT_MAX)==0);
    CHECK(Midpoint(INT_MAX,INT_MAX)==INT_MAX);
    CHECK(Midpoint(INT_MIN,INT_MIN)==INT_MIN);
    CHECK(Angle(2040,8,2048)==0 && Angle(8,2040,2048)==0);
    CHECK(Angle(4090,6,4096)==0 && Angle(6,4090,4096)==0);
    CHECK(Angle(-32,32,65536)==0);
    Snapshot a,b;a.stage=b.stage=1;a.players=b.players=2;a.tick=UINT_MAX;b.tick=0;
    CHECK(a.Consecutive(b));b.tick=1;CHECK(!a.Consecutive(b));
    b.tick=0;b.stage=2;CHECK(!a.Consecutive(b));b.stage=1;
    b.eye[1]=true;CHECK(!a.Consecutive(b));b.eye[1]=false;
    b.players=1;CHECK(!a.Consecutive(b));b.players=2;
    Write<int16_t>(a.actors[0],0x34,2);Write<int16_t>(b.actors[0],0x34,2);
    Write<int32_t>(a.actors[0],0x28,-100*65536);Write<int32_t>(b.actors[0],0x28,100*65536);
    Write<int32_t>(a.actors[0],0x10,10);Write<int32_t>(b.actors[0],0x10,30);
    Write<int16_t>(a.actors[0],0x38,2040);Write<int16_t>(b.actors[0],0x38,8);
    Write<int16_t>(a.palettes[0],0x118,4090);Write<int16_t>(b.palettes[0],0x118,6);
    Write<int32_t>(b.palettes[0],4,0x12345678); // Opaque native data stays exact.
    uint8_t actor[0xF4],palette[0x238];memcpy(actor,b.actors[0],sizeof(actor));memcpy(palette,b.palettes[0],sizeof(palette));
    CHECK(Actor(actor,palette,a.actors[0],b.actors[0],a.palettes[0],b.palettes[0]));
    CHECK(Read<int32_t>(actor,0x28)==0 && Read<int32_t>(actor,0x10)==20);
    CHECK(Read<int16_t>(actor,0x38)==0 && Read<int16_t>(palette,0x118)==0);
    CHECK(Read<int32_t>(palette,4)==0x12345678);
    Write<int16_t>(b.actors[0],0x34,3);
    CHECK(!Actor(actor,palette,a.actors[0],b.actors[0],a.palettes[0],b.palettes[0]));
    Write<int16_t>(b.actors[0],0x34,2);Write<int32_t>(b.actors[0],0x28,2000*65536);
    CHECK(!Actor(actor,palette,a.actors[0],b.actors[0],a.palettes[0],b.palettes[0]));
    Write<int32_t>(a.objects[0],0,500);Write<int32_t>(b.objects[0],0,500);
    Write<int32_t>(b.objects[0],4,100*65536);Write<int16_t>(a.objects[0],0x10,-32);Write<int16_t>(b.objects[0],0x10,32);
    uint8_t object[36];memcpy(object,b.objects[0],sizeof(object));
    CHECK(Object(object,a.objects[0],b.objects[0]));
    CHECK(Read<int32_t>(object,4)==50*65536 && Read<int16_t>(object,0x10)==0);
    Write<int32_t>(b.objects[0],0,-1);CHECK(!Object(object,a.objects[0],b.objects[0]));
    Cadence pace;
    for(int i=1;i<=600;++i)CHECK(pace.Next((i-1)*100,6000)==i*100);
    CHECK(pace.Next(100000,6000)==100100); // A pause cannot cause catch-up simulation.
    CHECK(pace.Next(10,6000)==110);pace.Reset();CHECK(pace.Next(200,6000)==300);
    printf("Render interpolation and 60 Hz cadence: %s\n",failures?"FAILED":"passed");
    return failures?1:0;
}
