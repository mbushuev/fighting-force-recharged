#pragma once
#include "camera_collision.h"
#include <vector>
#include <atomic>
#include "camera_diagnostics.h"
#include "render_phase.h"
#include "room_completion.h"

struct CameraRetailModel {
    const int16_t* vertices; unsigned vertex_count;
    const int16_t* faces; unsigned face_count;
};
static_assert(sizeof(CameraRetailModel)==16,"Retail model header");
struct CameraObjectGeometry {
    BYTE identity[22]={};
    bool valid=false;
    std::vector<ff_camera::Triangle> triangles;
};
struct CameraGeometryFrame {
    bool active=false, basis=false, pending=false, environment=false;
    unsigned epoch=0, models=0;
    double inverse[9]={};
    ff_camera::Point origin;
    std::vector<ff_camera::Point> vertices;
    std::vector<ff_camera::Triangle> triangles;
    CameraObjectGeometry objects[64];
    CameraObjectGeometry environment_objects[128];
    std::vector<ff_camera::Triangle> object_triangles;
    std::vector<int16_t> visible_faces;
    int object=-1,environment_object=-1,stage=-1;
    unsigned object_models=0;
};
static thread_local CameraGeometryFrame g_camera_geometry_frame;
static SRWLOCK g_camera_geometry_lock=SRWLOCK_INIT;
static std::vector<ff_camera::Triangle> g_camera_triangles;
static std::atomic<unsigned> g_camera_geometry_epoch{0};
static int g_camera_geometry_stage=-1;
static ULONGLONG g_camera_geometry_stamp;
static void* g_camera_model_trampoline;
static int(__cdecl* g_camera_scene_projector)(const CameraRetailModel*,int,int);
static bool g_camera_collision_enabled;
static double g_camera_boom;
static ff_camera::BoomResponse g_camera_boom_response;
static double CameraObstacleDistance(ff_camera::Point focus,ff_camera::Point desired);
static int __cdecl GameCameraModel(const CameraRetailModel* model,int flags,int depth);
static bool CameraSceneViewActive() {
    return g_free_orbit && g_camera_active && CameraMayOwnPose();
}
static int __cdecl CameraSceneObjectOrigin() {
    // A visible part of a nearby mesh may remain in view after its origin
    // leaves it. Custom cameras defer this point-only gate to face clipping.
    return CameraSceneViewActive()?1:reinterpret_cast<int(__cdecl*)()>(g_base+0x20E0)();
}
static bool CameraOwnsHallGeometry() {
    return g_free_orbit && g_camera_active && CameraMayOwnPose() && CameraField<int>(0x85A6B8)==1;
}
static bool CameraRetainsHallWall(int type) {
    // Stage 1's 12GATE1 is the fixed hall wall surrounding a doorway, distinct
    // from its destructible partitions (type 124). The handler only gates yaw.
    return type==123 && CameraOwnsHallGeometry() &&
        CameraField<uintptr_t>(0x872780+123*4)==g_base+0xFC20;
}
static void __cdecl GameHallPartitionCleanup(int32_t* record) {
    // Only the section-transition bulk cleanup reaches this hook. Native hits
    // and breakage use separate paths and can still remove either partition.
    if(!CameraOwnsHallGeometry())record[0]=55;
}
__declspec(naked) static void GameHallPartitionCleanupEntry() {
    __asm {
        pushfd
        pushad
        push eax
        call GameHallPartitionCleanup
        add esp,4
        popad
        popfd
        ret
    }
}
static bool CameraMayCaptureGeometry() {
    if(g_render_intermediate)return false;
    const int stage=CameraField<int>(0x85A6B8);
    return g_free_orbit && stage>=0 && stage<25 && (CameraField<int>(0x8745A0)==1 || g_camera_cooperative) &&
        CameraField<int16_t>(0x86A542)>0 && (CameraMayOwnPose() || g_camera_trace);
}

