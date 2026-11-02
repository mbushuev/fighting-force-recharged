#pragma once
static void DrawThrowTrajectory(void* device,bool world) {
    const ThrowTrajectoryVisual previews[]={g_throw_frames[0],g_throw_frames[1]};
    g_throw_frames[0]={};g_throw_frames[1]={};
    if(!world || !g_controls.trajectory || !g_camera_active || !CameraMayOwnPose() || !ModernHasFocus() ||
       g_controls_menu_active || g_campaign_menu_active)return;
    bool any=false;
    for(unsigned p=0;p<2;++p)any|=previews[p].path.count>=2 && previews[p].stage==CameraField<int>(0x85A6B8) &&
        (g_camera_owner<0 || unsigned(g_camera_owner)==p);
    if(!any)return;
    const ff_camera::Pose view={CameraField<int32_t>(0x8677B0),CameraField<int32_t>(0x8677B4),CameraField<int32_t>(0x8677B8),
        CameraField<int16_t>(0x873CE0),CameraField<int16_t>(0x87459C)};
    const double fx=CameraField<float>(0xAD214),fy=CameraField<float>(0xAD234);
    const double cx=CameraField<float>(0x20C50C),cy=CameraField<float>(0x20C508);
    const double width=CameraField<float>(0x9D170),height=CameraField<float>(0x9D174);
    if(width<=0 || height<=0 || !isfinite(width) || !isfinite(height))return;
    const double sy=ff_camera::Clamp(height/720,0.75,2),sx=sy*(g_coop_render_pass>=0?2:1);
    CombatOverlayDraw draw(device);
    auto project=[&](ff_camera::Point point,ff_focus::ScreenPoint& p){return ff_focus::Project(point,view,fx,fy,cx,cy,p);};
    for(unsigned player=0;player<2;++player) {
        const auto& preview=previews[player];const auto& path=preview.path;
        if(path.count<2 || preview.stage!=CameraField<int>(0x85A6B8) ||
           (g_camera_owner>=0 && unsigned(g_camera_owner)!=player))continue;
        for(unsigned i=1;i<path.count;++i) {
            auto a=path.points[i-1],b=ff_camera::Add(a,ff_camera::Scale(ff_camera::Sub(path.points[i],a),0.75));
            ff_focus::ScreenPoint first,last;
            if(!ff_laser::ClipBeam(a,b,view) || !project(a,first) || !project(b,last) ||
               !ff_focus::ClipLine(first.x,first.y,last.x,last.y,width,height))continue;
            draw.Line(first.x,first.y,last.x,last.y,sy+2,0xA0101820);
            draw.Line(first.x,first.y,last.x,last.y,sy,player?0xD0FFE090:0xD090D8FF);
        }
        ff_focus::ScreenPoint end;
        if(!path.hit || !project(path.points[path.count-1],end))continue;
        for(unsigned i=0;i<16;++i) {
            const double a=i*6.283185307179586/16,b=(i+1)*6.283185307179586/16;
            double ax=end.x+6*sx*cos(a),ay=end.y+6*sy*sin(a),bx=end.x+6*sx*cos(b),by=end.y+6*sy*sin(b);
            if(!ff_focus::ClipLine(ax,ay,bx,by,width,height))continue;
            draw.Line(ax,ay,bx,by,sy+2,0xC0101820);draw.Line(ax,ay,bx,by,sy,0xE0FFE090);
        }
    }
}
