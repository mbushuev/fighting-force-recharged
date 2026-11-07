#include "../src/analog_movement.h"
#include <stdio.h>
static int failures;
#define CHECK(x) do {if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);++failures;}}while(0)
int main() {
    using ff_input::AnalogMotion;
    double previous=0;
    for(int i=1;i<=100;++i) {
        AnalogMotion motion;
        for(int n=0;n<30;++n)motion.Update(i/100.0,10,30,false,1.0/30);
        CHECK(motion.speed>previous && motion.speed<=10 && !motion.running);previous=motion.speed;
    }
    AnalogMotion low,high;
    for(int i=0;i<30;++i) {low.Update(0.6499,10,30,false,0.033);high.Update(0.6501,10,30,false,0.033);}
    CHECK(fabs(low.speed-6.499)<0.001 && fabs(high.speed-low.speed)<0.01);
    AnalogMotion ramp;
    CHECK(ramp.Update(1,10,30,false,0.033)>0 && ramp.speed<6);
    previous=ramp.speed;
    for(int i=0;i<8;++i) {ramp.Update(1,10,30,false,0.033);CHECK(ramp.speed>=previous && ramp.speed<=10);previous=ramp.speed;}
    CHECK(ramp.speed==10 && !ramp.running);
    CHECK(ramp.Update(0,10,30,false,0.033)==0 && !ramp.running);
    ramp.Update(0.1,10,30,true,0.033);CHECK(ramp.running);
    for(int i=0;i<20;++i)ramp.Update(0.1,10,30,true,0.033);
    CHECK(ramp.speed==30);
    ramp.Update(1,10,30,false,0.033);CHECK(!ramp.running && ramp.speed==10);
    ramp={};CHECK(ramp.Update(1,10,30,false,0)==0);
    CHECK(ramp.Update(1,10,30,false,100)<=15); // Bound long frame gaps.
    AnalogMotion automatic;
    for(int i=0;i<20;++i)automatic.Update(0.85,10,30,false,0.033,true);
    CHECK(!automatic.running && automatic.speed<10 && automatic.speed>9.8);
    automatic.Update(0.86,10,30,false,0.033,true);
    CHECK(automatic.running && fabs(automatic.speed-10)<0.001);
    automatic.Update(0.82,10,30,false,0.033,true);
    CHECK(automatic.running && fabs(automatic.speed-10)<0.001);
    for(int i=0;i<20;++i)automatic.Update(1,10,30,false,0.033,true);
    CHECK(automatic.running && automatic.speed==30);
    automatic.Update(0.75,10,30,false,0.033,true);
    CHECK(!automatic.running && automatic.speed>10 && automatic.speed<30);
    automatic.Update(0.82,10,30,false,0.033,true);
    CHECK(!automatic.running);
    printf("Continuous analog speed: %s\n",failures?"FAIL":"PASS");return failures?1:0;
}
