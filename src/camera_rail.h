#pragma once
#include "camera_collision.h"

namespace ff_camera {
inline bool SubwayRailOcclusionPlane(const float* matrix,double (&plane)[4]) {
    double inverse[9];if(!InverseBasis(matrix,inverse))return false;
    const double x=-(inverse[0]*matrix[3]+inverse[1]*matrix[7]+inverse[2]*matrix[11]);
    const double y=-(inverse[3]*matrix[3]+inverse[4]*matrix[7]+inverse[5]*matrix[11]);
    // SUBRAIL1's opening is between local X 0 and 471 at Y 0. Its left
    // edge varies by up to three units outward; use its inner boundary.
    // A plane through the eye and the near lip hides the recessed wall
    // below the platform silhouette, which the background floor cannot Z-test.
    if(!isfinite(x) || !isfinite(y) || y<=0 || (x>=0 && x<=471))return false;
    const double slope=y/(x-(x<0?0:471));
    for(int i=0;i<3;++i)plane[i]=inverse[3+i]-slope*inverse[i];
    plane[3]=0;return true;
}
} // namespace ff_camera
