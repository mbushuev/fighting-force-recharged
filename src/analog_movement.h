#pragma once
#include <math.h>

namespace ff_input {
constexpr double kStickRunStart=0.86,kStickRunEnd=0.76;
inline bool AutomaticRun(double magnitude,bool running) {
    return magnitude>=(running?kStickRunEnd:kStickRunStart);
}
struct AnalogMotion {
    double speed=0;
    bool running=false;
    double Update(double magnitude,double walk,double run,bool force_run,double seconds,bool automatic=false) {
        magnitude=fmax(0,fmin(1,magnitude));
        if(magnitude==0) {speed=0;running=false;return 0;}
        running=force_run || (automatic && AutomaticRun(magnitude,running));
        const double walking=walk*fmin(1,magnitude/(automatic?kStickRunStart:1));
        const double blend=fmax(0,(magnitude-kStickRunStart)/(1-kStickRunStart));
        const double target=force_run?run:running?walk+(run-walk)*blend:walking;
        if(!force_run && !automatic && speed>walk)speed=walk;
        const double step=run*fmax(0,fmin(0.1,seconds))/(target>speed?0.20:0.12);
        speed+=fmax(-step,fmin(step,target-speed));
        return speed;
    }
};
} // namespace ff_input
