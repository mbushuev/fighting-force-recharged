#pragma once
static bool FirstPersonActorHead(unsigned slot,ff_camera::Point& head);
// This adapter runs only at the existing, signature-checked camera input hook.
static ff_focus::Target CombatFocusActor(unsigned slot) {
    const DWORD offset=slot*0xF4;
    ff_focus::Target t;
    t.type=CameraField<int16_t>(0x86A534+offset);t.health=CameraField<int16_t>(0x86A542+offset);
    t.action=CameraField<int>(0x86A50C+offset);
    t.present=CameraField<uintptr_t>(0x86A500+offset)!=0 && CameraField<int16_t>(0x86A550+offset)==0;
    t.point={CameraField<int32_t>(0x86A528+offset)/65536.0,
        CameraField<int32_t>(0x86A52C+offset)/65536.0-160,CameraField<int32_t>(0x86A530+offset)/65536.0};
    if(slot>=2 && t.present) {
        ff_camera::Point head;
        const double feet=t.point.y+160;
        if(FirstPersonActorHead(slot,head))t.height=ff_camera::Clamp(feet-head.y,40,330);
        t.point.y=feet-t.height*0.82;
    }
    return t;
}
static bool CombatFocusVisible(ff_camera::Point from,ff_camera::Point to) {
    bool visible=false;
    AcquireSRWLockShared(&g_camera_geometry_lock);
    if(g_camera_geometry_stage==CameraField<int>(0x85A6B8) && GetTickCount64()-g_camera_geometry_stamp<1000) {
        visible=true;
        for(const auto& triangle:g_camera_triangles)
            if(ff_camera::SweepSphereSegment(from,to,0,2,triangle,1)<0.98){visible=false;break;}
    }
    ReleaseSRWLockShared(&g_camera_geometry_lock);return visible;
}
static bool CombatFocusActive() {
    const unsigned player=g_camera_owner==1?1:0;
    return g_controls.focus && g_camera_active && g_free_orbit && CameraMayOwnPose() &&
        g_focus_states[player].Active();
}
static double CombatFocusBodyYaw() {
    const unsigned player=g_camera_owner==1?1:0;
    return ff_focus::Yaw(CombatFocusActor(player).point,g_focus_states[player].last);
}
static bool CombatFocusInput(unsigned player,ff_camera::Orbit& orbit,ff_camera::FirstPersonView& eye,
                             bool press,bool edge,double axis,bool previous,bool next,double seconds,bool foreground) {
    auto& state=g_focus_states[player];
    if(!g_controls.focus || !foreground || g_controls_menu_active || g_controls_wait_release) {
        state.Reset();PublishCombatFocus(player,false);return foreground && edge;
    }
    ff_focus::Target targets[ff_focus::Actors];const auto actor=CombatFocusActor(player);
    auto origin=actor.point;
    auto view_pose=orbit.Follow({origin.x,origin.y+160,origin.z},g_camera_settings);
    if(eye.selected) {
        if(!FirstPersonActorHead(player,origin))origin.y-=80;
        origin=eye.Eye(origin,6,6);
        view_pose={ff_camera::Fixed(origin.x),ff_camera::Fixed(origin.y),ff_camera::Fixed(origin.z),
            ff_camera::NativeAngle(eye.yaw),ff_camera::NativeAngle(eye.pitch)};
    }
    const ff_camera::Point view_origin={view_pose.x/65536.0,view_pose.y/65536.0,view_pose.z/65536.0};
    // Avoid six geometry scans per tick when focus is idle.
    for(unsigned i=2;i<ff_focus::Actors;++i) {
        targets[i]=CombatFocusActor(i);
        if(ff_focus::Enemy(i,targets[i]) && (int(i)==state.target || (press && !state.button) ||
            (state.Active() && (fabs(axis)>.65 || previous || next))))
            targets[i].visible=CombatFocusVisible(view_origin,targets[i].point) && CombatFocusVisible(actor.point,targets[i].point);
    }
    const ff_focus::View view={view_pose,CameraField<float>(0xAD214)*(g_camera_cooperative?2:1),
        CameraField<float>(0xAD234),CameraField<float>(0x9D170),CameraField<float>(0x9D174)};
    const bool reset=state.Update(targets,actor.point,view,CameraField<int>(0x85A6B8),actor.health>0,
        press,axis,previous,next,seconds);
    if(state.Active()) {
        auto& yaw=eye.selected?eye.yaw:orbit.yaw;auto& pitch=eye.selected?eye.pitch:orbit.pitch;
        ff_focus::Track(eye.selected?origin:actor.point,state.last,seconds,eye.selected,-1.25,1.25,yaw,pitch);
    }
    PublishCombatFocus(player,state.Active() && targets[state.target].visible);
    return reset;
}
