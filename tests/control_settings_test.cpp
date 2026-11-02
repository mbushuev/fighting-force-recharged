#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../src/control_config.h"
#include "../src/control_menu.h"
static int failures;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
int main() {
    using namespace ff_controls;
    auto settings=DefaultsSettings();CHECK(settings.keyboard==0 && settings.dynamic && !settings.guard);
    CHECK(settings.focus && settings.marker && settings.laser);
    CHECK(settings.players[0].keys[Block]==0x10 && settings.players[0].pad[Block]==ff_input::LeftShoulder);
    auto migrated=settings;migrated.players[0].keys[Run]=0x10;migrated.players[0].pad[Run]=ff_input::LeftShoulder;
    Normalize(migrated);CHECK(!migrated.players[0].keys[Block] && !migrated.players[0].pad[Block]);
    migrated.players[0].keys[Hand]=0x2C;Normalize(migrated);CHECK(!migrated.players[0].keys[PreviousTarget]);
    CHECK(migrated.players[0].keys[Run]==0x10 && migrated.players[0].pad[Run]==ff_input::LeftShoulder);
    ff_input::Pad guard_pad;guard_pad.connected=true;guard_pad.buttons=ff_input::LeftShoulder;
    CHECK(MapPad(guard_pad,settings.players[0]).buttons&ff_input::LeftShoulder);
    CHECK(!(MapPad(guard_pad,migrated.players[0]).buttons&ff_input::LeftShoulder));
    SetKeyboard(settings,1,true);CHECK(settings.keyboard==1);
    SetKeyboard(settings,1,false);CHECK(settings.keyboard==-1);
    SetSlot(settings,0,2);SetSlot(settings,1,2);CHECK(settings.players[0].slot==-1 && settings.players[1].slot==2);
    auto& player=settings.players[1];CHECK(!Bind(player,Jump,ff_input::X,true));
    CHECK(Bind(player,Hand,0,true));CHECK(Bind(player,Jump,ff_input::X,true));
    CHECK(!Bind(player,Hand,0xFFFF,true));CHECK(!Bind(player,Hand,0xFFFFFFFF,false));
    ff_input::Pad raw;raw.connected=true;raw.buttons=ff_input::X|ff_input::DpadLeft;raw.rt=201;
    auto pad=MapPad(raw,player);CHECK((pad.buttons&ff_input::A) && !(pad.buttons&ff_input::X) && (pad.buttons&ff_input::DpadLeft));CHECK(pad.rt==201);
    CHECK(!Bind(player,Run,0x11,false));CHECK(Bind(player,Run,0x13,false));
    CHECK(Bind(player,Hand,Mouse4,false));
    Menu menu;menu.Open(settings);menu.row=0;menu.Change(1);CHECK(menu.draft.keyboard==0);
    menu.row=3;menu.Accept();CHECK(menu.draft.keyboard==1);
    menu.row=6;menu.Change(-1);CHECK(!menu.draft.dynamic);
    menu.row=2;menu.Accept();CHECK(menu.page==0 && menu.PadPage());
    menu.Accept();CHECK(menu.capture);menu.Back();CHECK(!menu.capture && menu.page==0);
    menu.row=menu.Actions();menu.Accept();CHECK(menu.draft.players[0].pad[Hand]==ff_input::X);
    menu.Back();CHECK(menu.page==-1 && menu.row==2);menu.row=8;menu.Accept();CHECK(menu.save);
    CHECK(settings.keyboard==-1 && settings.dynamic && !settings.guard); // Editing a draft does not apply it.
    menu.Open(settings);menu.Back();CHECK(menu.close && !menu.save);
    menu.Open(settings);menu.row=9;menu.Accept();CHECK(menu.close && !menu.save && !settings.guard);
    menu.Open(settings);menu.Move(-1);CHECK(menu.row==9);menu.Move(1);CHECK(menu.row==0);
    menu.row=7;menu.Accept();CHECK(!menu.draft.vibration && settings.vibration && !menu.save);
    wchar_t folder[MAX_PATH],path[MAX_PATH];CHECK(GetTempPathW(MAX_PATH,folder));CHECK(GetTempFileNameW(folder,L"ffc",0,path));
    CHECK(WritePrivateProfileStringW(L"Unrelated",L"Keep",L"73",path));
    CHECK(!ReadControlSettings(path).guard);
    CHECK(WritePrivateProfileStringW(L"Combat",L"PlayerGuard",L"1",path));CHECK(ReadControlSettings(path).guard);
    settings.dynamic=false;CHECK(SaveControlSettings(settings,path));
    auto restored=ReadControlSettings(path);CHECK(restored.keyboard==-1 && !restored.dynamic && !restored.guard);
    CHECK(restored.players[1].slot==2 && restored.players[1].pad[Jump]==ff_input::X);
    CHECK(restored.players[1].keys[Hand]==Mouse4 && restored.players[1].keys[Run]==0x13);
    CHECK(restored.players[1].keys[Block]==0x10 && restored.players[1].pad[Block]==ff_input::LeftShoulder);
    CHECK(GetPrivateProfileIntW(L"Unrelated",L"Keep",0,path)==73);
    auto gameplay=settings;gameplay.focus=false;gameplay.marker=false;gameplay.laser=false;gameplay.guard=true;
    CHECK(SaveGameplaySettings(gameplay,path));
    const auto play=ReadControlSettings(path);CHECK(!play.focus && !play.marker && !play.laser && play.guard);
    CHECK(play.players[1].keys[Hand]==settings.players[1].keys[Hand] && play.keyboard==settings.keyboard);
    Menu gameplay_menu;gameplay_menu.Open(settings);gameplay_menu.page=-2;
    CHECK(gameplay_menu.Rows()==6);gameplay_menu.row=2;gameplay_menu.Accept();
    CHECK(!gameplay_menu.draft.marker && gameplay_menu.draft.focus && settings.marker);
    gameplay_menu.row=5;gameplay_menu.Accept();CHECK(gameplay_menu.close && !gameplay_menu.save);
    settings.guard=true;settings.vibration=false;CHECK(SaveControlSettings(settings,path));
    CHECK(ReadControlSettings(path).guard && !ReadControlSettings(path).vibration);
    CHECK(SetFileAttributesW(path,FILE_ATTRIBUTE_READONLY));settings.dynamic=true;settings.guard=false;
    CHECK(!SaveControlSettings(settings,path));CHECK(!ReadControlSettings(path).dynamic && ReadControlSettings(path).guard);
    wchar_t temporary[MAX_PATH];swprintf_s(temporary,L"%s.controls-%lu.tmp",path,GetCurrentProcessId());
    CHECK(GetFileAttributesW(temporary)==INVALID_FILE_ATTRIBUTES);
    SetFileAttributesW(path,FILE_ATTRIBUTE_NORMAL);CHECK(DeleteFileW(path));
    puts(failures?"Control settings: FAIL":"Control settings: PASS; assignments, bindings, drafts, atomic save and failure isolation.");return failures?1:0;
}
