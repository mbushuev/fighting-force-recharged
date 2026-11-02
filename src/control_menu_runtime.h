#pragma once
#include "control_menu.h"
static ff_controls::Menu g_control_menu;
static uintptr_t g_control_return;
static bool g_control_capture_ready,g_control_nav_ready;
static unsigned g_control_nav_held;
static ULONGLONG g_control_repeat_at;
static const char* g_control_message="";
static BYTE g_control_keyboard[256];
static bool g_control_keyboard_valid;
static bool ControlKey(unsigned scan) {
    return g_control_keyboard_valid?PhysicalKeyboardKey(g_control_keyboard,scan):ModernKey(scan);
}
static void ApplyControlSettings(const ff_controls::Settings& s) {
    ResetCombatFocus();
    StopControllerFeedback();
    if(g_player_guard_available && g_controls.guard!=s.guard)ResetPlayerGuard();
    g_controls=s;g_controls_custom=true;
    g_modern_keyboard=g_modern_gamepad=g_modern_mouse_combat=g_mouse_combat=true;
    g_modern_coop_scheme=s.keyboard<0?2:s.keyboard==1?3:1;g_modern_coop_camera=s.dynamic;
    g_modern_controller=s.players[s.keyboard==0?1:0].slot;g_modern_second_controller=s.players[1].slot;
    g_modern_pad_session={};g_modern_second_session={};ResetAnalogMotion();
    g_coop_ready=false;g_coop_split={};CooperativeReset();
}
static ff_input::Pad ControlRawPad(unsigned slot) {
    ff_input::Pad p;XINPUT_STATE state={};
    if(g_modern_xinput && g_modern_xinput(slot,&state)==ERROR_SUCCESS) {
        const auto& s=state.Gamepad;p={true,s.wButtons,s.sThumbLX,s.sThumbLY,s.sThumbRX,s.sThumbRY,s.bLeftTrigger,s.bRightTrigger};
    }
    if(ModernHasFocus())g_input_activity.Controller(slot,p,g_modern_left_deadzone,g_modern_right_deadzone);
    return p;
}
static int ControlCaptureSlot() {
    const auto& s=g_control_menu.draft;const int player=g_control_menu.page;
    if(s.players[player].slot>=0)return s.players[player].slot;
    int slots[2]={-1,-1};
    for(int i=0;i<2;++i)if(s.keyboard!=i && s.players[i].slot>=0)slots[i]=s.players[i].slot;
    for(int i=0;i<2;++i)if(s.keyboard!=i && slots[i]<0)
        for(int slot=0;slot<4;++slot)if(slot!=slots[1-i] && ControlRawPad(slot).connected){slots[i]=slot;break;}
    return slots[player];
}
static unsigned ControlPressedKey() {
    if(g_control_keyboard_valid) {
        for(unsigned i=1;i<256;++i)if(i!=0xDB && i!=0xDC && (g_control_keyboard[i]&128))
            return i&128?0x100|(i&127):i;
    }
    const int mouse[]={VK_LBUTTON,VK_RBUTTON,VK_MBUTTON,VK_XBUTTON1,VK_XBUTTON2};
    for(unsigned i=0;i<5;++i)if(GetAsyncKeyState(mouse[i])&0x8000)return ff_controls::Mouse1+i;
    if(g_control_keyboard_valid)return 0;
    for(int key=8;key<256;++key) {
        if(key==VK_SHIFT || key==VK_CONTROL || key==VK_MENU || key==VK_LWIN || key==VK_RWIN)continue;
        if(!(GetAsyncKeyState(key)&0x8000))continue;
        UINT scan=MapVirtualKeyW(key,MAPVK_VK_TO_VSC_EX);
        if(scan)return (scan&255)|((scan&0xFF00)?0x100:0);
    }
    return 0;
}
static const char* ControlKeyName(unsigned code,char (&buffer)[40]) {
    if(!code)return "UNBOUND";
    if(code>=ff_controls::Mouse1 && code<=ff_controls::Mouse5){sprintf_s(buffer,"MOUSE %u",code-ff_controls::Mouse1+1);return buffer;}
    const struct {unsigned code;const char* name;} names[]={{1,"ESC"},{0x39,"SPACE"},{0x0F,"TAB"},{0x1C,"ENTER"},
        {0x2A,"LEFT SHIFT"},{0x36,"RIGHT SHIFT"},{0x1D,"LEFT CTRL"},{0x11D,"RIGHT CTRL"},{0x38,"LEFT ALT"},{0x138,"RIGHT ALT"},
        {0x148,"UP"},{0x150,"DOWN"},{0x14B,"LEFT"},{0x14D,"RIGHT"},{0x153,"DELETE"},{0x0E,"BACKSPACE"}};
    for(const auto& name:names)if(name.code==code)return name.name;
    if(code>=0x3B && code<=0x44){sprintf_s(buffer,"F%u",code-0x3A);return buffer;}
    if(code==0x57 || code==0x58){sprintf_s(buffer,"F%u",code-0x4C);return buffer;}
    const UINT vk=MapVirtualKeyW((code&0x100)?((code&255)|0xE000):code,MAPVK_VSC_TO_VK_EX);
    if((vk>='A' && vk<='Z') || (vk>='0' && vk<='9')){buffer[0]=static_cast<char>(vk);buffer[1]=0;return buffer;}
    sprintf_s(buffer,"KEY %03X",code);return buffer;
}
static const char* ControlPadName(unsigned code) {
    using namespace ff_input;
    const struct {unsigned code;const char* name;} names[]={{A,"A"},{B,"B"},{X,"X"},{Y,"Y"},{Start,"START"},{View,"VIEW"},
        {LeftThumb,"L3"},{RightThumb,"R3"},{LeftShoulder,"LB"},{RightShoulder,"RB"},{ff_controls::LT,"LT"},{ff_controls::RT,"RT"}};
    for(const auto& name:names)if(name.code==code)return name.name;return "UNBOUND";
}
static void ControlText(const char* text,int y,bool selected=false,bool hint=false) {
    // Native text uses signed five-bit channel offsets, not 8-bit RGB.
    // -8 is the Options menu's dim gray; +15 is its bright neutral highlight.
    // Supply all four corners to the original gradient font renderer.
    const BYTE normal[]={248,248,248,248,248,248,248,248,248,248,248,248};
    const BYTE highlight[]={15,15,15,15,15,15,15,15,15,15,15,15};
    const BYTE instructions[]={252,6,10,252,6,10,252,6,10,252,6,10};
    const int width=reinterpret_cast<int(__cdecl*)(const char*)>(g_base+0x4AEB0)(text);
    reinterpret_cast<void(__cdecl*)(const char*,int,int,const BYTE*)>(g_base+0x44A20)(text,(320-width)/2,y,hint?instructions:selected?highlight:normal);
}
static void ControlHint(const char* text,int y){ControlText(text,y,false,true);}
#include "input_hints_runtime.h"
static void DrawControlMenu() {
    auto& m=g_control_menu;char text[160];const bool pad=InputHintsUsePad();
    ControlText(m.GameplayPage()?"GAMEPLAY SETTINGS":"CONTROLS AND COOP",12,true);
    if(m.GameplayPage()) {
        const bool values[]={m.draft.guard,m.draft.focus,m.draft.marker,m.draft.laser};
        const char* labels[]={"PLAYER BLOCK","TARGET FOCUS","TARGET MARKER","WEAPON LASER"};
        for(int row=0;row<m.Rows();++row) {
            if(row<4)sprintf_s(text,"%s  %s",labels[row],values[row]?"ON":"OFF");
            else strcpy_s(text,row==4?"SAVE AND BACK":"CANCEL");
            ControlText(text,46+row*22,row==m.row);
        }
        const char* hints[]={"GUARD WITH EMPTY OR LIGHT HANDS","PRESS FOCUS TO LOCK OR RELEASE","HIDE MARKER WITHOUT RELEASING","FIRST PERSON FIREARM AIM GUIDE"};
        if(m.row<4)ControlHint(hints[m.row],190);
        ControlHint(pad?"A CHANGE  B BACK":"ENTER CHANGE  ESC BACK",213);
    } else if(m.page<0) {
        for(int row=0;row<m.Rows();++row) {
            if(row<6) {
                const int player=row/3,kind=row%3;const auto& p=m.draft.players[player];
                if(kind==0)sprintf_s(text,"P%d DEVICE  %s",player+1,m.draft.keyboard==player?"KEYBOARD MOUSE":"GAMEPAD");
                else if(kind==1) {
                    char slot[16];if(p.slot<0)strcpy_s(slot,"AUTOMATIC");else sprintf_s(slot,"%d",p.slot+1);
                    sprintf_s(text,"P%d GAMEPAD  %s",player+1,m.draft.keyboard==player?"NOT USED":slot);
                } else sprintf_s(text,"PLAYER %d BUTTONS",player+1);
            } else if(row==6)sprintf_s(text,"CAMERA  %s",m.draft.dynamic?"DYNAMIC SPLIT":"ORIGINAL SHARED");
            else if(row==7)sprintf_s(text,"VIBRATION  %s",m.draft.vibration?"ON":"OFF");
            else strcpy_s(text,row==8?"SAVE AND BACK":"CANCEL");
            ControlText(text,36+row*15,row==m.row);
        }
        ControlHint(pad?"D-PAD TO CHOOSE AND CHANGE":"ARROWS TO CHOOSE AND CHANGE",197);
        ControlHint(pad?"A SELECT  B BACK":"ENTER SELECT  ESC BACK",213);
    } else {
        sprintf_s(text,"PLAYER %d  %s",m.page+1,m.PadPage()?"GAMEPAD":"KEYBOARD MOUSE");ControlText(text,29,true);
        const int first=m.row/7*7;
        for(int row=first;row<m.Rows() && row<first+7;++row) {
            if(row==m.Actions())strcpy_s(text,"RESET PLAYER BUTTONS");
            else if(row==m.Actions()+1)strcpy_s(text,"BACK");
            else {
                const auto action=static_cast<ff_controls::Action>(row+(m.PadPage()?4:0));const auto& p=m.draft.players[m.page];
                char buffer[40];const char* value=m.PadPage()?ControlPadName(p.pad[action]):ControlKeyName(m.alternate?p.alternate[action]:p.keys[action],buffer);
                sprintf_s(text,"%s  %s",ff_controls::Label(action),value);
            }
            ControlText(text,54+(row-first)*18,row==m.row);
        }
        if(m.capture) {
            ControlHint(m.PadPage()?"PRESS A GAMEPAD BUTTON":"PRESS KEY OR MOUSE BUTTON",190);
            ControlHint(pad?"VIEW AND START CANCEL":"ESC CANCEL  DELETE CLEAR",207);
        } else {
            if(!m.PadPage())ControlHint(m.alternate?"LEFT OR RIGHT  ALTERNATE":"LEFT OR RIGHT  PRIMARY",190);
            else ControlHint("LEFT STICK MOVE  RIGHT LOOK",190);
            ControlHint(pad?"A REBIND  X CLEAR  B BACK":"ENTER REBIND  DELETE CLEAR",207);
        }
    }
    if(*g_control_message)ControlText(g_control_message,226,true);
}
static void __cdecl ControlMenuFrame();
static void OpenControlMenu(uintptr_t back) {
    g_control_menu.Open(g_controls);g_control_return=back;g_controls_menu_active=true;
    g_control_capture_ready=g_control_nav_ready=false;g_control_nav_held=0;g_control_message="";
    InputField<uintptr_t>(0x6C47C)=reinterpret_cast<uintptr_t>(ControlMenuFrame);
}
static void CloseControlMenu() {
    InputField<uintptr_t>(0x6C47C)=g_control_return;g_controls_menu_active=false;
    g_controls_wait_release=true;g_modern_quit_button=g_modern_main_back_button=true;
    g_modern_pad_session.ready=g_modern_second_session.ready=false;
}
static void PollControlMenu() {
    if(!ModernHasFocus()){g_control_nav_ready=g_control_capture_ready=false;return;}
    g_control_keyboard_valid=ReadModernPhysicalKeyboard(g_control_keyboard);
    InitializeModernPads(); // Resolve optional XInput outside DllMain.
    auto& m=g_control_menu;ff_input::Pad all;
    for(unsigned i=0;i<4;++i){auto p=ControlRawPad(i);all.buttons|=p.buttons;all.connected|=p.connected;if(!all.lx && !all.ly){all.lx=p.lx;all.ly=p.ly;}}
    const bool escape=ControlKey(1),clear=ControlKey(0x153)||ControlKey(0x0E)||(!m.capture && (all.buttons&ff_input::X));
    unsigned nav=ff_input::Menu(all,g_modern_left_deadzone,false)|ff_input::Directions(ControlKey(0x148)||ControlKey(0x11),
        ControlKey(0x150)||ControlKey(0x1F),ControlKey(0x14B)||ControlKey(0x1E),ControlKey(0x14D)||ControlKey(0x20));
    if(ControlKey(0x1C)||ControlKey(0x39))nav|=ff_input::Confirm;
    if(escape)nav|=ff_input::Cancel;
    if(m.capture) {
        const int slot=m.PadPage()?ControlCaptureSlot():-1;const auto p=slot>=0?ControlRawPad(slot):ff_input::Pad{};
        const bool chord=(all.buttons&(ff_input::View|ff_input::Start))==(ff_input::View|ff_input::Start);
        const unsigned key=ControlPressedKey();
        const bool down=m.PadPage()?(p.buttons || p.lt>30 || p.rt>30 || key):(key!=0);
        if(!g_control_capture_ready){if(!down)g_control_capture_ready=true;return;}
        if(escape || chord){m.Back();g_control_nav_ready=false;g_control_message="";return;}
        if(clear){m.Bind(0);g_control_nav_ready=false;g_control_message="";return;}
        unsigned code=key;
        if(m.PadPage()) {
            code=p.rt>180?ff_controls::RT:p.lt>180?ff_controls::LT:0;
            if(!code)for(unsigned bit=0x10;bit<=0x8000;bit<<=1)if(p.buttons&bit){code=bit;break;}
        }
        if(code) {
            if(m.Bind(code)){g_control_nav_ready=false;g_control_message="";}
            else {g_control_message="BUTTON ALREADY USED";g_control_capture_ready=false;}
        }
        return;
    }
    if(!g_control_nav_ready){if(!nav && !clear)g_control_nav_ready=true;g_control_nav_held=nav;return;}
    const ULONGLONG now=GetTickCount64();const unsigned fresh=nav&~g_control_nav_held;
    unsigned directions=fresh&15;
    if(nav!=g_control_nav_held)g_control_repeat_at=now+400;
    else if((nav&15) && now>=g_control_repeat_at){directions=nav&15;g_control_repeat_at=now+120;}
    g_control_nav_held=nav;
    if(clear && m.page>=0 && m.row<m.Actions()) {
        m.capture=true;m.Bind(0);g_control_nav_ready=false;g_control_message="";
    } else if(fresh&ff_input::Cancel)m.Back();
    else if(fresh&ff_input::Confirm){m.Accept();g_control_capture_ready=false;}
    else if(directions&ff_input::Up)m.Move(-1);else if(directions&ff_input::Down)m.Move(1);
    else if(directions&ff_input::Left)m.Change(-1);else if(directions&ff_input::Right)m.Change(1);
    if(m.save) {
        m.save=false;
        if(m.draft.guard && !g_player_guard_available){g_control_message="PLAYER BLOCK UNAVAILABLE";return;}
        if(!m.GameplayPage() && m.draft.dynamic && !g_coop_enabled){g_control_message="DYNAMIC CAMERA UNAVAILABLE";return;}
        if(!(m.GameplayPage()?SaveGameplaySettings(m.draft,g_config_path):SaveControlSettings(m.draft,g_config_path)))
            {g_control_message="SAVE FAILED  TRY AGAIN";return;}
        if(m.GameplayPage()) {
            ResetPlayerGuard();ResetCombatFocus();g_controls.guard=m.draft.guard;g_controls.focus=m.draft.focus;
            g_controls.marker=m.draft.marker;g_controls.laser=m.draft.laser;
        } else ApplyControlSettings(m.draft);
        CloseControlMenu();return;
    }
    if(m.close)CloseControlMenu();
}
static void __cdecl ControlMenuFrame(){PollControlMenu();if(g_controls_menu_active)DrawControlMenu();}
#include "options_menu_runtime.h"
static bool CampaignMainMenu();
static void __cdecl ControlFrontend() {
    StopControllerFeedback();
    g_control_keyboard_valid=ReadModernPhysicalKeyboard(g_control_keyboard);
    static bool shortcut=true;const bool down=ModernHasFocus() && ControlKey(0x40);
    auto callback=InputField<uintptr_t>(0x6C47C);
    if(!g_controls_menu_active && !g_campaign_menu_active && (callback==g_base+0x30800 || (ModernMainMenu() && down && !shortcut)))
        OpenControlMenu(callback==g_base+0x30800?g_base+0x2FC30:callback);
    shortcut=down;
    if(g_controls_menu_active)ControlMenuFrame();
    else if(callback && callback!=reinterpret_cast<uintptr_t>(ControlMenuFrame)) {
        if(callback!=g_base+0x2F3B0 || !CampaignMainMenu())
            reinterpret_cast<void(__cdecl*)()>(callback)();
        if(callback==g_base+0x2FC30)PollPlayerGuardOption();
        else g_guard_option_message="";
        if(ModernMainMenu() && !g_campaign_menu_active)ControlHint(InputHintsUsePad()?"CONTROLS IN OPTIONS":"CONTROLS  F6",222);
    }
}
static void InstallControlMenu() {
    const DWORD calls[]={0x2EBAF,0x2F091};const BYTE expected[]={0xFF,0x15,0x7C,0xC4,0x46,0};
    for(DWORD rva:calls)if(memcmp(reinterpret_cast<void*>(g_base+rva),expected,6)) {
        Log("Controls menu signatures differ; original menu retained.\r\n");return;
    }
    auto start=reinterpret_cast<BYTE*>(g_base+calls[0]);const SIZE_T length=calls[1]-calls[0]+6;DWORD old;
    if(!VirtualProtect(start,length,PAGE_EXECUTE_READWRITE,&old))return;
    for(DWORD rva:calls) {
        auto code=reinterpret_cast<BYTE*>(g_base+rva);code[0]=0xE8;code[5]=0x90;
        LONG relative=static_cast<LONG>(reinterpret_cast<uintptr_t>(ControlFrontend)-reinterpret_cast<uintptr_t>(code+5));memcpy(code+1,&relative,4);
    }
    DWORD ignored;VirtualProtect(start,length,old,&ignored);FlushInstructionCache(GetCurrentProcess(),start,length);
    g_wide_menu_overlay=nullptr;
    InstallPlayerGuardOption();
    InstallInputHints();
    Log("Native Controls entry now opens per-player wrapper settings (process memory only).\r\n");
}
