#pragma once
#include <math.h>

namespace ff_input {
struct AnalogMotion {
    double speed=0;
    bool running=false;
    double Update(double magnitude,double walk,double run,bool force_run,double seconds) {
        magnitude=fmax(0,fmin(1,magnitude));
        if(magnitude==0) {speed=0;running=false;return 0;}
        const double target=force_run?run:walk*magnitude;
        if(!force_run && speed>walk)speed=walk;
        const double step=run*fmax(0,fmin(0.1,seconds))/(target>speed?0.20:0.12);
        speed+=fmax(-step,fmin(step,target-speed));
        // Stick deflection controls walking speed only. Native run attacks
        // must require an explicit run button, including immediately on release.
        running=force_run;
        return speed;
    }
};
} // namespace ff_input
