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
    CHECK(ValidPad(ff_input::RightShoulder|ff_input::B));
    CHECK(!ValidPad(ff_input::RightShoulder|ff_input::A|ff_input::B));
    auto chords=Defaults();chords.pad[Run]=ff_input::RightShoulder;chords.pad[Pickup]=0;
    CHECK(Bind(chords,Backfist,ff_input::RightShoulder|ff_input::B,true));
    ff_input::Pad chord_pad;chord_pad.connected=true;chord_pad.buttons=ff_input::RightShoulder;
    CHECK((MapPad(chord_pad,chords).buttons&ff_input::RightShoulder) && !(MapPad(chord_pad,chords).buttons&ff_input::B));
    chord_pad.buttons|=ff_input::B;
    CHECK((MapPad(chord_pad,chords).buttons&ff_input::B) && !(MapPad(chord_pad,chords).buttons&ff_input::RightShoulder));
    chord_pad.buttons=ff_input::B;CHECK(!MapPad(chord_pad,chords).buttons);
    CHECK(settings.focus && settings.marker && settings.laser && settings.trajectory);
    CHECK(settings.separate_pickup && settings.players[0].keys[Pickup]==0x21 && settings.players[0].pad[Pickup]==ff_input::RightShoulder);
    chord_pad.buttons=ff_input::B;CHECK(!MapPad(chord_pad,settings.players[0]).pickup && (MapPad(chord_pad,settings.players[0]).buttons&ff_input::B));
    chord_pad.buttons=ff_input::RightShoulder;CHECK(MapPad(chord_pad,settings.players[0]).pickup && !MapPad(chord_pad,settings.players[0]).buttons);
    chord_pad.buttons=ff_input::LeftShoulder;CHECK(MapPad(chord_pad,settings.players[0]).throw_item && !MapPad(chord_pad,settings.players[0]).buttons);
    CHECK(settings.players[0].keys[Block]==0x13 && settings.players[0].keys[Throw]==0x10 &&
        settings.players[0].keys[Backfist]==0x12 && settings.players[0].pad[Block]==LT && settings.players[0].pad[Run]==RT);
    chord_pad.buttons=0;chord_pad.lt=chord_pad.rt=255;const auto triggers=MapPad(chord_pad,settings.players[0]);
    CHECK((triggers.buttons&ff_input::LeftShoulder) && (triggers.buttons&ff_input::RightShoulder) && !triggers.lt && !triggers.rt);
    chord_pad={};chord_pad.connected=true;chord_pad.buttons=ff_input::DpadUp;
    auto zoom=MapPad(chord_pad,settings.players[0]);CHECK(zoom.rt==255 && !zoom.lt && ff_input::Gameplay(zoom,7849)==0);
    chord_pad.buttons=ff_input::DpadDown;zoom=MapPad(chord_pad,settings.players[0]);CHECK(zoom.lt==255 && !zoom.rt && ff_input::Gameplay(zoom,7849)==0);
    const unsigned directions[]={ff_input::DpadUp,ff_input::DpadDown,ff_input::DpadLeft,ff_input::DpadRight};
    for(unsigned direction:directions) {
        chord_pad.buttons=static_cast<uint16_t>(direction);
        CHECK(!(ff_input::Gameplay(MapPad(chord_pad,settings.players[0]),7849)&15));
        CHECK(ff_input::Menu(chord_pad,7849)&15); // Raw frontend directions remain available.
    }
    chord_pad.buttons=ff_input::DpadLeft;CHECK(MapPad(chord_pad,settings.players[0]).look_left);
    chord_pad.buttons=ff_input::DpadRight;CHECK(MapPad(chord_pad,settings.players[0]).look_right);
    auto reserved=settings;reserved.players[0].pad[Foot]=ff_input::DpadLeft;Normalize(reserved);
    CHECK(!reserved.players[0].pad[LookLeft] && reserved.players[0].pad[Foot]==ff_input::DpadLeft);
    auto migrated=settings;migrated.players[0].keys[Run]=0x13;migrated.players[0].pad[Run]=LT;
    Normalize(migrated);CHECK(!migrated.players[0].keys[Block] && !migrated.players[0].pad[Block]);
    migrated.players[0].keys[Hand]=0x2C;Normalize(migrated);CHECK(!migrated.players[0].keys[PreviousTarget]);
    CHECK(migrated.players[0].keys[Run]==0x13 && migrated.players[0].pad[Run]==LT);
    ff_input::Pad guard_pad;guard_pad.connected=true;guard_pad.lt=255;
    CHECK(MapPad(guard_pad,settings.players[0]).buttons&ff_input::LeftShoulder);
    CHECK(!(MapPad(guard_pad,migrated.players[0]).buttons&ff_input::LeftShoulder));
    SetKeyboard(settings,1,true);CHECK(settings.keyboard==1);
    SetKeyboard(settings,1,false);CHECK(settings.keyboard==-1);
    SetSlot(settings,0,2);SetSlot(settings,1,2);CHECK(settings.players[0].slot==-1 && settings.players[1].slot==2);
    auto& player=settings.players[1];CHECK(!Bind(player,Jump,ff_input::X,true));
    CHECK(Bind(player,Hand,0,true));CHECK(Bind(player,Jump,ff_input::X,true));
    CHECK(!Bind(player,Hand,0xFFFF,true));CHECK(!Bind(player,Hand,0xFFFFFFFF,false));
    ff_input::Pad raw;raw.connected=true;raw.buttons=ff_input::X|ff_input::DpadLeft;raw.rt=201;
    auto pad=MapPad(raw,player);CHECK((pad.buttons&ff_input::A) && !(pad.buttons&ff_input::X) && !(pad.buttons&15));CHECK(!pad.rt && (pad.buttons&ff_input::RightShoulder));
    CHECK(!Bind(player,Run,0x11,false));CHECK(Bind(player,Run,0x14,false));
    CHECK(Bind(player,Hand,Mouse4,false));
    CHECK(Bind(player,LookLeft,0x16,false));
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
    CHECK(WritePrivateProfileStringW(L"Player1Bindings",L"PadBackfist",L"8192",path));
    auto legacy=ReadControlSettings(path);CHECK(legacy.players[0].pad[Backfist]==ff_input::B);
    CHECK(legacy.players[0].pad[Pickup]==ff_input::RightShoulder && legacy.players[0].pad[Block]==LT && legacy.players[0].pad[Throw]==ff_input::LeftShoulder);
    CHECK(WritePrivateProfileStringW(L"Player1Bindings",L"PadBackfist",L"8704",path));
    CHECK(ReadControlSettings(path).players[0].pad[Backfist]==ff_input::B); // Interim default layout migrates too.
    CHECK(WritePrivateProfileStringW(L"Player1Bindings",L"PadBackfist",L"8192",path));
    CHECK(WritePrivateProfileStringW(L"Player1Bindings",L"PadQuit",L"0",path));
    legacy=ReadControlSettings(path);CHECK(legacy.players[0].pad[Backfist]==ff_input::B && !legacy.players[0].pad[Pickup]);
    CHECK(!legacy.players[0].pad[Quit]);
    CHECK(legacy.players[0].keys[Block]==0x13 && legacy.players[0].keys[Throw]==0x10);
    CHECK(WritePrivateProfileStringW(L"Controls",L"BindingsVersion",L"3",path));
    CHECK(WritePrivateProfileStringW(L"Player1Bindings",L"PadRun",L"0",path));
    CHECK(WritePrivateProfileStringW(L"Player1Bindings",L"PadFire",L"131072",path));
    CHECK(WritePrivateProfileStringW(L"Player1Bindings",L"KeyBlock",L"19",path));
    CHECK(WritePrivateProfileStringW(L"Player1Bindings",L"KeyThrow",L"16",path));
    legacy=ReadControlSettings(path);
    CHECK(legacy.players[0].pad[Run]==RT && legacy.players[0].pad[Pickup]==ff_input::RightShoulder);
    CHECK(legacy.players[0].keys[Block]==0x13 && legacy.players[0].keys[Throw]==0x10 && !legacy.players[0].pad[Quit]);
    CHECK(WritePrivateProfileStringW(L"Player1Bindings",L"KeyBlock",L"20",path));
    CHECK(WritePrivateProfileStringW(L"Player1Bindings",L"PadZoomIn",L"131072",path));
    legacy=ReadControlSettings(path);
    CHECK(!legacy.players[0].pad[Run] && legacy.players[0].pad[ZoomIn]==RT);
    CHECK(legacy.players[0].keys[Block]==0x14 && legacy.players[0].keys[Throw]==0x10);
    CHECK(!ReadControlSettings(path).guard);
    CHECK(WritePrivateProfileStringW(L"Combat",L"PlayerGuard",L"1",path));CHECK(ReadControlSettings(path).guard);
    settings.dynamic=false;CHECK(SaveControlSettings(settings,path));
    auto restored=ReadControlSettings(path);CHECK(restored.keyboard==-1 && !restored.dynamic && !restored.guard);
    CHECK(restored.players[1].slot==2 && restored.players[1].pad[Jump]==ff_input::X);
    CHECK(restored.players[1].keys[Hand]==Mouse4 && restored.players[1].keys[Run]==0x14);
    CHECK(restored.players[1].keys[Block]==0x13 && restored.players[1].keys[Throw]==0x10 && restored.players[1].pad[Block]==LT);
    CHECK(restored.players[1].pad[Throw]==ff_input::LeftShoulder && GetPrivateProfileIntW(L"Controls",L"BindingsVersion",0,path)==4);
    CHECK(restored.players[1].keys[LookLeft]==0x16 && restored.players[1].pad[LookLeft]==ff_input::DpadLeft && restored.players[1].pad[LookRight]==ff_input::DpadRight);
    CHECK(GetPrivateProfileIntW(L"Unrelated",L"Keep",0,path)==73);
    auto gameplay=settings;gameplay.focus=false;gameplay.marker=false;gameplay.laser=false;gameplay.trajectory=false;gameplay.guard=true;gameplay.separate_pickup=false;gameplay.complete_rooms=false;
    CHECK(SaveGameplaySettings(gameplay,path));
    const auto play=ReadControlSettings(path);CHECK(!play.focus && !play.marker && !play.laser && !play.trajectory && play.guard);
    CHECK(!play.separate_pickup && !play.complete_rooms);
    CHECK(play.players[1].keys[Hand]==settings.players[1].keys[Hand] && play.keyboard==settings.keyboard);
    Menu gameplay_menu;gameplay_menu.Open(settings);gameplay_menu.page=-2;
    CHECK(gameplay_menu.Rows()==9);gameplay_menu.row=2;gameplay_menu.Accept();
    CHECK(!gameplay_menu.draft.marker && gameplay_menu.draft.focus && settings.marker);
    gameplay_menu.row=4;gameplay_menu.Accept();CHECK(!gameplay_menu.draft.trajectory && settings.trajectory);
    gameplay_menu.row=5;gameplay_menu.Accept();CHECK(!gameplay_menu.draft.separate_pickup && settings.separate_pickup);
    gameplay_menu.row=6;gameplay_menu.Accept();CHECK(!gameplay_menu.draft.complete_rooms && settings.complete_rooms);
    gameplay_menu.row=8;gameplay_menu.Accept();CHECK(gameplay_menu.close && !gameplay_menu.save);
    gameplay_menu.Open(settings);gameplay_menu.page=-2;gameplay_menu.row=7;gameplay_menu.Accept();
    CHECK(gameplay_menu.save && !gameplay_menu.close);
    settings.guard=true;settings.vibration=false;CHECK(SaveControlSettings(settings,path));
    CHECK(ReadControlSettings(path).guard && !ReadControlSettings(path).vibration);
    CHECK(SetFileAttributesW(path,FILE_ATTRIBUTE_READONLY));settings.dynamic=true;settings.guard=false;
    CHECK(!SaveControlSettings(settings,path));CHECK(!ReadControlSettings(path).dynamic && ReadControlSettings(path).guard);
    wchar_t temporary[MAX_PATH];swprintf_s(temporary,L"%s.controls-%lu.tmp",path,GetCurrentProcessId());
    CHECK(GetFileAttributesW(temporary)==INVALID_FILE_ATTRIBUTES);
    SetFileAttributesW(path,FILE_ATTRIBUTE_NORMAL);CHECK(DeleteFileW(path));
    puts(failures?"Control settings: FAIL":"Control settings: PASS; assignments, bindings, drafts, atomic save and failure isolation.");return failures?1:0;
}
