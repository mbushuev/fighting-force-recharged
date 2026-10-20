#pragma once
#include "camera_collision.h"
#include <vector>
#include <atomic>

struct CameraRetailModel {
    const int16_t* vertices; unsigned vertex_count;
    const int16_t* faces; unsigned face_count;
};
static_assert(sizeof(CameraRetailModel)==16,"Retail model header");
struct CameraGeometryFrame {
    bool active=false, basis=false, pending=false;
    unsigned epoch=0, models=0;
    double inverse[9]={};
    ff_camera::Point origin;
    std::vector<ff_camera::Point> vertices;
    std::vector<ff_camera::Triangle> triangles;
};
static thread_local CameraGeometryFrame g_camera_geometry_frame;
static SRWLOCK g_camera_geometry_lock=SRWLOCK_INIT;
static std::vector<ff_camera::Triangle> g_camera_triangles;
static std::atomic<unsigned> g_camera_geometry_epoch{0};
static int g_camera_geometry_stage=-1;
static ULONGLONG g_camera_geometry_stamp;
static void* g_camera_model_trampoline;
static bool g_camera_collision_enabled;
static double g_camera_boom;
static double CameraObstacleDistance(ff_camera::Point focus,ff_camera::Point desired);
static bool CameraMayCaptureGeometry() {
    const int stage=CameraField<int>(0x85A6B8);
    return g_free_orbit && stage>=0 && stage<25 && CameraField<int>(0x8745A0)==1 &&
        CameraField<int16_t>(0x86A542)>0 && (CameraMayOwnPose() || g_camera_trace);
}