static void ClearCameraGeometry() {
    ++g_camera_geometry_epoch;
    AcquireSRWLockExclusive(&g_camera_geometry_lock);
    g_camera_triangles.clear();g_camera_geometry_stage=-1;g_camera_geometry_stamp=0;
    ReleaseSRWLockExclusive(&g_camera_geometry_lock);
    g_camera_boom=0;g_camera_boom_response={};
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
static void CaptureCameraModel(const CameraRetailModel* model,unsigned solid_from=UINT_MAX) {
    auto& frame=g_camera_geometry_frame;
    if(!frame.active || !frame.basis || !CameraReadable(model,sizeof(*model)) || !model->vertex_count ||
       model->vertex_count>16384 || model->face_count>32768 || frame.triangles.size()>60000 ||
       !CameraReadable(model->vertices,model->vertex_count*8) || !CameraReadable(model->faces,model->face_count*12))return;
    CameraTiming timing(CameraCaptureTime);
    auto matrix=reinterpret_cast<const float*(__cdecl*)()>(g_base+0x1980)();
    auto& triangles=frame.object>=0 || frame.environment_object>=0?frame.object_triangles:frame.triangles;
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
        // Reconstructed room walls are solid architecture, including their
        // glass bays. Other alpha cutouts keep the normal capture exclusion.
        if(face<solid_from && (CameraField<BYTE>(0x451DBC+static_cast<unsigned>(p[5])*32)&3))continue;
        bool valid=true;for(int n=0;n<4;++n)if(p[n]<0 || unsigned(p[n])>=model->vertex_count)valid=false;
        if(!valid)continue;
        const ff_camera::Point a=frame.vertices[p[0]],b=frame.vertices[p[1]],c=frame.vertices[p[2]],d=frame.vertices[p[3]];
        auto add=[&triangles](ff_camera::Point x,ff_camera::Point y,ff_camera::Point z) {
            auto normal=ff_camera::Cross(ff_camera::Sub(y,x),ff_camera::Sub(z,x));
            if(triangles.size()<60000 && ff_camera::Dot(normal,normal)>1e-8)triangles.push_back({x,y,z});
        };
        add(a,b,c);add(a,c,d);
    }
    ++frame.models;if(frame.object>=0 || frame.environment_object>=0)++frame.object_models;
}
static void __cdecl GameCameraWorldObject(void* object) {
    auto& frame=g_camera_geometry_frame;
    const uintptr_t offset=reinterpret_cast<uintptr_t>(object)-(g_base+0x8677E0);
    const bool capture=frame.active && offset<128*36 && offset%36==0;
    if(capture) {
        frame.environment_object=static_cast<int>(offset/36);frame.object_models=0;frame.object_triangles.clear();
        auto& cached=frame.environment_objects[frame.environment_object];
        if(!cached.valid || memcmp(cached.identity,object,22)) {
            cached.valid=false;cached.triangles.clear();memcpy(cached.identity,object,22);
        }
    }
    const int type=*static_cast<const int*>(object);
    if(CameraRetainsHallWall(type))
        GameCameraModel(CameraField<const CameraRetailModel*>(0x85A6F8+123*4),0,0);
    else CameraField<void(__cdecl*)(void*)>(0x872780+type*4)(object);
    if(capture) {
        auto& cached=frame.environment_objects[frame.environment_object];
        if(frame.object_models) {cached.triangles.swap(frame.object_triangles);cached.valid=true;}
        frame.environment_object=-1;
    }
}
static void __cdecl GameCameraObject(void* object) {
    auto& frame=g_camera_geometry_frame;
    const uintptr_t offset=reinterpret_cast<uintptr_t>(object)-(g_base+0x870620);
    const bool capture=frame.active && offset<64*36 && offset%36==0;
    if(capture) {
        frame.object=static_cast<int>(offset/36);frame.object_models=0;frame.object_triangles.clear();
        auto& cached=frame.objects[frame.object];
        if(!cached.valid || memcmp(cached.identity,object,22)) {
            cached.valid=false;cached.triangles.clear();memcpy(cached.identity,object,22);
        }
    }
    const int type=*static_cast<const int*>(object);
    auto handler=CameraField<void(__cdecl*)(void*)>(0x878900+type*4);
    if(!handler)handler=reinterpret_cast<void(__cdecl*)(void*)>(g_base+0x49870);
    handler(object);
    if(capture) {
        auto& cached=frame.objects[frame.object];
        if(frame.object_models) {cached.triangles.swap(frame.object_triangles);cached.valid=true;}
        frame.object=-1;
    }
}
static void AppendCameraObjects() {
    auto& frame=g_camera_geometry_frame;
    bool held[64]={};
    for(unsigned actor=0;actor<8;++actor)for(unsigned hand=0;hand<2;++hand) {
        const int slot=CameraField<int8_t>(0x86A57D+actor*0xF4+hand);
        if(slot>=0 && slot<64)held[slot]=true;
    }
    for(unsigned i=0;i<64;++i) {
        auto& cached=frame.objects[i];const auto object=reinterpret_cast<const BYTE*>(g_base+0x870620+i*36);
        if(!cached.valid)continue;
        if(held[i] || *reinterpret_cast<const int*>(object)==-1 || memcmp(cached.identity,object,22)) {
            cached.valid=false;cached.triangles.clear();continue;
        }
        const size_t count=(std::min)(cached.triangles.size(),size_t(60000)-frame.triangles.size());
        frame.triangles.insert(frame.triangles.end(),cached.triangles.begin(),cached.triangles.begin()+count);
    }
}
static void AppendCameraEnvironment() {
    auto& frame=g_camera_geometry_frame;
    const int count=(std::max)(0,(std::min)(128,CameraField<int>(0x86F9C0)));
    bool ended=false;
    for(int i=0;i<128;++i) {
        auto& cached=frame.environment_objects[i];
        const auto object=reinterpret_cast<const BYTE*>(g_base+0x8677E0+i*36);
        if(i>=count || *reinterpret_cast<const int*>(object)<0x37)ended=true;
        if(!cached.valid)continue;
        if(ended || memcmp(cached.identity,object,22)) {cached.valid=false;cached.triangles.clear();continue;}
        const size_t available=60000-frame.triangles.size();
        const size_t triangles=(std::min)(available,cached.triangles.size());
        frame.triangles.insert(frame.triangles.end(),cached.triangles.begin(),cached.triangles.begin()+triangles);
    }
}
struct CameraCompletedRoom {
    const CameraRetailModel* source=nullptr;
    unsigned epoch=0,vertex_count=0,face_count=0;
    ff_rooms::Mesh mesh;CameraRetailModel model={};
};
static thread_local CameraCompletedRoom g_camera_completed_rooms[3];
static const CameraRetailModel* CompleteCameraRoom(const CameraRetailModel* source) {
    if(!g_controls.complete_rooms || !g_camera_geometry_frame.environment || !CameraSceneViewActive())return source;
    const int stage=CameraField<int>(0x85A6B8);int index=-1;ff_rooms::Room room=ff_rooms::Room::None;
    if(stage==2 && source==CameraField<const CameraRetailModel*>(0x85A6F8+132*4)){index=0;room=ff_rooms::Room::Elevator;}
    if(stage==4)for(int i=0;i<2;++i)if(source==CameraField<const CameraRetailModel*>(0x85A6F8+(141+i)*4)){index=i+1;room=ff_rooms::Room::Office;}
    if(index<0 || !CameraReadable(source,sizeof(*source)) || source->vertex_count>4096 || source->face_count>8192 ||
        !CameraReadable(source->vertices,source->vertex_count*8) || !CameraReadable(source->faces,source->face_count*12))return source;
    auto& cache=g_camera_completed_rooms[index];
    if(cache.source==source && cache.epoch==g_camera_geometry_epoch.load() && cache.vertex_count==source->vertex_count &&
       cache.face_count==source->face_count && cache.mesh.added &&
       !memcmp(cache.mesh.vertices.data(),source->vertices,source->vertex_count*8) &&
       !memcmp(cache.mesh.faces.data(),source->faces,source->face_count*12))return &cache.model;
    cache.source=nullptr;
    try {
        if(!cache.mesh.Build(room,reinterpret_cast<const ff_rooms::Vertex*>(source->vertices),source->vertex_count,
            reinterpret_cast<const ff_rooms::Face*>(source->faces),source->face_count))return source;
    } catch(const std::bad_alloc&){return source;}
    cache.source=source;cache.epoch=g_camera_geometry_epoch.load();cache.vertex_count=source->vertex_count;cache.face_count=source->face_count;
    cache.model={reinterpret_cast<const int16_t*>(cache.mesh.vertices.data()),unsigned(cache.mesh.vertices.size()),
        reinterpret_cast<const int16_t*>(cache.mesh.faces.data()),unsigned(cache.mesh.faces.size())};
    static bool reported[3]={};
    if(!reported[index]){reported[index]=true;char message[128];
        sprintf_s(message,"Room completion: stage %d, model %d, %u added wall faces.\r\n",stage,index?140+index:132,cache.mesh.added);Log(message);}
    return &cache.model;
}
static int __cdecl GameCameraModel(const CameraRetailModel* model,int flags,int depth) {
    const auto source=model;
    model=CompleteCameraRoom(model);
    try {CaptureCameraModel(model,model==source?UINT_MAX:source->face_count);} catch(const std::bad_alloc&) {g_camera_geometry_frame.active=false;}
    // Level walls were authored for a directed camera. Free orbit can expose
    // their reverse side, which the native CPU projector otherwise discards.
    // Its existing two-sided face bit handles both clipped and unclipped paths.
    // Copy only environment face records; retain all native material rules.
    auto& frame=g_camera_geometry_frame;
    CameraRetailModel visible;
    if(frame.environment && g_camera_active && CameraMayOwnPose() &&
       CameraReadable(model,sizeof(*model)) && model->face_count && model->face_count<=32768 &&
       CameraReadable(model->faces,model->face_count*12)) {
        CameraTiming timing(CameraFacesTime);
        try {
            frame.visible_faces.assign(model->faces,model->faces+model->face_count*6);
            for(unsigned i=0;i<model->face_count;++i) {
                auto p=frame.visible_faces.data()+i*6;
                if(p[3]>=0 && p[5]>=0 && !(CameraField<BYTE>(0x451DBC+unsigned(p[5])*32)&3))p[4]|=1;
            }
            visible=*model;visible.faces=frame.visible_faces.data();model=&visible;
        } catch(const std::bad_alloc&) {} // Fall back to the original model.
    }
    if(g_camera_scene_projector)return g_camera_scene_projector(model,flags,depth);
    return reinterpret_cast<int(__cdecl*)(const CameraRetailModel*,int,int)>(g_camera_model_trampoline)(model,flags,depth);
}
static void CompleteCameraGeometry() {
    auto& frame=g_camera_geometry_frame;
    if(frame.active && frame.basis && frame.epoch==g_camera_geometry_epoch.load()) {
        AppendCameraEnvironment();AppendCameraObjects();
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
static void DrawCameraHallBoundary() {
    if(!CameraRetainsHallWall(123) || CameraField<int>(0x85A6C0)<2)return;
    const int count=CameraField<int>(0x86F9C0);
    if(count<0 || count>=128)return;
    // The 1 -> 2 section transition drops the final appended wall by reducing
    // the environment count. Native publication still copies all 128 records.
    // Draw only that excluded wall from the render copy, without restoring the
    // simulation record/count, broken partitions or native collision zones.
    const auto record=reinterpret_cast<const int32_t*>(g_base+0x8677E0+count*36);
    if(record[0]!=123)return;
    const auto model=CameraField<const CameraRetailModel*>(0x85A6F8+123*4);
    if(!model)return;
    reinterpret_cast<void(__cdecl*)()>(g_base+0x1B20)();
    reinterpret_cast<void(__cdecl*)(int32_t,int32_t,int32_t)>(g_base+0x2010)(
        static_cast<int32_t>(uint32_t(record[1])-uint32_t(CameraField<int32_t>(0x8677B0))),
        static_cast<int32_t>(uint32_t(record[2])-uint32_t(CameraField<int32_t>(0x8677B4))),
        static_cast<int32_t>(uint32_t(record[3])-uint32_t(CameraField<int32_t>(0x8677B8))));
    reinterpret_cast<void(__cdecl*)(int)>(g_base+0x1F00)(0x8000);
    // Capture directly into this frame, outside the count-bounded record cache.
    // The wall remains an orbit-camera obstruction after the native count drops.
    GameCameraModel(model,0,0);
    reinterpret_cast<void(__cdecl*)()>(g_base+0x1BB0)();
}
static void __cdecl GameCameraEnvironment() {
    CameraTiming timing(CameraEnvironmentTime);
    auto& frame=g_camera_geometry_frame;
    const int stage=CameraField<int>(0x85A6B8);
    if(frame.epoch!=g_camera_geometry_epoch.load() || frame.stage!=stage) {
        for(auto& object:frame.objects) {object.valid=false;object.triangles.clear();}
        for(auto& object:frame.environment_objects) {object.valid=false;object.triangles.clear();}
        frame.stage=stage;
    }
    frame.active=CameraMayCaptureGeometry();frame.basis=false;frame.pending=false;
    frame.epoch=g_camera_geometry_epoch.load();frame.models=0;frame.triangles.clear();frame.object=frame.environment_object=-1;
    frame.environment=true;
    reinterpret_cast<void(__cdecl*)()>(g_base+0x43990)();
    DrawCameraHallBoundary();
    frame.environment=false;
    // Keep the previous complete snapshot visible while actors are rendered.
    // Publishing here would briefly remove vehicles from a concurrent query.
    frame.pending=frame.active && frame.basis && frame.epoch==g_camera_geometry_epoch.load();
    frame.active=false;
}
static void __cdecl GameCameraSolidObjects() {
    CameraTiming timing(CameraObjectsTime);
    auto& frame=g_camera_geometry_frame;
    frame.active=CameraMayCaptureGeometry() && frame.pending && frame.basis && frame.epoch==g_camera_geometry_epoch.load();
    // Destructible vehicles and solid pickups use this pass. Characters and
    // transient debris are outside the capture scope, avoiding self-collision.
    reinterpret_cast<void(__cdecl*)()>(g_base+0x3C100)();
    try {CompleteCameraGeometry();} catch(const std::bad_alloc&) {frame.active=false;frame.pending=false;}
}
static double CameraObstacleDistance(ff_camera::Point focus,ff_camera::Point desired) {
    CameraTiming timing(CameraQueryTime);
    const auto delta=ff_camera::Sub(desired,focus);const double length=sqrt(ff_camera::Dot(delta,delta));
    if(!g_camera_collision_enabled || length<1e-6)return length;
    double fraction=1;size_t count=0,hit=SIZE_MAX;
    AcquireSRWLockShared(&g_camera_geometry_lock);
    if(g_camera_geometry_stage==CameraField<int>(0x85A6B8) && GetTickCount64()-g_camera_geometry_stamp<1000) {
        count=g_camera_triangles.size();
        for(size_t i=0;i<count;++i) {
            const double next=ff_camera::SweepCameraProbe(focus,desired,48,g_camera_triangles[i],fraction);
            if(next<fraction) {fraction=next;hit=i;}
        }
    }
    ReleaseSRWLockShared(&g_camera_geometry_lock);
    const double allowed=fraction<1?fmax(1,length*fraction-8):length;
    if(g_camera_trace) {
        static double previous=0;static ULONGLONG reported=0;
        const ULONGLONG now=GetTickCount64();
        if(fabs(allowed-previous)>12 && now-reported>=250) {
            reported=now;char message[256];
            sprintf_s(message,"Camera contact: %.2f -> %.2f, boom %.2f, %zu triangles, hit %zu, focus %.2f,%.2f,%.2f; ray %.2f,%.2f,%.2f.\r\n",
                previous,allowed,g_camera_boom,count,hit,focus.x,focus.y,focus.z,delta.x,delta.y,delta.z);Log(message);
        }
        previous=allowed;
    }
    return allowed;
}
static bool InstallCameraGeometryHooks() {
    if(!g_free_orbit)return false;
    const DWORD world_calls[]={0x2EBC9,0x2F0AA,0x40C47,0x416E7,0x41B44,0x4235A,0x4AB2D};
    const DWORD object_calls[]={0x2EBCE,0x2F0AF,0x40C51,0x416F1,0x41B4E,0x4235F,0x4AB32};
    const DWORD origin_calls[]={0xFBF2,0x45D70,0x48ACB,0x48EB0,0x49870,0x498A0};
    const BYTE prologue[]={0x83,0xEC,0x3C,0x53,0x55,0x56};
    const BYTE object_dispatch[]={0x8B,0x04,0x85,0x00,0x89,0xC7,0x00,0x56,0x85,0xC0,0x74,0x04,
        0xFF,0xD0,0xEB,0x05,0xE8,0x07,0xD7,0x00,0x00,0x83,0xC4,0x04};
    const BYTE world_dispatch[]={0xFF,0x14,0xBD,0x80,0x27,0xC7,0x00};
    const BYTE hall_wall_selector[]={0x66,0xA1,0x8C,0x88,0xC7,0x00,0x66,0x3D,0x00,0x40,
        0x7F,0x06,0x66,0x3D,0x00,0xC0,0x7D,0x1A};
    if(memcmp(reinterpret_cast<void*>(g_base+0xFC20),hall_wall_selector,sizeof(hall_wall_selector)))return false;
    const BYTE partition_cleanup[]={0x83,0x38,0x7C,0x75,0x06,0xC7,0x00,0x37,0x00,0x00,0x00,
        0x83,0xC0,0x24,0x49,0x75,0xEF};
    if(memcmp(reinterpret_cast<void*>(g_base+0x3B224),partition_cleanup,sizeof(partition_cleanup)))return false;
    auto model=reinterpret_cast<BYTE*>(g_base+0x26DE0);
    if(memcmp(model,prologue,sizeof(prologue)))return false;
    auto object=reinterpret_cast<BYTE*>(g_base+0x3C154);
    if(memcmp(object,object_dispatch,sizeof(object_dispatch)))return false;
    auto world_object=reinterpret_cast<BYTE*>(g_base+0x439F7);
    if(memcmp(world_object,world_dispatch,sizeof(world_dispatch)))return false;
    auto matches=[](DWORD rva,DWORD target) {
        auto code=reinterpret_cast<BYTE*>(g_base+rva);LONG relative;memcpy(&relative,code+1,4);
        return code[0]==0xE8 && reinterpret_cast<uintptr_t>(code+5)+relative==g_base+target;
    };
    if(!matches(0x43993,0x2330))return false;
    for(DWORD site:world_calls)if(!matches(site,0x43990))return false;
    for(DWORD site:object_calls)if(!matches(site,0x3C100))return false;
    for(DWORD site:origin_calls)if(!matches(site,0x20E0))return false;
    auto trampoline=static_cast<BYTE*>(VirtualAlloc(nullptr,32,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    if(!trampoline)return false;
    memcpy(trampoline,prologue,6);trampoline[6]=0xE9;
    LONG back=static_cast<LONG>((model+6)-(trampoline+11));memcpy(trampoline+7,&back,4);
    DWORD old;if(!VirtualProtect(trampoline,32,PAGE_EXECUTE_READ,&old)){VirtualFree(trampoline,0,MEM_RELEASE);return false;}
    FlushInstructionCache(GetCurrentProcess(),trampoline,32);
    auto patch_begin=reinterpret_cast<BYTE*>(g_base+origin_calls[0]);
    const SIZE_T length=object_calls[6]+5-origin_calls[0];
    if(!VirtualProtect(patch_begin,length,PAGE_EXECUTE_READWRITE,&old)){VirtualFree(trampoline,0,MEM_RELEASE);return false;}
    g_camera_model_trampoline=trampoline;
    auto branch=[](BYTE* code,void* target) {
        code[0]=0xE8;LONG relative=static_cast<LONG>(static_cast<BYTE*>(target)-(code+5));memcpy(code+1,&relative,4);
    };
    branch(model,reinterpret_cast<void*>(GameCameraModel));model[0]=0xE9;model[5]=0x90;
    auto cleanup=reinterpret_cast<BYTE*>(g_base+0x3B229);
    branch(cleanup,reinterpret_cast<void*>(GameHallPartitionCleanupEntry));cleanup[5]=0x90;
    branch(reinterpret_cast<BYTE*>(g_base+0x43993),reinterpret_cast<void*>(GameCameraWorldBasis));
    for(DWORD site:world_calls)branch(reinterpret_cast<BYTE*>(g_base+site),reinterpret_cast<void*>(GameCameraEnvironment));
    for(DWORD site:object_calls)branch(reinterpret_cast<BYTE*>(g_base+site),reinterpret_cast<void*>(GameCameraSolidObjects));
    for(DWORD site:origin_calls)branch(reinterpret_cast<BYTE*>(g_base+site),reinterpret_cast<void*>(CameraSceneObjectOrigin));
    branch(world_object,reinterpret_cast<void*>(GameCameraWorldObject));world_object[5]=world_object[6]=0x90;
    memset(object,0x90,sizeof(object_dispatch));object[0]=0x56;
    branch(object+1,reinterpret_cast<void*>(GameCameraObject));
    const BYTE release[]={0x83,0xC4,0x04};memcpy(object+6,release,3);
    object[9]=0xE9;LONG next=static_cast<LONG>((object+sizeof(object_dispatch))-(object+14));memcpy(object+10,&next,4);
    DWORD ignored;VirtualProtect(patch_begin,length,old,&ignored);FlushInstructionCache(GetCurrentProcess(),patch_begin,length);
    g_camera_collision_enabled=true;Log("Camera obstruction mesh capture enabled before native projection/culling.\r\n");return true;
}
