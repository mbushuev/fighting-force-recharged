#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include "../src/orbit_camera.h"
static uintptr_t g_base;
static bool active=true;
template<class T> static T& CameraField(DWORD rva) {return *reinterpret_cast<T*>(g_base+rva);}
static bool FirstPersonActive() {return active;}
static bool CameraReadable(const void* p,SIZE_T n) {return p && n<1024*1024;}
#include "../src/first_person_props.h"
static int failures,native_calls,submissions;
#define CHECK(x) do {if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);++failures;}}while(0)
static float matrix[]={1,0,0,0,0,1,0,0,0,0,1,0};
static const float* __cdecl Matrix() {return matrix;}
static const FirstPersonPropModel* original;
static int native_flags,native_depth;
static int __cdecl Native(const FirstPersonPropModel* model,int flags,int depth) {
    ++native_calls;native_flags=flags;native_depth=depth;
    if(active && !depth) {
        CHECK(model!=original && model->vertices==original->vertices);
        CHECK(model->faces[5]==-1 && model->faces[4]==1);
        CHECK(model->faces[11]==0 && model->faces[10]==1);
        CHECK(model->faces[17]==-1);
    } else CHECK(model==original);
    CameraField<int>(0x8689EC)=100;return 42;
}
static const FirstPersonPropPolygon* retained;
static void __cdecl Submit(FirstPersonPropPolygon* polygon,const FirstPersonPropMaterial* material,int flags) {
    ++submissions;retained=polygon;CHECK(material==reinterpret_cast<void*>(g_base+0x451DA8));
    CHECK(flags==0 && polygon->count>=3 && polygon->count<=8 && polygon->kind==2);
    CHECK(polygon->texture==7 && polygon->blend==3);
    CHECK(uintptr_t(polygon)>=g_base+0xC84C8 && uintptr_t(polygon)<g_base+0x1EC4C8);
    for(unsigned i=0;i<polygon->count;++i) {
        const auto& v=polygon->vertices[i];
        CHECK(isfinite(v.x) && isfinite(v.y) && v.x>=0 && v.x<=320 && v.y>=0 && v.y<=240);
        CHECK(v.z>=0.000099f && v.rhw>0 && v.rhw<=1.00001f);
        CHECK(v.u>=0.09999f && v.u<=0.80001f && v.v>=0.19999f && v.v<=0.90001f);
        CHECK(v.color==0xFFFFFFFF && v.specular==0);
    }
}
static int world_calls,leaf_calls;
static bool expect_world_copy=true;
static int __cdecl WorldLeaf(const FirstPersonPropModel*,int,int) {++leaf_calls;return 64;}
static int __cdecl WorldNative(const FirstPersonPropModel* model,int flags,int depth) {
    ++world_calls;CHECK((model!=original)==expect_world_copy);
    if(expect_world_copy) {
        CHECK(model->faces[5]==-1 && model->faces[4]==original->faces[4]);
        CHECK(g_first_person_projecting);
        // The held-object entry can reach the global model hook. Its recursive
        // callback must forward without overwriting the outer fragment buffer.
        return FirstPersonProjectModel(model,flags,depth,false,WorldLeaf);
    }
    return 64;
}
static void Jump(DWORD rva,uintptr_t target) {
    auto p=reinterpret_cast<BYTE*>(g_base+rva);p[0]=0xE9;
    const LONG offset=LONG(target-uintptr_t(p+5));memcpy(p+1,&offset,4);
}
int main() {
    using namespace ff_camera;
    const PropProjection projection={100,100,160,120,320,240,0.0001};
    PropVertex output[8];
    const PropVertex crossing[]={{-2,-2,-5,0,0},{2,-2,5,1,0},{2,2,5,1,1}};
    const unsigned clipped=ClipPropTriangle(crossing,projection,output);
    CHECK(clipped>=3 && clipped<=8);
    bool edge=false;
    for(unsigned i=0;i<clipped;++i) {
        CHECK(output[i].z>=0.999999 && output[i].u>=0 && output[i].u<=1);
        // Intersection UVs are interpolated in view space, before division.
        if(fabs(output[i].z-1)<1e-8) {CHECK(fabs(output[i].u-0.6)<1e-8);edge=true;}
    }
    CHECK(edge);
    const PropVertex behind[]={{0,0,-1,0,0},{1,0,-2,1,0},{0,1,-3,0,1}};
    CHECK(!ClipPropTriangle(behind,projection,output));
    const PropVertex broad[]={{-500,-500,1,0,0},{500,-500,1,1,0},{0,500,1,0.5,1}};
    const unsigned broad_count=ClipPropTriangle(broad,projection,output);CHECK(broad_count>=3 && broad_count<=8);
    for(unsigned i=0;i<broad_count;++i) {
        CHECK(fabs(output[i].x*100)<=160.00001 && fabs(output[i].y*100)<=120.00001);
    }
    g_base=uintptr_t(VirtualAlloc(nullptr,0x900000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));
    if(!g_base)return 1;
    Jump(0x1980,uintptr_t(Matrix));Jump(0x26DE0,uintptr_t(Native));Jump(0x265B0,uintptr_t(Submit));
    CameraField<float>(0xAD214)=CameraField<float>(0xAD234)=100;
    CameraField<float>(0x20C50C)=160;CameraField<float>(0x20C508)=120;
    CameraField<float>(0x9D170)=320;CameraField<float>(0x9D174)=240;
    CameraField<float>(0x5E8A4)=0.0001f;CameraField<int>(0x9D188)=1;
    auto& material=CameraField<FirstPersonPropMaterial>(0x451DA8);
    material={0.1f,0.8f,0.2f,0.9f,0,3,7,{0,0}};
    const int16_t vertices[]={-2,-2,-5,0,2,-2,5,0,2,2,5,0,-2,2,-5,0,
        -2,-2,20,0,2,-2,20,0,2,2,20,0,-2,2,20,0,
        -2,-2,-20,0,2,-2,-20,0,2,2,-20,0,-2,2,-20,0};
    const int16_t faces[]={0,1,2,3,0,0,4,5,6,7,0,0,8,9,10,11,0,0};
    FirstPersonPropModel model={vertices,12,faces,3};original=&model;
    CameraField<int>(0x8689EC)=255;
    CHECK(FirstPersonHeldProp(&model,123,0)==42 && native_calls==1 && submissions>0 && native_flags==123);
    CHECK(!memcmp(faces,model.faces,sizeof(faces)) && faces[5]==0 && faces[4]==0);
    const auto saved=*retained;const auto pointer=retained;const unsigned count=CameraField<unsigned>(0x1EC4D0);
    CameraField<int>(0x8689EC)=255;FirstPersonHeldProp(&model,0,0);
    CHECK(CameraField<unsigned>(0x1EC4D0)>count && !memcmp(pointer,&saved,sizeof(saved)));
    active=false;const int emitted=submissions;
    FirstPersonHeldProp(&model,9,0);CHECK(submissions==emitted && native_flags==9);
    active=true;FirstPersonHeldProp(&model,8,4);CHECK(submissions==emitted && native_depth==4);
    CameraField<unsigned>(0x1EC4D0)=4096;FirstPersonHeldProp(&model,0,0);
    CHECK(submissions==emitted && CameraField<unsigned>(0x1EC4D0)==4096);
    CameraField<unsigned>(0x1EC4D0)=0;
    const int16_t floor_vertices[]={-64,8,-16,0,64,8,-16,0,64,8,128,0,-64,8,128,0};
    const int16_t front[]={0,3,2,1,0,0},back[]={0,1,2,3,0,0};
    FirstPersonPropModel floor={floor_vertices,4,front,1};original=&floor;
    CameraField<int>(0x8689EC)=255;
    CHECK(FirstPersonProjectModel(&floor,0,0,false,WorldNative)==64);
    CHECK(submissions>emitted && world_calls==1 && leaf_calls==1 && !g_first_person_projecting);
    CHECK(front[5]==0 && front[4]==0);
    const int visible_floor=submissions;
    floor.faces=back;CameraField<int>(0x8689EC)=255;
    FirstPersonProjectModel(&floor,0,0,false,WorldNative);
    CHECK(submissions==visible_floor && back[5]==0); // Original backface remains culled.
    active=false;expect_world_copy=false;
    FirstPersonProjectModel(&floor,0,0,false,WorldNative);
    CHECK(submissions==visible_floor && world_calls==3);
    active=true;FirstPersonProjectModel(&floor,0,1,false,WorldNative);
    CHECK(submissions==visible_floor && world_calls==4);
    // Shared scene clipping can be active with eye view disabled, while held
    // prop handling keeps its original eye-only scope and source geometry.
    active=false;expect_world_copy=true;floor.faces=front;CameraField<int>(0x8689EC)=255;
    CameraProjectNearModel(&floor,0,0,true,false,WorldNative);
    CHECK(submissions>visible_floor && world_calls==5 && front[5]==0 && front[4]==0);
    const int third_person=submissions;
    FirstPersonHeldProp(&floor,0,0);CHECK(submissions==third_person);
    VirtualFree(reinterpret_cast<void*>(g_base),0,MEM_RELEASE);
    printf("First-person held-prop near clipping and native render scope: %s\n",failures?"FAIL":"PASS");return failures?1:0;
}
