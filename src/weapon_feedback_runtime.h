#pragma once
// Each firing routine computes its muzzle once, after native firing eligibility
// and projectile allocation. Observe that call, not input or each aiming ray.
static int __cdecl FeedbackWeaponMuzzle(int player,int bone,void* position) {
    const int result=reinterpret_cast<int(__cdecl*)(int,int,void*)>(g_base+0x38FB0)(player,bone,position);
    if(player>=0 && player<2)PlayerCombatFeedback(player,ff_feedback::Shot);
    return result;
}
static bool InstallWeaponFeedback() {
    const DWORD sites[]={0xE128,0xE234,0xE311};
    for(DWORD rva:sites) {
        const auto code=reinterpret_cast<const BYTE*>(g_base+rva);LONG offset;memcpy(&offset,code+1,4);
        if(code[0]!=0xE8 || reinterpret_cast<uintptr_t>(code+5)+offset!=g_base+0x38FB0)return false;
    }
    auto start=reinterpret_cast<BYTE*>(g_base+sites[0]);const SIZE_T size=sites[2]+5-sites[0];DWORD old;
    if(!VirtualProtect(start,size,PAGE_EXECUTE_READWRITE,&old))return false;
    for(DWORD rva:sites) {
        auto code=reinterpret_cast<BYTE*>(g_base+rva);
        const LONG offset=static_cast<LONG>(reinterpret_cast<uintptr_t>(FeedbackWeaponMuzzle)-reinterpret_cast<uintptr_t>(code+5));
        memcpy(code+1,&offset,4);
    }
    DWORD ignored;VirtualProtect(start,size,old,&ignored);FlushInstructionCache(GetCurrentProcess(),start,size);
    Log("Controller recoil enabled for native weapon discharges.\r\n");return true;
}
