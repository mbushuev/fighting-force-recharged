#include "../src/campaign_menu.h"
#include <stdio.h>
static int failures;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
int main() {
    using namespace ff_campaign;using namespace ff_input;
    Library l;l.status[0]=SlotStatus::Ready;l.slots[0].order=1;l.slots[0].has_previous=true;
    l.slots[0].current.player[0]={3,380,384,2,0,100000};
    SaveMenu m;m.New(1,l);CHECK(m.row==1 && m.Accept(l)==SaveAction::Start && m.slot==1);
    m.New(2,l);m.row=0;CHECK(m.Accept(l)==SaveAction::None && m.page==SavePage::Replace && m.row==0);
    CHECK(m.Accept(l)==SaveAction::None && m.page==SavePage::NewGame); // No is the default.
    m.Accept(l);m.Move(1,l);CHECK(m.Accept(l)==SaveAction::Start);
    m.New(1,l);m.row=0;m.Accept(l);m.Back();CHECK(m.page==SavePage::NewGame && m.row==0);
    l.status[1]=SlotStatus::Unavailable;m.row=1;CHECK(m.Accept(l)==SaveAction::None && m.page==SavePage::NewGame);
    m.Load(l);CHECK(m.row==0 && m.Accept(l)==SaveAction::None && m.page==SavePage::Details);
    CHECK(m.Accept(l)==SaveAction::Resume);m.row=1;CHECK(m.Accept(l)==SaveAction::None && m.page==SavePage::Rewind && m.row==0);
    m.row=1;CHECK(m.Accept(l)==SaveAction::Previous);m.Back();CHECK(m.page==SavePage::Details);
    m.Back();CHECK(m.page==SavePage::Load && m.row==0);m.Back();CHECK(m.page==SavePage::Closed);
    m.Load(l);m.row=2;CHECK(m.Accept(l)==SaveAction::None && m.page==SavePage::Load);
    Navigation nav;CHECK(nav.Read(Confirm,1)==0 && nav.Read(Confirm,1000)==0);
    CHECK(nav.Read(0,1001)==0 && nav.Read(Confirm,1002)==Confirm && nav.Read(Confirm,2000)==0);
    nav.Reset();nav.Read(0,0);CHECK(nav.Read(Down,1)==Down && nav.Read(Down,400)==0 && nav.Read(Down,401)==Down);
    PauseMenu p;CHECK(p.Input(Down)==PauseAction::None && p.row==1);
    CHECK(p.Input(Confirm)==PauseAction::None && p.confirm && p.row==0);
    CHECK(p.Input(Confirm)==PauseAction::None && !p.confirm && p.row==1);
    p.Input(Confirm);p.Input(Down);CHECK(p.Input(Confirm)==PauseAction::Restart);
    p={};p.Input(Down);p.Input(Down);CHECK(p.Input(Cancel)==PauseAction::Resume);
    p={};p.Input(Down);p.Input(Confirm);CHECK(p.Input(Cancel)==PauseAction::None && !p.confirm && p.row==1);
    puts(failures?"Campaign menu safety: FAIL":"Campaign menu safety: PASS");return failures?1:0;
}
