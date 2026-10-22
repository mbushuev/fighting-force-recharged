#pragma once
#include "campaign_slots_file.h"
#include "campaign_menu.h"
static bool g_campaign_enabled,g_campaign_resume,g_campaign_capture,g_campaign_menu_added;
static bool g_campaign_write_failed,g_campaign_replace,g_campaign_rollback,g_campaign_entry_valid,g_campaign_restart;
static int g_campaign_last_stage=-1,g_campaign_session_slot=-1,g_campaign_pending_slot=-1,g_campaign_prefix;
static ff_campaign::Library g_campaign_library;
static ff_campaign::Checkpoint g_campaign_pending,g_campaign_entry;
static ff_campaign::SaveMenu g_campaign_menu;
static ff_campaign::PauseMenu g_campaign_pause;
static ff_campaign::Navigation g_campaign_nav,g_campaign_pause_nav;
static wchar_t g_campaign_path[MAX_PATH],g_campaign_slot_paths[3][MAX_PATH];
static const char* g_campaign_message="";
static uintptr_t g_campaign_select_return,g_campaign_labels[16];
static ff_campaign::ReadResult(*g_campaign_read)(const wchar_t*,ff_campaign::Checkpoint&)=ff_campaign::Read;
static ff_campaign::SlotStatus(*g_campaign_read_slot)(const wchar_t*,ff_campaign::Slot&)=ff_campaign::ReadSlot;
static bool(*g_campaign_write_slot)(const wchar_t*,const ff_campaign::Slot&)=ff_campaign::WriteSlot;
static bool CampaignPlayable() {
    const int stage=InputField<int>(0x85A6B8),players=InputField<int>(0x8745A0);
    return stage>=0 && stage<25 && (players==1 || players==2) &&
        InputField<int>(0x6F000)==-1 && InputField<int>(0x85A668)!=1 &&
        InputField<int>(0x85A6BC)==players-1;
}
static ff_campaign::Checkpoint CampaignCapture() {
    ff_campaign::Checkpoint c;
    c.stage=InputField<int>(0x85A6B8);c.players=InputField<int>(0x8745A0);
    c.mode=InputField<int>(0x85A6BC);c.difficulty=InputField<BYTE>(0x6F008);
    for(int i=0;i<c.players && i<2;++i) {
        auto& p=c.player[i];const DWORD actor=0x86A500+i*0xF4;
        p.character=InputField<int16_t>(actor+0x34);p.health=InputField<int16_t>(actor+0x42);
        p.maximum=InputField<int16_t>(actor+0x48);p.lives=InputField<int>(0x878890+i*4);
        p.score=InputField<int>(0x86BF10+i*4);p.next_life=InputField<int>(0x8689E0+i*4);
        if(p.health<0)p.health=0;
        if(p.character<0){p.health=0;p.lives=0;}
    }
    return c;
}
static void CampaignRestore(const ff_campaign::Checkpoint& c) {
    InputField<int>(0x85A6B8)=c.stage;InputField<int>(0x8745A0)=c.players;
    InputField<int>(0x85A6BC)=c.mode;InputField<BYTE>(0x6F008)=BYTE(c.difficulty);
    InputField<int>(0x6F000)=-1;
    for(int i=0;i<2;++i) {
        const auto& p=c.player[i];const DWORD actor=0x86A500+i*0xF4;
        InputField<int16_t>(actor+0x34)=int16_t(p.character);
        for(DWORD field:{0x42,0x44,0x46})InputField<int16_t>(actor+field)=int16_t(p.health);
        InputField<int16_t>(actor+0x48)=int16_t(p.maximum);
        InputField<int>(0x878890+i*4)=p.lives;InputField<int>(0x86BF10+i*4)=p.score;
        InputField<int>(0x8689E0+i*4)=p.next_life;
    }
}

