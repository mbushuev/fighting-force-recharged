#pragma once
#include "throw_trajectory.h"
struct ThrowTrajectoryVisual {ff_throw::Path path;int stage=-1;};
static thread_local ThrowTrajectoryVisual g_throw_frames[2];
static bool g_throw_trajectory_available;
static void CaptureThrowTrajectory(const FirstPersonPropModel*,uintptr_t actor,int type,unsigned hand) {
    if(!g_throw_trajectory_available || !g_controls.trajectory || !g_camera_active || !CameraMayOwnPose() ||
       !ff_throw::Eligible(type) || hand>1 || actor<g_base+0x86FEE2)return;
    const uintptr_t offset=actor-(g_base+0x86FEE2);
    if(offset%0xF4 || offset/0xF4>1)return;
    const unsigned player=static_cast<unsigned>(offset/0xF4);
    if(g_camera_owner>=0 && unsigned(g_camera_owner)!=player)return;
    const unsigned o=player*0xF4;
    if(CameraField<int16_t>(0x86FEB4+o)<0 || CameraField<int16_t>(0x86FEC2+o)<=0)return;
    const int slot=CameraField<int8_t>(0x86FEFD+o+hand);
    if(slot<0 || slot>=64 || CameraField<int>(0x870620+slot*36)!=type)return;
    // Native throw input checks the right hand before the left hand.
    const int right=CameraField<int8_t>(0x86FEFE+o);
    if(hand==0 && right>=0 && right<64 && ff_throw::Eligible(CameraField<int>(0x870620+right*36)))return;
    const ff_camera::Pose view={CameraField<int32_t>(0x8677B0),CameraField<int32_t>(0x8677B4),CameraField<int32_t>(0x8677B8),
        CameraField<int16_t>(0x873CE0),CameraField<int16_t>(0x87459C)};
    const auto matrix=reinterpret_cast<const float*(__cdecl*)()>(g_base+0x1980)();
    if(!matrix)return;
    // Reuse this render sample, rather than a previous simulation wrist or view.
    const auto start=ff_laser::ToWorld({matrix[3],matrix[7],matrix[11]},view);
    const unsigned heading=CameraField<uint16_t>(0x86FEB8+o)&0x7FF;
    ThrowTrajectoryVisual preview;preview.stage=CameraField<int>(0x85A6B8);
    preview.path=ff_throw::Flight(start,CameraField<int16_t>(0x67F50+heading*2),CameraField<int16_t>(0x68350+heading*2),
        ff_throw::Lift(type,hand,CameraField<int>(0x86FE8C+o)));
    if(preview.path.count<2)return;
    AcquireSRWLockShared(&g_camera_geometry_lock);
    if(g_camera_geometry_stage==preview.stage && GetTickCount64()-g_camera_geometry_stamp<1000)
        ff_throw::Obstruct(preview.path,g_camera_triangles.data(),g_camera_triangles.size());
    ReleaseSRWLockShared(&g_camera_geometry_lock);
    for(unsigned i=0;i<8;++i) {
        if(i==player)continue;const unsigned other=i*0xF4;
        if(CameraField<int16_t>(0x86FEB4+other)<0 || CameraField<int16_t>(0x86FEC2+other)<=0)continue;
        const ff_camera::Point feet={CameraField<int32_t>(0x86FEA8+other)/65536.0,
            CameraField<int32_t>(0x86FEAC+other)/65536.0,CameraField<int32_t>(0x86FEB0+other)/65536.0};
        for(unsigned p=1;p<preview.path.count;++p) {
            const auto a=preview.path.points[p-1],delta=ff_camera::Sub(preview.path.points[p],a);
            const double hit=ff_laser::Box(a,delta,{feet.x-48,feet.y-300,feet.z-48},{feet.x+48,feet.y,feet.z+48},1);
            if(hit<1) {
                preview.path.points[p]=ff_camera::Add(a,ff_camera::Scale(delta,hit));
                preview.path.count=p+1;preview.path.hit=true;break;
            }
        }
    }
    g_throw_frames[player]=preview;
}
static void CaptureHeldAim(const FirstPersonPropModel* model,uintptr_t actor,int type,unsigned hand) {
    CaptureWeaponLaser(model,actor,type,hand);CaptureThrowTrajectory(model,actor,type,hand);
}
static void DrawThrowTrajectory(void*,bool);
static void InstallThrowTrajectory() {
    // Check release speed, lift and physics order before exposing a prediction.
    const BYTE gravity[]={0x66,0x83,0x46,0x20,0xF0,0x66,0x8B,0x7E,0x1A,0x0F,0xBF,0x56,0x20};
    const BYTE drag[]={0x66,0x83,0x46,0x18,0xFD};
    const BYTE heavy[]={0x66,0xC7,0x87,0x40,0x06,0xC7,0,0x20,0};
    const BYTE light[]={0x66,0xC7,0x87,0x40,0x06,0xC7,0,0x40,0};
    g_throw_trajectory_available=g_weapon_laser_available &&
        !memcmp(reinterpret_cast<void*>(g_base+0xA7AC),gravity,sizeof(gravity)) &&
        !memcmp(reinterpret_cast<void*>(g_base+0xAA22),drag,sizeof(drag)) &&
        !memcmp(reinterpret_cast<void*>(g_base+0x51694),heavy,sizeof(heavy)) &&
        !memcmp(reinterpret_cast<void*>(g_base+0x515FD),light,sizeof(light));
    g_held_weapon_pose=CaptureHeldAim;g_combat_throw_overlay=DrawThrowTrajectory;
    if(!g_throw_trajectory_available)Log("Throw physics signatures differ; trajectory preview disabled.\r\n");
}
