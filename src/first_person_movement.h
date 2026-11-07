#pragma once
#include "modern_input.h"
#include "weapon_types.h"

static bool FirstPersonGroundLocomotion(int action) {
    // Native control normalizes these walk, run and backstep variants to
    // 0x52, 0x57 and 0x5C respectively. 0x5B and attacks are separate states.
    return (action>=0x52 && action<=0x5A) || (action>=0x5C && action<=0x61);
}
static bool FirstPersonIdle(int action) {
    // The native action normalizer also treats these weapon/idle stances as 0x45.
    return action==0x45 || action==0x46 || action==0x47 || action==0x48 ||
        action==0x49 || action==0x4C || action==0x4E || action==0xC6;
}
static bool FirstPersonGroundLoad() {
    if(CombatFocusActive())return true;
    for(DWORD hand=0x86A57D+CameraPlayerOffset();hand<=0x86A57E+CameraPlayerOffset();++hand) {
        const int slot=CameraField<int8_t>(hand);
        if(slot<0)continue;
        if(slot>=112)return false;
        const int type=CameraField<int32_t>(0x870620+slot*36);
        // Keep the firearm gait for pistols and launchers. Other carried
        // objects retain their original movement and body heading.
        if(!ff_weapon::Firearm(type))return false;
    }
    return true;
}
static void FirstPersonAlignBody() {
    if((!FirstPersonActive() && !CombatFocusActive()) || !FirstPersonGroundLoad())return;
    const unsigned actor=CameraPlayerOffset();
    const int action=CameraField<int>(0x86A50C+actor);
    // Native attacks, knockdowns and jumps retain their animation direction.
    if(FirstPersonIdle(action) || FirstPersonGroundLocomotion(action))
        CameraField<int16_t>(0x86A538+actor)=CombatFocusActive()?
            ((static_cast<uint16_t>(ff_camera::NativeAngle(CombatFocusBodyYaw()))-32768)>>5)&2047:CameraFirstPersonView().BodyHeading();
}
static void FirstPersonMovement(void* actor,int32_t* x,int32_t* z) {
    CameraPlayerScope scope(actor);const unsigned offset=CameraPlayerOffset();
    if((!FirstPersonActive() && !CombatFocusActive()) || actor!=reinterpret_cast<void*>(g_base+0x86A500+offset) || !x || !z)return;
    const int action=CameraField<int>(0x86A50C+offset);
    if(!FirstPersonGroundLocomotion(action) || CameraField<int16_t>(0x86A550+offset)!=0 ||
       !FirstPersonGroundLoad())return;
    const uint32_t directions=CameraField<uint32_t>(0x5523E0+(offset?4:0));
    const int horizontal=((directions&ff_input::Right)!=0)-((directions&ff_input::Left)!=0);
    const int forward=((directions&ff_input::Up)!=0)-((directions&ff_input::Down)!=0);
    if(!horizontal && !forward)return;
    const int32_t px=CameraField<int32_t>(0x86A528+offset),pz=CameraField<int32_t>(0x86A530+offset);
    const double length=hypot(double(*x)-px,double(*z)-pz);
    if(length<=0 || length>=256*65536.0)return;
    // Native locomotion otherwise turns toward the requested heading each
    // update, while the immersive body faces the view. Preserve animation speed
    // but express the proposal in that view's plane before collision handling.
    const double yaw=ff_camera::Radians(static_cast<int16_t>(ff_camera::NativeAngle(FirstPersonActive()?CameraFirstPersonView().yaw:CameraOrbit().yaw)&~31));
    const double scale=length/hypot(double(horizontal),double(forward));
    const double next_x=px+round((cos(yaw)*horizontal-sin(yaw)*forward)*scale);
    const double next_z=pz+round((sin(yaw)*horizontal+cos(yaw)*forward)*scale);
    if(next_x>=INT32_MIN && next_x<=INT32_MAX && next_z>=INT32_MIN && next_z<=INT32_MAX) {
        *x=static_cast<int32_t>(next_x);*z=static_cast<int32_t>(next_z);
    }
}
static int __cdecl FirstPersonStartBackstep(void* actor,const void* animations,int action) {
    CameraPlayerScope scope(actor);const unsigned offset=CameraPlayerOffset();
    // The retail backstep is a finite action that turns around at its end.
    // Holding retreat with a view-aligned body would restart that cycle. Use
    // the continuous walk gait; the movement proposal already supplies retreat.
    if(action==0x5C && (FirstPersonActive() || CombatFocusActive()) && actor==reinterpret_cast<void*>(g_base+0x86A500+offset) &&
       CameraField<int16_t>(0x86A550+offset)==0 && FirstPersonGroundLoad())action=0x52;
    return reinterpret_cast<int(__cdecl*)(void*,const void*,int)>(g_base+0x4E320)(actor,animations,action);
}
