#pragma once
#include "control_bindings.h"
static ff_controls::Settings g_controls=ff_controls::DefaultsSettings();
static bool g_controls_menu_active;
static bool g_campaign_menu_active;
static unsigned g_controls_keyboard_player;
static bool g_controls_custom;
static bool g_controls_wait_release;
static ff_controls::Settings ReadControlSettings(const wchar_t* path) {
    auto s=ff_controls::DefaultsSettings();
    s.guard=GetPrivateProfileIntW(L"Combat",L"PlayerGuard",0,path)!=0;
    s.focus=GetPrivateProfileIntW(L"Combat",L"TargetFocus",1,path)!=0;
    s.marker=GetPrivateProfileIntW(L"Combat",L"TargetMarker",1,path)!=0;
    s.laser=GetPrivateProfileIntW(L"Combat",L"WeaponLaser",1,path)!=0;
    s.trajectory=GetPrivateProfileIntW(L"Combat",L"ThrowTrajectory",1,path)!=0;
    s.separate_pickup=GetPrivateProfileIntW(L"Combat",L"SeparatePickup",1,path)!=0;
    s.complete_rooms=GetPrivateProfileIntW(L"Rendering",L"CompleteRooms",1,path)!=0;
    s.vibration=GetPrivateProfileIntW(L"Input",L"Vibration",1,path)!=0;
    const int scheme=GetPrivateProfileIntW(L"Input",L"CooperativeInput",1,path);
    s.keyboard=scheme==2?-1:scheme==3?1:0;
    s.dynamic=GetPrivateProfileIntW(L"Camera",L"CooperativeCamera",
        GetPrivateProfileIntW(L"Camera",L"DynamicSplitScreen",1,path),path)!=0;
    const unsigned binding_version=GetPrivateProfileIntW(L"Controls",L"BindingsVersion",0,path);
    for(unsigned i=0;i<2;++i) {
        auto& p=s.players[i];
        p.slot=static_cast<int>(GetPrivateProfileIntW(L"Input",i?L"Controller2Index":L"ControllerIndex",-1,path));
        wchar_t section[40];swprintf_s(section,L"Player%uBindings",i+1);
        const bool older=binding_version<2;
        if(binding_version<3) {
            p.keys[ff_controls::Block]=0x10;
            p.keys[ff_controls::Throw]=0x13;
        }
        auto old=ff_controls::Defaults();old.pad[ff_controls::Run]=ff_input::RightShoulder;
        old.pad[ff_controls::ZoomIn]=ff_controls::RT;old.pad[ff_controls::ZoomOut]=ff_controls::LT;
        old.pad[ff_controls::Block]=ff_input::LeftShoulder;old.pad[ff_controls::Pickup]=ff_input::B;old.pad[ff_controls::Throw]=0;
        bool has_pad=false;
        for(int a=0;a<ff_controls::Count;++a) {
            wchar_t key[48],value[32];swprintf_s(key,L"Pad%s",ff_controls::Key(static_cast<ff_controls::Action>(a)));
            has_pad=has_pad || GetPrivateProfileStringW(section,key,L"",value,32,path)!=0;
        }
        if(older && has_pad)for(int a=0;a<ff_controls::Count;++a)p.pad[a]=old.pad[a];
        for(int a=0;a<ff_controls::Count;++a) {
            wchar_t key[48];const auto name=ff_controls::Key(static_cast<ff_controls::Action>(a));
            swprintf_s(key,L"Key%s",name);p.keys[a]=GetPrivateProfileIntW(section,key,p.keys[a],path);
            swprintf_s(key,L"Alt%s",name);p.alternate[a]=GetPrivateProfileIntW(section,key,p.alternate[a],path);
            swprintf_s(key,L"Pad%s",name);p.pad[a]=GetPrivateProfileIntW(section,key,p.pad[a],path);
        }
        if(older && has_pad) {
            bool retail=true,development=true;
            for(int a=0;a<ff_controls::Count;++a) {
                retail=retail && p.pad[a]==old.pad[a];
                const unsigned before=a==ff_controls::Backfist?(ff_input::RightShoulder|ff_input::B):old.pad[a];
                development=development && p.pad[a]==before;
            }
            if(retail || development) {
                const auto current=ff_controls::Defaults();
                for(int a=0;a<ff_controls::Count;++a)p.pad[a]=current.pad[a];
            }
        }
        using namespace ff_controls;
        if(binding_version==3 && !p.pad[Run]) {
            // An empty Run slot selected automatic running in version 3.
            Bind(p,Run,RT,true);
        }
        if(binding_version<3 && p.keys[Block]==0x10 && p.keys[Throw]==0x13) {
            auto updated=p;
            updated.keys[Block]=updated.keys[Throw]=0;
            if(Bind(updated,Block,0x13,false) && Bind(updated,Throw,0x10,false))p=updated;
        }
    }
    if(s.keyboard==0 && !binding_version) {
        s.players[1].slot=s.players[0].slot;s.players[0].slot=-1;
    }
    ff_controls::Normalize(s);return s;
}
template<typename Edit> static bool SaveWrapperSettings(const wchar_t* path,Edit edit) {
    wchar_t temporary[MAX_PATH];
    if(swprintf_s(temporary,L"%s.controls-%lu.tmp",path,GetCurrentProcessId())<0)return false;
    if(!CopyFileW(path,temporary,FALSE)) {
        if(GetLastError()!=ERROR_FILE_NOT_FOUND)return false;
        HANDLE file=CreateFileW(temporary,GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(file==INVALID_HANDLE_VALUE)return false;CloseHandle(file);
    }
    SetFileAttributesW(temporary,FILE_ATTRIBUTE_NORMAL);
    bool ok=edit(temporary);
    WritePrivateProfileStringW(nullptr,nullptr,nullptr,temporary);
    if(ok)ok=MoveFileExW(temporary,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
    if(!ok)DeleteFileW(temporary);
    return ok;
}
static bool SaveGameplaySettings(const ff_controls::Settings& s,const wchar_t* path) {
    return SaveWrapperSettings(path,[&](const wchar_t* temporary) {
        return WritePrivateProfileStringW(L"Combat",L"PlayerGuard",s.guard?L"1":L"0",temporary) &&
            WritePrivateProfileStringW(L"Combat",L"TargetFocus",s.focus?L"1":L"0",temporary) &&
            WritePrivateProfileStringW(L"Combat",L"TargetMarker",s.marker?L"1":L"0",temporary) &&
            WritePrivateProfileStringW(L"Combat",L"WeaponLaser",s.laser?L"1":L"0",temporary) &&
            WritePrivateProfileStringW(L"Combat",L"ThrowTrajectory",s.trajectory?L"1":L"0",temporary) &&
            WritePrivateProfileStringW(L"Combat",L"SeparatePickup",s.separate_pickup?L"1":L"0",temporary) &&
            WritePrivateProfileStringW(L"Rendering",L"CompleteRooms",s.complete_rooms?L"1":L"0",temporary);
    });
}
static bool SaveControlSettings(const ff_controls::Settings& settings,const wchar_t* path) {
    return SaveWrapperSettings(path,[&](const wchar_t* temporary) {
    auto value=[&](const wchar_t* section,const wchar_t* key,int number) {
        wchar_t text[32];swprintf_s(text,L"%d",number);return WritePrivateProfileStringW(section,key,text,temporary)!=0;
    };
    bool ok=value(L"Controls",L"BindingsVersion",4) && value(L"Input",L"ModernKeyboard",1) &&
        value(L"Input",L"Gamepad",1) && value(L"Input",L"MouseCombat",1) &&
        value(L"Input",L"Vibration",settings.vibration?1:0) &&
        value(L"Input",L"CooperativeInput",settings.keyboard<0?2:settings.keyboard==1?3:1) &&
        value(L"Camera",L"CooperativeCamera",settings.dynamic?1:0) &&
        value(L"Combat",L"PlayerGuard",settings.guard?1:0) &&
        value(L"Combat",L"TargetFocus",settings.focus?1:0) && value(L"Combat",L"TargetMarker",settings.marker?1:0) &&
        value(L"Combat",L"WeaponLaser",settings.laser?1:0) && value(L"Combat",L"ThrowTrajectory",settings.trajectory?1:0) &&
        value(L"Combat",L"SeparatePickup",settings.separate_pickup?1:0) &&
        value(L"Rendering",L"CompleteRooms",settings.complete_rooms?1:0);
    for(unsigned i=0;i<2 && ok;++i) {
        const auto& p=settings.players[i];wchar_t section[40];swprintf_s(section,L"Player%uBindings",i+1);
        ok=value(L"Input",i?L"Controller2Index":L"ControllerIndex",p.slot);
        for(int a=0;a<ff_controls::Count && ok;++a) {
            wchar_t key[48];const auto name=ff_controls::Key(static_cast<ff_controls::Action>(a));
            swprintf_s(key,L"Key%s",name);ok=value(section,key,p.keys[a]);
            swprintf_s(key,L"Alt%s",name);ok=ok && value(section,key,p.alternate[a]);
            swprintf_s(key,L"Pad%s",name);ok=ok && value(section,key,p.pad[a]);
        }
    }
    return ok;
    });
}
