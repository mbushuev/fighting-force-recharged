#pragma once
#include "modern_input.h"

namespace ff_controls {
enum Action {Forward,Backward,StrafeLeft,StrafeRight,Hand,Foot,Jump,Backfist,Run,View,Center,Pause,Quit,ZoomIn,ZoomOut,Block,PreviousTarget,NextTarget,Count};
static constexpr unsigned Mouse1=0x1001,Mouse2=0x1002,Mouse3=0x1003,Mouse4=0x1004,Mouse5=0x1005;
static constexpr unsigned LT=0x10000,RT=0x20000;
struct Player {
    unsigned keys[Count]={},alternate[Count]={},pad[Count]={};
    int slot=-1;
};
struct Settings {
    Player players[2];int keyboard=0;bool dynamic=true,guard=false,vibration=true;
    bool focus=true,marker=true,laser=true,trajectory=true;
};
inline Player Defaults() {
    Player p;
    const unsigned keys[]={0x11,0x1F,0x1E,0x20,Mouse1,Mouse2,0x39,0x12,0x2A,0x0F,Mouse3,0x01,0x44,0,0,0x10,0x2C,0x2E};
    const unsigned alternate[]={0x148,0x150,0x14B,0x14D,0x24,0x25,0,0,0,0,0,0x19,0,0,0,0,0,0};
    const unsigned pad[]={0,0,0,0,ff_input::X,ff_input::Y,ff_input::A,ff_input::B,ff_input::RightShoulder,
        ff_input::LeftThumb,ff_input::RightThumb,ff_input::Start,ff_input::View,RT,LT,ff_input::LeftShoulder,0,0};
    for(int i=0;i<Count;++i){p.keys[i]=keys[i];p.alternate[i]=alternate[i];p.pad[i]=pad[i];}return p;
}
inline Settings DefaultsSettings() {Settings s;s.players[0]=s.players[1]=Defaults();return s;}
inline bool ValidKey(unsigned key) {return !key || key<=0x17F || (key>=Mouse1 && key<=Mouse5);}
inline bool ValidPad(unsigned key) {
    const unsigned buttons=ff_input::A|ff_input::B|ff_input::X|ff_input::Y|ff_input::LeftShoulder|
        ff_input::RightShoulder|ff_input::LeftThumb|ff_input::RightThumb|ff_input::Start|ff_input::View;
    const unsigned remainder=key&(key-1);
    return !key || key==LT || key==RT || ((key&buttons)==key && !(remainder&(remainder-1)));
}
inline void Normalize(Settings& s) {
    if(s.keyboard< -1 || s.keyboard>1)s.keyboard=0;
    for(auto& p:s.players) {
        if(p.slot< -1 || p.slot>3)p.slot=-1;
        const auto defaults=Defaults();
        for(int a=0;a<Count;++a) {
            if(!ValidKey(p.keys[a]))p.keys[a]=defaults.keys[a];
            if(!ValidKey(p.alternate[a]))p.alternate[a]=defaults.alternate[a];
            if(!ValidPad(p.pad[a]))p.pad[a]=defaults.pad[a];
        }
        // A new default must not steal a button in a previously saved layout.
        for(int a=0;a<Block;++a) {
            if(p.pad[Block] && p.pad[Block]==p.pad[a])p.pad[Block]=0;
            if(p.keys[Block] && (p.keys[Block]==p.keys[a] || p.keys[Block]==p.alternate[a]))p.keys[Block]=0;
        }
        for(int a=PreviousTarget;a<Count;++a)for(int old=0;old<a;++old) {
            if(p.keys[a] && (p.keys[a]==p.keys[old] || p.keys[a]==p.alternate[old]))p.keys[a]=0;
            if(p.alternate[a] && (p.alternate[a]==p.keys[old] || p.alternate[a]==p.alternate[old]))p.alternate[a]=0;
            if(p.pad[a] && p.pad[a]==p.pad[old])p.pad[a]=0;
        }
    }
    if(s.keyboard<0 && s.players[0].slot>=0 && s.players[0].slot==s.players[1].slot)s.players[1].slot=-1;
}
inline void SetKeyboard(Settings& s,unsigned player,bool keyboard) {
    if(keyboard)s.keyboard=static_cast<int>(player);
    else if(s.keyboard==static_cast<int>(player))s.keyboard=-1;
    Normalize(s);
}
inline void SetSlot(Settings& s,unsigned player,int slot) {
    s.players[player].slot=slot;
    if(slot>=0 && s.keyboard<0 && s.players[1-player].slot==slot)s.players[1-player].slot=-1;
    Normalize(s);
}
inline bool Bind(Player& p,Action action,unsigned code,bool gamepad,bool alternate=false) {
    if((gamepad && !ValidPad(code)) || (!gamepad && !ValidKey(code)))return false;
    for(int a=0;a<Count;++a)if(a!=action && code &&
        (gamepad?p.pad[a]==code:(p.keys[a]==code || p.alternate[a]==code)))return false;
    if(gamepad)p.pad[action]=code;
    else {
        (alternate?p.alternate[action]:p.keys[action])=code;
        auto& other=alternate?p.keys[action]:p.alternate[action];if(other==code)other=0;
    }
    return true;
}
inline bool Down(const ff_input::Pad& p,unsigned code) {
    return p.connected && (code==LT?p.lt>30:code==RT?p.rt>30:code && (p.buttons&code)==code);
}
inline bool ActionDown(const ff_input::Pad& raw,const Player& bindings,Action action) {
    const unsigned code=bindings.pad[action];if(!Down(raw,code))return false;
    if(code<LT && !(code&(code-1)))for(unsigned other:bindings.pad)
        if(other<LT && (other&(other-1)) && (other&code) && Down(raw,other))return false;
    return true;
}
inline unsigned Trigger(const ff_input::Pad& p,unsigned code) {
    return !p.connected?0:code==LT?p.lt:code==RT?p.rt:Down(p,code)?255:0;
}
inline ff_input::Pad MapPad(const ff_input::Pad& raw,const Player& bindings) {
    ff_input::Pad p=raw;p.buttons=raw.buttons&15;
    const Action actions[]={Hand,Foot,Jump,Backfist,Run,View,Center,Pause,Quit,Block};
    const unsigned canonical[]={ff_input::X,ff_input::Y,ff_input::A,ff_input::B,ff_input::RightShoulder,
        ff_input::LeftThumb,ff_input::RightThumb,ff_input::Start,ff_input::View,ff_input::LeftShoulder};
    for(unsigned i=0;i<10;++i)if(ActionDown(raw,bindings,actions[i]))p.buttons|=static_cast<uint16_t>(canonical[i]);
    p.rt=static_cast<uint8_t>(ActionDown(raw,bindings,ZoomIn)?Trigger(raw,bindings.pad[ZoomIn]):0);
    p.lt=static_cast<uint8_t>(ActionDown(raw,bindings,ZoomOut)?Trigger(raw,bindings.pad[ZoomOut]):0);
    p.previous_target=ActionDown(raw,bindings,PreviousTarget);p.next_target=ActionDown(raw,bindings,NextTarget);return p;
}
inline const wchar_t* Key(Action a) {
    static const wchar_t* keys[]={L"Forward",L"Backward",L"Left",L"Right",L"Punch",L"Kick",L"Jump",L"Backfist",L"Run",L"View",L"Recenter",L"Pause",L"Quit",L"ZoomIn",L"ZoomOut",L"Block",L"PreviousTarget",L"NextTarget"};return keys[a];
}
inline const char* Label(Action a) {
    static const char* labels[]={"FORWARD","BACKWARD","LEFT","RIGHT","PUNCH","KICK","JUMP","BACKFIST","RUN","FIRST PERSON","FOCUS RECENTER","PAUSE","QUIT PROMPT","ZOOM IN","ZOOM OUT","BLOCK","PREVIOUS TARGET","NEXT TARGET"};return labels[a];
}
}