static void CampaignLoadSlots() {
    g_campaign_library={};bool empty=true,recovered=false,damaged=false;
    for(int i=0;i<3;++i) {
        const auto status=g_campaign_read_slot(g_campaign_slot_paths[i],g_campaign_library.slots[i]);
        g_campaign_library.status[i]=status;empty&=status==ff_campaign::SlotStatus::Missing;
        recovered|=status==ff_campaign::SlotStatus::Recovered;
        damaged|=status==ff_campaign::SlotStatus::Invalid || status==ff_campaign::SlotStatus::Unavailable;
    }
    g_campaign_message=g_campaign_write_failed?"PROGRESS SAVE FAILED":recovered?"SAVE BACKUP RECOVERED":damaged?"CHECK DAMAGED SAVE SLOTS":"";
    if(empty) {
        ff_campaign::Checkpoint legacy;const auto read=g_campaign_read(g_campaign_path,legacy);
        if(read==ff_campaign::ReadResult::Ready) {
            auto slot=g_campaign_library.Updated(0,legacy,true);
            if(g_campaign_write_slot(g_campaign_slot_paths[0],slot)) {
                g_campaign_library.slots[0]=slot;g_campaign_library.status[0]=ff_campaign::SlotStatus::Ready;
                g_campaign_message="SAVE IMPORTED TO SLOT 1";
            } else {g_campaign_write_failed=true;g_campaign_message="SAVE IMPORT FAILED";}
        } else if(read==ff_campaign::ReadResult::Invalid)g_campaign_message="LEGACY SAVE DATA INVALID";
    }
}
static int __cdecl CampaignFrontend() {
    g_campaign_last_stage=-1;g_campaign_session_slot=g_campaign_pending_slot=-1;
    g_campaign_resume=g_campaign_capture=g_campaign_entry_valid=false;
    g_campaign_replace=g_campaign_rollback=g_campaign_menu_active=false;g_campaign_menu={};
    CampaignLoadSlots();
    const int result=reinterpret_cast<int(__cdecl*)()>(g_base+0x2EC10)();
    g_campaign_menu_active=false;
    if(result==0 && g_campaign_pending_slot>=0) {
        g_campaign_session_slot=g_campaign_pending_slot;
        if(g_campaign_resume) {
            // Restore after frontend cleanup, before resolving stage resources.
            CampaignRestore(g_campaign_pending);
            reinterpret_cast<void(__cdecl*)()>(g_base+0x484D0)();
        }
    } else g_campaign_resume=false;
    return result;
}
static int __cdecl CampaignStage() {
    for(;;) {
        g_campaign_capture=CampaignPlayable() && InputField<int>(0x85A6B8)!=g_campaign_last_stage;
        const int result=reinterpret_cast<int(__cdecl*)()>(g_base+0x40D40)();g_campaign_capture=false;
        if(!g_campaign_restart)return result;
        // Native stage cleanup has stopped workers, audio and rendering.
        // Retry through the normal loader without nesting another game loop.
        g_campaign_restart=false;g_campaign_resume=true;g_campaign_pending=g_campaign_entry;
        CampaignRestore(g_campaign_pending);g_campaign_last_stage=-1;
        reinterpret_cast<void(__cdecl*)()>(g_base+0x484D0)();
    }
}
static void __cdecl CampaignStageReady() {
    reinterpret_cast<void(__cdecl*)()>(g_base+0x40A40)();
    if(!g_campaign_capture || !CampaignPlayable())return;
    if(g_campaign_resume) {
        if(InputField<int>(0x85A6B8)!=g_campaign_pending.stage)return;
        CampaignRestore(g_campaign_pending);g_campaign_resume=false;
    }
    const auto point=CampaignCapture();g_campaign_capture=false;g_campaign_last_stage=point.stage;
    g_campaign_entry_valid=ff_campaign::Valid(point);g_campaign_entry=point;
    if(!g_campaign_entry_valid || g_campaign_session_slot<0 || g_campaign_session_slot>=3) {
        g_campaign_write_failed=true;g_campaign_message="PROGRESS SAVE FAILED";return;
    }
    const int index=g_campaign_session_slot;
    const auto slot=g_campaign_library.Updated(index,point,g_campaign_replace,g_campaign_rollback);
    if(g_campaign_write_slot(g_campaign_slot_paths[index],slot)) {
        g_campaign_library.slots[index]=slot;g_campaign_library.status[index]=ff_campaign::SlotStatus::Ready;
        g_campaign_replace=g_campaign_rollback=g_campaign_write_failed=false;g_campaign_message="";
        char message[112];sprintf_s(message,"Campaign slot %d saved: stage %d, %d players.\r\n",index+1,point.stage,point.players);Log(message);
    } else {g_campaign_write_failed=true;g_campaign_message="PROGRESS SAVE FAILED";Log("Campaign slot write failed; existing saves retained.\r\n");}
}
#include "campaign_menu_runtime.h"

