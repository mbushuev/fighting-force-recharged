#pragma once
static ff_campaign::PauseAction g_campaign_pause_action;
static thread_local bool g_campaign_pause_present;
static void DrawCampaignPause() {
    if(g_move_list.page!=ff_moves::Page::Closed){DrawMoveList();return;}
    DrawMenuBackdrop(0x99000000);
    if(g_campaign_pause.confirm) {
        ControlText("RESTART THIS LEVEL",76,true);
        MenuChoiceText("NO",108,g_campaign_pause.row==0);MenuChoiceText("YES RESTART",132,g_campaign_pause.row==1);
    } else {
        ControlText("PAUSED",36,true);const char* options[]={"CONTINUE","MOVE LIST","RESTART LEVEL"};
        for(int i=0;i<3;++i)MenuChoiceText(options[i],66+i*21,g_campaign_pause.row==i);
        if(g_campaign_pause.cooperative)
            MenuChoiceText(FriendlyFireSetting()?"FRIENDLY FIRE  ON":"FRIENDLY FIRE  OFF",129,g_campaign_pause.row==3);
        MenuChoiceText("QUIT TO MENU",66+g_campaign_pause.QuitRow()*21,g_campaign_pause.row==g_campaign_pause.QuitRow());
    }
    char text[64];
    if(g_campaign_write_failed)sprintf_s(text,"NOT SAVED  %s",ff_campaign::StageName(g_campaign_entry.stage));
    else sprintf_s(text,"SAVED SLOT %d  %s",g_campaign_session_slot+1,ff_campaign::StageName(g_campaign_entry.stage));
    ControlHint(text,181);
    ControlHint(InputHintsUsePad()?"D-PAD CHOOSE  A SELECT  B BACK":"ARROWS CHOOSE  ENTER SELECT",214);
}
static uint32_t CampaignPauseRead() {
    if(!ModernHasFocus()){g_campaign_pause_nav.Reset();g_move_list_nav.Reset();return 0;}
    if(g_move_list.page!=ff_moves::Page::Closed) {
        if(!PollMoveList())g_campaign_pause_nav.Reset();
        return ff_input::Left;
    }
    const unsigned pressed=g_campaign_pause_nav.Read(g_campaign_raw_reader(true),GetTickCount64());
    if(g_campaign_pause_action==ff_campaign::PauseAction::None) {
        const auto action=g_campaign_pause.Input(pressed);
        if(action==ff_campaign::PauseAction::Moves){OpenMoveList(true);return ff_input::Left;}
        if(action==ff_campaign::PauseAction::FriendlyFire){FriendlyFireSetting()=!FriendlyFireSetting();return ff_input::Left;}
        g_campaign_pause_action=action;
    }
    const auto action=g_campaign_pause_action;
    if(action==ff_campaign::PauseAction::Restart){g_campaign_restart=true;return ff_input::Right|ff_input::Confirm;}
    if(action==ff_campaign::PauseAction::Quit)return ff_input::Right|ff_input::Confirm;
    if(action==ff_campaign::PauseAction::Resume)return ff_input::Left|ff_input::Confirm;
    return !g_campaign_pause.confirm && g_campaign_pause.row==g_campaign_pause.QuitRow()?ff_input::Right:ff_input::Left;
}
static void __cdecl CampaignPauseChoice(const char* text,int x,int y,const BYTE* rgb) {
    if(!g_campaign_pause_draw)reinterpret_cast<void(__cdecl*)(const char*,int,int,const BYTE*)>(g_base+0x44A20)(text,x,y,rgb);
}
static void __cdecl CampaignPausePresent(int hud) {
    g_campaign_pause_present=true;
    reinterpret_cast<void(__cdecl*)(int)>(g_base+0x25880)(hud);
    g_campaign_pause_present=false;
}
static void __cdecl CampaignPauseHud() {
    if(g_campaign_pause_present) {
        // Present has flushed both world queues and enabled immediate UI draws.
        g_combat_hide_overlays=true;
        if(g_campaign_pause_draw)g_campaign_pause_draw();
    } else reinterpret_cast<void(__cdecl*)()>(g_base+0x49AD0)();
}
static int __cdecl CampaignPause() {
    const bool available=g_campaign_entry_valid && CampaignPlayable() && g_campaign_entry.stage==InputField<int>(0x85A6B8);
    g_move_list.page=ff_moves::Page::Closed;
    if(available){g_campaign_pause={};g_campaign_pause.cooperative=InputField<int>(0x8745A0)==2;g_campaign_pause_action=ff_campaign::PauseAction::None;g_campaign_pause_nav.Reset();g_campaign_pause_reader=CampaignPauseRead;g_campaign_pause_draw=DrawCampaignPause;}
    const int result=reinterpret_cast<int(__cdecl*)()>(g_base+0x4AA50)();
    g_campaign_pause_reader=nullptr;g_campaign_pause_draw=nullptr;g_move_list.page=ff_moves::Page::Closed;return result;
}
