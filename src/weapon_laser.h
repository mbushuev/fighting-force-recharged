#pragma once
#include "first_person.h"
#include "combat_focus.h"
namespace ff_laser {
inline ff_camera::Point ToWorld(ff_camera::Point p,ff_camera::Pose view,bool direction=false) {
    const double yaw=ff_camera::Radians(static_cast<int16_t>(view.yaw&~31));
    const double pitch=ff_camera::Radians(static_cast<int16_t>(view.pitch&~31));
    const double flat=-sin(pitch)*p.y+cos(pitch)*p.z;
    return {cos(yaw)*p.x-sin(yaw)*flat+(direction?0:view.x/65536.0),
        cos(pitch)*p.y+sin(pitch)*p.z+(direction?0:view.y/65536.0),
        sin(yaw)*p.x+cos(yaw)*flat+(direction?0:view.z/65536.0)};
}
inline bool Muzzle(const int16_t* vertices,unsigned count,const float* matrix,ff_camera::Pose view,
                   ff_camera::Point& start,ff_camera::Point& direction) {
    if(!vertices || !count || count>16384 || !matrix)return false;
    // The supported firearm models point along local +Z. Average the front
    // cap instead of using the wrist joint or an actor-relative screen offset.
    int front=-32768;for(unsigned i=0;i<count;++i)front=(std::max)(front,int(vertices[i*4+2]));
    ff_camera::Point tip;unsigned points=0;
    for(unsigned i=0;i<count;++i)if(vertices[i*4+2]==front) {
        tip.x+=vertices[i*4];tip.y+=vertices[i*4+1];tip.z+=vertices[i*4+2];++points;
    }
    if(!points)return false;tip=ff_camera::Scale(tip,1.0/points);
    direction=ToWorld({matrix[2],matrix[6],matrix[10]},view,true);
    const double length=hypot(hypot(direction.x,direction.z),direction.y);
    if(!isfinite(length) || length<0.01)return false;direction=ff_camera::Scale(direction,1/length);
    start=ToWorld({matrix[0]*tip.x+matrix[1]*tip.y+matrix[2]*tip.z+matrix[3],
        matrix[4]*tip.x+matrix[5]*tip.y+matrix[6]*tip.z+matrix[7],
        matrix[8]*tip.x+matrix[9]*tip.y+matrix[10]*tip.z+matrix[11]},view);
    return isfinite(start.x) && isfinite(start.y) && isfinite(start.z);
}
inline double Box(ff_camera::Point from,ff_camera::Point delta,ff_camera::Point lo,ff_camera::Point hi,double limit) {
    const double a[]={from.x,from.y,from.z},d[]={delta.x,delta.y,delta.z};
    const double lower[]={lo.x,lo.y,lo.z},upper[]={hi.x,hi.y,hi.z};double entry=0,exit=limit;
    for(unsigned axis=0;axis<3;++axis) {
        if(fabs(d[axis])<1e-8){if(a[axis]<lower[axis] || a[axis]>upper[axis])return limit;continue;}
        const double x=(lower[axis]-a[axis])/d[axis],y=(upper[axis]-a[axis])/d[axis];
        entry=fmax(entry,fmin(x,y));exit=fmin(exit,fmax(x,y));if(entry>exit)return limit;
    }
    return entry>0 && entry<limit?entry:limit;
}
inline uint32_t Color(bool loaded){return loaded?0xFF4098FF:0xFFFF4040;}
inline double Depth(ff_camera::Point p,ff_camera::Pose view) {
    const double yaw=ff_camera::Radians(static_cast<int16_t>(view.yaw&~31));
    const double pitch=ff_camera::Radians(static_cast<int16_t>(view.pitch&~31));
    return sin(pitch)*(p.y-view.y/65536.0)+cos(pitch)*(-sin(yaw)*(p.x-view.x/65536.0)+cos(yaw)*(p.z-view.z/65536.0));
}
inline bool ClipBeam(ff_camera::Point& a,ff_camera::Point& b,ff_camera::Pose view) {
    const double da=Depth(a,view),db=Depth(b,view),plane=33;
    if(da<plane && db<plane)return false;
    if(da<plane)a=ff_camera::Add(a,ff_camera::Scale(ff_camera::Sub(b,a),(plane-da)/(db-da)));
    else if(db<plane)b=ff_camera::Add(a,ff_camera::Scale(ff_camera::Sub(b,a),(plane-da)/(db-da)));
    return true;
}
inline bool ClipLine(double& ax,double& ay,double& bx,double& by,double width,double height) {return ff_focus::ClipLine(ax,ay,bx,by,width,height);}

}
