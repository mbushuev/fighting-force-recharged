#pragma once
#include <initializer_list>
#include "orbit_camera.h"

namespace ff_camera {
inline Point Midpoint(Point a,Point b) {return {(a.x+b.x)*0.5,(a.y+b.y)*0.5,(a.z+b.z)*0.5};}
inline Pose BlendPose(Pose a,Pose b,double amount) {
    amount=Clamp(amount,0,1);
    return {Fixed((a.x+(double(b.x)-a.x)*amount)/65536),
        Fixed((a.y+(double(b.y)-a.y)*amount)/65536),
        Fixed((a.z+(double(b.z)-a.z)*amount)/65536),
        NativeAngle(Radians(a.yaw)+Wrap(Radians(b.yaw)-Radians(a.yaw))*amount),
        NativeAngle(Radians(a.pitch)+Wrap(Radians(b.pitch)-Radians(a.pitch))*amount)};
}
// Normalized screen occupancy of a standing actor, including a conservative
// horizontal radius. The camera uses positive-down Y and looks along +Z.
inline double ActorOccupancy(Pose camera,Point feet,double focal_x,double focal_y,double width,double height) {
    if(width<=0 || height<=0 || focal_x<=0 || focal_y<=0)return 100;
    const double yaw=Radians(static_cast<int16_t>(camera.yaw&~31));
    const double pitch=Radians(static_cast<int16_t>(camera.pitch&~31));
    double occupancy=0;
    for(double y:{0.0,-300.0})for(double x:{-70.0,70.0}) {
        const double dx=feet.x+x-camera.x/65536.0,dy=feet.y+y-camera.y/65536.0;
        const double dz=feet.z-camera.z/65536.0;
        const double vx=cos(yaw)*dx+sin(yaw)*dz;
        const double flat=-sin(yaw)*dx+cos(yaw)*dz;
        const double vy=cos(pitch)*dy-sin(pitch)*flat;
        const double vz=sin(pitch)*dy+cos(pitch)*flat;
        if(vz<=32)return 100;
        occupancy=fmax(occupancy,fmax(fabs(vx*focal_x/vz)/(width*0.5),fabs(vy*focal_y/vz)/(height*0.5)));
    }
    return occupancy;
}
struct CooperativeSplit {
    bool separate=false;
    double amount=0,unsafe_seconds=0,safe_seconds=0;
    void Update(double occupancy,bool personal_view,double seconds) {
        const double dt=Clamp(seconds,0,0.1);
        if(personal_view) {separate=true;safe_seconds=unsafe_seconds=0;}
        else if(!separate) {
            safe_seconds=0;
            unsafe_seconds=occupancy>0.90?unsafe_seconds+dt:0;
            if(occupancy>1.05 || unsafe_seconds>=0.15) {separate=true;unsafe_seconds=0;}
        } else {
            unsafe_seconds=0;
            safe_seconds=occupancy<0.70?safe_seconds+dt:0;
            if(safe_seconds>=0.85) {separate=false;safe_seconds=0;}
        }
        const double step=dt/(separate?0.40:0.65);
        amount=Clamp(amount+(separate?step:-step),0,1);
    }
    double Blend() const {return amount*amount*(3-2*amount);}
};
struct CooperativeFrustum {double focal_x,center_x;};
inline CooperativeFrustum SplitFrustum(unsigned player,double amount,double focal_x,double width) {
    // Each full-size intermediate is reduced to half width. At amount zero
    // these off-axis halves reproduce one continuous shared projection exactly.
    return {focal_x*2,width*((player?0.0:1.0)*(1-amount)+0.5*amount)};
}
} // namespace ff_camera