static void ClearCameraGeometry() {
    ++g_camera_geometry_epoch;
    AcquireSRWLockExclusive(&g_camera_geometry_lock);
    g_camera_triangles.clear();g_camera_geometry_stage=-1;g_camera_geometry_stamp=0;
    ReleaseSRWLockExclusive(&g_camera_geometry_lock);
    g_camera_boom=0;
}
static bool CameraReadable(const void* pointer,SIZE_T bytes) {
    uintptr_t cursor=reinterpret_cast<uintptr_t>(pointer),end=cursor+bytes;
    if(!cursor || end<cursor)return false;
    while(cursor<end) {
        MEMORY_BASIC_INFORMATION page={};
        if(!VirtualQuery(reinterpret_cast<void*>(cursor),&page,sizeof(page)) || page.State!=MEM_COMMIT ||
           (page.Protect&(PAGE_NOACCESS|PAGE_GUARD)))return false;
        uintptr_t next=reinterpret_cast<uintptr_t>(page.BaseAddress)+page.RegionSize;
        if(next<=cursor)return false;cursor=next;
    }
    return true;
}
static void __cdecl GameCameraWorldBasis() {
    reinterpret_cast<void(__cdecl*)()>(g_base+0x2330)();
    auto& frame=g_camera_geometry_frame;
    if(!frame.active)return;
    auto matrix=reinterpret_cast<const float*(__cdecl*)()>(g_base+0x1980)();
    frame.basis=matrix && ff_camera::InverseBasis(matrix,frame.inverse);
    frame.origin={CameraField<int32_t>(0x8677B0)/65536.0,CameraField<int32_t>(0x8677B4)/65536.0,CameraField<int32_t>(0x8677B8)/65536.0};
}
static void CaptureCameraModel(const CameraRetailModel* model) {
    auto& frame=g_camera_geometry_frame;
    if(!frame.active || !frame.basis || !CameraReadable(model,sizeof(*model)) || !model->vertex_count ||
       model->vertex_count>16384 || model->face_count>32768 || frame.triangles.size()>60000 ||
       !CameraReadable(model->vertices,model->vertex_count*8) || !CameraReadable(model->faces,model->face_count*12))return;
    auto matrix=reinterpret_cast<const float*(__cdecl*)()>(g_base+0x1980)();
    frame.vertices.resize(model->vertex_count);
    for(unsigned i=0;i<model->vertex_count;++i) {
        const int16_t* p=model->vertices+i*4;
        frame.vertices[i]=ff_camera::WorldVertex(matrix,frame.inverse,frame.origin,{double(p[0]),double(p[1]),double(p[2])});
    }
    for(unsigned face=0;face<model->face_count;++face) {
        const int16_t* p=model->faces+face*6;
        if(p[5]<0 || p[3]<0)continue; // disabled faces and native line primitives
        // Alpha cutouts and translucent material rectangles are not solid
        // volumes. Their transparent pixels must not become invisible walls.
        if(CameraField<BYTE>(0x451DBC+static_cast<unsigned>(p[5])*32)&3)continue;
        bool valid=true;for(int n=0;n<4;++n)if(p[n]<0 || unsigned(p[n])>=model->vertex_count)valid=false;
        if(!valid)continue;
        const ff_camera::Point a=frame.vertices[p[0]],b=frame.vertices[p[1]],c=frame.vertices[p[2]],d=frame.vertices[p[3]];
        auto add=[&frame](ff_camera::Point x,ff_camera::Point y,ff_camera::Point z) {
            auto normal=ff_camera::Cross(ff_camera::Sub(y,x),ff_camera::Sub(z,x));
            if(frame.triangles.size()<60000 && ff_camera::Dot(normal,normal)>1e-8)frame.triangles.push_back({x,y,z});
        };
        add(a,b,c);add(a,c,d);
    }
    ++frame.models;
}
static int __cdecl GameCameraModel(const CameraRetailModel* model,int flags,int depth) {
    try {CaptureCameraModel(model);} catch(const std::bad_alloc&) {g_camera_geometry_frame.active=false;}
    return reinterpret_cast<int(__cdecl*)(const CameraRetailModel*,int,int)>(g_camera_model_trampoline)(model,flags,depth);
}
static void CompleteCameraGeometry() {
    auto& frame=g_camera_geometry_frame;
    if(frame.active && frame.basis && frame.epoch==g_camera_geometry_epoch.load()) {
        AcquireSRWLockExclusive(&g_camera_geometry_lock);
        if(frame.epoch!=g_camera_geometry_epoch.load()) {
            ReleaseSRWLockExclusive(&g_camera_geometry_lock);frame.active=false;frame.pending=false;return;
        }
        g_camera_triangles.swap(frame.triangles);
        g_camera_geometry_stage=CameraField<int>(0x85A6B8);g_camera_geometry_stamp=GetTickCount64();
        const size_t count=g_camera_triangles.size();
        ReleaseSRWLockExclusive(&g_camera_geometry_lock);
        static unsigned samples=0;
        if(g_camera_trace && (++samples%120)==1) {
            char message[160];sprintf_s(message,"Complete camera geometry: stage %d, %u models, %zu solid triangles.\r\n",CameraField<int>(0x85A6B8),frame.models,count);Log(message);
        }
    }
    frame.active=false;frame.pending=false;
}
static void __cdecl GameCameraEnvironment() {
    auto& frame=g_camera_geometry_frame;
    frame.active=CameraMayCaptureGeometry();frame.basis=false;frame.pending=false;
    frame.epoch=g_camera_geometry_epoch.load();frame.models=0;frame.triangles.clear();
    reinterpret_cast<void(__cdecl*)()>(g_base+0x43990)();
    // Keep the previous complete snapshot visible while actors are rendered.
    // Publishing here would briefly remove vehicles from a concurrent query.
    frame.pending=frame.active && frame.basis && frame.epoch==g_camera_geometry_epoch.load();
    frame.active=false;
}
static void __cdecl GameCameraSolidObjects() {
    auto& frame=g_camera_geometry_frame;
    frame.active=CameraMayCaptureGeometry() && frame.pending && frame.basis && frame.epoch==g_camera_geometry_epoch.load();
    // Destructible vehicles and solid pickups use this pass. Characters and
    // transient debris are outside the capture scope, avoiding self-collision.
    reinterpret_cast<void(__cdecl*)()>(g_base+0x3C100)();
    CompleteCameraGeometry();
}
static double CameraObstacleDistance(ff_camera::Point focus,ff_camera::Point desired) {
    const auto delta=ff_camera::Sub(desired,focus);const double length=sqrt(ff_camera::Dot(delta,delta));
    if(!g_camera_collision_enabled || length<1e-6)return length;
    double fraction=1;
    AcquireSRWLockShared(&g_camera_geometry_lock);
    if(g_camera_geometry_stage==CameraField<int>(0x85A6B8) && GetTickCount64()-g_camera_geometry_stamp<1000)
        for(const auto& triangle:g_camera_triangles)fraction=ff_camera::SweepCameraProbe(focus,desired,48,triangle,fraction);
    ReleaseSRWLockShared(&g_camera_geometry_lock);
    return fraction<1?fmax(1,length*fraction-8):length;
}
static bool InstallCameraGeometryHooks() {
    if(!g_free_orbit)return false;
    const DWORD world_calls[]={0x2EBC9,0x2F0AA,0x40C47,0x416E7,0x41B44,0x4235A,0x4AB2D};
    const DWORD object_calls[]={0x2EBCE,0x2F0AF,0x40C51,0x416F1,0x41B4E,0x4235F,0x4AB32};
    const BYTE prologue[]={0x83,0xEC,0x3C,0x53,0x55,0x56};
    auto model=reinterpret_cast<BYTE*>(g_base+0x26DE0);
    if(memcmp(model,prologue,sizeof(prologue)))return false;
    auto matches=[](DWORD rva,DWORD target) {
        auto code=reinterpret_cast<BYTE*>(g_base+rva);LONG relative;memcpy(&relative,code+1,4);
        return code[0]==0xE8 && reinterpret_cast<uintptr_t>(code+5)+relative==g_base+target;
    };
    if(!matches(0x43993,0x2330))return false;
    for(DWORD site:world_calls)if(!matches(site,0x43990))return false;
    for(DWORD site:object_calls)if(!matches(site,0x3C100))return false;
    auto trampoline=static_cast<BYTE*>(VirtualAlloc(nullptr,32,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    if(!trampoline)return false;
    memcpy(trampoline,prologue,6);trampoline[6]=0xE9;
    LONG back=static_cast<LONG>((model+6)-(trampoline+11));memcpy(trampoline+7,&back,4);
    DWORD old;if(!VirtualProtect(trampoline,32,PAGE_EXECUTE_READ,&old)){VirtualFree(trampoline,0,MEM_RELEASE);return false;}
    FlushInstructionCache(GetCurrentProcess(),trampoline,32);
    const SIZE_T length=object_calls[6]+5-0x26DE0;
    if(!VirtualProtect(model,length,PAGE_EXECUTE_READWRITE,&old)){VirtualFree(trampoline,0,MEM_RELEASE);return false;}
    g_camera_model_trampoline=trampoline;
    auto branch=[](BYTE* code,void* target) {
        code[0]=0xE8;LONG relative=static_cast<LONG>(static_cast<BYTE*>(target)-(code+5));memcpy(code+1,&relative,4);
    };
    branch(model,reinterpret_cast<void*>(GameCameraModel));model[0]=0xE9;model[5]=0x90;
    branch(reinterpret_cast<BYTE*>(g_base+0x43993),reinterpret_cast<void*>(GameCameraWorldBasis));
    for(DWORD site:world_calls)branch(reinterpret_cast<BYTE*>(g_base+site),reinterpret_cast<void*>(GameCameraEnvironment));
    for(DWORD site:object_calls)branch(reinterpret_cast<BYTE*>(g_base+site),reinterpret_cast<void*>(GameCameraSolidObjects));
    DWORD ignored;VirtualProtect(model,length,old,&ignored);FlushInstructionCache(GetCurrentProcess(),model,length);
    g_camera_collision_enabled=true;Log("Camera obstruction mesh capture enabled before native projection/culling.\r\n");return true;
}