static bool InstallCampaignSaves() {
    const struct {DWORD rva,target;void* hook;} calls[]={
        {0x41CBC,0x2EC10,reinterpret_cast<void*>(CampaignFrontend)},
        {0x41D5F,0x40D40,reinterpret_cast<void*>(CampaignStage)},
        {0x40E88,0x40A40,reinterpret_cast<void*>(CampaignStageReady)}};
    const BYTE select[]={0xA1,0xB8,0x22,0x95,0x00},table[]={0xC1,0xE6,4,0x81,0xC6,0xE0,0xAC,0xC7,0};
    if(memcmp(reinterpret_cast<void*>(g_base+0x2F623),select,sizeof(select)) ||
       memcmp(reinterpret_cast<void*>(g_base+0x2F4D8),table,sizeof(table)))return false;
    for(const auto& call:calls) {
        auto code=reinterpret_cast<const BYTE*>(g_base+call.rva);LONG relative;memcpy(&relative,code+1,4);
        if(code[0]!=0xE8 || g_base+call.rva+5+relative!=g_base+call.target)return false;
    }
    wcscpy_s(g_campaign_path,g_config_path);auto slash=wcsrchr(g_campaign_path,L'\\');
    if(!slash || wcscpy_s(slash+1,MAX_PATH-(slash+1-g_campaign_path),L"fightingforce-progress.sav"))return false;
    for(int i=0;i<3;++i) {
        wcscpy_s(g_campaign_slot_paths[i],g_config_path);auto name=wcsrchr(g_campaign_slot_paths[i],L'\\')+1;
        if(swprintf_s(name,MAX_PATH-(name-g_campaign_slot_paths[i]),L"fightingforce-save-%d.sav",i+1)<0)return false;
    }
    auto start=reinterpret_cast<BYTE*>(g_base+0x2F4D8);const SIZE_T length=0x4AC82-0x2F4D8;DWORD previous;
    if(!VirtualProtect(start,length,PAGE_EXECUTE_READWRITE,&previous))return false;
    for(const auto& call:calls) {
        auto code=reinterpret_cast<BYTE*>(g_base+call.rva);
        const LONG relative=static_cast<LONG>(reinterpret_cast<uintptr_t>(call.hook)-reinterpret_cast<uintptr_t>(code+5));memcpy(code+1,&relative,4);
    }
    start[2]=5;const auto labels=reinterpret_cast<uintptr_t>(g_campaign_labels);memcpy(start+5,&labels,4);
    auto selection=reinterpret_cast<BYTE*>(g_base+0x2F623);g_campaign_select_return=g_base+0x2F628;selection[0]=0xE9;
    const LONG relative=static_cast<LONG>(reinterpret_cast<uintptr_t>(CampaignSelect)-reinterpret_cast<uintptr_t>(selection+5));memcpy(selection+1,&relative,4);
    DWORD ignored;VirtualProtect(start,length,previous,&ignored);FlushInstructionCache(GetCurrentProcess(),start,length);g_campaign_enabled=true;
    Log("Three campaign slots, load menu and checkpoint recovery enabled.\r\n");return true;
}
