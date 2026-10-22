#pragma once
#include "campaign_save_file.h"

static bool g_campaign_enabled,g_campaign_available,g_campaign_resume,g_campaign_capture,g_campaign_menu_added;
static bool g_campaign_write_failed;
static int g_campaign_last_stage=-1;
static ff_campaign::Checkpoint g_campaign_checkpoint,g_campaign_pending;
static wchar_t g_campaign_path[MAX_PATH];
static const char* g_campaign_message="";
static uintptr_t g_campaign_select_return;
static ff_campaign::ReadResult(*g_campaign_read)(const wchar_t*,ff_campaign::Checkpoint&)=ff_campaign::Read;
static bool(*g_campaign_write)(const wchar_t*,const ff_campaign::Checkpoint&)=ff_campaign::Write;

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
static int __cdecl CampaignFrontend() {
    g_campaign_last_stage=-1;g_campaign_resume=g_campaign_capture=false;
    const auto read=g_campaign_read(g_campaign_path,g_campaign_checkpoint);
    g_campaign_available=read==ff_campaign::ReadResult::Ready;
    g_campaign_message=read==ff_campaign::ReadResult::Invalid?"SAVE DATA INVALID":
        read==ff_campaign::ReadResult::Unavailable?"SAVE DATA UNAVAILABLE":
        g_campaign_write_failed?"PROGRESS SAVE FAILED":"";
    const int result=reinterpret_cast<int(__cdecl*)()>(g_base+0x2EC10)();
    if(result==0 && g_campaign_resume) {
        // The original frontend has finished its fade, cleanup and character
        // selection writes. Restore values before native stage asset loading.
        CampaignRestore(g_campaign_pending);
        reinterpret_cast<void(__cdecl*)()>(g_base+0x484D0)();
    } else g_campaign_resume=false;
    return result;
}
static int __cdecl CampaignStage() {
    g_campaign_capture=CampaignPlayable() && InputField<int>(0x85A6B8)!=g_campaign_last_stage;
    const int result=reinterpret_cast<int(__cdecl*)()>(g_base+0x40D40)();
    g_campaign_capture=false;return result;
}
static void __cdecl CampaignStageReady() {
    reinterpret_cast<void(__cdecl*)()>(g_base+0x40A40)();
    if(!g_campaign_capture || !CampaignPlayable())return;
    // Resource/pose initialization stays native. Reapply only scalar player
    // values after initialization, then save the exact playable level entry.
    if(g_campaign_resume) {
        if(InputField<int>(0x85A6B8)!=g_campaign_pending.stage)return;
        CampaignRestore(g_campaign_pending);g_campaign_resume=false;
    }
    const auto checkpoint=CampaignCapture();g_campaign_capture=false;
    g_campaign_last_stage=checkpoint.stage;
    if(!ff_campaign::Valid(checkpoint)) {
        g_campaign_write_failed=true;g_campaign_message="PROGRESS SAVE FAILED";
        Log("Campaign checkpoint values rejected; previous save retained.\r\n");return;
    }
    if(g_campaign_write(g_campaign_path,checkpoint)) {
        g_campaign_checkpoint=checkpoint;g_campaign_available=true;g_campaign_message="";g_campaign_write_failed=false;
        char message[96];sprintf_s(message,"Campaign checkpoint saved: stage %d, %d players.\r\n",checkpoint.stage,checkpoint.players);Log(message);
    } else {g_campaign_write_failed=true;g_campaign_message="PROGRESS SAVE FAILED";Log("Campaign checkpoint write failed; previous save retained.\r\n");}
}
static void __declspec(naked) CampaignSelect() {
    __asm {
        mov eax,dword ptr [g_base]
        mov eax,[eax+5522B8h]
        cmp byte ptr [g_campaign_menu_added],0
        je native_row
        dec eax
    native_row:
        jmp dword ptr [g_campaign_select_return]
    }
}
static bool CampaignMainMenu() {
    if(!g_campaign_enabled)return false;
    auto native=reinterpret_cast<void(__cdecl*)()>(g_base+0x2F3B0);
    if(!g_campaign_available){native();if(*g_campaign_message)ControlHint(g_campaign_message,202);return true;}
    // Each native table has four slots. Insert Continue ahead of the existing
    // rows, retaining native layout, color animation, device input and sounds.
    uintptr_t labels[8];memcpy(labels,reinterpret_cast<void*>(g_base+0x87ACE0),sizeof(labels));
    const int counts[]={InputField<int>(0x6C498),InputField<int>(0x6C49C)};
    if(counts[0]!=2 || counts[1]!=3)return false;
    for(unsigned table=0;table<2;++table) {
        for(int row=counts[table];row>0;--row)InputField<uintptr_t>(0x87ACE0+(table*4+row)*4)=labels[table*4+row-1];
        InputField<uintptr_t>(0x87ACE0+table*16)=reinterpret_cast<uintptr_t>("CONTINUE");
        InputField<int>(0x6C498+table*4)=counts[table]+1;
    }
    g_campaign_menu_added=true;native();g_campaign_menu_added=false;
    memcpy(reinterpret_cast<void*>(g_base+0x87ACE0),labels,sizeof(labels));
    for(unsigned table=0;table<2;++table)InputField<int>(0x6C498+table*4)=counts[table];
    if(ModernMainMenu()) {
        if(InputField<int>(0x5522B8)==0) {
            char text[64];sprintf_s(text,"STAGE %d  %d PLAYER",g_campaign_checkpoint.stage+1,g_campaign_checkpoint.players);
            ControlHint(text,190);
            const unsigned pressed=InputField<unsigned>(0x5522E8)|InputField<unsigned>(0x5522EC);
            if((pressed&ff_input::Confirm) && ModernHasFocus()) {
                g_campaign_pending=g_campaign_checkpoint;g_campaign_resume=true;
                InputField<int>(0x86E9C8)=g_campaign_pending.players;
                InputField<int>(0x5522D8)=1;InputField<int>(0x5522DC)=0;
            }
        }
        if(*g_campaign_message)ControlHint(g_campaign_message,206);
    }
    return true;
}
static bool InstallCampaignSaves() {
    const struct {DWORD rva,target;void* hook;} calls[]={
        {0x41CBC,0x2EC10,reinterpret_cast<void*>(CampaignFrontend)},
        {0x41D5F,0x40D40,reinterpret_cast<void*>(CampaignStage)},
        {0x40E88,0x40A40,reinterpret_cast<void*>(CampaignStageReady)}};
    const BYTE select[]={0xA1,0xB8,0x22,0x95,0x00};
    if(memcmp(reinterpret_cast<void*>(g_base+0x2F623),select,sizeof(select)))return false;
    for(const auto& call:calls) {
        auto code=reinterpret_cast<const BYTE*>(g_base+call.rva);LONG relative;memcpy(&relative,code+1,4);
        if(code[0]!=0xE8 || g_base+call.rva+5+relative!=g_base+call.target)return false;
    }
    wcscpy_s(g_campaign_path,g_config_path);auto slash=wcsrchr(g_campaign_path,L'\\');
    if(!slash || wcscpy_s(slash+1,MAX_PATH-(slash+1-g_campaign_path),L"fightingforce-progress.sav"))return false;
    auto start=reinterpret_cast<BYTE*>(g_base+0x2F623);const SIZE_T length=0x41D64-0x2F623;DWORD previous;
    if(!VirtualProtect(start,length,PAGE_EXECUTE_READWRITE,&previous))return false;
    for(const auto& call:calls) {
        auto code=reinterpret_cast<BYTE*>(g_base+call.rva);
        const LONG relative=static_cast<LONG>(reinterpret_cast<uintptr_t>(call.hook)-reinterpret_cast<uintptr_t>(code+5));
        memcpy(code+1,&relative,4);
    }
    g_campaign_select_return=g_base+0x2F628;start[0]=0xE9;
    const LONG relative=static_cast<LONG>(reinterpret_cast<uintptr_t>(CampaignSelect)-reinterpret_cast<uintptr_t>(start+5));
    memcpy(start+1,&relative,4);DWORD ignored;VirtualProtect(start,length,previous,&ignored);
    FlushInstructionCache(GetCurrentProcess(),start,length);g_campaign_enabled=true;
    Log("Campaign level-entry saves and native Continue menu enabled.\r\n");return true;
}
