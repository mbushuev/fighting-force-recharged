#pragma once
#include "guard_retarget.h"
#include "player_guard.h"

static bool g_player_guard_available;
static bool g_guard_assets_attempted[4],g_guard_assets_ready[4];
static ff_guard::GuardClips g_guard_clips[4];
static const unsigned g_guard_joints[]={20,30,27,32};
static ff_guard::GuardState g_guard_states[2];
static ff_guard::AttackWindow g_guard_attacks[8];
static int g_guard_stage=-1,g_guard_tick=-1;
static ULONGLONG g_guard_time;
static unsigned g_guard_focus_epoch;
static uintptr_t g_guard_update_native,g_guard_update_skip,g_guard_hit_native,g_guard_hit_success,g_guard_hit_repeat;
template<typename T> static T& GuardField(void* actor,unsigned offset){return *reinterpret_cast<T*>(static_cast<BYTE*>(actor)+offset);}
static void* GuardActor(unsigned slot){return reinterpret_cast<void*>(g_base+0x86A500+slot*0xF4);}
static unsigned GuardClipIndex(int action){return action==ff_guard::Entry?0:action==ff_guard::Hold?1:action==ff_guard::Impact?2:3;}
static std::vector<uint8_t> ReadGuardArchive(const wchar_t* name,const BYTE (&hash)[32]) {
    wchar_t path[MAX_PATH]={};const DWORD length_name=GetModuleFileNameW(nullptr,path,MAX_PATH);
    if(!length_name || length_name>=MAX_PATH)throw std::runtime_error("Guard folder unavailable");
    auto slash=wcsrchr(path,L'\\');if(!slash)throw std::runtime_error("Guard folder unavailable");*slash=0;
    if(wcscat_s(path,L"\\Wads\\") || wcscat_s(path,name))throw std::runtime_error("Guard path too long");
    struct File {HANDLE value=INVALID_HANDLE_VALUE;~File(){if(value!=INVALID_HANDLE_VALUE)CloseHandle(value);}} file;
    // Keep the same archive locked against writes throughout verification/read.
    file.value=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file.value==INVALID_HANDLE_VALUE || !ff_image::FileHasHash(path,hash))throw std::runtime_error("Unreviewed guard archive");
    LARGE_INTEGER length;if(!GetFileSizeEx(file.value,&length) || length.QuadPart<4 || length.QuadPart>32*1024*1024)
        throw std::runtime_error("Invalid guard archive size");
    std::vector<uint8_t> bytes(static_cast<size_t>(length.QuadPart));DWORD read=0;
    if(!ReadFile(file.value,bytes.data(),static_cast<DWORD>(bytes.size()),&read,nullptr) || read!=bytes.size())
        throw std::runtime_error("Cannot read guard archive");return bytes;
}
static bool PreparePlayerGuard(unsigned hero) {
    if(hero>=4)return false;
    if(g_guard_assets_attempted[hero])return g_guard_assets_ready[hero];g_guard_assets_attempted[hero]=true;
    try {
        const BYTE hero_hash[][32]={
            {0x0C,0xBE,0x8B,0xE8,0x2E,0x68,0x30,0xFE,0xE3,0xED,0x05,0x6E,0xB8,0x2D,0x3D,0xF3,0x32,0x4B,0xE6,0xAE,0xAE,0xE0,0x82,0xDE,0xBB,0xC7,0x93,0x3C,0xBA,0x2B,0x1F,0x8E},
            {0x79,0x67,0x9F,0x5F,0xE6,0x91,0x60,0x3D,0x78,0xAF,0x14,0x97,0x29,0xBC,0xAC,0x9F,0xAE,0x9E,0x31,0xB6,0x8C,0x68,0xFA,0x9A,0x68,0x0B,0xA3,0xC1,0xBC,0x22,0x1E,0x4C},
            {0xFF,0x1B,0x26,0x44,0xA1,0x2D,0x27,0x20,0xE0,0xC8,0xC4,0x9C,0x96,0xAA,0xFA,0xC0,0x93,0x2C,0xE4,0xE5,0x7B,0x2D,0xCE,0xD3,0x0E,0xF8,0x21,0x09,0xBA,0xF3,0x80,0x4B},
            {0x96,0xA1,0x92,0x5B,0xD4,0xF1,0x24,0x27,0x44,0x06,0xBD,0xAB,0xE3,0x78,0x2B,0xB7,0x02,0x55,0xD8,0x7C,0x0E,0x43,0x2C,0x1F,0x32,0x2E,0xE3,0x6E,0x0E,0x9D,0xF6,0xDC}};
        const BYTE donor_hash[][32]={
            {0xE8,0x9F,0x21,0x1C,0x9A,0xCD,0x11,0x99,0x3F,0x28,0x3A,0x04,0x6E,0xD1,0x0F,0x9B,0xAD,0xD0,0xCE,0x84,0x10,0x37,0x6B,0x93,0xA9,0xC9,0xA9,0x3A,0x0E,0x21,0x8E,0x3D},
            {0x60,0xB4,0xFE,0x94,0x76,0xCA,0xAA,0x2C,0xC3,0xD2,0x7D,0x39,0x6B,0xA4,0x4F,0x59,0x2F,0x9E,0xFA,0xA9,0x44,0x99,0x29,0x52,0x34,0xC5,0xF6,0xAC,0x0C,0xDA,0xE7,0x6A}};
        const wchar_t* names[]={L"HAWK.WAD",L"MACE.WAD",L"SMASHER.WAD",L"ALANA.WAD"};
        const char* models[]={"HAWK.ACH","MACE.ACH","SMASHER.ACH","ALANA.ACH"};
        const char* animations[]={"HAWK.ADT","MACE.ADT","SMASHER.ADT","ALANA.ADT"};
        const bool female=hero==1 || hero==3;
        auto original=ReadGuardArchive(names[hero],hero_hash[hero]);
        auto donor=ReadGuardArchive(female?L"CHARS15.WAD":L"CHARS11.WAD",donor_hash[female?1:0]);
        const ff_guard::Bytes target={original.data(),original.size()},source={donor.data(),donor.size()};
        if(hero==2) {ff_guard::SmasherClips clips;clips.Build(target,source);g_guard_clips[hero]=std::move(clips);}
        else {
            ff_guard::UpperBodyClips clips;
            clips.Build(target,source,models[hero],animations[hero],female?"VAMP.ACH":"SUIT.ACH",
                female?"GENFMALE.ADT":"GENMALE.ADT",g_guard_joints[hero],female?12:8);
            g_guard_clips[hero]=std::move(clips);
        }
        g_guard_assets_ready[hero]=true;char message[96];
        sprintf_s(message,"Player guard: validated private clips prepared for hero %u.\r\n",hero);Log(message);
    } catch(const std::exception&) {Log("Player guard unavailable: original archive validation/conversion failed.\r\n");}
    return g_guard_assets_ready[hero];
}
static bool GuardOwns(void* actor) {
    const int action=GuardField<int>(actor,0xC);const unsigned hero=GuardField<int16_t>(actor,0x34);
    return hero<4 && g_guard_assets_ready[hero] && ff_guard::GuardAction(action) &&
        GuardField<void*>(actor,0)==g_guard_clips[hero].clips[GuardClipIndex(action)].data();
}
static bool AttachPlayerGuard(void* actor) {
    const unsigned hero=GuardField<int16_t>(actor,0x34);if(!PreparePlayerGuard(hero))return false;
    auto& clips=g_guard_clips[hero];
    auto model=InputField<const FirstPersonModel*>(0x85A6F8+hero*4);
    auto table=InputField<void**>(0x879780+hero*4);
    if(!CameraReadable(model,sizeof(*model)) || model->joints!=g_guard_joints[hero] || !CameraReadable(table,0x121*sizeof(void*)))return false;
    const int actions[]={ff_guard::Entry,ff_guard::Hold,ff_guard::Impact,ff_guard::Exit};
    for(unsigned i=0;i<4;++i) {
        if(InputField<uint16_t>(0x8674C0+actions[i]*2)!=0)return false;
        if(table[actions[i]] && table[actions[i]]!=clips.clips[i].data())return false;
    }
    for(unsigned i=0;i<4;++i)table[actions[i]]=clips.clips[i].data();return true;
}
static void GuardSetAction(void* actor,ff_guard::Phase phase) {
    const unsigned clip=GuardClipIndex(phase);auto& clips=g_guard_clips[GuardField<int16_t>(actor,0x34)];
    GuardField<void*>(actor,0)=clips.clips[clip].data();GuardField<int>(actor,4)=0;
    GuardField<int>(actor,0xC)=phase;GuardField<int>(actor,0x10)=clips.height[clip];
    GuardField<int>(actor,0x14)=GuardField<int>(actor,0x18)=GuardField<int>(actor,0x1C)=GuardField<int>(actor,0x20)=0;
    GuardField<int16_t>(actor,0x52)=GuardField<int16_t>(actor,0x54)=0;
}
static void GuardRestoreIdle(void* actor) {
    const unsigned hero=GuardField<int16_t>(actor,0x34);if(hero>=4)return;
    auto table=InputField<void**>(0x879780+hero*4);
    // The original selector maps idle to the current firearm/prop stance.
    // Keep both hand references intact throughout guard and its release.
    if(CameraReadable(table,0x46*sizeof(void*)) && table[0x45])
        reinterpret_cast<int(__cdecl*)(void*,void**,int)>(g_base+0x4E320)(actor,table,0x45);
}
static bool GuardLoadAllowed(void* actor) {
    for(unsigned offset:{0x7Du,0x7Eu}) {
        const int slot=GuardField<int8_t>(actor,offset);
        if(slot==-1)continue;
        if(slot<0 || slot>=112)return false;
        const int type=InputField<int>(0x870620+slot*36);
        // Native pickup types start after 0x1DC (RVA 0x3255E). Types above
        // 0x209 use the heavy carry stance and cannot run (RVA 0x4E51D).
        // Apply that restriction to either hand, including the wheel (0x210).
        if(type<=0x1DC || type>0x209)return false;
    }
    return true;
}
static void ObserveGuardAttacker(unsigned slot) {
    auto actor=GuardActor(slot);g_guard_attacks[slot].Observe(GuardField<uintptr_t>(actor,0),GuardField<int>(actor,0xC),
        GuardField<int>(actor,4),GuardField<unsigned>(actor,0x1C));
}
static void ResetPlayerGuard() {
    for(unsigned p=0;p<2;++p) {if(GuardOwns(GuardActor(p)))GuardRestoreIdle(GuardActor(p));g_guard_states[p]={};}
    for(auto& attack:g_guard_attacks)attack={};
    g_guard_stage=g_guard_tick=-1;g_guard_time=0;
    g_guard_focus_epoch=g_modern_focus_epoch;
}
static void __cdecl GuardPlayerTick() {
    if(!g_controls.guard) {reinterpret_cast<void(__cdecl*)()>(g_base+0x4E970)();return;}
    const int stage=InputField<int>(0x85A6B8),tick=InputField<int>(0x85B560);const auto time=GetTickCount64();
    const bool reset=stage!=g_guard_stage || tick<=g_guard_tick || g_guard_focus_epoch!=g_modern_focus_epoch ||
        (g_guard_time && time-g_guard_time>500);
    if(reset)ResetPlayerGuard();
    g_guard_stage=stage;g_guard_tick=tick;g_guard_time=time;
    for(unsigned i=2;i<8;++i)ObserveGuardAttacker(i);
    reinterpret_cast<void(__cdecl*)()>(g_base+0x4E970)();
}
static bool GuardControlUnlocked() {
    const int tick=InputField<int>(0x85B560);
    return InputField<int>(0x85DE60)<=0 &&
        !(InputField<int>(0x85A6BC)==1 && tick<60) && !(InputField<int>(0x85A6B8)==4 && tick<660);
}
static int __cdecl UpdatePlayerGuard(void* actor,unsigned player) {
    if(!g_controls.guard || player>1 || actor!=GuardActor(player))return 0;
    auto& state=g_guard_states[player];
    const bool live=InputField<int>(0x85A6B8)>=0 && InputField<int>(0x85A6B8)<25 &&
        InputField<int>(0x85A668)!=1;
    const bool eligible=live && unsigned(GuardField<int16_t>(actor,0x34))<4 && GuardField<int16_t>(actor,0x42)>0 &&
        GuardLoadAllowed(actor) &&
        GuardField<int8_t>(actor,0x78)<0 && GuardField<int8_t>(actor,0x7A)<0 && !GuardField<int8_t>(actor,0x77);
    const bool owned=GuardOwns(actor);
    if(!eligible) {if(owned)GuardRestoreIdle(actor);state={};return 0;}
    const bool focus=ModernHasFocus() && !ControlInputBlocked();
    const bool coop=InputField<int>(0x8745A0)==2;
    const bool keyboard=!coop || ModernKeyboardPlayer()==static_cast<int>(player);
    const auto keys=keyboard?ReadModernPlayerKeyboard(player):ff_input::Keyboard{};
    const auto pad=coop?ReadModernCoopPlayerPad(player):MapModernPad(ReadModernPad(),0);
    const bool down=focus && (keys.block || (pad.buttons&ff_input::LeftShoulder));
    const bool unlocked=GuardControlUnlocked();
    const bool can_start=ff_guard::StartAction(GuardField<int>(actor,0xC)) && !GuardField<int16_t>(actor,0x50);
    if(down && can_start && !AttachPlayerGuard(actor))return 0;
    const auto previous=state.phase;const bool finished=GuardField<int>(actor,4)==-1;
    const auto next=state.Tick(down,focus && unlocked,owned,finished,can_start);
    if(next==ff_guard::None) {if(owned)GuardRestoreIdle(actor);return 0;}
    if(next!=previous || finished)GuardSetAction(actor,next);
    // Keep collision/gravity/common pose processing, but do not run native
    // player action selection on an enemy-only action or retain queued attacks.
    if(focus && unlocked) {
        CameraPlayerScope scope(actor);
        GuardField<int16_t>(actor,0x38)=static_cast<int16_t>(FirstPersonActive()?CameraFirstPersonView().BodyHeading():
            ff_guard::Turn(GuardField<int16_t>(actor,0x38),CooperativeHeading(actor),InputField<uint32_t>(0x5523E0+player*4)));
    }
    memset(reinterpret_cast<void*>(g_base+0x871850+player*6),0,6);
    InputField<uint32_t>(0x5523E0+player*4)&=0xFFFF0000;
    GuardField<int>(actor,0x14)=GuardField<int>(actor,0x18)=0;
    GuardField<int16_t>(actor,0x3E)=static_cast<int16_t>(GuardField<int16_t>(actor,0x38)*32);
    return 1;
}
static int __cdecl GuardMeleeContact(unsigned attacker,unsigned player,int contact_y) {
    if(!g_controls.guard || attacker<2 || attacker>=8 || player>1)return 0;
    if(!ModernHasFocus() || g_guard_focus_epoch!=g_modern_focus_epoch || !GuardControlUnlocked() ||
       g_controls_menu_active || g_controls_wait_release || InputField<int>(0x85A668)==1)return 0;
    auto target=GuardActor(player),source=GuardActor(attacker);auto& state=g_guard_states[player];
    if(!state.Protects() || !GuardOwns(target) || !GuardLoadAllowed(target) || GuardField<int16_t>(target,0x42)<=0 ||
       !ff_guard::OrdinaryMelee(GuardField<int>(source,0xC)) ||
       GuardField<int8_t>(source,0x7D)>=0 || GuardField<int8_t>(source,0x7E)>=0 ||
       GuardField<int8_t>(target,0x78)>=0 || GuardField<int8_t>(target,0x7A)>=0)return 0;
    const double dx=double(GuardField<int>(source,0x28))-GuardField<int>(target,0x28);
    const double dz=double(GuardField<int>(source,0x30))-GuardField<int>(target,0x30);
    if(!ff_guard::Frontal(GuardField<int16_t>(target,0x38),dx,dz))return 0;
    ObserveGuardAttacker(attacker);
    if(!g_guard_attacks[attacker].First(player))return 2;
    // First playable policy: no damage/knockback for ordinary frontal melee.
    // Incoming weapons, rear hits, throws, grabs and specials stay native.
    if(state.phase!=ff_guard::Impact || GuardField<int>(target,4)==-1)GuardSetAction(target,ff_guard::Impact);
    state.phase=ff_guard::Impact;
    reinterpret_cast<void(__cdecl*)(int,int,int,int)>(g_base+0x31370)(0x362,GuardField<int>(target,0x28),contact_y,GuardField<int>(target,0x30));
    return 1;
}
static void __declspec(naked) GuardUpdateBoundary() {
    __asm {
        pushfd
        pushad
        mov eax,[esp+38h] // native player local at +14h, plus 36 saved bytes
        push eax
        push esi
        call UpdatePlayerGuard
        add esp,8
        test eax,eax
        jnz guarding
        popad
        popfd
        mov ecx,[esp+28h]
        mov edi,[esi+0Ch]
        jmp dword ptr [g_guard_update_native]
    guarding:
        popad
        popfd
        mov dword ptr [esp+28h],0
        jmp dword ptr [g_guard_update_skip]
    }
}
static void __declspec(naked) GuardHitBoundary() {
    __asm {
        pushfd
        pushad
        mov eax,[esp+38h] // contact height local at +14h
        push eax
        push edi
        push ebx
        call GuardMeleeContact
        add esp,12
        cmp eax,2
        je repeated
        test eax,eax
        jnz blocked
        popad
        popfd
        mov ecx,[ebp+30h]
        mov edx,[ebp+28h]
        jmp dword ptr [g_guard_hit_native]
    blocked:
        popad
        popfd
        jmp dword ptr [g_guard_hit_success]
    repeated:
        popad
        popfd
        jmp dword ptr [g_guard_hit_repeat]
    }
}
static bool InstallPlayerGuard() {
    // Install inert hooks even when disabled, so the menu can enable guard
    // without restarting. Disabled hooks always take the native paths.
    const BYTE update[]={0x8B,0x4C,0x24,0x28,0x8B,0x7E,0x0C},hit[]={0x8B,0x4D,0x30,0x8B,0x55,0x28};
    const BYTE next_target[]={0x47,0x81,0xC6,0xF4,0,0,0,0x83,0xFF,2,0x0F,0x8C,8,0xFF,0xFF,0xFF};
    auto tick=reinterpret_cast<BYTE*>(g_base+0x411C8);LONG relative;memcpy(&relative,tick+1,4);
    if(tick[0]!=0xE8 || reinterpret_cast<uintptr_t>(tick+5)+relative!=g_base+0x4E970 ||
       memcmp(reinterpret_cast<void*>(g_base+0x4EE8B),update,sizeof(update)) ||
       memcmp(reinterpret_cast<void*>(g_base+0x2B1BB),hit,sizeof(hit)) ||
       memcmp(reinterpret_cast<void*>(g_base+0x2B19D),next_target,sizeof(next_target)))return false;
    g_guard_update_native=g_base+0x4EE92;g_guard_update_skip=g_base+0x51EFF;
    // A repeated blocked contact consumes only this defender. Preserve the
    // native search for the other player, including their normal damage path.
    g_guard_hit_native=g_base+0x2B1C1;g_guard_hit_success=g_base+0x2B4B8;g_guard_hit_repeat=g_base+0x2B19D;
    auto start=reinterpret_cast<BYTE*>(g_base+0x2B1BB);const SIZE_T length=0x4EE92-0x2B1BB;DWORD previous;
    if(!VirtualProtect(start,length,PAGE_EXECUTE_READWRITE,&previous))return false;
    auto branch=[](DWORD rva,void* target,unsigned size,BYTE opcode) {
        auto code=reinterpret_cast<BYTE*>(g_base+rva);memset(code,0x90,size);code[0]=opcode;
        const LONG displacement=static_cast<LONG>(reinterpret_cast<uintptr_t>(target)-reinterpret_cast<uintptr_t>(code+5));memcpy(code+1,&displacement,4);
    };
    branch(0x411C8,reinterpret_cast<void*>(GuardPlayerTick),5,0xE8);
    branch(0x4EE8B,reinterpret_cast<void*>(GuardUpdateBoundary),7,0xE9);
    branch(0x2B1BB,reinterpret_cast<void*>(GuardHitBoundary),6,0xE9);
    DWORD ignored;VirtualProtect(start,length,previous,&ignored);FlushInstructionCache(GetCurrentProcess(),start,length);
    g_player_guard_available=true;Log("Optional player guard hooks available; Combat/PlayerGuard controls activation.\r\n");return true;
}
