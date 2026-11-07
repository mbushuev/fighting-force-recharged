#pragma once
#include "coop_camera.h"

struct CooperativePlayer {
    ff_camera::Orbit orbit;
    ff_camera::FirstPersonView eye;
    ff_camera::BoomResponse boom;
    ff_camera::Pose pose={};
};
static CooperativePlayer g_coop_players[2];
static ff_camera::Orbit g_coop_shared;
static ff_camera::BoomResponse g_coop_shared_boom;
static ff_camera::Pose g_coop_shared_pose={};
static ff_camera::CooperativeSplit g_coop_split;
static bool g_coop_enabled,g_coop_ready;
static bool g_coop_trace;
static int g_coop_stage=-1;
static void* g_coop_frame_trampoline;
#include "coop_compositor.h"
#include "coop_pacing.h"
struct CooperativeFrameTiming {
    LARGE_INTEGER start={};bool divided;
    explicit CooperativeFrameTiming(bool split):divided(split) {
        if(g_coop_trace && g_camera_cooperative)QueryPerformanceCounter(&start);
    }
    ~CooperativeFrameTiming() {
        if(!start.QuadPart || !g_camera_frequency.QuadPart)return;
        LARGE_INTEGER end;QueryPerformanceCounter(&end);
        struct Sample {double total=0,maximum=0;unsigned count=0;};
        static Sample samples[2];auto& sample=samples[divided?1:0];
        const double ms=(end.QuadPart-start.QuadPart)*1000.0/g_camera_frequency.QuadPart;
        sample.total+=ms;sample.maximum=fmax(sample.maximum,ms);
        if(++sample.count>=60) {
            char message[192];sprintf_s(message,"Cooperative %s frame: %u samples, mean %.3f ms, max %.3f ms.\r\n",
                divided?"split":"shared",sample.count,sample.total/sample.count,sample.maximum);Log(message);sample={};
        }
    }
};

