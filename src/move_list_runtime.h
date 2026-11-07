#pragma once
#include "move_list.h"
#include <vector>
static ff_moves::Menu g_move_list;
static ff_moves::Navigation g_move_list_nav;
static unsigned CampaignRawMenu(bool);
static unsigned(*g_move_list_reader)(bool)=CampaignRawMenu;

static bool MoveListUsePad(unsigned player) {
    if(!g_move_list.gameplay || g_move_list.players==1)return InputHintsUsePad();
    return ModernKeyboardPlayer()!=static_cast<int>(player);
}
static void OpenMoveList(bool gameplay) {
    const int first=gameplay?GuardField<int16_t>(GuardActor(0),0x34):0;
    const int second=gameplay?GuardField<int16_t>(GuardActor(1),0x34):0;
    const bool coop=gameplay && InputField<int>(0x8745A0)==2;
    g_move_list.Open(gameplay,!gameplay || coop?2:1,first,second,coop?CooperativeSoloPlayer():-1);
    if(!gameplay && InputField<int>(0x85A6B8)==26) {
        int order[4];unsigned seen=0;
        for(unsigned i=0;i<4;++i){order[i]=GuardField<int16_t>(GuardActor(i),0x34);if(order[i]>=0 && order[i]<4)seen|=1u<<order[i];}
        if(seen==15)for(unsigned i=0;i<4;++i)g_move_list.order[i]=order[i];
    }
    g_move_list_nav.Reset();
}
static int MoveListWidth(const std::string& text) {
    return reinterpret_cast<int(__cdecl*)(const char*)>(g_base+0x4AEB0)(text.c_str());
}
static std::string MoveListBinding(unsigned player,ff_controls::Action action,bool pad) {
    const auto raw=PlayerHintBinding(player,action,pad);std::string text;
    // The retail font maps ASCII '+' to a PlayStation shoulder-button glyph.
    for(char c:raw)text+=c=='+'?" AND ":std::string(1,c);return text;
}
static void MenuChoiceText(const char* text,int y,bool selected=false) {
    if(selected){ControlText(text,y,true);return;}
    const BYTE neutral[]={6,6,6,6,6,6,6,6,6,6,6,6};
    reinterpret_cast<void(__cdecl*)(const char*,int,int,const BYTE*)>(g_base+0x44A20)(text,(320-MoveListWidth(text))/2,y,neutral);
}
static void DrawMenuBackdrop(DWORD color) {
    g_combat_hide_overlays=true;
    auto device=InputField<IDirect3DDevice2*>(0xAD210);
    if(!device || (*reinterpret_cast<void***>(device))[11]!=reinterpret_cast<void*>(RenderEndScene))return;
    const double width=CameraField<float>(0x9D170),height=CameraField<float>(0x9D174);
    if(!isfinite(width) || !isfinite(height) || width<=0 || height<=0)return;
    CombatOverlayDraw draw(device);draw.Line(width/2,0,width/2,height,width,color);
}
static std::vector<std::string> MoveListWrap(const std::string& text,int width=304) {
    std::vector<std::string> result;std::string line,word;
    auto append=[&]() {
        if(word.empty())return;
        const auto combined=line.empty()?word:line+" "+word;
        if(!line.empty() && MoveListWidth(combined)>width){result.push_back(line);line=word;}
        else line=combined;
        word.clear();
    };
    for(char c:text){if(c==' ')append();else word+=c;}append();if(!line.empty())result.push_back(line);return result;
}
static void DrawMoveList(bool dim_backdrop=true) {
    using namespace ff_moves;auto& menu=g_move_list;char text[96];
    if(menu.page==Page::Closed)return;
    if(dim_backdrop)DrawMenuBackdrop(0xE0101820);
    ControlText("MOVE LIST",10,true);
    if(menu.page==Page::Players) {
        ControlHint("CHOOSE PLAYER LAYOUT",35);
        for(int row=0;row<menu.players;++row) {
            sprintf_s(text,"PLAYER %d",row+1);MenuChoiceText(text,76+row*44,menu.row==row);
            ControlHint(MoveListUsePad(row)?"GAMEPAD":"KEYBOARD AND MOUSE",91+row*44);
        }
        MenuChoiceText("BACK",76+menu.players*44,menu.row==menu.players);
        ControlHint(InputHintsUsePad()?"D-PAD CHOOSE  A SELECT  B BACK":"ARROWS CHOOSE  ENTER SELECT",222);return;
    }
    if(menu.page==Page::Heroes) {
        sprintf_s(text,"PLAYER %d CHARACTER",menu.player+1);ControlHint(text,35);
        for(int row=0;row<5;++row)MenuChoiceText(row==4?"BACK":HeroName(menu.order[row]),65+row*27,menu.row==row);
        ControlHint(InputHintsUsePad()?"D-PAD CHOOSE  A SELECT  B BACK":"ARROWS CHOOSE  ENTER SELECT",222);return;
    }
    sprintf_s(text,"P%d  %s  %s",menu.player+1,HeroName(menu.hero[menu.player]),MoveListUsePad(menu.player)?"GAMEPAD":"KEYBOARD MOUSE");
    ControlHint(text,27);
    sprintf_s(text,"%s  %d OF %d",GroupName(menu.group),int(menu.group)+1,int(Group::Count));ControlText(text,43,true);
    const bool pad=MoveListUsePad(menu.player);
    for(int row=0;row<Rows(menu.group);++row) {
        const auto& move=At(menu.group,row);
        MenuChoiceText(Name(move,menu.hero[menu.player]),61+row*22,row==menu.row);
        const auto command=Command(move,g_controls,menu.player,pad,[&](ff_controls::Action action){return MoveListBinding(menu.player,action,pad);},menu.hero[menu.player]);
        // Compact primary/fallback bindings fit in one line, including pad chords.
        if(row==menu.row)ControlHint(command.c_str(),73+row*22);else MenuChoiceText(command.c_str(),73+row*22);
    }
    const auto& move=At(menu.group,menu.row);
    auto note=std::string(move.note);
    if(move.id==Id::Guard && !g_controls.guard)note="ENABLE PLAYER BLOCK IN GAMEPLAY SETTINGS.";
    if(move.id==Id::GrabFinish && menu.hero[menu.player]==2)note="WHILE HOLDING. LIFT FIRST THEN THROW.";
    const auto command=Command(move,g_controls,menu.player,pad,[&](ff_controls::Action action){return MoveListBinding(menu.player,action,pad);},menu.hero[menu.player]);
    if(command=="UNBOUND" || command=="BINDING CONFLICT")note="ASSIGN COMPATIBLE BUTTONS IN CONTROLS.";
    const auto wrapped=MoveListWrap(note);
    for(unsigned line=0;line<wrapped.size() && line<2;++line)ControlHint(wrapped[line].c_str(),196+line*12);
    ControlHint(InputHintsUsePad()?"UP DOWN CHOOSE LR PAGE B BACK":"UP DOWN CHOOSE LR PAGE ESC BACK",226);
}
static bool PollMoveList() {
    if(!ModernHasFocus()){g_move_list_nav.Reset();return true;}
    const auto pressed=g_move_list_nav.Read(g_move_list_reader(g_move_list.gameplay),GetTickCount64());
    g_move_list.Input(pressed);return g_move_list.page!=ff_moves::Page::Closed;
}
