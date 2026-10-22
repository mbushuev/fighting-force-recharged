#pragma once
static BYTE* g_feedback_damage_trampoline;
static int FeedbackControllerSlot(unsigned player) {
    if(player>1 || !g_modern_gamepad)return -1;
    const bool coop=InputField<int>(0x8745A0)==2;
    if(!coop && player)return -1;
    if(coop && ModernKeyboardPlayer()==static_cast<int>(player))return -1;
    const auto& session=coop && player && g_modern_coop_scheme==2?g_modern_second_session:g_modern_pad_session;
    return session.ready && session.raw.connected?session.selected:-1;
}
static void PlayerCombatFeedback(unsigned player,ff_feedback::Event event) {
    const int stage=InputField<int>(0x85A6B8);
    if(!g_controls.vibration || stage<0 || stage>=25 || InputField<int>(0x85A668)==1 ||
       g_controls_menu_active || g_controls_wait_release || !ModernHasFocus())return;
    const int slot=FeedbackControllerSlot(player);
    if(slot>=0)QueueControllerFeedback(static_cast<unsigned>(slot),event);
}
static int __cdecl FeedbackDamage(int attacker,int target,int factor) {
    const bool actor=target>=0 && target<=8;
    const int before=actor?InputField<int16_t>(0x86A542+target*0xF4):0;
    const int result=reinterpret_cast<int(__cdecl*)(int,int,int)>(g_feedback_damage_trampoline)(attacker,target,factor);
    if(actor && before>0 && InputField<int16_t>(0x86A542+target*0xF4)<before) {
        if(target<2)PlayerCombatFeedback(target,ff_feedback::Hurt);
        if(factor>0 && attacker>=0 && attacker<2 && attacker!=target)PlayerCombatFeedback(attacker,ff_feedback::Hit);
    }
    return result;
}
static bool InstallCombatFeedback() {
    // Observe completed native damage (attacker, target, factor), preserving its
    // side effects, return value and stack. This prologue has no relocations.
    const BYTE expected[]={0x53,0x8B,0x5C,0x24,0x0C};
    auto code=reinterpret_cast<BYTE*>(g_base+0x3C380);
    if(memcmp(code,expected,sizeof(expected)))return false;
    auto trampoline=static_cast<BYTE*>(VirtualAlloc(nullptr,10,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));
    if(!trampoline)return false;
    memcpy(trampoline,expected,5);trampoline[5]=0xE9;
    const LONG back=static_cast<LONG>(reinterpret_cast<uintptr_t>(code+5)-reinterpret_cast<uintptr_t>(trampoline+10));
    memcpy(trampoline+6,&back,4);
    DWORD old;
    if(!VirtualProtect(code,5,PAGE_EXECUTE_READWRITE,&old)){VirtualFree(trampoline,0,MEM_RELEASE);return false;}
    g_feedback_damage_trampoline=trampoline;code[0]=0xE9;
    const LONG offset=static_cast<LONG>(reinterpret_cast<uintptr_t>(FeedbackDamage)-reinterpret_cast<uintptr_t>(code+5));memcpy(code+1,&offset,4);
    DWORD ignored;VirtualProtect(code,5,old,&ignored);FlushInstructionCache(GetCurrentProcess(),trampoline,10);
    FlushInstructionCache(GetCurrentProcess(),code,5);
    Log("Controller combat feedback enabled for native damage and player guard.\r\n");return true;
}
