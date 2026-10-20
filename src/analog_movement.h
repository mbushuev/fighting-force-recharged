#pragma once
#include <math.h>

namespace ff_input {
struct AnalogMotion {
    double speed=0;
    bool running=false;
    double Update(double magnitude,double walk,double run,bool force_run,double seconds) {
        magnitude=fmax(0,fmin(1,magnitude));
        if(magnitude==0) {speed=0;running=false;return 0;}
        const double target=force_run?run:(magnitude<=0.65 ? walk*magnitude/0.65 :
            walk+(run-walk)*(magnitude-0.65)/0.35);
        const double step=run*fmax(0,fmin(0.1,seconds))/(target>speed?0.20:0.12);
        speed+=fmax(-step,fmin(step,target-speed));
        // Native animation selection has two gaits. Hysteresis avoids repeated
        // walk/run restarts while physical speed remains continuous across it.
        if(force_run || magnitude>=0.72)running=true;
        else if(magnitude<=0.64)running=false;
        return speed;
    }
};
} // namespace ff_input
