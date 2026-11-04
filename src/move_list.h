#pragma once
#include "control_bindings.h"
#include <string>

namespace ff_moves {
enum class Page { Players,Heroes,Moves,Closed };
enum class Group { Strikes,AirRun,Grabs,Weapons,Items,Count };
enum class Input { Single,Repeat,Together,After,Hold,MoveAway };
enum class Id { Punch,Kick,Backfist,LowKick,Ground,Guard,Jump,JumpKick,Spin,RunPunch,RunKick,AirGrab,
    Grab,GrabPunch,GrabKick,GrabThrow,GrabFinish,Release,Fire,Melee,MeleeCombo,LightThrow,HeavyThrow,Grenade,
    Pickup,Pull,Drop };
struct Move {
    Id id;Group group;const char* name;Input input;
    ff_controls::Action first,second;
    const char* note;
};
inline const Move* Catalog(unsigned& count) {
    using namespace ff_controls;
    static const Move moves[]={
        {Id::Punch,Group::Strikes,"PUNCH COMBO",Input::Repeat,Hand,Hand,"UNARMED. REPEAT THROUGH THE COMBO."},
        {Id::Kick,Group::Strikes,"KICK COMBO",Input::Repeat,Foot,Foot,"UNARMED. FOLLOW WITH ANOTHER KICK."},
        {Id::Backfist,Group::Strikes,"BACKFIST COMBO",Input::Repeat,Backfist,Backfist,"ATTACK BEHIND. REPEAT FOR FOLLOW-UP."},
        {Id::LowKick,Group::Strikes,"LOW OR GROUND KICK",Input::Together,Hand,Foot,"UNARMED. PRESS BOTH TOGETHER."},
        {Id::Ground,Group::Strikes,"GROUND ATTACK",Input::Single,Foot,Foot,"CLOSE TO A FALLEN ENEMY. FACE THEM."},
        {Id::Guard,Group::Strikes,"PLAYER BLOCK",Input::Hold,Block,Block,"FRONTAL UNARMED HITS. EMPTY OR LIGHT HANDS."},
        {Id::Jump,Group::AirRun,"JUMP",Input::Single,Jump,Jump,"MOVE TO CHANGE THE JUMP DIRECTION."},
        {Id::JumpKick,Group::AirRun,"JUMP KICK",Input::Together,Jump,Foot,"UNARMED. ALSO WORKS EARLY IN A JUMP."},
        {Id::Spin,Group::AirRun,"SPIN ATTACK",Input::Together,Jump,Hand,"UNARMED. USES HEALTH. ALSO EARLY IN JUMP."},
        {Id::RunPunch,Group::AirRun,"RUNNING STRIKE",Input::Together,Run,Hand,"MOVE AND RUN FIRST. THEN PRESS PUNCH."},
        {Id::RunKick,Group::AirRun,"RUNNING KICK",Input::Together,Run,Foot,"MOVE AND RUN FIRST. THEN PRESS KICK."},
        {Id::AirGrab,Group::AirRun,"AIR GRAB",Input::After,Jump,Backfist,"JUMP FIRST. CLOSE RANGE. SOME ENEMIES ONLY."},
        {Id::Grab,Group::Grabs,"GRAB ENEMY",Input::Single,Backfist,Backfist,"WALK CLOSE AND FACE AN ELIGIBLE ENEMY."},
        {Id::GrabPunch,Group::Grabs,"GRAB PUNCHES",Input::Repeat,Hand,Hand,"WHILE HOLDING AN ENEMY. REPEAT TO FOLLOW."},
        {Id::GrabKick,Group::Grabs,"GRAB KICK",Input::Single,Foot,Foot,"WHILE HOLDING AN ENEMY."},
        {Id::GrabThrow,Group::Grabs,"GRAB THROW",Input::Single,Jump,Jump,"WHILE HOLDING AN ENEMY."},
        {Id::GrabFinish,Group::Grabs,"GRAB FINISHER",Input::Single,Backfist,Backfist,"WHILE HOLDING. DIRECTION CAN CHANGE IT."},
        {Id::Release,Group::Grabs,"LET GO",Input::MoveAway,Backward,Backward,"MOVE AWAY FROM THE HELD ENEMY."},
        {Id::Fire,Group::Weapons,"FIRE WEAPON",Input::Single,Hand,Hand,"WITH A LOADED FIREARM. NO MANUAL RELOAD."},
        {Id::Melee,Group::Weapons,"MELEE WEAPON STRIKE",Input::Single,Hand,Hand,"WITH A LIGHT MELEE WEAPON IN HAND."},
        {Id::MeleeCombo,Group::Weapons,"MELEE WEAPON COMBO",Input::Repeat,Hand,Hand,"REPEAT PUNCH. FOLLOW-UPS DEPEND ON WEAPON."},
        {Id::Pickup,Group::Items,"PICK UP ITEM",Input::Single,Pickup,Pickup,"CLOSE TO AN ELIGIBLE ITEM. FACE IT."},
        {Id::Pull,Group::Items,"PULL SCENERY FREE",Input::Single,Pickup,Pickup,"AT DETACHABLE SCENERY. FOLLOW THE PROMPT."},
        {Id::Drop,Group::Items,"THROW OR DROP",Input::Single,Throw,Throw,"ANY CARRIED ITEM OR GRENADE. PRESS ONCE."}
    };
    count=sizeof(moves)/sizeof(*moves);return moves;
}
inline const char* GroupName(Group group) {
    const char* names[]={"STRIKES AND BLOCK","JUMPS AND RUNNING","ENEMY GRABS","WEAPONS AND THROWS","ITEM INTERACTIONS"};
    return names[static_cast<unsigned>(group)];
}
inline const char* HeroName(int hero) {
    const char* names[]={"HAWK","MACE","SMASHER","ALANA"};return hero>=0 && hero<4?names[hero]:"UNKNOWN";
}
inline const char* Name(const Move& move,int hero) {
    if(move.id==Id::RunPunch && hero==2)return "FORWARD ROLL";
    if(move.id==Id::GrabPunch && hero==1)return "GRAB SLAP COMBO";
    if(move.id==Id::GrabPunch && hero==2)return "GRAB POWER ATTACK";
    if(move.id==Id::GrabKick && hero==0)return "GRAB KNEE STRIKE";
    if(move.id==Id::GrabKick && hero==2)return "GRAB POWER THROW";
    if(move.id==Id::GrabKick && hero==3)return "GRAB SPIN KICK";
    if(move.id==Id::GrabThrow && hero==1)return "SHOULDER THROW";
    if(move.id==Id::GrabFinish && hero==0)return "GRAB HEADBUTT";
    if(move.id==Id::GrabFinish && hero==2)return "GRAB LIFT AND THROW";
    return move.name;
}
inline ff_controls::Action Effective(ff_controls::Action action,const ff_controls::Settings& settings) {
    return !settings.separate_pickup && action==ff_controls::Pickup?ff_controls::Hand:
        !settings.separate_pickup && action==ff_controls::Throw?ff_controls::Foot:action;
}
inline bool TogetherAvailable(const Move& move,const ff_controls::Player& bindings) {
    if(move.input!=Input::Together)return true;
    ff_input::Pad raw;raw.connected=true;
    for(auto action:{move.first,move.second}) {
        const unsigned code=bindings.pad[action];
        if(code==ff_controls::LT)raw.lt=255;else if(code==ff_controls::RT)raw.rt=255;
        else raw.buttons|=static_cast<uint16_t>(code);
    }
    return ff_controls::ActionDown(raw,bindings,move.first) && ff_controls::ActionDown(raw,bindings,move.second);
}
template<typename Binding> inline std::string Command(const Move& move,const ff_controls::Settings& settings,unsigned player,bool pad,Binding binding,int hero=-1) {
    if(move.id==Id::Guard && !settings.guard)return "DISABLED IN GAMEPLAY SETTINGS";
    if(move.input==Input::MoveAway)return pad?"LEFT STICK AWAY":"MOVE AWAY";
    const auto first=binding(Effective(move.first,settings));
    if(first=="UNBOUND")return "UNBOUND";
    if(move.id==Id::GrabFinish && hero==2) {
        const auto second=binding(ff_controls::Hand);return second=="UNBOUND"?"UNBOUND":first+" THEN "+second;
    }
    if(move.input==Input::Repeat)return first+" REPEATEDLY";
    if(move.input==Input::Hold)return "HOLD "+first;
    if(move.input==Input::Together || move.input==Input::After) {
        const auto second=binding(Effective(move.second,settings));if(second=="UNBOUND")return "UNBOUND";
        if(pad && !TogetherAvailable(move,settings.players[player]))return "BINDING CONFLICT";
        return first+(move.input==Input::Together?" AND ":" THEN ")+second;
    }
    return first;
}
inline int Rows(Group group){unsigned count;const auto* moves=Catalog(count);int rows=0;for(unsigned i=0;i<count;++i)rows+=moves[i].group==group;return rows;}
inline const Move& At(Group group,int row){unsigned count;const auto* moves=Catalog(count);for(unsigned i=0;i<count;++i)if(moves[i].group==group && row--==0)return moves[i];return moves[0];}
struct Menu {
    Page page=Page::Closed;int player=0,hero[2]={},players=1,row=0,order[4]={0,1,2,3};Group group=Group::Strikes;bool gameplay=false;
    void Open(bool in_game,int count,int first,int second) {
        *this={};gameplay=in_game;players=count==2?2:1;hero[0]=first>=0 && first<4?first:0;hero[1]=second>=0 && second<4?second:0;
        page=gameplay && players==1?Page::Moves:Page::Players;
    }
    int Count()const{return page==Page::Players?players+1:page==Page::Heroes?5:Rows(group);}
    void Move(int delta){row=(row+Count()+delta)%Count();}
    void Change(int delta){if(page==Page::Moves){group=static_cast<Group>((int(group)+int(Group::Count)+delta)%int(Group::Count));row=0;}}
    void Accept() {
        if(page==Page::Players){if(row==players){page=Page::Closed;return;}player=row;row=0;page=gameplay?Page::Moves:Page::Heroes;}
        else if(page==Page::Heroes){if(row==4){page=Page::Players;row=player;}else{hero[player]=order[row];row=0;page=Page::Moves;}}
    }
    void Back() {
        if(page==Page::Moves && !gameplay){page=Page::Heroes;for(row=0;row<3 && order[row]!=hero[player];++row){}}
        else if(page==Page::Moves && players==2){page=Page::Players;row=player;}
        else if(page==Page::Heroes){page=Page::Players;row=player;}
        else page=Page::Closed;
    }
    void Input(unsigned pressed) {
        if(pressed&ff_input::Cancel)Back();else if(pressed&ff_input::Confirm)Accept();
        else if(pressed&ff_input::Up)Move(-1);else if(pressed&ff_input::Down)Move(1);
        else if(pressed&ff_input::Left)Change(-1);else if(pressed&ff_input::Right)Change(1);
    }
};
struct Navigation {
    bool ready=false;unsigned held=0;uint64_t repeat=0;
    void Reset(){*this={};}
    unsigned Read(unsigned raw,uint64_t now) {
        if(!ready){if(!raw)ready=true;held=raw;return 0;}
        unsigned pressed=raw&~held;
        if(raw!=held)repeat=now+400;
        else if((raw&15) && now>=repeat){pressed|=raw&15;repeat=now+120;}
        held=raw;return pressed;
    }
};
}
