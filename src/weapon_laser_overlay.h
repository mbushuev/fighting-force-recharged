#pragma once
static void DrawWeaponLaser(void* device,bool world) {
    const auto laser=g_weapon_laser_frame;g_weapon_laser_frame={};
    if(!world || !g_camera_active || !CameraMayOwnPose() || !ModernHasFocus() || g_controls_menu_active || g_campaign_menu_active)return;
    const bool beam=g_controls.laser && laser.active && FirstPersonActive();
    if(!beam)return;
    const ff_camera::Pose pose={CameraField<int32_t>(0x8677B0),CameraField<int32_t>(0x8677B4),CameraField<int32_t>(0x8677B8),
        CameraField<int16_t>(0x873CE0),CameraField<int16_t>(0x87459C)};
    const double fx=CameraField<float>(0xAD214),fy=CameraField<float>(0xAD234);
    const double cx=CameraField<float>(0x20C50C),cy=CameraField<float>(0x20C508);
    const double width=CameraField<float>(0x9D170),height=CameraField<float>(0x9D174);
    if(width<=0 || height<=0 || !isfinite(width) || !isfinite(height))return;
    const double sy=ff_camera::Clamp(height/720,0.75,2),sx=sy*(g_coop_render_pass>=0?2:1);
    CombatOverlayDraw draw(device);
    auto project=[&](ff_camera::Point point,ff_focus::ScreenPoint& p){return ff_focus::Project(point,pose,fx,fy,cx,cy,p);};
    if(beam) {
        ff_focus::ScreenPoint end;
        {
            const DWORD color=ff_laser::Color(laser.loaded);
            auto a=laser.start,b=laser.end;ff_focus::ScreenPoint first,last;
            if(ff_laser::ClipBeam(a,b,pose) && project(a,first) && project(b,last) &&
                ff_laser::ClipLine(first.x,first.y,last.x,last.y,width,height)) {
                draw.Line(first.x,first.y,last.x,last.y,sy,(color&0xFFFFFF)|0xB0000000);
            }
            if(laser.hit && project(laser.end,end) && end.x>=4*sx && end.x<width-4*sx && end.y>=4*sy && end.y<height-4*sy) {
                draw.Line(end.x-2*sx,end.y,end.x+2*sx,end.y,4*sy,0xEE102018);
                draw.Line(end.x-sx,end.y,end.x+sx,end.y,2*sy,color);
            }
        }
    }
}
