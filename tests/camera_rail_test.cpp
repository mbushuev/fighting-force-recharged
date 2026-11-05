#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <vector>
#include "../src/camera_rail.h"
static uintptr_t g_base;
template<class T> static T& CameraField(DWORD rva) {return *reinterpret_cast<T*>(g_base+rva);}
static bool FirstPersonActive(){return true;}
static bool CameraReadable(const void* p,SIZE_T n){return p && n<1024*1024;}
#include "../src/first_person_props.h"
static int failures,submissions,native_calls;
#define CHECK(x) do {if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);++failures;}}while(0)
static float matrix[12];
static const float* __cdecl Matrix(){return matrix;}
static bool pixels[120][160];
static const FirstPersonPropModel* original;
static bool far_rejected;
static void Cover(const FirstPersonPropScreenVertex& a,const FirstPersonPropScreenVertex& b,const FirstPersonPropScreenVertex& c) {
    for(int y=0;y<120;++y)for(int x=0;x<160;++x) {
        const double sx=x*2+1,sy=y*2+1;
        const double u=(b.x-a.x)*(sy-a.y)-(b.y-a.y)*(sx-a.x);
        const double v=(c.x-b.x)*(sy-b.y)-(c.y-b.y)*(sx-b.x);
        const double w=(a.x-c.x)*(sy-c.y)-(a.y-c.y)*(sx-c.x);
        if((u>=0 && v>=0 && w>=0) || (u<=0 && v<=0 && w<=0))pixels[y][x]=true;
    }
}
static int __cdecl Native(const FirstPersonPropModel* model,int flags,int depth) {
    ++native_calls;CHECK(flags==0 && depth==1024 && model->vertices==original->vertices);
    for(unsigned i=0;i<model->face_count;++i) {
        const auto f=model->faces+i*6;if(f[5]<0)continue;
        FirstPersonPropScreenVertex v[4]={};bool visible=true;
        for(int j=0;j<4;++j) {
            const auto p=model->vertices+f[j]*4;
            const double z=matrix[8]*p[0]+matrix[9]*p[1]+matrix[10]*p[2]+matrix[11];
            visible&=z>0 && z<CameraField<float>(0x6BC64);
            v[j].x=float(160+100*(matrix[0]*p[0]+matrix[1]*p[1]+matrix[2]*p[2]+matrix[3])/z);
            v[j].y=float(120+100*(matrix[4]*p[0]+matrix[5]*p[1]+matrix[6]*p[2]+matrix[7])/z);
        }
        if(!visible)continue;
        const double facing=(v[1].x-v[0].x)*(v[2].y-v[0].y)-(v[1].y-v[0].y)*(v[2].x-v[0].x);
        if(!(f[4]&1) && facing<0)continue;
        Cover(v[0],v[1],v[2]);Cover(v[0],v[2],v[3]);
    }
    return 17;
}
static void __cdecl Submit(FirstPersonPropPolygon* polygon,const FirstPersonPropMaterial*,int flags) {
    ++submissions;CHECK(!far_rejected && flags==0 && polygon->count>=3 && polygon->count<=8);
    for(unsigned i=0;i<polygon->count;++i) {
        const auto& v=polygon->vertices[i];
        const double z=1.0/v.rhw-1024;
        CHECK(z>=0.999 && fabs(v.z-(z+1024)*0.00001)<0.000001);
        CHECK(v.x>=0 && v.x<=320 && v.y>=0 && v.y<=240);
        CHECK(v.color==0xFFFFFFFF && v.u>=-0.00001 && v.u<=1.00001 && v.v>=-0.00001 && v.v<=1.00001);
        // Recover local height independently; cutting a wall must crop its
        // texture, not stretch the original full height into the visible part.
        const double vx=(v.x-160)*z/100,vy=(v.y-120)*z/100;
        const double local_y=matrix[1]*(vx-matrix[3])+matrix[5]*(vy-matrix[7])+matrix[9]*(z-matrix[11]);
        CHECK(fabs(v.v+local_y/198)<0.00002);
    }
    for(unsigned i=2;i<polygon->count;++i)Cover(polygon->vertices[0],polygon->vertices[i-1],polygon->vertices[i]);
}
static void Jump(DWORD rva,uintptr_t target) {
    auto p=reinterpret_cast<BYTE*>(g_base+rva);p[0]=0xE9;
    const LONG offset=LONG(target-uintptr_t(p+5));memcpy(p+1,&offset,4);
}
static void View(ff_camera::Point eye,ff_camera::Point target) {
    using namespace ff_camera;
    auto forward=Sub(target,eye);forward=Scale(forward,1/sqrt(Dot(forward,forward)));
    auto right=Cross(forward,{0,1,0});right=Scale(right,1/sqrt(Dot(right,right)));
    const auto down=Cross(forward,right);const Point axes[]={right,down,forward};
    for(int i=0;i<3;++i) {
        matrix[i*4]=float(axes[i].x);matrix[i*4+1]=float(axes[i].y);matrix[i*4+2]=float(axes[i].z);
        matrix[i*4+3]=float(-Dot(axes[i],eye));
    }
}
int main() {
    using namespace ff_camera;
    g_base=uintptr_t(VirtualAlloc(nullptr,0x900000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));
    if(!g_base)return 2;
    Jump(0x1980,uintptr_t(Matrix));Jump(0x265B0,uintptr_t(Submit));
    CameraField<float>(0xAD214)=CameraField<float>(0xAD234)=100;
    CameraField<float>(0x20C50C)=160;CameraField<float>(0x20C508)=120;
    CameraField<float>(0x9D170)=320;CameraField<float>(0x9D174)=240;
    CameraField<float>(0x5E8A4)=0.00001f;CameraField<float>(0x6BC64)=65535;
    CameraField<int>(0x9D188)=1;CameraField<int>(0x8689EC)=255;
    CameraField<FirstPersonPropMaterial>(0x451DA8)={0,1,0,1,0,0,7,{0,0}};
    // Two inward trench walls, independent of original asset data.
    const int16_t vertices[]={0,0,2048,0,0,0,0,0,0,-198,0,0,0,-198,2048,0,
        471,0,0,0,471,0,2048,0,471,-198,2048,0,471,-198,0,0};
    const int16_t faces[]={0,1,2,3,0,0,4,5,6,7,0,0};
    const FirstPersonPropModel model={vertices,8,faces,2};original=&model;
    int cases=0,expected_total=0;
    for(double ex:{-2000.,-800.,-50.,521.,1271.,2471.})for(double height:{40.,160.,600.,1200.})
    for(double ez:{-300.,1024.,2300.})for(double target_y:{-80.,-800.}) {
        const Point eye={ex,height,ez};View(eye,{235,target_y,1024});double plane[4];
        CHECK(SubwayRailOcclusionPlane(matrix,plane));
        memset(pixels,0,sizeof(pixels));CameraField<unsigned>(0x1EC4D0)=0;
        CHECK(CameraProjectNearModel(&model,0,1024,true,false,Native,false,plane)==17);
        CHECK(!memcmp(model.faces,faces,sizeof(faces)) && faces[5]==0 && faces[11]==0);
        int missing=0,overlap=0;
        // Independent ray/wall and ray/ground intersections establish which
        // samples the foreground platform hides, including steep views.
        for(int y=0;y<120;++y)for(int x=0;x<160;++x) {
            const double rx=(x*2+1-160)/100.,ry=(y*2+1-120)/100.;
            const Point ray={matrix[0]*rx+matrix[4]*ry+matrix[8],matrix[1]*rx+matrix[5]*ry+matrix[9],
                matrix[2]*rx+matrix[6]*ry+matrix[10]};
            const double wall=ex<0?471:0,t=(wall-ex)/ray.x;
            const Point hit=Add(eye,Scale(ray,t));
            const double ground=-height/ray.y,gx=ex+ray.x*ground;
            const bool hidden=ground>0 && ground<t && (gx<0 || gx>471);
            const bool inside=t>1 && hit.y<0 && hit.y>-198 && hit.z>0 && hit.z<2048;
            if(inside && !hidden){++expected_total;if(!pixels[y][x])++missing;}
            if(pixels[y][x] && hidden)++overlap;
        }
        if(missing || overlap)printf("Rail x=%.0f h=%.0f z=%.0f target=%.0f: missing=%d overlap=%d\n",ex,height,ez,target_y,missing,overlap);
        CHECK(!missing && !overlap);++cases;
    }
    CHECK(submissions>0 && expected_total>0);
    const int emitted=submissions;double plane[4];
    CameraField<float>(0x6BC64)=1;far_rejected=true;
    CHECK(SubwayRailOcclusionPlane(matrix,plane));
    CameraProjectNearModel(&model,0,1024,true,false,Native,false,plane);CHECK(submissions==emitted);
    CameraProjectNearModel(&model,0,1024,false,false,Native,false,plane);CHECK(submissions==emitted);
    for(Point eye: {Point{235,100,1024},Point{-100,-1,1024},Point{600,0,1024}}) {
        View(eye,{235,-100,0});CHECK(!SubwayRailOcclusionPlane(matrix,plane));
    }
    memset(matrix,0,sizeof(matrix));CHECK(!SubwayRailOcclusionPlane(matrix,plane));
    VirtualFree(reinterpret_cast<void*>(g_base),0,MEM_RELEASE);
    printf("Subway rail occlusion: %s (%d views, %d visible samples)\n",failures?"FAIL":"PASS",cases,expected_total);
    return failures?1:0;
}
