#pragma once
static ff_campaign::PauseAction g_campaign_pause_action;
static void DrawCampaignPause() {
    if(g_move_list.page!=ff_moves::Page::Closed){DrawMoveList();return;}
    const bool coop=InputField<int>(0x8745A0)==2;
    DrawPlayerInputHints(0,8,coop?ModernKeyboardPlayer()!=0:InputHintsUsePad());
    if(coop)DrawPlayerInputHints(1,184,ModernKeyboardPlayer()!=1);
    if(g_campaign_pause.confirm) {
        ControlText("RESTART THIS LEVEL",76,true);
        ControlText("NO",108,g_campaign_pause.row==0);ControlText("YES RESTART",132,g_campaign_pause.row==1);
    } else {
        ControlText("PAUSED",62,true);const char* options[]={"CONTINUE","MOVE LIST","RESTART LEVEL","QUIT TO MENU"};
        for(int i=0;i<4;++i)ControlText(options[i],82+i*20,g_campaign_pause.row==i);
    }
    char text[64];
    if(g_campaign_write_failed)sprintf_s(text,"NOT SAVED  STAGE %d",g_campaign_entry.stage+1);
    else sprintf_s(text,"SAVED  SLOT %d  STAGE %d",g_campaign_session_slot+1,g_campaign_entry.stage+1);
    ControlHint(text,158);ControlHint("LOAD GAME USES THE LEVEL START",172);
    if(!coop)ControlHint(InputHintsUsePad()?"D-PAD CHOOSE  A SELECT  B BACK":"ARROWS CHOOSE  ENTER SELECT",214);
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
        g_campaign_pause_action=action;
    }
    const auto action=g_campaign_pause_action;
    if(action==ff_campaign::PauseAction::Restart){g_campaign_restart=true;return ff_input::Right|ff_input::Confirm;}
    if(action==ff_campaign::PauseAction::Quit)return ff_input::Right|ff_input::Confirm;
    if(action==ff_campaign::PauseAction::Resume)return ff_input::Left|ff_input::Confirm;
    return !g_campaign_pause.confirm && g_campaign_pause.row==3?ff_input::Right:ff_input::Left;
}
static void __cdecl CampaignPauseChoice(const char* text,int x,int y,const BYTE* rgb) {
    if(!g_campaign_pause_draw)reinterpret_cast<void(__cdecl*)(const char*,int,int,const BYTE*)>(g_base+0x44A20)(text,x,y,rgb);
}
static int __cdecl CampaignPause() {
    const bool available=g_campaign_entry_valid && CampaignPlayable() && g_campaign_entry.stage==InputField<int>(0x85A6B8);
    g_move_list.page=ff_moves::Page::Closed;
    if(available){g_campaign_pause={};g_campaign_pause_action=ff_campaign::PauseAction::None;g_campaign_pause_nav.Reset();g_campaign_pause_reader=CampaignPauseRead;g_campaign_pause_draw=DrawCampaignPause;}
    const int result=reinterpret_cast<int(__cdecl*)()>(g_base+0x4AA50)();
    g_campaign_pause_reader=nullptr;g_campaign_pause_draw=nullptr;g_move_list.page=ff_moves::Page::Closed;return result;
}
