#pragma once
#include "control_bindings.h"
namespace ff_controls {
struct Menu {
    Settings draft;
    int page=-1,row=0;bool alternate=false,capture=false,save=false,close=false;
    void Open(const Settings& s){*this={};draft=s;page=-1;}
    bool PadPage()const{return page>=0 && draft.keyboard!=page;}
    bool GameplayPage()const{return page==-2;}
    int Actions()const{return Count-(PadPage()?4:0);}
    int Rows()const{return GameplayPage()?6:page<0?10:Actions()+2;}
    Action Selected()const{return static_cast<Action>(row+(PadPage()?4:0));}
    void Move(int direction){row=(row+Rows()+direction)%Rows();}
    void Change(int direction) {
        if(GameplayPage()) {
            bool* option=row==0?&draft.guard:row==1?&draft.focus:row==2?&draft.marker:row==3?&draft.laser:nullptr;
            if(option)*option=!*option;return;
        }
        if(page>=0){if(!PadPage())alternate=!alternate;return;}
        if(row==0 || row==3){unsigned player=row/3;SetKeyboard(draft,player,draft.keyboard!=static_cast<int>(player));}
        if(row==1 || row==4){unsigned player=row/3;if(draft.keyboard!=static_cast<int>(player))SetSlot(draft,player,(draft.players[player].slot+1+5+direction)%5-1);}
        if(row==6)draft.dynamic=!draft.dynamic;
        if(row==7)draft.vibration=!draft.vibration;
    }
    void Accept() {
        if(GameplayPage()) {if(row==4)save=true;else if(row==5)close=true;else Change(1);return;}
        if(page<0) {
            if(row==2 || row==5){page=row/3;row=0;alternate=false;}
            else if(row==8)save=true;else if(row==9)close=true;else Change(1);
        } else if(row==Actions()) {int slot=draft.players[page].slot;draft.players[page]=Defaults();draft.players[page].slot=slot;}
        else if(row==Actions()+1)Back();else capture=true;
    }
    void Back() {
        if(capture)capture=false;
        else if(page>=0){row=page*3+2;page=-1;alternate=false;}
        else close=true;
    }
    bool Bind(unsigned code){if(!capture)return false;if(!ff_controls::Bind(draft.players[page],Selected(),code,PadPage(),alternate))return false;capture=false;return true;}
};
}
