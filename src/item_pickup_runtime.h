#pragma once
#include "item_pickup.h"
static bool g_item_pickup_available;
static ff_pickup::Press g_item_pickup_press[2];
static SRWLOCK g_item_pickup_lock=SRWLOCK_INIT;
struct ItemPickupVisual {int slots[2]={-1,-1},types[2]={-1,-1},stage=-1;};
static ItemPickupVisual g_item_pickup_visual;
static void PublishItemPickup(unsigned player,int slot) {
    AcquireSRWLockExclusive(&g_item_pickup_lock);
    g_item_pickup_visual.slots[player]=slot;
    g_item_pickup_visual.types[player]=slot>=0?InputField<int>(0x870620+slot*36):-1;
    g_item_pickup_visual.stage=InputField<int>(0x85A6B8);
    ReleaseSRWLockExclusive(&g_item_pickup_lock);
}
static void ResetItemPickup() {
    for(auto& press:g_item_pickup_press)press={};
    AcquireSRWLockExclusive(&g_item_pickup_lock);g_item_pickup_visual={};ReleaseSRWLockExclusive(&g_item_pickup_lock);
}
static bool SeparatePickupActive() {
    return g_item_pickup_available && g_controls.separate_pickup && (g_modern_keyboard || g_modern_gamepad) &&
        InputField<int>(0x85A668)!=1 && InputField<int>(0x85A6B8)>=0 && InputField<int>(0x85A6B8)<25;
}
static bool ItemPickupReady(void* actor) {
    return unsigned(GuardField<int16_t>(actor,0x34))<4 && GuardField<int16_t>(actor,0x42)>0 &&
        ff_pickup::ReadyAction(GuardField<int>(actor,0xC)) && !GuardField<int16_t>(actor,0x50) &&
        GuardField<int8_t>(actor,0x7D)==-1 && GuardField<int8_t>(actor,0x7E)==-1 &&
        GuardField<int8_t>(actor,0x78)==-1 && GuardField<int8_t>(actor,0x7A)==-1 &&
        !GuardField<int8_t>(actor,0x77) && !(GuardField<BYTE>(actor,0x66)&15);
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
    const bool active=SeparatePickupActive() && ModernHasFocus() && !ControlInputBlocked() &&
        InputField<int>(0x85A668)!=1 && GuardControlUnlocked();
    const bool coop=InputField<int>(0x8745A0)==2;
    const auto keys=!coop || ModernKeyboardPlayer()==int(player)?ReadModernPlayerKeyboard(player):ff_input::Keyboard{};
    const auto pad=coop?ReadModernCoopPlayerPad(player):MapModernPad(ReadModernPad(),0);
    const bool pressed=g_item_pickup_press[player].Read(keys.pickup || pad.pickup,active,
        InputField<int>(0x85A6B8),InputField<int>(0x85B560),g_modern_focus_epoch,GetTickCount64());
    const bool ready=active && !guarded && ItemPickupReady(actor);
    const int slot=ready?FindItemPickup(actor):-1;PublishItemPickup(player,slot);
    const unsigned combat=ff_input::Punch|ff_input::Kick|ff_input::Jump|ff_input::Backfist|ff_input::Run;
    if(!pressed || slot<0 || (InputField<unsigned>(0x5523E0+player*4)&combat))return 0;
    const unsigned hero=GuardField<int16_t>(actor,0x34);
    auto table=InputField<void**>(0x879780+hero*4);
    const int action=ff_pickup::Action(InputField<int>(0x870620+slot*36));
    if(action<0 || !CameraReadable(table,0x81*sizeof(void*)) || !table[action])return 0;
    // Use the original pose selector and input-buffer flush. Its normal pickup
    // frames still attach the object, resolve competing hands and handle damage.
    GuardField<int8_t>(actor,0x83)=static_cast<int8_t>(slot);
    reinterpret_cast<int(__cdecl*)(void*,void**,int)>(g_base+0x4E320)(actor,table,action);
    reinterpret_cast<void(__cdecl*)(int)>(g_base+0x49600)(player);
    PublishItemPickup(player,-1);return 1;
}
static void __cdecl ItemPickupPunchScan(unsigned player,int radius) {
    reinterpret_cast<void(__cdecl*)(unsigned,int)>(g_base+0x32430)(player,radius);
    if(player<2 && SeparatePickupActive())InputField<int>(0x8776E0+player*4)=-1;
}
static bool InstallItemPickup() {
    const BYTE hand[]={0x8A,0x46,0x7E,0x3C,0xFF};
    const BYTE heavy[]={0x3D,0x09,0x02,0,0,0x7E,0x04,0x6A,0x79};
    const BYTE scanner[]={0x83,0xEC,0x24,0x8B,0x4C,0x24,0x28,0x53};
    auto site=reinterpret_cast<BYTE*>(g_base+0x4FB21);LONG relative;memcpy(&relative,site+1,4);
    if(!g_player_guard_available || site[0]!=0xE8 || g_base+0x4FB26+relative!=g_base+0x32430 ||
        memcmp(reinterpret_cast<void*>(g_base+0x4FB67),hand,sizeof(hand)) ||
        memcmp(reinterpret_cast<void*>(g_base+0x4FBB1),heavy,sizeof(heavy)) ||
        memcmp(reinterpret_cast<void*>(g_base+0x32430),scanner,sizeof(scanner)))return false;
    DWORD previous;if(!VirtualProtect(site,5,PAGE_EXECUTE_READWRITE,&previous))return false;
    relative=static_cast<LONG>(reinterpret_cast<uintptr_t>(ItemPickupPunchScan)-(g_base+0x4FB26));memcpy(site+1,&relative,4);
    DWORD ignored;VirtualProtect(site,5,previous,&ignored);FlushInstructionCache(GetCurrentProcess(),site,5);
    g_item_pickup_available=true;g_player_pickup_update=UpdateItemPickup;ResetItemPickup();
    Log("Separate item pickup available; native punch, pickup poses and automatic consumables retained.\r\n");return true;
}
