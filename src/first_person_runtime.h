#pragma once
#include "modern_input.h"
#include "camera_player.h"

// Retail skeletal headers include a vertex-count array after these fields.
struct FirstPersonModel {
    const int16_t* vertices;unsigned vertex_count;
    const int16_t* faces;unsigned face_count;
    const void* other;unsigned joints;
    const int32_t* commands;unsigned groups;
};
static bool g_first_person_available;
static void (*g_camera_world_drawn)();
static void (*g_camera_actor_bounds)(const FirstPersonModel*,void*);
static bool FirstPersonActive() {
    if(g_camera_cooperative && g_camera_owner<0)return false;
    return g_first_person_available && CameraFirstPersonView().selected && g_free_orbit &&
        g_camera_active && CameraMayOwnPose();
}
struct FirstPersonMesh {
    const FirstPersonModel* model=nullptr;
    int stage=-1;
    ff_camera::HeadSkeleton skeleton;
    ff_camera::Point center;
    std::vector<uint32_t> header;
    std::vector<int16_t> faces;
    bool Read(const FirstPersonModel* source) {
        const int current=CameraField<int>(0x85A6B8);
        if(model==source && stage==current)return skeleton.eye_step>=0;
        model=source;stage=current;skeleton={};
        if(!CameraReadable(source,sizeof(*source)) || !source->groups || source->groups>96 ||
           !source->vertex_count || source->vertex_count>16384 || !source->face_count ||
           source->face_count>32768 || !source->joints || source->joints>48)return false;
        const auto begin=reinterpret_cast<uintptr_t>(source->commands);
        const auto end=reinterpret_cast<uintptr_t>(source->vertices);
        if(end<=begin || (end-begin)%4 || end-begin>16384*4 ||
           !CameraReadable(source->commands,end-begin) ||
           !CameraReadable(source,sizeof(*source)+source->groups*4) ||
           !CameraReadable(source->vertices,source->vertex_count*8) ||
           !CameraReadable(source->faces,source->face_count*12))return false;
        if(!skeleton.Build(source->commands,(end-begin)/4,
            reinterpret_cast<const unsigned*>(source+1),source->groups,source->vertex_count,source->joints))return false;
        const auto& eye=skeleton.steps[skeleton.eye_step];
        ff_camera::Point lo={32767,32767,32767},hi={-32768,-32768,-32768};
        for(unsigned i=eye.first;i<eye.first+eye.count;++i) {
            const auto vertex=source->vertices+i*4;
            lo.x=fmin(lo.x,vertex[0]);hi.x=fmax(hi.x,vertex[0]);
            lo.y=fmin(lo.y,vertex[1]);hi.y=fmax(hi.y,vertex[1]);
            lo.z=fmin(lo.z,vertex[2]);hi.z=fmax(hi.z,vertex[2]);
        }
        center=ff_camera::Scale(ff_camera::Add(lo,hi),0.5);
        header.resize(8+source->groups);faces.resize(source->face_count*6);
        return true;
    }
};
static bool FirstPersonActorHead(unsigned slot,ff_camera::Point& head) {
    if(slot>=8)return false;
    static thread_local FirstPersonMesh meshes[8];
    auto& mesh=meshes[slot];
    const unsigned actor=slot*0xF4,palette=slot*0x238;
    const int character=CameraField<int16_t>(0x86A534+actor);
    if(character<0 || character>0x36)return false;
    const auto model=CameraField<const FirstPersonModel*>(0x85A6F8+character*4);
    if(!mesh.Read(model))return false;
    const auto angles=reinterpret_cast<const int16_t*>(g_base+0x868A40+palette+0x118);
    auto push=reinterpret_cast<void(__cdecl*)()>(g_base+0x1170);
    auto pop=reinterpret_cast<void(__cdecl*)()>(g_base+0x11F0);
    auto translate=reinterpret_cast<void(__cdecl*)(int32_t,int32_t,int32_t)>(g_base+0x15F0);
    auto rz=reinterpret_cast<void(__cdecl*)(int)>(g_base+0x12F0);
    auto ry=reinterpret_cast<void(__cdecl*)(int)>(g_base+0x13F0);
    auto rx=reinterpret_cast<void(__cdecl*)(int)>(g_base+0x14F0);
    push();reinterpret_cast<void(__cdecl*)()>(g_base+0x10F0)();
    translate(CameraField<int32_t>(0x86A528+actor),
        CameraField<int32_t>(0x86A52C+actor)+CameraField<int32_t>(0x8651C0+character*4),CameraField<int32_t>(0x86A530+actor));
    ry(-CameraField<int16_t>(0x86A538+actor)*32+32768);
    translate(0,CameraField<int32_t>(0x86A510+actor),CameraField<int32_t>(0x86A520+actor));
    rx(-angles[2]*16);ry(-angles[1]*16);rz(angles[0]*16);
    unsigned depth=0;
    for(int index=0;index<=mesh.skeleton.eye_step;++index) {
        const auto& step=mesh.skeleton.steps[index];
        if(step.operation==1) {push();++depth;continue;}
        if(step.operation==2) {pop();--depth;continue;}
        translate(step.translation[0],step.translation[1],step.translation[2]);
        rz(step.bind[0]*16);ry(step.bind[1]*16);rx(step.bind[2]*16);
        rz(angles[step.joint*3]*16);ry(angles[step.joint*3+1]*16);rx(angles[step.joint*3+2]*16);
        if(step.special==1)ry(CameraField<int32_t>(0x868A40+palette)*16);
        if(index==mesh.skeleton.eye_step) {
            const auto m=reinterpret_cast<const float*(__cdecl*)()>(g_base+0x1000)();
            const auto p=mesh.center;
            head={m[0]*p.x+m[1]*p.y+m[2]*p.z+m[3],
                m[4]*p.x+m[5]*p.y+m[6]*p.z+m[7],m[8]*p.x+m[9]*p.y+m[10]*p.z+m[11]};
            break;
        }
        rz(step.post[0]*16);ry(step.post[1]*16);rx(step.post[2]*16);
    }
    while(depth) {pop();--depth;}pop();
    return isfinite(head.x) && isfinite(head.y) && isfinite(head.z);
}
static bool FirstPersonHead(ff_camera::Point& head) {return FirstPersonActorHead(g_camera_owner==1?1:0,head);}
#include "first_person_movement.h"
static bool FirstPersonPose(ff_camera::Pose& pose) {
    if(!FirstPersonActive())return false;
    ff_camera::Point head;
    if(!FirstPersonHead(head)) {
        CameraFirstPersonView().selected=false;Log("First-person skeleton unavailable; orbit restored.\r\n");return false;
    }
    auto eye=CameraFirstPersonView().Eye(head,6,6);
    // Slide/knockdown root poses may put the skull at or below foot level.
    // Follow the animation while keeping a small eye clearance above that level.
    const unsigned actor=CameraPlayerOffset();
    const double lowest_eye=CameraField<int32_t>(0x86A52C+actor)/65536.0-16;
    eye.y=fmin(eye.y,lowest_eye);
    // Only the short eye offset is constrained, not the third-person boom.
    const ff_camera::Point pivot={CameraField<int32_t>(0x86A528+actor)/65536.0,fmin(head.y,lowest_eye),CameraField<int32_t>(0x86A530+actor)/65536.0};
    double contact=1;
    AcquireSRWLockShared(&g_camera_geometry_lock);
    if(g_camera_geometry_stage==CameraField<int>(0x85A6B8))
        for(const auto& triangle:g_camera_triangles)
            contact=ff_camera::SweepSphereSegment(pivot,eye,0,8,triangle,contact);
    ReleaseSRWLockShared(&g_camera_geometry_lock);
    pose=CameraFirstPersonView().Follow(ff_camera::Add(pivot,ff_camera::Scale(ff_camera::Sub(eye,pivot),contact)));
    return true;
}
static int __cdecl FirstPersonSkeleton(const FirstPersonModel* model,void* palette,int flags,int r,int g,int b) {
    if(g_camera_world_drawn)g_camera_world_drawn();
    auto native=reinterpret_cast<int(__cdecl*)(const FirstPersonModel*,void*,int,int,int,int)>(g_base+0x27180);
    if(!FirstPersonActive() || palette!=reinterpret_cast<void*>(g_base+0x8623E0+(g_camera_owner==1?0x238:0))) {
        const int result=native(model,palette,flags,r,g,b);
        if(g_camera_actor_bounds)g_camera_actor_bounds(model,palette);return result;
    }
    // Rendering may run on a separate native thread. Keep its temporary model
    // and face records private; source vertices, faces and animation stay intact.
    static thread_local FirstPersonMesh mesh;
    if(!mesh.Read(model))return native(model,palette,flags,r,g,b);
    memcpy(mesh.header.data(),model,mesh.header.size()*4);
    memcpy(mesh.faces.data(),model->faces,mesh.faces.size()*2);
    for(unsigned i=0;i<model->face_count;++i)
        if(mesh.skeleton.HeadFace(mesh.faces.data()+i*6))mesh.faces[i*6+5]=-1;
    reinterpret_cast<FirstPersonModel*>(mesh.header.data())->faces=mesh.faces.data();
    return native(reinterpret_cast<FirstPersonModel*>(mesh.header.data()),palette,flags,r,g,b);
}
static int __cdecl FirstPersonRenderOrigin() {
    return FirstPersonActive()?1:reinterpret_cast<int(__cdecl*)()>(g_base+0x20E0)();
}
static int __cdecl FirstPersonPoseOrigin() {
    return (g_camera_cooperative || FirstPersonActive())?1:reinterpret_cast<int(__cdecl*)()>(g_base+0x16A0)();
}
#include "first_person_props.h"
static bool InstallFirstPersonHooks() {
    if(!GetPrivateProfileIntW(L"Camera",L"FirstPerson",1,g_config_path))return false;
    struct Call {DWORD rva,target;void* replacement;};
    const Call calls[]={{0x440BC,0x27180,reinterpret_cast<void*>(FirstPersonSkeleton)},
        {0x44056,0x20E0,reinterpret_cast<void*>(FirstPersonRenderOrigin)},
        {0x27D22,0x16A0,reinterpret_cast<void*>(FirstPersonPoseOrigin)},
        {0x4426F,0x26DE0,reinterpret_cast<void*>(FirstPersonHeldRight)},
        {0x44488,0x26DE0,reinterpret_cast<void*>(FirstPersonHeldLeft)},
        {0x4F4E3,0x4E320,reinterpret_cast<void*>(FirstPersonStartBackstep)}};
    for(const auto& call:calls) {
        const auto code=reinterpret_cast<BYTE*>(g_base+call.rva);LONG offset;memcpy(&offset,code+1,4);
        if(code[0]!=0xE8 || reinterpret_cast<uintptr_t>(code+5)+offset!=g_base+call.target) {
            Log("First-person signatures differ; orbit retained.\r\n");return false;
        }
    }
    auto start=reinterpret_cast<BYTE*>(g_base+0x27D22);const SIZE_T length=0x4F4E8-0x27D22;
    DWORD old;if(!VirtualProtect(start,length,PAGE_EXECUTE_READWRITE,&old))return false;
    for(const auto& call:calls) {
        auto code=reinterpret_cast<BYTE*>(g_base+call.rva);
        const LONG offset=static_cast<LONG>(reinterpret_cast<BYTE*>(call.replacement)-(code+5));memcpy(code+1,&offset,4);
    }
    DWORD ignored;VirtualProtect(start,length,old,&ignored);FlushInstructionCache(GetCurrentProcess(),start,length);
    Log("Animated first-person camera and local head exclusion enabled.\r\n");return true;
}
