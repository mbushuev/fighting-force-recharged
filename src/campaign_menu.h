#pragma once
#include "campaign_slots.h"
#include "modern_input.h"

namespace ff_campaign {
struct Navigation {
    bool ready=false;unsigned held=0;uint64_t repeat=0;
    void Reset(){ready=false;held=0;repeat=0;}
    unsigned Read(unsigned raw,uint64_t now) {
        if(!ready){if(!raw)ready=true;held=raw;return 0;}
        unsigned fresh=raw&~held;
        if(raw!=held)repeat=now+400;
        else if((raw&15) && now>=repeat){fresh|=raw&15;repeat=now+120;}
        held=raw;return fresh;
    }
};
enum class SavePage { Closed,Load,Details,NewGame,Replace,Rewind };
enum class SaveAction { None,Resume,Previous,Start };
struct SaveMenu {
    SavePage page=SavePage::Closed;int row=0,slot=0,players=1;
    void Load(const Library& l){page=SavePage::Load;row=l.Latest()<0?0:l.Latest();}
    void New(int count,const Library& l){page=SavePage::NewGame;players=count;row=l.Empty()<0?0:l.Empty();}
    int Rows(const Library& l)const {
        return page==SavePage::Load || page==SavePage::NewGame?4:
            page==SavePage::Replace || page==SavePage::Rewind?2:page==SavePage::Details && l.slots[slot].has_previous?3:2;
    }
    void Move(int delta,const Library& l){row=(row+Rows(l)+delta)%Rows(l);}
    void Back() {
        if(page==SavePage::Details){page=SavePage::Load;row=slot;}
        else if(page==SavePage::Replace){page=SavePage::NewGame;row=slot;}
        else if(page==SavePage::Rewind){page=SavePage::Details;row=1;}
        else page=SavePage::Closed;
    }
    SaveAction Accept(const Library& l) {
        if(page==SavePage::Load || page==SavePage::NewGame) {
            if(row==3){Back();return SaveAction::None;}
            slot=row;
            if(page==SavePage::Load) {if(Usable(l.status[slot])){page=SavePage::Details;row=0;}}
            else if(l.status[slot]==SlotStatus::Missing)return SaveAction::Start;
            else if(l.status[slot]!=SlotStatus::Unavailable){page=SavePage::Replace;row=0;}
        } else if(page==SavePage::Replace) {
            if(row==1)return SaveAction::Start;Back();
        } else if(page==SavePage::Rewind) {
            if(row==1)return SaveAction::Previous;Back();
        } else if(page==SavePage::Details) {
            if(row==0)return SaveAction::Resume;
            if(row==1 && l.slots[slot].has_previous){page=SavePage::Rewind;row=0;return SaveAction::None;}
            Back();
        }
        return SaveAction::None;
    }
};
enum class PauseAction { None,Resume,Restart,Quit,Moves,FriendlyFire };
struct PauseMenu {
    int row=0;bool confirm=false,cooperative=false;
    int QuitRow()const{return cooperative?4:3;}
    void Move(int delta){const int count=confirm?2:QuitRow()+1;row=(row+count+delta)%count;}
    PauseAction Input(unsigned pressed) {
        if(pressed&ff_input::Cancel){if(confirm){confirm=false;row=2;return PauseAction::None;}return PauseAction::Resume;}
        if(pressed&ff_input::Confirm) {
            if(confirm){if(row==1)return PauseAction::Restart;confirm=false;row=2;}
            else if(row==0)return PauseAction::Resume;
            else if(row==1)return PauseAction::Moves;
            else if(row==QuitRow())return PauseAction::Quit;
            else if(row==3)return PauseAction::FriendlyFire;
            else {confirm=true;row=0;}
        } else if(pressed&(ff_input::Up|ff_input::Left))Move(-1);
        else if(pressed&(ff_input::Down|ff_input::Right))Move(1);
        return PauseAction::None;
    }
};
}
