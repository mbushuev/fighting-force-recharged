#pragma once
// Native object contacts do not pass through character health damage. Observe
// the accepted dispatch result, without changing object durability or effects.
static thread_local int g_feedback_shooter=-1;
static int __cdecl FeedbackObjectContact(BYTE* obstacle,void* object,void* point,BYTE* actor) {
    int player=-1;
    for(int i=0;i<2;++i)if(actor==reinterpret_cast<BYTE*>(g_base+0x86A500+i*0xF4))player=i;
    if(!actor)player=g_feedback_shooter;
    const int kind=*reinterpret_cast<int16_t*>(obstacle+0xC);
    using Handler=int(__cdecl*)(BYTE*,void*,void*,BYTE*);
    const auto native=InputField<Handler>(0x86D240+kind*4);
    const int result=native(obstacle,object,point,actor);
    if(result>0 && player>=0 && player<2)PlayerCombatFeedback(player,ff_feedback::Hit);
    return result;
}
static int __cdecl FeedbackPlayerShot(void* projectile,int shooter) {
    // Scope attribution to actual firing sites. AI line-of-fire probes use the
    // same native ray routine and must never produce controller feedback.
    const int previous=g_feedback_shooter;g_feedback_shooter=shooter;
    const int result=reinterpret_cast<int(__cdecl*)(void*,int)>(g_base+0xDE60)(projectile,shooter);
    g_feedback_shooter=previous;return result;
}
static bool InstallPropFeedback() {
    const struct {DWORD rva;BYTE index;} contacts[]={
        {0x3846E,0x9D},{0x393C9,0x95},{0x393ED,0x95},{0x3C22D,0x95}};
    const DWORD shots[]={0xE270,0xE371,0xE398,0xE3BF};
    for(const auto& site:contacts) {
        const auto code=reinterpret_cast<const BYTE*>(g_base+site.rva);
        DWORD table;memcpy(&table,code+3,4);
        if(code[0]!=0xFF || code[1]!=0x14 || code[2]!=site.index || table!=g_base+0x86D240)return false;
    }
    for(DWORD rva:shots) {
        const auto code=reinterpret_cast<const BYTE*>(g_base+rva);LONG offset;memcpy(&offset,code+1,4);
        if(code[0]!=0xE8 || reinterpret_cast<uintptr_t>(code+5)+offset!=g_base+0xDE60)return false;
    }
    auto start=reinterpret_cast<BYTE*>(g_base+shots[0]);const SIZE_T size=0x3C234-shots[0];DWORD old;
    if(!VirtualProtect(start,size,PAGE_EXECUTE_READWRITE,&old))return false;
    const auto patch=[](DWORD rva,void* target) {
        auto code=reinterpret_cast<BYTE*>(g_base+rva);code[0]=0xE8;
        const LONG offset=static_cast<LONG>(reinterpret_cast<uintptr_t>(target)-reinterpret_cast<uintptr_t>(code+5));
        memcpy(code+1,&offset,4);
    };
    for(const auto& site:contacts) {
        patch(site.rva,reinterpret_cast<void*>(FeedbackObjectContact));
        memset(reinterpret_cast<void*>(g_base+site.rva+5),0x90,2);
    }
    for(DWORD rva:shots)patch(rva,reinterpret_cast<void*>(FeedbackPlayerShot));
    DWORD ignored;VirtualProtect(start,size,old,&ignored);FlushInstructionCache(GetCurrentProcess(),start,size);
    Log("Controller feedback enabled for accepted object impacts and player shots.\r\n");return true;
}
