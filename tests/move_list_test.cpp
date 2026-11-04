#include "../src/move_list.h"
#include <cstdio>
#include <set>
static int failures;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
int main() {
    using namespace ff_moves;using namespace ff_controls;
    unsigned count;const auto* catalog=Catalog(count);std::set<int> ids;
    CHECK(count==24);
    for(unsigned i=0;i<count;++i){CHECK(ids.insert(int(catalog[i].id)).second);CHECK(*catalog[i].note);CHECK(catalog[i].first<ff_controls::Count);}
    for(int group=0;group<int(Group::Count);++group){CHECK(Rows(Group(group))>0 && Rows(Group(group))<=6);for(int row=0;row<Rows(Group(group));++row)CHECK(At(Group(group),row).group==Group(group));}
    auto settings=DefaultsSettings();
    const auto bind=[&](Action action){return std::string(action==Hand?"X":action==Jump?"A":action==Pickup?"RB":action==Throw?"LB":action==Foot?"Y":"Q");};
    CHECK(Command(At(Group::AirRun,2),settings,0,true,bind)=="A AND X");
    CHECK(Command(At(Group::Items,0),settings,0,true,bind)=="RB");
    settings.separate_pickup=false;CHECK(Command(At(Group::Items,0),settings,0,true,bind)=="X");
    CHECK(Command(At(Group::Items,2),settings,0,true,bind)=="Y");
    CHECK(Command(At(Group::Strikes,5),settings,0,true,bind)=="DISABLED IN GAMEPLAY SETTINGS");
    settings.guard=true;CHECK(Command(At(Group::Strikes,5),settings,0,true,bind)=="HOLD Q");
    settings.players[0].pad[Hand]=ff_input::X;settings.players[0].pad[Jump]=ff_input::A|ff_input::X;
    CHECK(Command(At(Group::AirRun,2),settings,0,true,bind)=="BINDING CONFLICT");
    CHECK(Command(At(Group::AirRun,2),settings,1,true,bind)=="A AND X");
    CHECK(Command(At(Group::Strikes,0),settings,0,false,[](Action){return std::string("UNBOUND");})=="UNBOUND");
    for(int hero=0;hero<4;++hero)for(unsigned i=0;i<count;++i)CHECK(*Name(catalog[i],hero));
    CHECK(std::string(Name(At(Group::AirRun,3),2))=="FORWARD ROLL");
    CHECK(std::string(Name(At(Group::Grabs,3),1))=="SHOULDER THROW");
    CHECK(Command(At(Group::Grabs,4),settings,1,true,bind,2)=="Q THEN X");
    Menu m;m.Open(true,1,3,2);CHECK(m.page==Page::Moves && m.hero[0]==3);
    m.Change(-1);CHECK(m.group==Group::Items);m.Move(-1);CHECK(m.row==2);m.Back();CHECK(m.page==Page::Closed);
    m.Open(true,2,1,2);m.Move(1);m.Accept();CHECK(m.player==1 && m.page==Page::Moves && m.hero[1]==2);
    m.Back();CHECK(m.page==Page::Players && m.row==1);m.Back();CHECK(m.page==Page::Closed);
    m.Open(false,2,-1,9);m.Move(1);m.Accept();CHECK(m.page==Page::Heroes && m.player==1);
    m.Move(3);m.Accept();CHECK(m.page==Page::Moves && m.hero[1]==3);m.Back();CHECK(m.page==Page::Heroes && m.row==3);
    m.Back();CHECK(m.page==Page::Players && m.row==1);m.Move(1);m.Accept();CHECK(m.page==Page::Closed);
    puts(failures?"Move list policy: FAIL":"Move list policy: PASS");return failures?1:0;
}
