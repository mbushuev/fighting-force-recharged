#pragma once
#include <string>
static thread_local void(*g_campaign_pause_draw)();

static bool InputHintsUsePad() {
    if(g_input_activity.device!=ff_input::Device::Gamepad)return false;
    for(const auto& pad:g_input_activity.pads)if(pad.connected)return true;return false;
}
static const char* CompactHintKey(unsigned code,char (&buffer)[40]) {
    const struct {unsigned code;const char* name;} names[]={
        {0x2A,"LSHIFT"},{0x36,"RSHIFT"},{0x1D,"LCTRL"},{0x11D,"RCTRL"},
        {0x38,"LALT"},{0x138,"RALT"},{0x0E,"BKSPACE"}};
    for(const auto& name:names)if(code==name.code)return name.name;
    return ControlKeyName(code,buffer);
}
static std::string PlayerHintBinding(unsigned player,ff_controls::Action action,bool pad) {
    const auto& bindings=g_controls.players[player];char buffer[40];
    if(pad)return action==ff_controls::Run && !bindings.pad[action]?"STICK":ControlPadName(bindings.pad[action]);
    return CompactHintKey(bindings.keys[action]?bindings.keys[action]:bindings.alternate[action],buffer);
}
static void DrawPlayerInputHints(unsigned player,int y,bool pad) {
    using namespace ff_controls;
    const Action actions[]={Hand,Foot,Jump,Run,Block,View,Center,Pickup};
    const char* labels[]={"PUNCH","KICK","JUMP","RUN","BLOCK","VIEW","FOCUS","PICK"};
    std::string line=player?"P2":"P1";
    for(unsigned i=0;i<8;++i) {
        if(actions[i]==Block && !g_controls.guard)continue;
        if(actions[i]==Center && !g_controls.focus)continue;
        if(actions[i]==Pickup && !g_controls.separate_pickup)continue;
        const auto item=std::string(labels[i])+" "+PlayerHintBinding(player,actions[i],pad);
        const auto combined=line+"  "+item;
        const int width=reinterpret_cast<int(__cdecl*)(const char*)>(g_base+0x4AEB0)(combined.c_str());
        if(width>304) {ControlHint(line.c_str(),y);y+=14;line=item;}
        else line=combined;
    }
    if(!line.empty())ControlHint(line.c_str(),y);
}
static void __cdecl PauseInputHints(const char* text,int x,int y) {
    if(g_campaign_pause_draw)return; // The campaign menu draws after the world flush.
    reinterpret_cast<void(__cdecl*)(const char*,int,int)>(g_base+0x44830)(text,x,y);
    if(const auto reconnect=ModernPadReconnectHint()){ControlHint(reconnect,214);return;}
    const bool coop=InputField<int>(0x8745A0)==2;
    DrawPlayerInputHints(0,40,coop?ModernKeyboardPlayer()!=0:InputHintsUsePad());
    if(coop)DrawPlayerInputHints(1,182,ModernKeyboardPlayer()!=1);
    else {
        const auto resume=PlayerHintBinding(0,ff_controls::Pause,InputHintsUsePad());
        ControlHint((resume+" RESUME").c_str(),200);
        ControlHint(InputHintsUsePad()?"D-PAD CHOOSE  A SELECT":"ARROWS CHOOSE  ENTER SELECT",216);
    }
}
static void __cdecl QuitInputHints(const char*,int,int y,const BYTE*) {
    ControlText("TO QUIT",y,true);
    ControlHint(InputHintsUsePad()?"A YES  B NO":"ENTER YES  ESC NO",160);
}
static bool InstallInputHints() {
    const struct {DWORD rva,target;void* replacement;} calls[]={
        {0x4ACC5,0x44830,reinterpret_cast<void*>(PauseInputHints)},
        {0x4244B,0x44A20,reinterpret_cast<void*>(QuitInputHints)}};
    for(const auto& call:calls) {
        auto code=reinterpret_cast<BYTE*>(g_base+call.rva);LONG offset;memcpy(&offset,code+1,4);
        if(code[0]!=0xE8 || reinterpret_cast<uintptr_t>(code+5)+offset!=g_base+call.target)return false;
    }
    auto start=reinterpret_cast<BYTE*>(g_base+0x4244B);const SIZE_T size=0x4ACCA-0x4244B;DWORD old;
    if(!VirtualProtect(start,size,PAGE_EXECUTE_READWRITE,&old))return false;
    for(const auto& call:calls) {
        auto code=reinterpret_cast<BYTE*>(g_base+call.rva);
        const LONG offset=static_cast<LONG>(reinterpret_cast<uintptr_t>(call.replacement)-reinterpret_cast<uintptr_t>(code+5));memcpy(code+1,&offset,4);
    }
    DWORD ignored;VirtualProtect(start,size,old,&ignored);FlushInstructionCache(GetCurrentProcess(),start,size);
    Log("Device-aware control hints enabled for settings, pause and quit.\r\n");return true;
}
