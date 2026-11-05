#pragma once
#include <math.h>
#include <algorithm>

namespace ff_camera {
struct PropVertex {double x,y,z,u,v;};
struct PropProjection {
    double focal_x,focal_y,center_x,center_y,width,height,depth_scale;
};
// A far-plane cut produces at most four vertices. Triangulate the result
// before the five-plane clipper so every emitted polygon still fits eight.
inline unsigned ClipPropFarTriangle(const PropVertex* triangle,double far_depth,PropVertex* output) {
    unsigned count=0;
    auto previous=triangle[2];double before=far_depth-previous.z;
    for(unsigned i=0;i<3;++i) {
        const auto current=triangle[i];const double after=far_depth-current.z;
        if((before>=0)!=(after>=0)) {
            const double t=before/(before-after);
            output[count++]={previous.x+(current.x-previous.x)*t,
                previous.y+(current.y-previous.y)*t,far_depth,
                previous.u+(current.u-previous.u)*t,previous.v+(current.v-previous.v)*t};
        }
        if(after>=0)output[count++]=current;
        previous=current;before=after;
    }
    return count>=3?count:0;
}
// Clip native triangles before perspective division. Five planes add at most
// five vertices to a triangle, fitting the retail eight-vertex polygon record.
inline unsigned ClipPropTriangle(const PropVertex* triangle,const PropProjection& p,PropVertex* output) {
    PropVertex buffers[2][8];unsigned count=3,source=0;
    for(unsigned i=0;i<3;++i)buffers[0][i]=triangle[i];
    const double planes[5][4]={{0,0,1,-1},{p.focal_x,0,p.center_x,0},
        {-p.focal_x,0,p.width-p.center_x,0},{0,p.focal_y,p.center_y,0},
        {0,-p.focal_y,p.height-p.center_y,0}};
    for(const auto& plane:planes) {
        unsigned next=0;
        auto distance=[&](const PropVertex& v){return v.x*plane[0]+v.y*plane[1]+v.z*plane[2]+plane[3];};
        auto previous=buffers[source][count-1];double before=distance(previous);
        for(unsigned i=0;i<count;++i) {
            const auto current=buffers[source][i];const double after=distance(current);
            if((before>=0)!=(after>=0)) {
                if(next==8)return 0;
                const double t=before/(before-after);
                buffers[1-source][next++]={previous.x+(current.x-previous.x)*t,
                    previous.y+(current.y-previous.y)*t,previous.z+(current.z-previous.z)*t,
                    previous.u+(current.u-previous.u)*t,previous.v+(current.v-previous.v)*t};
            }
            if(after>=0) {if(next==8)return 0;buffers[1-source][next++]=current;}
            previous=current;before=after;
        }
        count=next;source=1-source;if(count<3)return 0;
    }
    for(unsigned i=0;i<count;++i)output[i]=buffers[source][i];
    return count;
}
} // namespace ff_camera
