#pragma once
#include "control_bindings.h"
static ff_controls::Settings g_controls=ff_controls::DefaultsSettings();
static bool g_controls_menu_active;
static unsigned g_controls_keyboard_player;
static bool g_controls_custom;
static bool g_controls_wait_release;
static ff_controls::Settings ReadControlSettings(const wchar_t* path) {
    auto s=ff_controls::DefaultsSettings();
    const int scheme=GetPrivateProfileIntW(L"Input",L"CooperativeInput",1,path);
    s.keyboard=scheme==2?-1:scheme==3?1:0;
    s.dynamic=GetPrivateProfileIntW(L"Camera",L"CooperativeCamera",
        GetPrivateProfileIntW(L"Camera",L"DynamicSplitScreen",1,path),path)!=0;
    for(unsigned i=0;i<2;++i) {
        auto& p=s.players[i];
        p.slot=static_cast<int>(GetPrivateProfileIntW(L"Input",i?L"Controller2Index":L"ControllerIndex",-1,path));
        wchar_t section[40];swprintf_s(section,L"Player%uBindings",i+1);
        for(int a=0;a<ff_controls::Count;++a) {
            wchar_t key[48];const auto name=ff_controls::Key(static_cast<ff_controls::Action>(a));
            swprintf_s(key,L"Key%s",name);p.keys[a]=GetPrivateProfileIntW(section,key,p.keys[a],path);
            swprintf_s(key,L"Alt%s",name);p.alternate[a]=GetPrivateProfileIntW(section,key,p.alternate[a],path);
            swprintf_s(key,L"Pad%s",name);p.pad[a]=GetPrivateProfileIntW(section,key,p.pad[a],path);
        }
    }
    if(s.keyboard==0 && !GetPrivateProfileIntW(L"Controls",L"BindingsVersion",0,path)) {
        s.players[1].slot=s.players[0].slot;s.players[0].slot=-1;
    }
    ff_controls::Normalize(s);return s;
}
static bool SaveControlSettings(const ff_controls::Settings& settings,const wchar_t* path) {
    wchar_t temporary[MAX_PATH];
    if(swprintf_s(temporary,L"%s.controls-%lu.tmp",path,GetCurrentProcessId())<0)return false;
    if(!CopyFileW(path,temporary,FALSE)) {
        if(GetLastError()!=ERROR_FILE_NOT_FOUND)return false;
        HANDLE file=CreateFileW(temporary,GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(file==INVALID_HANDLE_VALUE)return false;CloseHandle(file);
    }
    SetFileAttributesW(temporary,FILE_ATTRIBUTE_NORMAL);
    auto value=[&](const wchar_t* section,const wchar_t* key,int number) {
        wchar_t text[32];swprintf_s(text,L"%d",number);return WritePrivateProfileStringW(section,key,text,temporary)!=0;
    };
    bool ok=value(L"Controls",L"BindingsVersion",1) && value(L"Input",L"ModernKeyboard",1) &&
        value(L"Input",L"Gamepad",1) && value(L"Input",L"MouseCombat",1) &&
        value(L"Input",L"CooperativeInput",settings.keyboard<0?2:settings.keyboard==1?3:1) &&
        value(L"Camera",L"CooperativeCamera",settings.dynamic?1:0);
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
    WritePrivateProfileStringW(nullptr,nullptr,nullptr,temporary);
    if(ok)ok=MoveFileExW(temporary,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
    if(!ok)DeleteFileW(temporary);
    return ok;
}
