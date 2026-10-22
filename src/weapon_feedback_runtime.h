#pragma once
// Each firing routine computes its muzzle once, after native firing eligibility
// and projectile allocation. Observe that call, not input or each aiming ray.
static ff_feedback::Event FeedbackRayWeapon(int player) {
    // Match the native trace routine's hand priority and object-table stride.
    const DWORD actor=player*0xF4;
    int slot=InputField<int8_t>(0x86A57E+actor);
    if(slot==-1)slot=InputField<int8_t>(0x86A57D+actor);
    if(slot<0 || slot>=112)return ff_feedback::PistolShot;
    const int type=InputField<int>(0x870620+slot*36);
    // PISTOL2..6 use 0x1F3..0x1F7; GUNSHOT1 is the shotgun.
    if(type==0x1F8)return ff_feedback::ShotgunShot;
    if(type==0x203)return ff_feedback::HeavyShot;
    if(type==0x1F9)return ff_feedback::LauncherShot;
    return ff_feedback::PistolShot;
}
template<ff_feedback::Event recoil>
static int __cdecl FeedbackWeaponMuzzle(int player,int bone,void* position) {
    const int result=reinterpret_cast<int(__cdecl*)(int,int,void*)>(g_base+0x38FB0)(player,bone,position);
    if(player>=0 && player<2)PlayerCombatFeedback(player,
        recoil==ff_feedback::PistolShot?FeedbackRayWeapon(player):recoil);
    return result;
}
static bool InstallWeaponFeedback() {
    using Muzzle=int(__cdecl*)(int,int,void*);
    const struct {DWORD rva;Muzzle target;} sites[]={
        {0xE128,FeedbackWeaponMuzzle<ff_feedback::LauncherShot>},
        {0xE234,FeedbackWeaponMuzzle<ff_feedback::HeavyShot>},
        {0xE311,FeedbackWeaponMuzzle<ff_feedback::PistolShot>}};
    for(const auto& site:sites) {
        const auto code=reinterpret_cast<const BYTE*>(g_base+site.rva);LONG offset;memcpy(&offset,code+1,4);
        if(code[0]!=0xE8 || reinterpret_cast<uintptr_t>(code+5)+offset!=g_base+0x38FB0)return false;
    }
    auto start=reinterpret_cast<BYTE*>(g_base+sites[0].rva);const SIZE_T size=sites[2].rva+5-sites[0].rva;DWORD old;
    if(!VirtualProtect(start,size,PAGE_EXECUTE_READWRITE,&old))return false;
    for(const auto& site:sites) {
        auto code=reinterpret_cast<BYTE*>(g_base+site.rva);
        const LONG offset=static_cast<LONG>(reinterpret_cast<uintptr_t>(site.target)-reinterpret_cast<uintptr_t>(code+5));
        memcpy(code+1,&offset,4);
    }
    DWORD ignored;VirtualProtect(start,size,old,&ignored);FlushInstructionCache(GetCurrentProcess(),start,size);
    Log("Controller recoil enabled for native weapon discharges.\r\n");return true;
}
