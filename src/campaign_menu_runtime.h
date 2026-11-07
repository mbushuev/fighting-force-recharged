#pragma once
#include "campaign_stages.h"
static const char* CampaignHero(int hero) {
    const char* names[]={"HAWK","MACE","SMASHER","ALANA"};return hero>=0 && hero<4?names[hero]:"OUT";
}
static void CampaignSummary(const ff_campaign::Checkpoint& c,int y) {
    char line[96];
    const char* difficulty[]={"EASY","NORMAL","HARD"};
    ControlHint(ff_campaign::StageName(c.stage),y);
    if(c.players==2)sprintf_s(line,"%s AND %s  %s",CampaignHero(c.player[0].character),CampaignHero(c.player[1].character),difficulty[c.difficulty]);
    else sprintf_s(line,"%s  %s  1 PLAYER",CampaignHero(c.player[0].character),difficulty[c.difficulty]);
    ControlHint(line,y+14);
}
static unsigned CampaignRawMenu(bool pause=false) {
    if(!ModernHasFocus())return 0;
    g_control_keyboard_valid=ReadModernPhysicalKeyboard(g_control_keyboard);InitializeModernPads();
    unsigned raw=ff_input::Directions(ControlKey(0x148)||ControlKey(0x11),ControlKey(0x150)||ControlKey(0x1F),
        ControlKey(0x14B)||ControlKey(0x1E),ControlKey(0x14D)||ControlKey(0x20));
    if(ControlKey(0x1C)||ControlKey(0x39))raw|=ff_input::Confirm;
    if(ControlKey(1) || (pause && ReadModernCommandKeyboard().pause))raw|=ff_input::Cancel;
    if(pause && g_modern_gamepad && ModernCooperativeGameplay()) {
        for(unsigned player=0;player<2;++player) {
            const auto pad=ReadModernCoopRawPad(player);
            raw|=ff_input::Menu(pad,g_modern_left_deadzone,false);
            if(pad.buttons&ff_input::Start)raw|=ff_input::Cancel;
        }
        g_modern_pad_pair.pause_requested=false; // Already inside the native pause loop.
        return raw;
    }
    for(unsigned slot=0;slot<4;++slot) {
        const auto pad=ControlRawPad(slot);raw|=ff_input::Menu(pad,g_modern_left_deadzone,false);
        if(pause && (pad.buttons&ff_input::Start))raw|=ff_input::Cancel;
    }
    return raw;
}
static unsigned(*g_campaign_raw_reader)(bool)=CampaignRawMenu;
static void CampaignCloseMenu() {
    g_campaign_menu.page=ff_campaign::SavePage::Closed;g_campaign_menu_active=false;
    g_controls_wait_release=true;g_modern_main_back_button=g_modern_quit_button=true;
    g_modern_pad_session.ready=g_modern_second_session.ready=false;
}
static void CampaignResumeSlot(int slot,bool previous=false) {
    if(slot<0 || slot>=3 || !ff_campaign::Usable(g_campaign_library.status[slot]))return;
    const auto& record=g_campaign_library.slots[slot];if(previous && !record.has_previous)return;
    g_campaign_pending_slot=slot;g_campaign_pending=previous?record.previous:record.current;
    g_campaign_resume=true;g_campaign_replace=false;g_campaign_rollback=previous;
    CampaignCloseMenu();InputField<int>(0x86E9C8)=g_campaign_pending.players;
    InputField<int>(0x5522D8)=1;InputField<int>(0x5522DC)=0;
}
static void CampaignNewGame(int slot,int players) {
    if(slot<0 || slot>=3 || players<1 || players>2)return;
    g_campaign_pending_slot=slot;g_campaign_replace=true;g_campaign_resume=g_campaign_rollback=false;
    CampaignCloseMenu();
    // Preserve the native camera transition and character selection path.
    InputField<int>(0x86E9C8)=players;InputField<int>(0x85A6BC)=players-1;
    InputField<uintptr_t>(0x6C47C)=g_base+0x2F730;InputField<int>(0x5522B4)=0;
    reinterpret_cast<void(__cdecl*)()>(g_base+0x2E9F0)();
}
static void DrawCampaignMenu() {
    using namespace ff_campaign;const auto& m=g_campaign_menu;char line[96];
    if(m.page==SavePage::Load || m.page==SavePage::NewGame) {
        ControlText(m.page==SavePage::Load?"LOAD GAME":"NEW GAME",12,true);
        for(int i=0;i<3;++i) {
            const auto status=g_campaign_library.status[i];
            if(Usable(status))sprintf_s(line,"SLOT %d  %s",i+1,StageName(g_campaign_library.slots[i].current.stage));
            else sprintf_s(line,"SLOT %d  %s",i+1,status==SlotStatus::Missing?"EMPTY":status==SlotStatus::Invalid?"DAMAGED":"UNAVAILABLE");
            ControlText(line,42+i*22,m.row==i);
        }
        ControlText("BACK",108,m.row==3);
        if(m.row<3 && Usable(g_campaign_library.status[m.row])) {
            const auto& c=g_campaign_library.slots[m.row].current;CampaignSummary(c,136);
            if(c.players==2)sprintf_s(line,"LIVES %d AND %d",c.player[0].lives,c.player[1].lives);
            else sprintf_s(line,"LIVES %d  HEALTH %d",c.player[0].lives,c.player[0].health);
            ControlHint(line,164);
            if(g_campaign_library.status[m.row]==SlotStatus::Recovered)ControlHint("BACKUP RECOVERED",182);
        } else if(m.page==SavePage::NewGame)ControlHint("YOUR OTHER GAMES ARE KEPT",150);
    } else if(m.page==SavePage::Rewind) {
        ControlText("LOAD PREVIOUS STAGE",30,true);ControlHint("THIS REWINDS YOUR PROGRESS",70);
        ControlHint(StageName(g_campaign_library.slots[m.slot].previous.stage),88);
        ControlText("NO",112,m.row==0);ControlText("YES LOAD",138,m.row==1);
    } else if(m.page==SavePage::Replace) {
        sprintf_s(line,"REPLACE SLOT %d",m.slot+1);ControlText(line,30,true);
        ControlHint("THIS REPLACES ITS PROGRESS",70);
        ControlText("NO",112,m.row==0);ControlText("YES REPLACE",138,m.row==1);
    } else if(m.page==SavePage::Details) {
        const auto& s=g_campaign_library.slots[m.slot];sprintf_s(line,"SLOT %d",m.slot+1);ControlText(line,14,true);
        CampaignSummary(s.current,42);ControlText("CONTINUE",102,m.row==0);
        if(s.has_previous){sprintf_s(line,"PREVIOUS  %s",StageName(s.previous.stage));ControlText(line,126,m.row==1);}
        ControlText("BACK",s.has_previous?150:126,m.row==(s.has_previous?2:1));
        ControlHint("RESUME FROM THE LEVEL START",182);
    }
    ControlHint(InputHintsUsePad()?"D-PAD CHOOSE  A SELECT":"ARROWS CHOOSE  ENTER SELECT",208);
    ControlHint(InputHintsUsePad()?"B BACK":"ESC BACK",224);
}
static void CampaignMenuFrame() {
    // This screen keeps the native main callback identity for layout/input
    // compatibility. Prevent its attract timeout while choosing a save slot.
    InputField<unsigned>(0x5522C8)=InputField<unsigned>(0x85B560);
    if(!ModernHasFocus()){g_campaign_nav.Reset();DrawCampaignMenu();return;}
    const auto pressed=g_campaign_nav.Read(g_campaign_raw_reader(false),GetTickCount64());
    using namespace ff_campaign;auto& m=g_campaign_menu;
    if(pressed&ff_input::Cancel)m.Back();
    else if(pressed&ff_input::Confirm) {
        const auto action=m.Accept(g_campaign_library);
        if(action==SaveAction::Resume || action==SaveAction::Previous){CampaignResumeSlot(m.slot,action==SaveAction::Previous);return;}
        if(action==SaveAction::Start){CampaignNewGame(m.slot,m.players);return;}
        g_campaign_nav.Reset();
    } else if(pressed&ff_input::Up)m.Move(-1,g_campaign_library);
    else if(pressed&ff_input::Down)m.Move(1,g_campaign_library);
    if(m.page==SavePage::Closed){CampaignCloseMenu();return;}DrawCampaignMenu();
}
static int CampaignNativeRow(int row) {
    if(!g_campaign_menu_added)return row;
    row-=g_campaign_prefix;
    if(row==0 || (row==1 && InputField<int>(0x86D2A8)!=1))return -1;
    return row;
}
static void __declspec(naked) CampaignSelect() {
    __asm {
        push ecx
        push edx
        mov eax,dword ptr [g_base]
        push [eax+5522B8h]
        call CampaignNativeRow
        add esp,4
        pop edx
        pop ecx
        jmp dword ptr [g_campaign_select_return]
    }
}
static bool CampaignMainMenu() {
    if(!g_campaign_enabled)return false;
    if(g_campaign_menu_active){CampaignMenuFrame();return true;}
    const int latest=g_campaign_library.Latest();g_campaign_prefix=latest>=0?2:1;
    const int counts[]={InputField<int>(0x6C498),InputField<int>(0x6C49C)};
    if(counts[0]!=2 || counts[1]!=3)return false;
    for(int table=0;table<2;++table) {
        auto labels=g_campaign_labels+table*8;int row=0;
        if(latest>=0)labels[row++]=reinterpret_cast<uintptr_t>("CONTINUE");
        labels[row++]=reinterpret_cast<uintptr_t>("LOAD GAME");
        for(int old=0;old<counts[table];++old)labels[row++]=InputField<uintptr_t>(0x87ACE0+(table*4+old)*4);
        InputField<int>(0x6C498+table*4)=row;
    }
    g_campaign_menu_added=true;reinterpret_cast<void(__cdecl*)()>(g_base+0x2F3B0)();g_campaign_menu_added=false;
    for(unsigned table=0;table<2;++table)InputField<int>(0x6C498+table*4)=counts[table];
    if(!ModernMainMenu())return true;
    const int row=InputField<int>(0x5522B8);
    if(row==0 && latest>=0)CampaignSummary(g_campaign_library.slots[latest].current,182);
    if(*g_campaign_message)ControlHint(g_campaign_message,208);
    const unsigned pressed=InputField<unsigned>(0x5522E8)|InputField<unsigned>(0x5522EC);
    if(!(pressed&ff_input::Confirm) || !ModernHasFocus())return true;
    if(row==0 && latest>=0){CampaignResumeSlot(latest);return true;}
    if(row==g_campaign_prefix-1)g_campaign_menu.Load(g_campaign_library);
    else {
        const int choice=row-g_campaign_prefix;
        if(choice!=0 && !(choice==1 && InputField<int>(0x86D2A8)!=1))return true;
        g_campaign_menu.New(choice+1,g_campaign_library);
    }
    g_campaign_menu_active=true;g_campaign_nav.Reset();return true;
}