static ff_camera::Point CooperativeFeet(unsigned player) {
    const unsigned offset=player*0xF4;
    return {CameraField<int32_t>(0x86A528+offset)/65536.0,
        CameraField<int32_t>(0x86A52C+offset)/65536.0,CameraField<int32_t>(0x86A530+offset)/65536.0};
}
static unsigned CooperativeLivingPlayers() {
    return (CameraField<int16_t>(0x86A542)>0?1u:0u)|(CameraField<int16_t>(0x86A636)>0?2u:0u);
}
static int CooperativeSoloPlayer() {
    const auto living=CooperativeLivingPlayers();return living==1?0:living==2?1:-1;
}
static void CooperativePublish(ff_camera::Pose pose) {
    CameraField<int32_t>(0x866220)=pose.x;CameraField<int32_t>(0x866224)=pose.y;
    CameraField<int32_t>(0x866228)=pose.z;CameraField<int16_t>(0x87888C)=pose.yaw;
    CameraField<int16_t>(0x87888E)=pose.pitch;
    CameraField<int32_t>(0x86D31C)=pose.x;CameraField<int32_t>(0x86D324)=pose.y;
    CameraField<int32_t>(0x86D328)=pose.z;CameraField<int16_t>(0x86D318)=pose.yaw;
    CameraField<int16_t>(0x874CF2)=pose.pitch;
    CameraField<int32_t>(0x8744A8)=CameraField<int32_t>(0x8651A4)=CameraField<int32_t>(0x8689F8)=0;
    CameraField<int32_t>(0x866214)=pose.y>>12;
    CameraField<int32_t>(0x878708)=static_cast<int32_t>(-static_cast<int64_t>(pose.x)>>12);
    CameraField<int32_t>(0x86BF0C)=static_cast<int32_t>(-static_cast<int64_t>(pose.z)>>12);
}
static ff_camera::Pose CooperativeOrbitPose(ff_camera::Orbit orbit,ff_camera::Point feet,
                                           ff_camera::BoomResponse& boom,double seconds) {
    const auto desired=orbit.Follow(feet,g_camera_settings);
    const ff_camera::Point focus={feet.x,feet.y-g_camera_settings.height,feet.z};
    const double allowed=CameraObstacleDistance(focus,{desired.x/65536.0,desired.y/65536.0,desired.z/65536.0});
    orbit.distance=boom.Update(allowed,orbit.distance,seconds);
    return orbit.Follow(feet,g_camera_settings);
}
static bool CooperativeVisible(ff_camera::Point feet,ff_camera::Pose camera) {
    feet.y-=g_camera_settings.height;
    const ff_camera::Point eye={camera.x/65536.0,camera.y/65536.0,camera.z/65536.0};
    bool visible=true;AcquireSRWLockShared(&g_camera_geometry_lock);
    if(g_camera_geometry_stage==CameraField<int>(0x85A6B8) && GetTickCount64()-g_camera_geometry_stamp<1000)
        for(const auto& triangle:g_camera_triangles)
            if(ff_camera::SweepSphereSegment(feet,eye,8,8,triangle,1)<0.98) {visible=false;break;}
    ReleaseSRWLockShared(&g_camera_geometry_lock);return visible;
}
static void CooperativeUpdatePoses(double seconds,bool update_policy) {
    if(!g_camera_cooperative || !CameraMayOwnPose())return;
    const unsigned living=CooperativeLivingPlayers();
    for(int i=0;i<2;++i) {
        if(!(living&(1u<<i)))continue;
        CameraPlayerScope scope(i);FirstPersonAlignBody();
        if(!FirstPersonPose(g_coop_players[i].pose))
            g_coop_players[i].pose=CooperativeOrbitPose(g_coop_players[i].orbit,CooperativeFeet(i),g_coop_players[i].boom,seconds);
    }
    const int solo=CooperativeSoloPlayer();
    if(solo>=0) {
        g_coop_split={};g_coop_shared_pose=g_coop_players[solo].pose;
        CooperativePublish(g_coop_shared_pose);return;
    }
    const auto midpoint=ff_camera::Midpoint(CooperativeFeet(0),CooperativeFeet(1));
    g_coop_shared.yaw=ff_camera::Wrap(g_coop_players[0].orbit.yaw+
        ff_camera::Wrap(g_coop_players[1].orbit.yaw-g_coop_players[0].orbit.yaw)*0.5);
    g_coop_shared.pitch=(g_coop_players[0].orbit.pitch+g_coop_players[1].orbit.pitch)*0.5;
    g_coop_shared.distance=ff_camera::Clamp((g_coop_players[0].orbit.distance+g_coop_players[1].orbit.distance)*0.5,600,1100);
    g_coop_shared_pose=CooperativeOrbitPose(g_coop_shared,midpoint,g_coop_shared_boom,seconds);
    if(update_policy) {
        const double width=CameraField<float>(0x9D170),height=CameraField<float>(0x9D174);
        double occupancy=0;
        for(unsigned i=0;i<2;++i)occupancy=fmax(occupancy,ff_camera::ActorOccupancy(g_coop_shared_pose,CooperativeFeet(i),
            CameraField<float>(0xAD214),CameraField<float>(0xAD234),width,height));
        if(occupancy<=0.90 && (!CooperativeVisible(CooperativeFeet(0),g_coop_shared_pose) ||
                              !CooperativeVisible(CooperativeFeet(1),g_coop_shared_pose)))occupancy=1.1;
        g_coop_split.Update(occupancy,g_coop_players[0].eye.selected||g_coop_players[1].eye.selected||
            g_focus_states[0].Active()||g_focus_states[1].Active(),seconds);
        if(g_coop_composition_failed)g_coop_split={};
    }
    CooperativePublish(g_coop_shared_pose);
}
static void CooperativeReset() {
    ResetCombatFocus();
    for(auto& player:g_coop_players) {player.eye.RequireRelease();player.boom={};}
    g_modern_second_session.ready=false;g_modern_second_session.last_poll=0;
}
static bool CooperativeInput(double seconds) {
    if(!g_coop_enabled || !ModernCooperativeGameplay()) {
        if(g_camera_cooperative) {g_camera_active=false;g_camera_cooperative=false;}
        g_coop_ready=false;return false;
    }
    for(unsigned i=0;i<2;++i)CameraField<uint32_t>(0x5523E0+i*4)=ModernCooperativeActions(i);
    if(!g_modern_coop_camera) {
        g_camera_active=false;g_camera_cooperative=false;g_coop_ready=false;g_coop_split={};
        ReadCameraMouse(false);return true;
    }
    g_camera_cooperative=true;
    if(!CameraMayOwnPose()) {
        g_camera_active=false;CooperativeReset();ReadCameraMouse(false);return true;
    }
    const int stage=CameraField<int>(0x85A6B8);
    if(!g_coop_ready || stage!=g_coop_stage) {
        g_coop_stage=stage;g_coop_ready=true;g_coop_split={};g_coop_shared_boom={};
        for(int i=0;i<2;++i) {
            auto& player=g_coop_players[i];
            player.orbit.Begin(CameraField<int16_t>(0x87888C),g_camera_settings);
            player.eye.Begin(CameraField<int16_t>(0x86A538+i*0xF4));
            player.eye.RequireRelease();player.boom={};
            g_camera_player_views[i]=&player.eye;
            g_camera_player_orbits[i]=&player.orbit;
        }
        Log("Cooperative cameras own two live player views.\r\n");
    }
    g_camera_active=true;
    const bool focus=ModernHasFocus();
    const int keyboard_player=ModernKeyboardPlayer();const bool keyboard_mouse=keyboard_player>=0;
    const ff_input::Pad pads[]={ReadModernCoopPlayerPad(0),ReadModernCoopPlayerPad(1)};
    const auto keys=ReadModernPlayerKeyboard(keyboard_mouse?keyboard_player:0);
    const DIMOUSESTATE mouse=ReadCameraMouse(focus&&keyboard_mouse);
    if(keyboard_mouse)CameraField<uint32_t>(0x5523E0+keyboard_player*4)=MergeMouseCombat(CameraField<uint32_t>(0x5523E0+keyboard_player*4),
        (mouse.rgbButtons[0]&0x80)!=0,(mouse.rgbButtons[1]&0x80)!=0,g_mouse_combat&&focus&&!g_controls_custom);
    for(int i=0;i<2;++i) {
        auto& player=g_coop_players[i];const auto& pad=pads[i];
        if(!(CooperativeLivingPlayers()&(1u<<i))) {player.eye.RequireRelease();g_focus_states[i].Reset();PublishCombatFocus(i,false);continue;}
        if(seconds>0.25 || !focus)player.eye.RequireRelease();
        const bool local_mouse=keyboard_player==i;
        if(player.eye.Toggle((pad.buttons&ff_input::LeftThumb)!=0 || (local_mouse&&keys.first_person),focus&&g_first_person_available)) {
            if(player.eye.selected)player.eye.Begin(CameraField<int16_t>(0x86A538+i*0xF4));
            else {player.orbit.yaw=player.eye.yaw;player.orbit.pitch=ff_camera::Clamp(player.eye.pitch,
                ff_camera::MinimumPitch(player.orbit.distance,g_camera_settings),g_camera_settings.max_pitch);}
        }
        const bool focus_pressed=(pad.buttons&ff_input::RightThumb)!=0 || (local_mouse&&(keys.recenter||(!g_controls_custom&&(mouse.rgbButtons[2]&0x80))));
        const auto stick=ff_input::CameraLook(pad,g_modern_right_deadzone,local_mouse?keys:ff_input::Keyboard{});
        CameraPlayerScope owner(i);
        CombatFocusInput(i,player.orbit,player.eye,focus_pressed,stick.x,
            pad.previous_target||(local_mouse&&keys.previous_target),pad.next_target||(local_mouse&&keys.next_target),seconds,focus);
        const double zoom=(pad.rt>30?(pad.rt-30)/225.0:0)-(pad.lt>30?(pad.lt-30)/225.0:0)+
            (local_mouse?keys.zoom_in-keys.zoom_out:0);
        if(focus) {
            if(player.eye.selected) {if(!CombatFocusActive())player.eye.Look(local_mouse?mouse.lX:0,local_mouse?mouse.lY:0,stick.x,stick.y,seconds,g_first_person_settings);}
            else player.orbit.Look(local_mouse&&!CombatFocusActive()?mouse.lX:0,local_mouse?mouse.lY:0,CombatFocusActive()?0:stick.x,stick.y,seconds,
                (local_mouse?mouse.lZ/120.0:0)+zoom*ff_camera::Clamp(seconds,0,0.1)*g_camera_settings.trigger_zoom_speed/60,g_camera_settings);
        }
    }
    // Both controllers can steer the common view until it actually separates.
    if(CooperativeLivingPlayers()==3 && !g_coop_split.separate && g_coop_split.amount==0 && !g_coop_players[0].eye.selected && !g_coop_players[1].eye.selected &&
       !g_focus_states[0].Active() && !g_focus_states[1].Active()) {
        const double yaw=ff_camera::Wrap(g_coop_players[0].orbit.yaw+
            ff_camera::Wrap(g_coop_players[1].orbit.yaw-g_coop_players[0].orbit.yaw)*0.5);
        for(auto& player:g_coop_players)player.orbit.yaw=yaw;
    }
    CooperativeUpdatePoses(seconds,true);return true;
}
static void CooperativeStage() {if(g_coop_ready && g_camera_cooperative)CooperativeUpdatePoses(0,false);}
static int __cdecl CooperativeHeading(void* actor) {
    if(g_coop_ready && g_camera_cooperative && g_camera_active) {
        const auto address=reinterpret_cast<uintptr_t>(actor);
        const int player=address==g_base+0x86A500?0:address==g_base+0x86A5F4?1:-1;
        if(player>=0)return ff_camera::NativeAngle(g_coop_players[player].eye.selected?
            g_coop_players[player].eye.yaw:g_coop_players[player].orbit.yaw);
    }
    return CameraField<int>(0x85A68C);
}
__declspec(naked) static void CooperativeHeadingEntry() {
    __asm {
        pushfd
        push eax
        push edx
        push esi
        call CooperativeHeading
        add esp,4
        mov ecx,eax
        pop edx
        pop eax
        popfd
        ret
    }
}
static bool CooperativeSkipPresent() {return g_coop_render_pass==0;}
static int __fastcall CooperativeWindowBlt(void* surface,void*,RECT* destination,void* source,RECT* area,DWORD flags,void* effects) {
    if(CooperativeSkipPresent())return 1;
    using Native=int(__thiscall*)(void*,RECT*,void*,RECT*,DWORD,void*);
    return reinterpret_cast<Native>(g_base+0x23B00)(surface,destination,source,area,flags,effects);
}
static bool CooperativeScreenPolygon(const RetailScreenPolygon* source,const RetailScreenPolygon* centered,RetailScreenPolygon& copy) {
    if(g_coop_render_pass<0 || !source || source->kind<5 || source->kind>8 ||
       !(source->flags&1) || !source->count || source->count>8)return false;
    copy=*centered;
    const float width=CameraField<float>(0x9D170),height=CameraField<float>(0x9D174);
    if(copy.flags&1) {
        const float scale=height/240,left=(width-320*scale)*0.5f;
        for(unsigned i=0;i<copy.count;++i) {copy.vertices[i].sx=copy.vertices[i].sx*scale+left;copy.vertices[i].sy*=scale;}
        copy.flags&=~1u;
    }
    const auto frustum=ff_camera::SplitFrustum(g_coop_render_pass,g_coop_render_blend,1,width);
    const float offset=g_wide_background?float(frustum.center_x-width):(g_coop_render_pass?-width:0);
    for(unsigned i=0;i<copy.count;++i)copy.vertices[i].sx=copy.vertices[i].sx*2+offset;
    return true;
}
static void __cdecl CooperativeFrame() {
    CooperativeFrameTiming timing(g_coop_split.amount>0);
    auto native=reinterpret_cast<void(__cdecl*)()>(g_coop_frame_trampoline);
    if(!g_coop_enabled || !g_coop_ready || g_coop_stage!=CameraField<int>(0x85A6B8) ||
       !g_camera_cooperative || !g_camera_active || !CameraMayOwnPose()) {native();return;}
    if(g_coop_split.amount<=0) {CameraPlayerScope scope(CooperativeSoloPlayer());native();return;}
    const ff_camera::Pose saved={CameraField<int32_t>(0x866220),CameraField<int32_t>(0x866224),CameraField<int32_t>(0x866228),
        CameraField<int16_t>(0x87888C),CameraField<int16_t>(0x87888E)};
    const float focal=CameraField<float>(0xAD214),scale=CameraField<float>(0x9D178),center=CameraField<float>(0x20C50C);
    // Presentation-only health interpolation and fade counters must advance
    // once per game frame, even though the world is drawn from two viewpoints.
    BYTE hud[8][6];for(unsigned i=0;i<8;++i)memcpy(hud[i],reinterpret_cast<void*>(g_base+0x86A544+i*0xF4),6);
    const DWORD fade_rvas[]={0x20D53C,0x20D528,0x9D200};DWORD fades[3];
    for(unsigned i=0;i<3;++i)fades[i]=CameraField<DWORD>(fade_rvas[i]);
    g_coop_render_blend=g_coop_split.Blend();
    for(int i=0;i<2;++i) {
        g_coop_render_pass=i;CameraPlayerScope scope(i);
        const auto frustum=ff_camera::SplitFrustum(i,g_coop_render_blend,focal,CameraField<float>(0x9D170));
        CameraField<float>(0xAD214)=float(frustum.focal_x);CameraField<float>(0x9D178)=scale*2;
        CameraField<float>(0x20C50C)=float(frustum.center_x);
        CooperativePublish(ff_camera::BlendPose(g_coop_shared_pose,g_coop_players[i].pose,g_coop_render_blend));
        native();
        if(!i) {
            for(unsigned j=0;j<8;++j)memcpy(reinterpret_cast<void*>(g_base+0x86A544+j*0xF4),hud[j],6);
            for(unsigned j=0;j<3;++j)CameraField<DWORD>(fade_rvas[j])=fades[j];
        }
    }
    g_coop_render_pass=-1;CameraField<float>(0xAD214)=focal;CameraField<float>(0x9D178)=scale;
    CameraField<float>(0x20C50C)=center;CooperativePublish(saved);
}
static void InstallCooperativeHooks() {
    if(!g_free_orbit || !g_first_person_available || !g_presentation_installed)return;
    const BYTE entry[]={0x8B,0x0D,0x24,0x62,0xC6,0x00};
    const BYTE heading[]={0x8B,0x0D,0x8C,0xA6,0xC5,0x00};
    auto window_call=reinterpret_cast<BYTE*>(g_base+0x25A5A);LONG relative;memcpy(&relative,window_call+1,4);
    struct ClockCall {DWORD rva,target;void* replacement;};
    const ClockCall clocks[]={{0x25978,0x23060,reinterpret_cast<void*>(CooperativeClockStop)},
        {0x25985,0x23090,reinterpret_cast<void*>(CooperativeClockElapsed)},
        {0x259CB,0x23030,reinterpret_cast<void*>(CooperativeClockReset)},
        {0x259D0,0x23040,reinterpret_cast<void*>(CooperativeClockStart)}};
    for(const auto& call:clocks) {
        auto code=reinterpret_cast<const BYTE*>(g_base+call.rva);LONG offset;memcpy(&offset,code+1,4);
        if(code[0]!=0xE8 || g_base+call.rva+5+offset!=g_base+call.target) {
            Log("Cooperative clock signatures differ; native shared camera retained.\r\n");return;
        }
    }
    if(memcmp(reinterpret_cast<void*>(g_base+0x40BC0),entry,sizeof(entry)) ||
       memcmp(reinterpret_cast<void*>(g_base+0x4F197),heading,sizeof(heading)) ||
       window_call[0]!=0xE8 || g_base+0x25A5F+relative!=g_base+0x23B00) {
        Log("Cooperative signatures differ; native shared camera retained.\r\n");return;
    }
    auto trampoline=static_cast<BYTE*>(VirtualAlloc(nullptr,16,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    if(!trampoline)return;
    memcpy(trampoline,entry,6);trampoline[6]=0xE9;
    relative=static_cast<LONG>(g_base+0x40BC6-reinterpret_cast<uintptr_t>(trampoline+11));memcpy(trampoline+7,&relative,4);
    auto first=reinterpret_cast<BYTE*>(g_base+0x25978);const SIZE_T length=0x4F19D-0x25978;DWORD old;
    if(!VirtualProtect(first,length,PAGE_EXECUTE_READWRITE,&old)) {VirtualFree(trampoline,0,MEM_RELEASE);return;}
    auto branch=[](DWORD rva,void* target,BYTE opcode) {
        auto code=reinterpret_cast<BYTE*>(g_base+rva);code[0]=opcode;
        const LONG offset=static_cast<LONG>(reinterpret_cast<BYTE*>(target)-(code+5));memcpy(code+1,&offset,4);
    };
    branch(0x40BC0,reinterpret_cast<void*>(CooperativeFrame),0xE9);CameraField<BYTE>(0x40BC5)=0x90;
    branch(0x4F197,reinterpret_cast<void*>(CooperativeHeadingEntry),0xE8);CameraField<BYTE>(0x4F19C)=0x90;
    branch(0x25A5A,reinterpret_cast<void*>(CooperativeWindowBlt),0xE8);
    for(const auto& call:clocks)branch(call.rva,call.replacement,0xE8);
    DWORD ignored;VirtualProtect(first,length,old,&ignored);FlushInstructionCache(GetCurrentProcess(),first,length);
    g_coop_frame_trampoline=trampoline;g_coop_enabled=true;g_modern_coop_input=true;
    g_coop_trace=GetPrivateProfileIntW(L"Diagnostics",L"TraceCooperative",0,g_config_path)!=0;
    g_cooperative_input=CooperativeInput;g_cooperative_stage=CooperativeStage;g_cooperative_reset=CooperativeReset;
    g_render_frame_finished=CooperativeFrameFinished;g_render_extra_release=CooperativeReleaseSurface;
    g_presentation_skip_frame=CooperativeSkipPresent;g_wide_view_polygon=CooperativeScreenPolygon;
    Log("Dynamic cooperative split-screen hooks enabled in process memory.\r\n");
}
