#pragma once
#include "weapon_laser.h"
static bool g_weapon_laser_available;
static bool g_weapon_laser_trace;
struct WeaponLaserVisual {ff_camera::Point start,end,aim;bool active=false,hit=false,loaded=false,launcher=false;};
static thread_local WeaponLaserVisual g_weapon_laser_frame;
static void CaptureWeaponLaser(const FirstPersonPropModel* model,uintptr_t actor,int type,unsigned hand) {
    const unsigned player=g_camera_owner==1?1:0;
    if(g_weapon_laser_trace && FirstPersonActive()) {
        static unsigned count=0;
        if((++count%120)==1) {char text[160];sprintf_s(text,"Weapon pose candidate: player %u actor delta %ld type %d hand %u.\r\n",
            player,long(actor-(g_base+0x86FEE2)),type,hand);Log(text);}
    }
    if(!g_weapon_laser_available || !g_controls.laser || !FirstPersonActive() ||
        ((type<0x1F3 || type>0x1F9) && type!=0x203) ||
        actor!=g_base+0x86FEE2+player*0xF4 || hand>1 || !CameraReadable(model,sizeof(*model)) ||
        !model->vertex_count || model->vertex_count>16384 || !CameraReadable(model->vertices,model->vertex_count*8))return;
    const int slot=CameraField<int8_t>(0x86FEFD+player*0xF4+hand);
    if(slot<0 || slot>=112 || CameraField<int>(0x870620+slot*36)!=type)return;
    const ff_camera::Pose view={CameraField<int32_t>(0x8677B0),CameraField<int32_t>(0x8677B4),CameraField<int32_t>(0x8677B8),
        CameraField<int16_t>(0x873CE0),CameraField<int16_t>(0x87459C)};
    const auto matrix=reinterpret_cast<const float*(__cdecl*)()>(g_base+0x1980)();
    WeaponLaserVisual beam;ff_camera::Point direction;
    if(!ff_laser::Muzzle(model->vertices,model->vertex_count,matrix,view,beam.start,direction))return;
    const auto delta=ff_camera::Scale(direction,4096);const auto end=ff_camera::Add(beam.start,delta);double contact=1;
    AcquireSRWLockShared(&g_camera_geometry_lock);
    if(g_camera_geometry_stage==CameraField<int>(0x85A6B8))for(const auto& triangle:g_camera_triangles)
        contact=ff_camera::SweepSphereSegment(beam.start,end,0,0,triangle,contact);
    ReleaseSRWLockShared(&g_camera_geometry_lock);
    for(unsigned i=0;i<8;++i) {
        if(i==player)continue;const unsigned o=i*0xF4;
        if(CameraField<int16_t>(0x86FEB4+o)<0 || CameraField<int16_t>(0x86FEC2+o)<=0)continue;
        const ff_camera::Point feet={CameraField<int32_t>(0x86FEA8+o)/65536.0,
            CameraField<int32_t>(0x86FEAC+o)/65536.0,CameraField<int32_t>(0x86FEB0+o)/65536.0};
        contact=ff_laser::Box(beam.start,delta,{feet.x-48,feet.y-300,feet.z-48},{feet.x+48,feet.y,feet.z+48},contact);
    }
    beam.end=ff_camera::Add(beam.start,ff_camera::Scale(delta,contact));beam.active=true;beam.hit=contact<1;
    beam.launcher=type==0x1F9;beam.aim=end;
    beam.loaded=CameraField<int16_t>(0x870636+slot*36)>0;g_weapon_laser_frame=beam;
    if(g_weapon_laser_trace) {
        static unsigned count=0;
        if((++count%120)==1) {char text[240];sprintf_s(text,"Weapon beam: %.2f,%.2f,%.2f -> %.2f,%.2f,%.2f; direction %.3f,%.3f,%.3f; hit %d loaded %d.\r\n",
            beam.start.x,beam.start.y,beam.start.z,beam.end.x,beam.end.y,beam.end.z,direction.x,direction.y,direction.z,beam.hit,beam.loaded);Log(text);}
    }
}
static void DrawWeaponLaser(void*,bool);
static void InstallWeaponLaser() {
    g_weapon_laser_trace=GetPrivateProfileIntW(L"Diagnostics",L"TraceCombatFocus",0,g_config_path)!=0;
    const BYTE actor[]={0xBE,0xE2,0xFE,0xC6,0};
    const BYTE right[]={0x8B,0x04,0x9D,0xF8,0xA6,0xC5,0,0x6A,0,0x6A,0,0x50};
    g_weapon_laser_available=!memcmp(reinterpret_cast<void*>(g_base+0x43F88),actor,sizeof(actor)) &&
        !memcmp(reinterpret_cast<void*>(g_base+0x44263),right,sizeof(right));
    g_held_weapon_pose=CaptureWeaponLaser;g_combat_weapon_overlay=DrawWeaponLaser;
    if(g_weapon_laser_trace)Log("Weapon laser capture diagnostics enabled.\r\n");
    if(!g_weapon_laser_available)Log("Weapon pose signatures differ; laser disabled.\r\n");
}
