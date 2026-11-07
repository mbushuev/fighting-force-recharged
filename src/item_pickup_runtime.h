#pragma once
#include "item_pickup.h"
static bool g_item_pickup_available;
static ff_pickup::Press g_item_pickup_press[2];
static ff_pickup::Press g_item_throw_press[2];
static SRWLOCK g_item_pickup_lock=SRWLOCK_INIT;
struct ItemPickupVisual {
    int slots[2]={-1,-1},types[2]={-1,-1},stage=-1;
    void* fixtures[2]={};void* objects[2]={};
};
static ItemPickupVisual g_item_pickup_visual;
static void* WorldPickupObject(void* fixture) {
    const auto address=reinterpret_cast<uintptr_t>(fixture),first=g_base+0x86DD20;
    const int count=InputField<int>(0x8689E8);
    if(count<=0 || count>512 || address<first || (address-first)%24 ||
        (address-first)/24>=unsigned(count) || !CameraReadable(fixture,24) ||
        !ff_pickup::WorldSide(GuardField<int16_t>(fixture,8)))return nullptr;
    auto object=GuardField<void*>(fixture,0x14);
    return CameraReadable(object,24) && ff_pickup::WorldWeapon(GuardField<int>(object,0))>=0?object:nullptr;
}
static void* FindWorldPickup(void* actor) {
    auto fixture=GuardField<void*>(actor,0x6C);
    if(!WorldPickupObject(fixture) ||
        !(GuardField<BYTE>(actor,0x66)&ff_pickup::WorldSide(GuardField<int16_t>(fixture,8))))return nullptr;
    const unsigned hero=GuardField<int16_t>(actor,0x34);if(hero>=4)return nullptr;
    auto table=InputField<void**>(0x879780+hero*4);
    return CameraReadable(table,0x83*sizeof(void*)) && table[0x82]?fixture:nullptr;
}
static void PublishItemPickup(unsigned player,int slot,void* fixture=nullptr) {
    auto object=fixture?WorldPickupObject(fixture):nullptr;if(fixture && !object){slot=-1;fixture=nullptr;}
    AcquireSRWLockExclusive(&g_item_pickup_lock);
    g_item_pickup_visual.slots[player]=slot;
    g_item_pickup_visual.types[player]=object?GuardField<int>(object,0):slot>=0?InputField<int>(0x870620+slot*36):-1;
    g_item_pickup_visual.fixtures[player]=fixture;g_item_pickup_visual.objects[player]=object;
    g_item_pickup_visual.stage=InputField<int>(0x85A6B8);
    ReleaseSRWLockExclusive(&g_item_pickup_lock);
}
static void ResetItemPickup() {
    for(auto& press:g_item_pickup_press)press={};
    for(auto& press:g_item_throw_press)press={};
    AcquireSRWLockExclusive(&g_item_pickup_lock);g_item_pickup_visual={};ReleaseSRWLockExclusive(&g_item_pickup_lock);
}
static bool ItemInteractionActive() {
    return g_item_pickup_available && (g_modern_keyboard || g_modern_gamepad) &&
        InputField<int>(0x85A668)!=1 && InputField<int>(0x85A6B8)>=0 && InputField<int>(0x85A6B8)<25;
}
static bool SeparatePickupActive(){return g_controls.separate_pickup && ItemInteractionActive();}
static bool ItemHandsOccupied(void* actor) {
    return GuardField<int8_t>(actor,0x7D)!=-1 || GuardField<int8_t>(actor,0x7E)!=-1;
}
static bool __cdecl ItemBlocksPunchPull(void* actor) {
    return ItemInteractionActive() && (g_controls.separate_pickup || ItemHandsOccupied(actor));
}
static bool __cdecl ItemBlocksSpin(void* actor) {
    return ItemInteractionActive() && ItemHandsOccupied(actor);
}
static DWORD ItemHeldSlot(void* actor,unsigned hand) {
    const int slot=GuardField<int8_t>(actor,hand);
    return slot>=0 && slot<112?0x870620+slot*36:0;
}
static int ItemType(DWORD slot) {return slot?InputField<int>(slot):-1;}
static bool ItemIsGrenade(int type) {return type==0x1F0 || type==0x1F1;}
static bool __cdecl ItemBlocksPunchThrow(int type) {
    return ItemInteractionActive() && (type>0x209 || ItemIsGrenade(type) || type==0x201);
}
static bool __cdecl ItemBlocksHeavyAttack(void* actor) {
    if(!ItemInteractionActive())return false;
    for(unsigned hand:{0x7Du,0x7Eu}) {
        if(ItemType(ItemHeldSlot(actor,hand))>0x209)return true;
    }
    return false;
}
static bool ItemActorReady(void* actor) {
    return unsigned(GuardField<int16_t>(actor,0x34))<4 && GuardField<int16_t>(actor,0x42)>0 &&
        !GuardField<int16_t>(actor,0x50) && GuardField<int8_t>(actor,0x78)==-1 &&
        GuardField<int8_t>(actor,0x7A)==-1 && !GuardField<int8_t>(actor,0x77);
}
static bool ItemPickupReady(void* actor) {
    return ItemActorReady(actor) && ff_pickup::ReadyAction(GuardField<int>(actor,0xC)) && !ItemHandsOccupied(actor);
}
static int ThrowHeldItem(void* actor,unsigned player) {
    const int pose=GuardField<int>(actor,0xC);
    if(!ItemActorReady(actor) || (!ff_pickup::ReadyAction(pose) && !(pose>=0x57 && pose<=0x5A)))return 0;
    const DWORD right=ItemHeldSlot(actor,0x7E);
    const int type=ItemType(right);
    const int action=ff_pickup::ThrowAction(type,ItemType(ItemHeldSlot(actor,0x7D)));if(action<0)return 0;
    const unsigned hero=GuardField<int16_t>(actor,0x34);auto table=InputField<void**>(0x879780+hero*4);
    if(!CameraReadable(table,0xA4*sizeof(void*)) || !table[action])return 0;
    // Native release frames retain projectile velocity, damage and ownership.
    // The original left-hand fallback drops items without a throwing pose.
    if(action==0x45)reinterpret_cast<void(__cdecl*)(unsigned)>(g_base+0x32330)(player);
    reinterpret_cast<int(__cdecl*)(void*,void**,int)>(g_base+0x4E320)(actor,table,action);
    // Reproduce the original grenade throw's timer and pin effect.
    if(ItemIsGrenade(type) && !InputField<int16_t>(right+0x16)) {
        InputField<int16_t>(right+0x16)=270;
        reinterpret_cast<void(__cdecl*)(unsigned,int)>(g_base+0xEFD0)(player,0x241);
    }
    reinterpret_cast<void(__cdecl*)(int)>(g_base+0x49600)(player);return 1;
}
static int FindItemPickup(void* actor) {
    const unsigned heading=GuardField<uint16_t>(actor,0x38)&2047;
    const ff_pickup::Actor player={GuardField<int32_t>(actor,0x28),GuardField<int32_t>(actor,0x30),
        GuardField<int16_t>(actor,0x34),InputField<int16_t>(0x67F50+heading*2),InputField<int16_t>(0x68350+heading*2)};
    // Match the native scanner's slot order, range and heavy-item restrictions.
    // Consumables remain in its automatic health/score pass, not this action.
    for(unsigned i=0;i<64;++i) {
        const DWORD offset=0x870620+i*36;
        const ff_pickup::Item item={InputField<int>(offset),InputField<int32_t>(offset+4),
            InputField<int32_t>(offset+8),InputField<int32_t>(offset+12),InputField<int16_t>(offset+22)};
        if(ff_pickup::Reachable(player,item))return int(i);
    }
    return -1;
}
static int UpdateItemPickup(void* actor,unsigned player,bool guarded) {
    if(player>1 || actor!=GuardActor(player))return 0;
    const bool active=ItemInteractionActive() && ModernHasFocus() && !ControlInputBlocked() &&
        InputField<int>(0x85A668)!=1 && GuardControlUnlocked();
    const bool coop=InputField<int>(0x8745A0)==2;
    const auto keys=!coop || ModernKeyboardPlayer()==int(player)?ReadModernPlayerKeyboard(player):ff_input::Keyboard{};
    const auto pad=coop?ReadModernCoopPlayerPad(player):MapModernPad(ReadModernPad(),0);
    const bool pressed=g_item_pickup_press[player].Read(keys.pickup || pad.pickup,active && g_controls.separate_pickup,
        InputField<int>(0x85A6B8),InputField<int>(0x85B560),g_modern_focus_epoch,GetTickCount64());
    const bool thrown=g_item_throw_press[player].Read(keys.throw_item || pad.throw_item,active,
        InputField<int>(0x85A6B8),InputField<int>(0x85B560),g_modern_focus_epoch,GetTickCount64());
    const bool ready=active && !guarded && ItemPickupReady(actor);
    auto fixture=ready && g_controls.separate_pickup?FindWorldPickup(actor):nullptr;
    const int slot=fixture?-2:ready && g_controls.separate_pickup?FindItemPickup(actor):-1;
    PublishItemPickup(player,slot,fixture);
    const unsigned combat=ff_input::Punch|ff_input::Kick|ff_input::Jump|ff_input::Backfist|ff_input::Run;
    if(thrown && active && !guarded && !(InputField<unsigned>(0x5523E0+player*4)&(combat&~ff_input::Run))) {
        const int result=ThrowHeldItem(actor,player);if(result)PublishItemPickup(player,-1);return result;
    }
    if(!pressed || (slot<0 && !fixture) || (InputField<unsigned>(0x5523E0+player*4)&combat))return 0;
    const unsigned hero=GuardField<int16_t>(actor,0x34);
    auto table=InputField<void**>(0x879780+hero*4);
    const int action=fixture?0x82:ff_pickup::Action(InputField<int>(0x870620+slot*36));
    if(action<0 || !CameraReadable(table,0x83*sizeof(void*)) || !table[action])return 0;
    // Use the original pose selector and input-buffer flush. Its normal pickup
    // frames still attach the object, resolve competing hands and handle damage.
    if(!fixture)GuardField<int8_t>(actor,0x83)=static_cast<int8_t>(slot);
    reinterpret_cast<int(__cdecl*)(void*,void**,int)>(g_base+0x4E320)(actor,table,action);
    reinterpret_cast<void(__cdecl*)(int)>(g_base+0x49600)(player);
    PublishItemPickup(player,-1);return 1;
}
static void __cdecl ItemPickupPunchScan(unsigned player,int radius) {
    reinterpret_cast<void(__cdecl*)(unsigned,int)>(g_base+0x32430)(player,radius);
    if(player<2 && SeparatePickupActive())InputField<int>(0x8776E0+player*4)=-1;
}
static uintptr_t g_item_native_drop,g_item_after_drop;
static uintptr_t g_item_native_pull,g_item_after_pull;
static void __declspec(naked) ItemPunchPullBoundary() {
    __asm {
        pushfd
        pushad
        push esi
        call ItemBlocksPunchPull
        add esp,4
        test al,al
        jz native_pull
        popad
        popfd
        add esp,8
        jmp dword ptr [g_item_after_pull]
    native_pull:
        popad
        popfd
        mov al,[esi+66h]
        add esp,8
        jmp dword ptr [g_item_native_pull]
    }
}
static void __declspec(naked) ItemCombatDropBoundary() {
    __asm {
        pushfd
        pushad
        call ItemInteractionActive
        test al,al
        jz native_drop
        popad
        popfd
        mov edi,[esp+2Ch]
        jmp dword ptr [g_item_after_drop]
    native_drop:
        popad
        popfd
        mov edi,[esp+2Ch]
        cmp edi,-1
        jmp dword ptr [g_item_native_drop]
    }
}
static uintptr_t g_item_native_jump,g_item_after_jump,g_item_native_kick,g_item_after_kick;
static void __declspec(naked) ItemHeavyJumpBoundary() {
    __asm {
        pushfd
        pushad
        push esi
        call ItemBlocksHeavyAttack
        add esp,4
        test al,al
        jnz skip_jump
        popad
        popfd
        push ecx
        mov ecx,dword ptr [g_base]
        mov al,[ecx+edx+871851h]
        pop ecx
        jmp dword ptr [g_item_native_jump]
    skip_jump:
        popad
        popfd
        jmp dword ptr [g_item_after_jump]
    }
}
static void __declspec(naked) ItemKickBoundary() {
    __asm {
        pushfd
        pushad
        push esi
        call ItemBlocksHeavyAttack
        add esp,4
        test al,al
        jnz skip_kick
        popad
        popfd
        mov eax,[esp+24h]
        push edx
        mov edx,dword ptr [g_base]
        mov cl,[edx+eax+871852h]
        pop edx
        jmp dword ptr [g_item_native_kick]
    skip_kick:
        popad
        popfd
        jmp dword ptr [g_item_after_kick]
    }
}
static uintptr_t g_item_native_punch,g_item_after_punch;
static void __declspec(naked) ItemPunchThrowBoundary() {
    __asm {
        pushad
        movsx eax,ax
        push eax
        call ItemBlocksPunchThrow
        add esp,4
        test al,al
        jnz skip_throw
        popad
        cmp ax,209h
        mov edi,dword ptr [g_base]
        lea edi,[edi+ecx*4+870620h]
        jmp dword ptr [g_item_native_punch]
    skip_throw:
        popad
        jmp dword ptr [g_item_after_punch]
    }
}
static uintptr_t g_item_native_ground_spin,g_item_after_ground_spin;
static uintptr_t g_item_native_air_spin,g_item_after_air_spin;
static void __declspec(naked) ItemGroundSpinBoundary() {
    __asm {
        jle skip_spin
        pushad
        push esi
        call ItemBlocksSpin
        add esp,4
        test al,al
        popad
        jnz skip_spin
        jmp dword ptr [g_item_native_ground_spin]
    skip_spin:
        jmp dword ptr [g_item_after_ground_spin]
    }
}
static void __declspec(naked) ItemAirSpinBoundary() {
    __asm {
        jle skip_spin
        pushad
        push esi
        call ItemBlocksSpin
        add esp,4
        test al,al
        popad
        jnz skip_spin
        jmp dword ptr [g_item_native_air_spin]
    skip_spin:
        jmp dword ptr [g_item_after_air_spin]
    }
}
static bool InstallItemPickup() {
    const BYTE hand[]={0x8A,0x46,0x7E,0x3C,0xFF};
    const BYTE heavy[]={0x3D,0x09,0x02,0,0,0x7E,0x04,0x6A,0x79};
    const BYTE scanner[]={0x83,0xEC,0x24,0x8B,0x4C,0x24,0x28,0x53};
    const BYTE drop[]={0x8B,0x7C,0x24,0x2C,0x83,0xFF,0xFF};
    const BYTE jump[]={0x8A,0x82,0x51,0x18,0xC7,0};
    const BYTE kick[]={0x8B,0x44,0x24,0x24,0x8A,0x88,0x52,0x18,0xC7,0x00};
    const BYTE pull[]={0x8A,0x46,0x66,0x83,0xC4,0x08};
    const BYTE punch[]={0x66,0x3D,0x09,0x02,0x8D,0x3C,0x8D,0x20,0x06,0xC7,0};
    const BYTE air_spin[]={0x0F,0x8E,0x89,0x06,0,0};
    const BYTE ground_spin[]={0x0F,0x8E,0x74,0xD9,0xFF,0xFF};
    const BYTE detach[]={0x8B,0x44,0x24,0x04,0x83,0xEC,0x10};
    const struct {DWORD rva;const BYTE* bytes;SIZE_T size;void (*target)();} jumps[]={
        {0x4F638,drop,sizeof(drop),ItemCombatDropBoundary},
        {0x4F830,jump,sizeof(jump),ItemHeavyJumpBoundary},
        {0x4F8E9,kick,sizeof(kick),ItemKickBoundary},
        {0x4FB26,pull,sizeof(pull),ItemPunchPullBoundary},
        {0x4FC1B,punch,sizeof(punch),ItemPunchThrowBoundary},
        {0x50B0D,air_spin,sizeof(air_spin),ItemAirSpinBoundary},
        {0x51F6B,ground_spin,sizeof(ground_spin),ItemGroundSpinBoundary}
    };
    auto site=reinterpret_cast<BYTE*>(g_base+0x4FB21);LONG relative;memcpy(&relative,site+1,4);
    if(!g_player_guard_available || site[0]!=0xE8 || g_base+0x4FB26+relative!=g_base+0x32430 ||
        memcmp(reinterpret_cast<void*>(g_base+0x4FB67),hand,sizeof(hand)) ||
        memcmp(reinterpret_cast<void*>(g_base+0x4FBB1),heavy,sizeof(heavy)) ||
        memcmp(reinterpret_cast<void*>(g_base+0x32430),scanner,sizeof(scanner)) ||
        memcmp(reinterpret_cast<void*>(g_base+0x11BB0),detach,sizeof(detach)))return false;
    for(const auto& patch:jumps)if(memcmp(reinterpret_cast<void*>(g_base+patch.rva),patch.bytes,patch.size))return false;
    auto start=reinterpret_cast<BYTE*>(g_base+0x4F638);const SIZE_T length=0x51F71-0x4F638;
    DWORD previous;if(!VirtualProtect(start,length,PAGE_EXECUTE_READWRITE,&previous))return false;
    g_item_native_drop=g_base+0x4F63F;g_item_after_drop=g_base+0x4F823;
    g_item_native_jump=g_base+0x4F836;g_item_after_jump=g_base+0x4F8E5;
    g_item_native_kick=g_base+0x4F8F3;g_item_after_kick=g_base+0x4F9F6;
    g_item_native_pull=g_base+0x4FB2C;g_item_after_pull=g_base+0x4FB67;
    g_item_native_punch=g_base+0x4FC26;g_item_after_punch=g_base+0x4FF1F;
    g_item_native_air_spin=g_base+0x50B13;g_item_after_air_spin=g_base+0x5119C;
    g_item_native_ground_spin=g_base+0x51F71;g_item_after_ground_spin=g_base+0x4F8E5;
    for(const auto& patch:jumps) {
        auto at=reinterpret_cast<BYTE*>(g_base+patch.rva);at[0]=0xE9;
        relative=static_cast<LONG>(reinterpret_cast<uintptr_t>(patch.target)-(g_base+patch.rva+5));
        memcpy(at+1,&relative,4);memset(at+5,0x90,patch.size-5);
    }
    relative=static_cast<LONG>(reinterpret_cast<uintptr_t>(ItemPickupPunchScan)-(g_base+0x4FB26));memcpy(site+1,&relative,4);
    DWORD ignored;VirtualProtect(start,length,previous,&ignored);FlushInstructionCache(GetCurrentProcess(),start,length);
    g_item_pickup_available=true;g_player_pickup_update=UpdateItemPickup;ResetItemPickup();
    Log("Separate item pickup and dedicated item throw/drop available.\r\n");return true;
}
