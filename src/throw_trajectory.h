#pragma once
#include "camera_collision.h"
#include <stdint.h>
namespace ff_throw {
constexpr unsigned Capacity=48;
struct Path {ff_camera::Point points[Capacity];unsigned count=0;bool hit=false;};
inline bool Eligible(int type) {return type==0x1F0 || type==0x1F1 || (type>0x209 && type<0x233);}
inline int Lift(int type,unsigned hand,int action) {
    // THROW2A releases with 0x40; THROW3A/THROW5A release with 0x20.
    return hand==0 || action==0xA1 || action==0xA3 || type>0x209?32:64;
}
inline Path Flight(ff_camera::Point start,int sine,int cosine,int lift) {
    Path path;
    if(!isfinite(start.x) || !isfinite(start.y) || !isfinite(start.z) || start.y>=0 ||
       abs(sine)>32767 || abs(cosine)>32767 || (lift!=32 && lift!=64))return path;
    path.points[path.count++]=start;
    int speed=128;
    for(unsigned tick=1;tick<Capacity;++tick) {
        // RVA 0xA2E0: two truncated horizontal half-steps, then gravity,
        // vertical motion, and -3 horizontal drag. World Y is positive down.
        const auto previous=path.points[path.count-1];lift-=16;
        ff_camera::Point next={previous.x+double((speed*sine)>>1)/32768.0,
            previous.y-lift*0.5,previous.z-double((speed*cosine)>>1)/32768.0};
        speed=(std::max)(0,speed-3);
        if(next.y>=0) {
            const double t=-previous.y/(next.y-previous.y);
            next=ff_camera::Add(previous,ff_camera::Scale(ff_camera::Sub(next,previous),t));
            next.y=0;path.hit=true;
        }
        path.points[path.count++]=next;if(path.hit)break;
    }
    return path;
}
inline double Intersect(ff_camera::Point a,ff_camera::Point b,const ff_camera::Triangle& tri,double limit) {
    using namespace ff_camera;
    const auto direction=Sub(b,a);
    if(!SegmentBounds(a,direction,tri,0,limit))return limit;
    const auto e1=Sub(tri.b,tri.a),e2=Sub(tri.c,tri.a),h=Cross(direction,e2);
    const double determinant=Dot(e1,h);if(fabs(determinant)<1e-9)return limit;
    const auto s=Sub(a,tri.a);const double u=Dot(s,h)/determinant;
    if(u<0 || u>1)return limit;
    const auto q=Cross(s,e1);const double v=Dot(direction,q)/determinant;
    if(v<0 || u+v>1)return limit;
    const double t=Dot(e2,q)/determinant;return t>=0 && t<limit?t:limit;
}
inline void Obstruct(Path& path,const ff_camera::Triangle* triangles,size_t count) {
    if(path.count<2 || !triangles)return;
    auto lo=path.points[0],hi=lo;
    for(unsigned i=1;i<path.count;++i) {
        const auto p=path.points[i];lo={fmin(lo.x,p.x),fmin(lo.y,p.y),fmin(lo.z,p.z)};
        hi={fmax(hi.x,p.x),fmax(hi.y,p.y),fmax(hi.z,p.z)};
    }
    unsigned segment=path.count-1;double fraction=1;bool contact=false;
    for(size_t n=0;n<count;++n) {
        const auto& t=triangles[n];
        if(fmax(t.a.x,fmax(t.b.x,t.c.x))<lo.x || fmin(t.a.x,fmin(t.b.x,t.c.x))>hi.x ||
           fmax(t.a.y,fmax(t.b.y,t.c.y))<lo.y || fmin(t.a.y,fmin(t.b.y,t.c.y))>hi.y ||
           fmax(t.a.z,fmax(t.b.z,t.c.z))<lo.z || fmin(t.a.z,fmin(t.b.z,t.c.z))>hi.z)continue;
        for(unsigned i=1;i<=segment;++i) {
            const double limit=i==segment?fraction:1;
            const double hit=Intersect(path.points[i-1],path.points[i],t,limit);
            if(hit<limit){segment=i;fraction=hit;contact=true;break;}
        }
    }
    if(contact) {
        path.points[segment]=ff_camera::Add(path.points[segment-1],
            ff_camera::Scale(ff_camera::Sub(path.points[segment],path.points[segment-1]),fraction));
        path.count=segment+1;path.hit=true;
    }
}
}
