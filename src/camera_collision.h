#pragma once
#include "orbit_camera.h"
#include <algorithm>

namespace ff_camera {
inline Point Add(Point a, Point b) { return {a.x+b.x,a.y+b.y,a.z+b.z}; }
inline Point Sub(Point a, Point b) { return {a.x-b.x,a.y-b.y,a.z-b.z}; }
inline Point Scale(Point a, double s) { return {a.x*s,a.y*s,a.z*s}; }
inline double Dot(Point a, Point b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
inline Point Cross(Point a, Point b) { return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x}; }
struct Triangle { Point a,b,c; };
inline Point ClosestSegment(Point p, Point a, Point b) {
    Point e=Sub(b,a); double length=Dot(e,e);
    return Add(a,Scale(e,length>1e-12?Clamp(Dot(Sub(p,a),e)/length,0,1):0));
}
inline Point ClosestTriangle(Point p, const Triangle& t) {
    Point ab=Sub(t.b,t.a),ac=Sub(t.c,t.a),ap=Sub(p,t.a);
    Point normal=Cross(ab,ac);
    if(Dot(normal,normal)<1e-16) {
        Point a=ClosestSegment(p,t.a,t.b),b=ClosestSegment(p,t.b,t.c),c=ClosestSegment(p,t.c,t.a);
        if(Dot(Sub(p,b),Sub(p,b))<Dot(Sub(p,a),Sub(p,a)))a=b;
        return Dot(Sub(p,c),Sub(p,c))<Dot(Sub(p,a),Sub(p,a))?c:a;
    }
    double d1=Dot(ab,ap),d2=Dot(ac,ap);
    if(d1<=0 && d2<=0)return t.a;
    Point bp=Sub(p,t.b); double d3=Dot(ab,bp),d4=Dot(ac,bp);
    if(d3>=0 && d4<=d3)return t.b;
    double vc=d1*d4-d3*d2;
    if(vc<=0 && d1>=0 && d3<=0)return Add(t.a,Scale(ab,d1/(d1-d3)));
    Point cp=Sub(p,t.c); double d5=Dot(ab,cp),d6=Dot(ac,cp);
    if(d6>=0 && d5<=d6)return t.c;
    double vb=d5*d2-d1*d6;
    if(vb<=0 && d2>=0 && d6<=0)return Add(t.a,Scale(ac,d2/(d2-d6)));
    double va=d3*d6-d5*d4;
    if(va<=0 && d4>=d3 && d5>=d6)return Add(t.b,Scale(Sub(t.c,t.b),(d4-d3)/((d4-d3)+(d5-d6))));
    const double sum=va+vb+vc;
    if(fabs(sum)<1e-12) {
        Point a=ClosestSegment(p,t.a,t.b),b=ClosestSegment(p,t.b,t.c),c=ClosestSegment(p,t.c,t.a);
        if(Dot(Sub(p,b),Sub(p,b))<Dot(Sub(p,a),Sub(p,a)))a=b;
        return Dot(Sub(p,c),Sub(p,c))<Dot(Sub(p,a),Sub(p,a))?c:a;
    }
    return Add(t.a,Add(Scale(ab,vb/sum),Scale(ac,vc/sum)));
}
inline double FirstRoot(double a,double b,double c) {
    // a*t*t + 2*b*t + c = 0; initial overlaps are checked by the caller.
    if(fabs(a)<1e-12) {
        if(fabs(b)<1e-12)return 2;
        const double t=-c/(2*b);return t>=0 && t<=1?t:2;
    }
    double discriminant=b*b-a*c;
    if(discriminant<0)return 2;
    double first=(-b-sqrt(discriminant))/a,last=(-b+sqrt(discriminant))/a;
    if(first>last)std::swap(first,last);
    if(first>=0 && first<=1)return first;
    return last>=0 && last<=1?last:2;
}
inline bool SegmentBounds(Point p,Point v,const Triangle& tri,double radius,double best) {
    const double start[]={p.x,p.y,p.z},delta[]={v.x,v.y,v.z};
    const double a[]={tri.a.x,tri.a.y,tri.a.z},b[]={tri.b.x,tri.b.y,tri.b.z},c[]={tri.c.x,tri.c.y,tri.c.z};
    double enter=0,leave=best;
    for(int i=0;i<3;++i) {
        double lo=fmin(a[i],fmin(b[i],c[i]))-radius,hi=fmax(a[i],fmax(b[i],c[i]))+radius;
        if(fabs(delta[i])<1e-12){if(start[i]<lo || start[i]>hi)return false;}
        else {
            double first=(lo-start[i])/delta[i],last=(hi-start[i])/delta[i];
            if(first>last)std::swap(first,last);
            enter=fmax(enter,first);leave=fmin(leave,last);if(enter>leave)return false;
        }
    }
    return true;
}
inline double SweepSphereSegment(Point start,Point end,double start_radius,double end_radius,const Triangle& tri,double best=1) {
    Point delta=Sub(end,start);
    const double growth=end_radius-start_radius;
    if(!SegmentBounds(start,delta,tri,fmax(start_radius,end_radius),best))return best;
    Point closest=ClosestTriangle(start,tri),difference=Sub(start,closest);
    const double separation_squared=Dot(difference,difference);
    if(separation_squared<=start_radius*start_radius) {
        // A pivot sphere can already touch a wall while its boom points away.
        // Distance to a convex triangle cannot decrease along a separating
        // straight segment; this contact must not collapse every view angle.
        return Dot(difference,delta)>sqrt(separation_squared)*growth+1e-8?best:0;
    }
    Point normal=Cross(Sub(tri.b,tri.a),Sub(tri.c,tri.a));double length=sqrt(Dot(normal,normal));
    if(length>1e-8) {
        normal=Scale(normal,1/length);double distance=Dot(Sub(start,tri.a),normal),speed=Dot(delta,normal);
        for(int sign=-1;sign<=1;sign+=2) {
            const double closing=speed-sign*growth;
            if(fabs(closing)<1e-12)continue;
            double t=(sign*start_radius-distance)/closing;
            if(t<0 || t>best)continue;
            Point contact=Sub(Add(start,Scale(delta,t)),Scale(normal,sign*(start_radius+growth*t)));
            Point projection=ClosestTriangle(contact,tri),gap=Sub(contact,projection);
            if(Dot(gap,gap)<1e-8)best=t;
        }
    }
    Point vertices[]={tri.a,tri.b,tri.c};
    for(int i=0;i<3;++i) {
        Point w=Sub(start,vertices[i]);
        best=fmin(best,FirstRoot(Dot(delta,delta)-growth*growth,Dot(w,delta)-start_radius*growth,Dot(w,w)-start_radius*start_radius));
        Point edge=Sub(vertices[(i+1)%3],vertices[i]);double ee=Dot(edge,edge);
        if(ee<1e-12)continue;
        Point perpendicular=Sub(delta,Scale(edge,Dot(delta,edge)/ee));
        Point offset=Sub(w,Scale(edge,Dot(w,edge)/ee));
        double t=FirstRoot(Dot(perpendicular,perpendicular)-growth*growth,Dot(offset,perpendicular)-start_radius*growth,Dot(offset,offset)-start_radius*start_radius);
        if(t<=best) {
            double along=(Dot(w,edge)+Dot(delta,edge)*t)/ee;
            if(along>=0 && along<=1)best=t;
        }
    }
    return best;
}
inline double SweepSphere(Point start,Point end,double radius,const Triangle& tri,double best=1) {
    return SweepSphereSegment(start,end,radius,radius,tri,best);
}
inline double SweepCameraProbe(Point start,Point end,double radius,const Triangle& tri,double best=1) {
    const Point delta=Sub(end,start);const double length=sqrt(Dot(delta,delta));
    if(length<1e-8)return best;
    // Grow the comfort radius over the first character-height of the boom.
    // A pivot beside a hood need not fit a full camera sphere. The centerline
    // is still swept, so a nearby thin wall cannot be skipped.
    const double prefix=fmin(length,160.0),fraction=prefix/length;
    const Point join=Add(start,Scale(delta,fraction));
    const double join_radius=radius*prefix/160.0;
    const double prefix_contact=SweepSphereSegment(start,join,0,join_radius,tri,fmin(1,best/fraction));
    if(prefix_contact<1)best=fmin(best,prefix_contact*fraction);
    if(fraction<1 && best>fraction) {
        const double suffix_contact=SweepSphere(join,end,radius,tri,fmin(1,(best-fraction)/(1-fraction)));
        if(suffix_contact<1)best=fmin(best,fraction+(1-fraction)*suffix_contact);
    }
    return best;
}
inline bool InverseBasis(const float* m,double (&inverse)[9]) {
    const double a=m[0],b=m[1],c=m[2],d=m[4],e=m[5],f=m[6],g=m[8],h=m[9],i=m[10];
    const double det=a*(e*i-f*h)-b*(d*i-f*g)+c*(d*h-e*g);
    if(!isfinite(det) || fabs(det)<1e-8)return false;
    const double out[]={e*i-f*h,c*h-b*i,b*f-c*e,f*g-d*i,a*i-c*g,c*d-a*f,d*h-e*g,b*g-a*h,a*e-b*d};
    for(int n=0;n<9;++n)inverse[n]=out[n]/det;
    return true;
}
inline Point WorldVertex(const float* matrix,const double* inverse,Point camera,Point vertex) {
    const double x=matrix[0]*vertex.x+matrix[1]*vertex.y+matrix[2]*vertex.z+matrix[3];
    const double y=matrix[4]*vertex.x+matrix[5]*vertex.y+matrix[6]*vertex.z+matrix[7];
    const double z=matrix[8]*vertex.x+matrix[9]*vertex.y+matrix[10]*vertex.z+matrix[11];
    return Add(camera,{inverse[0]*x+inverse[1]*y+inverse[2]*z,inverse[3]*x+inverse[4]*y+inverse[5]*z,inverse[6]*x+inverse[7]*y+inverse[8]*z});
}
} // namespace ff_camera
