#pragma once
#include "orbit_camera.h"

namespace ff_focus {
using ff_camera::Point;
using ff_camera::Pose;
constexpr unsigned Actors=8;
struct Target {
    Point point;
    int type=-1,health=0,action=0;
    bool present=false,visible=false;
};
struct ScreenPoint {double x=0,y=0,depth=0;};
inline bool Project(Point p,Pose view,double fx,double fy,double cx,double cy,ScreenPoint& out) {
    if(!isfinite(fx) || !isfinite(fy) || fx<=0 || fy<=0)return false;
    const double yaw=ff_camera::Radians(static_cast<int16_t>(view.yaw&~31));
    const double pitch=ff_camera::Radians(static_cast<int16_t>(view.pitch&~31));
    const double dx=p.x-view.x/65536.0,dy=p.y-view.y/65536.0,dz=p.z-view.z/65536.0;
    const double x=cos(yaw)*dx+sin(yaw)*dz,flat=-sin(yaw)*dx+cos(yaw)*dz;
    const double y=cos(pitch)*dy-sin(pitch)*flat,z=sin(pitch)*dy+cos(pitch)*flat;
    if(!isfinite(z) || z<=32)return false;
    out={cx+x*fx/z,cy+y*fy/z,z};return isfinite(out.x) && isfinite(out.y);
}
inline double Distance(Point a,Point b) {return hypot(hypot(a.x-b.x,a.z-b.z),a.y-b.y);}
inline bool Enemy(unsigned slot,const Target& t) {
    // Retail melee scans eight records; types 0x11/0x33 are noncombatants.
    return slot>=2 && slot<Actors && t.present && t.type>=0 && t.type!=0x11 && t.type!=0x33 &&
        t.health>0 && t.action!=0xCB && t.action!=0xCD;
}
struct View {
    Pose pose={};double fx=160,fy=160,width=320,height=240;
    bool OnScreen(Point p,ScreenPoint& out)const {
        return width>0 && height>0 && Project(p,pose,fx,fy,width/2,height/2,out) &&
            out.x>=0 && out.x<=width && out.y>=0 && out.y<=height;
    }
};
inline int Choose(const Target (&targets)[Actors],Point player,const View& view,int current=-1,int direction=0) {
    ScreenPoint origin;
    if(direction && (current<2 || current>=int(Actors) || !view.OnScreen(targets[current].point,origin)))return -1;
    double best=1e30;int selected=-1;
    for(unsigned i=2;i<Actors;++i) {
        const auto& t=targets[i];ScreenPoint p;
        if(int(i)==current || !Enemy(i,t) || !t.visible || Distance(player,t.point)>1800 || !view.OnScreen(t.point,p))continue;
        const double horizontal=direction?(p.x-origin.x)*direction:fabs(p.x-view.width/2);
        if(direction && horizontal<2)continue;
        const double score=horizontal/view.width+fabs(p.y-(direction?origin.y:view.height/2))/view.height*0.35+
            Distance(player,t.point)/1800*(direction?0.04:0.15);
        if(score<best){best=score;selected=int(i);}
    }
    return selected;
}
inline double Approach(double from,double to,double seconds,double speed) {
    const double delta=ff_camera::Wrap(to-from),dt=ff_camera::Clamp(seconds,0,0.1);
    const double step=ff_camera::Clamp(delta*(1-exp(-dt/0.10)),-speed*dt,speed*dt);
    return ff_camera::Wrap(from+step);
}
struct State {
    int target=-1,type=-1,stage=-1;
    bool button=true,switch_ready=false,previous=true,next=true;
    double hidden=0;
    Point last;
    void Reset(){*this={};}
    bool Active()const{return target>=2 && target<int(Actors);}
    // An unhandled rising press retains the existing recenter operation.
    bool Update(const Target (&targets)[Actors],Point player,const View& view,int scene,bool enabled,
                bool pressed,double switch_x,bool prev,bool following,double seconds) {
        if(!enabled || scene!=stage || seconds>0.25) {
            Reset();stage=scene;button=pressed;previous=prev;next=following;return false;
        }
        if(Active()) {
            const auto& t=targets[target];hidden=t.visible?0:hidden+ff_camera::Clamp(seconds,0,0.1);
            if(!Enemy(unsigned(target),t) || t.type!=type || Distance(player,t.point)>2200 ||
               Distance(last,t.point)>700 || hidden>0.4)target=-1;
            else last=t.point;
        }
        bool recenter=false;
        if(pressed && !button) {
            if(Active())target=-1;
            else {target=Choose(targets,player,view);hidden=0;recenter=target<0;}
        }
        const bool flick=fabs(switch_x)>=0.65 && switch_ready;
        if(fabs(switch_x)<0.25)switch_ready=true;
        else if(flick)switch_ready=false;
        const int direction=(following&&!next)?1:(prev&&!previous)?-1:flick?(switch_x>0?1:-1):0;
        if(Active() && direction) {
            const int other=Choose(targets,player,view,target,direction);if(other>=0){target=other;hidden=0;}
        }
        if(Active()){type=targets[target].type;last=targets[target].point;}
        button=pressed;previous=prev;next=following;return recenter;
    }
};
inline double Yaw(Point from,Point to) {return atan2(from.x-to.x,to.z-from.z);}
inline void Track(Point origin,Point target,double seconds,bool eye,double minimum,double maximum,double& yaw,double& pitch) {
    yaw=Approach(yaw,Yaw(origin,target),seconds,240*ff_camera::kPi/180);
    if(eye) {
        const double wanted=atan2(target.y-origin.y,hypot(target.x-origin.x,target.z-origin.z));
        pitch=ff_camera::Clamp(Approach(pitch,wanted,seconds,160*ff_camera::kPi/180),minimum,maximum);
    }
}
} // namespace ff_focus
