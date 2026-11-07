#pragma once
#include <Xinput.h>
#include "coop_pad.h"
#include "control_config.h"
#include "keyboard_read.h"
#include "input_activity.h"
static ff_input::Activity g_input_activity;

static bool g_modern_keyboard=true,g_modern_gamepad=true;
static unsigned g_modern_focus_epoch;
static int g_modern_controller=-1,g_modern_left_deadzone=7849,g_modern_right_deadzone=8689;
static ff_input::PadSession g_modern_pad_session;
static ff_input::PadSession g_modern_second_session;
static ff_input::CooperativePads g_modern_pad_pair;
static bool g_modern_coop_input;
static int g_modern_second_controller=-1;
static int g_modern_coop_scheme=1; // 1: keyboard + pad; 2: two pads; 3: pad + keyboard.
static bool g_modern_coop_camera=true,g_modern_mouse_combat=true;
using ModernXInput=DWORD(WINAPI*)(DWORD,XINPUT_STATE*);
static ModernXInput g_modern_xinput;
static bool g_modern_quit_button,g_modern_pause_accept_blocked,g_modern_main_back_button;
static ff_input::ConfirmGate g_modern_confirm;
static uint32_t(*g_campaign_pause_reader)();
static bool CampaignPauseOwnsInput(bool inside){return inside && g_campaign_pause_reader!=nullptr;}
static ff_input::PauseGate g_modern_pause;
static bool DefaultModernFocus() {
    DWORD process=0;GetWindowThreadProcessId(GetForegroundWindow(),&process);
    return process==GetCurrentProcessId();
}
static bool (*g_modern_focus)()=DefaultModernFocus;
static bool ModernHasFocus() {return g_modern_focus();}
#include "controller_feedback_output.h"
template<typename T> static T& InputField(DWORD rva) { return *reinterpret_cast<T*>(g_base+rva); }
static bool ModernLiveGameplay() {
    return InputField<int>(0x85A6B8)>=0 && InputField<int>(0x85A6B8)<25 &&
        InputField<int>(0x8745A0)==1 && InputField<int>(0x85A668)!=1 && InputField<int16_t>(0x86A542)>0;
}
static bool ModernKey(unsigned scan) {
    // Query physical positions, independent of the active text-entry layout.
    // No OS input events are generated and no native binding table is changed.
    if(scan&0x100)scan=(scan&255)|0xE000;
    const UINT key=MapVirtualKeyW(scan,MAPVK_VSC_TO_VK_EX);
    return key && (GetAsyncKeyState(static_cast<int>(key))&0x8000)!=0;
}
static bool ReadModernPhysicalKeyboard(BYTE (&state)[256]) {
    ZeroMemory(state,sizeof(state));if(!ModernHasFocus())return false;
    auto keyboard=InputField<IDirectInputDeviceA**>(0x9D180);
    if(!keyboard || !*keyboard)return false;
    bool recovered=false;auto device=*keyboard;
    const bool ok=SUCCEEDED(ReadKeyboardState(sizeof(state),state,true,
        [device](DWORD size,void* output){return device->GetDeviceState(size,output);},
        [device](){return device->Acquire();},&recovered));
    if(ok)g_input_activity.Physical(state);return ok;
}
static bool PhysicalKeyboardKey(const BYTE (&state)[256],unsigned scan) {
    const unsigned index=(scan&0x100)?((scan&127)|128):scan;
    return index<256 && (state[index]&128)!=0;
}
static bool ModernBinding(unsigned code) {
    if(code>=ff_controls::Mouse1 && code<=ff_controls::Mouse5) {
        const int buttons[]={VK_LBUTTON,VK_RBUTTON,VK_MBUTTON,VK_XBUTTON1,VK_XBUTTON2};
        return g_modern_mouse_combat && (GetAsyncKeyState(buttons[code-ff_controls::Mouse1])&0x8000)!=0;
    }
    return code && ModernKey(code);
}
static ff_input::Keyboard PollModernKeyboard() {
    ff_input::Keyboard k;
    if(!g_modern_keyboard || !ModernHasFocus())return k;
    const auto& p=g_controls.players[g_controls_keyboard_player];
    auto down=[&](ff_controls::Action a){return ModernBinding(p.keys[a])||ModernBinding(p.alternate[a]);};
    using namespace ff_controls;
    k.up=down(Forward);k.down=down(Backward);k.left=down(StrafeLeft);k.right=down(StrafeRight);
    k.punch=down(Hand);k.kick=down(Foot);k.jump=down(Jump);k.backfist=down(Backfist);k.run=down(Run);
    k.pause=down(Pause);k.quit=down(Quit);k.first_person=down(View);k.recenter=down(Center);
    k.zoom_in=down(ZoomIn);k.zoom_out=down(ZoomOut);k.block=down(Block);
    k.previous_target=down(PreviousTarget);k.next_target=down(NextTarget);
    k.pickup=down(Pickup);k.throw_item=down(Throw);
    k.look_left=down(LookLeft);k.look_right=down(LookRight);
    k.confirm=ModernKey(0x1C);k.cancel=ModernKey(0x01);
    return k;
}
static ff_input::Keyboard (*g_modern_keyboard_reader)()=PollModernKeyboard;
static ff_input::Keyboard ReadModernKeyboard() {
    auto keys=g_modern_keyboard?g_modern_keyboard_reader():ff_input::Keyboard{};
    if(ModernHasFocus())g_input_activity.Keyboard(g_controls_keyboard_player,keys);return keys;
}
static ff_input::Keyboard ReadModernMenuKeyboard() {
    auto k=ReadModernKeyboard();BYTE raw[256];
    if(!ReadModernPhysicalKeyboard(raw))return k;
    auto down=[&](unsigned scan){return PhysicalKeyboardKey(raw,scan);};
    // Frontend navigation is fixed and independent of gameplay device/layout.
    // Poll the existing DirectInput keyboard: exclusive acquisition can hide
    // held keys from GetAsyncKeyState while still supplying this native device.
    k.up=down(0x148)||down(0x11);k.down=down(0x150)||down(0x1F);
    k.left=down(0x14B)||down(0x1E);k.right=down(0x14D)||down(0x20);
    k.confirm=down(0x1C);k.jump=down(0x39);k.cancel=down(1);return k;
}
static ff_input::Keyboard ReadModernPlayerKeyboard(unsigned player) {
    const auto previous=g_controls_keyboard_player;g_controls_keyboard_player=player;
    auto keys=ReadModernKeyboard();g_controls_keyboard_player=previous;return keys;
}
static int ModernKeyboardPlayer(){return g_modern_coop_scheme==1?0:g_modern_coop_scheme==3?1:-1;}
static ff_input::Pad MapModernPad(ff_input::Pad pad,unsigned player) {return ff_controls::MapPad(pad,g_controls.players[player]);}
static void InitializeModernPads() {
    // Resolve optional controller support outside the loader lock. A missing
    // runtime or disconnected controller leaves keyboard/mouse available.
    static bool initialized=false;
    if(!initialized && !g_modern_xinput) {
        initialized=true;
        const wchar_t* names[]={L"xinput1_4.dll",L"xinput1_3.dll",L"xinput9_1_0.dll"};
        for(const auto name:names) {
            HMODULE runtime=LoadLibraryExW(name,nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
            if(!runtime)continue;
            g_modern_xinput=reinterpret_cast<ModernXInput>(GetProcAddress(runtime,"XInputGetState"));
            g_modern_xinput_set=reinterpret_cast<ModernXInputSet>(GetProcAddress(runtime,"XInputSetState"));
            if(g_modern_xinput)break;
        }
    }
}
static bool ModernCooperativeGameplay();
static bool TwoModernPads(){return g_modern_gamepad && g_modern_coop_input && g_modern_coop_scheme==2;}
static int ModernPadContext(int context) {
    return context>=0?context:InputField<int>(0x85A6B8)*16+InputField<int>(0x8745A0)*4+(InputField<int>(0x85A668)==1?1:0);
}
static bool ReadXInputSlot(int slot,ff_input::Pad& output) {
    XINPUT_STATE state={};
    if(!g_modern_xinput || g_modern_xinput(slot,&state)!=ERROR_SUCCESS)return false;
    const auto& p=state.Gamepad;
    output={true,p.wButtons,p.sThumbLX,p.sThumbLY,p.sThumbRX,p.sThumbRY,p.bLeftTrigger,p.bRightTrigger};return true;
}
static ff_input::Pad ReadModernPadPair(unsigned owner,int context) {
    if(InputField<int>(0x85A6B8)==26 && (g_modern_pad_pair.awaiting_owner || g_modern_pad_pair.pause_requested)) {
        g_modern_pad_pair={};g_modern_pad_session={};g_modern_second_session={};
    }
    const int fixed[]={g_modern_controller,g_modern_second_controller};
    ff_input::PadSession* sessions[]={&g_modern_pad_session,&g_modern_second_session};
    const int previous[]={sessions[0]->selected,sessions[1]->selected};
    const bool focused=ModernHasFocus();
    g_modern_pad_pair.Poll(*sessions[0],*sessions[1],fixed,GetTickCount64(),ModernPadContext(context),
        focused,ModernCooperativeGameplay(),g_modern_left_deadzone,g_modern_right_deadzone,ReadXInputSlot);
    if(!focused)return {};
    ff_input::Pad pads[2];
    for(unsigned i=0;i<2;++i) {
        const int slot=sessions[i]->selected;
        if(previous[i]>=0 && previous[i]!=slot) {
            StopControllerFeedbackSlot(previous[i]);
            g_input_activity.Controller(previous[i],{},g_modern_left_deadzone,g_modern_right_deadzone);
        }
        pads[i]=sessions[i]->Input(g_modern_left_deadzone,g_modern_right_deadzone);
    }
    for(unsigned i=0;i<2;++i)if(sessions[i]->selected>=0)
        g_input_activity.Controller(sessions[i]->selected,pads[i],g_modern_left_deadzone,g_modern_right_deadzone);
    return pads[owner];
}
static const char* ModernPadReconnectHint() {
    if(!TwoModernPads() || !ModernCooperativeGameplay())return nullptr;
    if(g_modern_pad_pair.awaiting_owner && g_modern_pad_pair.connected)return "A FOR PLAYER 1  X FOR PLAYER 2";
    if(g_modern_pad_session.selected<0 && g_modern_second_session.selected<0 &&
       g_modern_pad_session.last_slot>=0 && g_modern_second_session.last_slot>=0)return "RECONNECT A CONTROLLER";
    return nullptr;
}
static ff_input::Pad ReadModernPad(int context=-1) {
    if(!g_modern_gamepad)return {};
    InitializeModernPads();
    if(TwoModernPads())return ReadModernPadPair(0,context);
    const int previous_slot=g_modern_pad_session.selected;
    const auto pad=g_modern_pad_session.Read(GetTickCount64(),ModernHasFocus(),g_modern_controller,ModernPadContext(context),
        g_modern_left_deadzone,g_modern_right_deadzone,ReadXInputSlot);
    if(previous_slot>=0 && previous_slot!=g_modern_pad_session.selected) {
        StopControllerFeedbackSlot(previous_slot);
        g_input_activity.Controller(previous_slot,{},g_modern_left_deadzone,g_modern_right_deadzone);
    }
    static int reported=-1;
    if(g_modern_pad_session.selected!=reported) {
        reported=g_modern_pad_session.selected;
        char message[96];sprintf_s(message,"Modern controller slot: %d (-1 disconnected).\r\n",reported);Log(message);
    }
    if(ModernHasFocus() && g_modern_pad_session.selected>=0)
        g_input_activity.Controller(g_modern_pad_session.selected,pad,g_modern_left_deadzone,g_modern_right_deadzone);
    return pad;
}
static ff_input::Pad ReadModernSecondPad(int context=-1) {
    if(!g_modern_coop_input || !g_modern_gamepad)return {};
    if(g_modern_coop_scheme==1)return ReadModernPad(context);
    if(g_modern_coop_scheme==3)return {};
    InitializeModernPads();
    return ReadModernPadPair(1,context);
}
static ff_input::Pad ReadModernCoopRawPad(unsigned player) {
    if(ModernKeyboardPlayer()==static_cast<int>(player))return {};
    return player && g_modern_coop_scheme==2?ReadModernSecondPad():ReadModernPad();
}
static ff_input::Pad ReadModernCoopPlayerPad(unsigned player){return MapModernPad(ReadModernCoopRawPad(player),player);}
static ff_input::Pad ReadModernFrontendPad(int context=-1) {
    auto pad=ReadModernPad(context);
    if(g_modern_coop_input && (InputField<int>(0x8745A0)==2 || g_modern_coop_scheme==2)) {
        const auto second=ReadModernSecondPad(context);
        if(!pad.connected)return second;
        pad.buttons|=second.buttons;
        if(!pad.lx && !pad.ly) {pad.lx=second.lx;pad.ly=second.ly;}
    }
    return pad;
}
static ff_input::Pad ReadModernCommandPad() {
    const bool coop=g_modern_coop_input && InputField<int>(0x8745A0)==2;
    ff_input::Pad result;
    for(unsigned i=0;i<(coop?2u:1u);++i) {
        auto raw=coop?ReadModernCoopRawPad(i):ReadModernPad();auto mapped=MapModernPad(raw,i);
        raw.buttons=static_cast<uint16_t>((raw.buttons&~(ff_input::Start|ff_input::View))|
            (mapped.buttons&(ff_input::Start|ff_input::View)));
        result.connected|=raw.connected;result.buttons|=raw.buttons;
        if(!result.lx && !result.ly){result.lx=raw.lx;result.ly=raw.ly;}
    }
    return result;
}
static ff_input::Keyboard ReadModernCommandKeyboard() {
    const int owner=InputField<int>(0x8745A0)==2?ModernKeyboardPlayer():0;
    return ReadModernPlayerKeyboard(owner>=0?owner:0);
}
static bool ControlInputBlocked() {
    if(g_controls_menu_active || g_campaign_menu_active)return true;
    if(!g_controls_wait_release)return false;
    if(!ModernHasFocus())return true;
    const auto k=ReadModernMenuKeyboard();ReadModernFrontendPad();
    bool held=k.confirm||k.cancel||k.jump||k.pause||k.up||k.down||k.left||k.right;
    ff_input::PadSession* sessions[]={&g_modern_pad_session,&g_modern_second_session};
    for(auto session:sessions) {
        const auto& p=session->raw;const auto stick=ff_input::Stick(p.lx,p.ly,g_modern_left_deadzone);
        held=held || p.buttons || stick.x!=0 || stick.y!=0;
    }
    if(!held)g_controls_wait_release=false;
    return true;
}
static bool ModernCooperativeGameplay() {
    return g_modern_coop_input && InputField<int>(0x8745A0)==2 &&
        InputField<int>(0x85A6B8)>=0 && InputField<int>(0x85A6B8)<25 && InputField<int>(0x85A668)!=1;
}
static uint32_t ModernCooperativeActions(unsigned player) {
    uint32_t native=InputField<uint32_t>(0x5523E0+player*4);
    const auto pad=ReadModernCoopPlayerPad(player);
    if(!ModernHasFocus())return native&0xFCFF0000;
    native&=0xFCFF0000; // Each modern device belongs to exactly one cooperative player.
    const auto keys=ModernKeyboardPlayer()==static_cast<int>(player)?ReadModernPlayerKeyboard(player):ff_input::Keyboard{};
    return native|ff_input::Gameplay(keys)|ff_input::Gameplay(pad,g_modern_left_deadzone);
}
static void ModernMovieInput(bool begin) {
    static bool held=true;
    if(begin) {StopControllerFeedback();held=true;g_modern_pad_session.ready=false;return;}
    if(!g_modern_gamepad || !InputField<int>(0x55207C))return;
    const auto pad=ReadModernFrontendPad(0x10000); // Movie playback has its own neutral boundary.
    if(!ModernHasFocus()) {held=true;return;}
    const bool down=(pad.buttons&(ff_input::A|ff_input::B|ff_input::Start))!=0;
    if(down && !held) {
        // Use the same abort flag as the native movie window's key handler.
        // Its playback loop still stops sound/timer and destroys every handle.
        InputField<int>(0x20D544)=1;Log("Controller requested native movie skip.\r\n");
    }
    held=down;
}
#include "analog_movement_runtime.h"
static uint32_t ModernGameplayActions(uint32_t native) {
    auto pad=MapModernPad(ReadModernPad(),0);
    if(!ModernLiveGameplay()) {ResetAnalogMotion();return native;}
    if(!ModernHasFocus()) {ResetAnalogMotion();return (g_modern_keyboard||g_modern_gamepad)?native&0xFFFF0000:native;}
    // An XInput pad may also appear as a legacy joystick. Discard that device's
    // duplicate axes/buttons while preserving native command flags.
    if(g_controls_custom || (pad.connected && (InputField<int>(0x6F040)&1)))native&=0xFCFF0000;
    const auto keys=ReadModernKeyboard();
    return native|ff_input::Gameplay(keys)|ff_input::Gameplay(pad,g_modern_left_deadzone)|
        UpdateAnalogMovement(native,keys,pad);
}
template<bool Joystick> static uint32_t NativeModernRead(int player) {
    return reinterpret_cast<uint32_t(__cdecl*)(int)>(g_base+(Joystick?0x22D70:0x22BE0))(player);
}
static bool ModernMainMenu() {
    return InputField<int>(0x85A6B8)==26 && InputField<uintptr_t>(0x6C47C)==g_base+0x2F3B0;
}
static bool ModernCharacterSelection() {
    return InputField<int>(0x85A6B8)==26 && InputField<uintptr_t>(0x6C47C)==g_base+0x2F730;
}
static bool ModernSharedFrontend() {
    return InputField<int>(0x85A6B8)==26 && !ModernCharacterSelection();
}
static bool ModernCooperativeMenu() {
    // Character selection temporarily uses the actor count for its display.
    // The requested game mode, not that count, owns the two input assignments.
    return g_modern_coop_input && InputField<int>(ModernCharacterSelection()?0x86E9C8:0x8745A0)==2;
}
template<bool Joystick> static uint32_t ModernPlayerMenuRead(int device,int player) {
    if(ControlInputBlocked())return 0;
    uint32_t native=NativeModernRead<Joystick>(device);
    if(!ModernSharedFrontend() && (player || InputField<int>(0x93D20)!=0))return native;
    auto pad=ReadModernFrontendPad();
    if(!ModernHasFocus())return Joystick&&g_modern_gamepad?0:native;
    if(pad.connected && Joystick)native=0;
    const auto keys=ReadModernMenuKeyboard();
    const bool main=ModernMainMenu();
    const bool cooperative=ModernCooperativeMenu() &&
        (InputField<int>(0x85A6B8)!=26 || InputField<uintptr_t>(0x6C47C)==g_base+0x2F730);
    if(cooperative) {pad=ReadModernCoopRawPad(0);native=0;}
    const auto player_keys=cooperative && ModernKeyboardPlayer()!=0?ff_input::Keyboard{}:keys;
    const bool back=keys.cancel||(pad.buttons&ff_input::B);
    // Returning from a submenu while Back is held must not open main-menu quit.
    if(!main || !back)g_modern_main_back_button=back;
    uint32_t mapped=ff_input::Menu(pad,g_modern_left_deadzone);
    uint32_t keyboard=ff_input::Menu(player_keys);
    if(main) {
        mapped&=~ff_input::Cancel;keyboard&=~ff_input::Cancel;
        if(keys.cancel)native&=~ff_input::Cancel;
    }
    return native|keyboard|mapped;
}
template<bool Joystick> static uint32_t __cdecl ModernMenuRead(int player) {
    return ModernPlayerMenuRead<Joystick>(player,player);
}
template<bool Joystick> static uint32_t __cdecl ModernSecondMenuRead(int player) {
    if(ControlInputBlocked())return 0;
    if(ModernSharedFrontend())return ModernMenuRead<Joystick>(0);
    if(!g_modern_coop_input)return ModernMenuRead<Joystick>(player);
    uint32_t native=NativeModernRead<Joystick>(player);
    if(InputField<int>(0x93D20)!=0)return native;
    const auto pad=ReadModernCoopRawPad(1);
    if(!ModernHasFocus())return 0;
    native=0;
    const auto keys=ModernKeyboardPlayer()==1?ReadModernMenuKeyboard():ff_input::Keyboard{};
    return native|ff_input::Menu(keys)|ff_input::Menu(pad,g_modern_left_deadzone);
}
template<bool Joystick> static uint32_t ModernAssignedMenuRead(int device,int owner) {
    if(ModernSharedFrontend())return ModernMenuRead<Joystick>(0);
    // Solo accepts both devices for one cursor. Confirming both native cursors
    // sets selection mask 3, which the one-player start condition never accepts.
    if(ModernCharacterSelection() && !ModernCooperativeMenu() && owner!=0)return 0;
    if(!ModernCooperativeMenu())return ModernPlayerMenuRead<Joystick>(device,0);
    if(owner==1)return ModernSecondMenuRead<Joystick>(device);
    if(owner==0)return ModernPlayerMenuRead<Joystick>(device,0);
    return NativeModernRead<Joystick>(device);
}
static uint32_t __cdecl ModernAssignedKeyboard(int device,int owner){return ModernAssignedMenuRead<false>(device,owner);}
static uint32_t __cdecl ModernAssignedJoystick(int device,int owner){return ModernAssignedMenuRead<true>(device,owner);}
// These native readers are shared by both actors. Their argument identifies a
// legacy device, while EDI retains the caller's logical player index. Preserve
// that distinction instead of assigning an entire reader function to player two.
static __declspec(naked) uint32_t __cdecl ModernAssignedKeyboardThunk(int) {
    __asm {
        mov eax,[esp+4]
        push edi
        push eax
        call ModernAssignedKeyboard
        add esp,8
        ret
    }
}
static __declspec(naked) uint32_t __cdecl ModernAssignedJoystickThunk(int) {
    __asm {
        mov eax,[esp+4]
        push edi
        push eax
        call ModernAssignedJoystick
        add esp,8
        ret
    }
}
template<bool Joystick,bool Inside> static uint32_t __cdecl ModernPauseRead(int player) {
    if(CampaignPauseOwnsInput(Inside))return g_campaign_pause_reader();
    if(ControlInputBlocked())return 0;
    uint32_t native=NativeModernRead<Joystick>(player);
    const bool cooperative=ModernCooperativeGameplay();
    if((!g_modern_keyboard&&!g_modern_gamepad) || (!ModernLiveGameplay() && !cooperative))return native;
    const auto pad=ReadModernCommandPad();
    if(!ModernHasFocus()) {g_modern_pause.held=true;g_modern_pause_accept_blocked=true;return 0;}
    const auto keys=ReadModernCommandKeyboard();
    const bool pair_game=cooperative && TwoModernPads();
    native=Joystick && (pad.connected || pair_game)?0:native;
    // The native pause reader switches to frontend mode and treats Space as
    // pause. Reserve Space for jumping; Esc/P pause instead. The paused menu
    // uses horizontal choice: Left selects Continue before confirming it.
    if(Inside)g_modern_pad_pair.pause_requested=false;
    const bool open=keys.pause || (pad.buttons&ff_input::Start) ||
        (!Inside && pair_game && g_modern_pad_pair.pause_requested);
    if(open || Inside)StopControllerFeedback();
    uint32_t pause=0;
    if(!Inside) {
        g_modern_pause.held=open||(pad.buttons&ff_input::B);
        g_modern_pause_accept_blocked=(pad.buttons&ff_input::A)!=0;
        pause=open?ff_input::Pause:0;
    } else {
        pause=g_modern_pause.Read(open||(pad.buttons&ff_input::B),true);
        if(!(pad.buttons&ff_input::A))g_modern_pause_accept_blocked=false;
    }
    if(pause)return pause;
    if(g_modern_keyboard)native&=~ff_input::Pause;
    return native|(Inside ? (ff_input::Menu(keys,false)|ff_input::Menu(pad,g_modern_left_deadzone,false,
        !g_modern_pause_accept_blocked))&~ff_input::Cancel : 0);
}
static int __cdecl ModernQuitKey() {
    if(ControlInputBlocked())return -1;
    int native=reinterpret_cast<int(__cdecl*)()>(g_base+0x22F70)();
    const auto pad=ReadModernCommandPad();
    const auto k=ReadModernCommandKeyboard();
    const bool main=ModernMainMenu();
    const bool escape=g_modern_keyboard && main && (k.cancel || native==0x01);
    if(g_modern_keyboard && native==0x10)native=-1; // Q is no longer a quit shortcut.
    const bool keyboard=g_modern_keyboard && (k.quit || (!g_controls_custom && native==0x44));
    if(g_modern_keyboard && (native==0x44 || (main && native==0x01)))native=-1;
    const bool quit=ModernHasFocus() && (keyboard || (pad.buttons&ff_input::View) ||
        (main && (escape || (pad.buttons&ff_input::B)) && !g_modern_main_back_button));
    g_modern_main_back_button=k.cancel || (pad.buttons&ff_input::B) || escape;
    if(quit&&!g_modern_quit_button)native=0x10;
    g_modern_quit_button=quit;
    if(native==0x10) {
        StopControllerFeedback();
        g_modern_confirm.Begin(k.confirm||(pad.buttons&ff_input::A),k.cancel||(pad.buttons&(ff_input::B|ff_input::View)));
    }
    return native;
}
static int __cdecl ModernQuitAnswer() {
    const int native=reinterpret_cast<int(__cdecl*)()>(g_base+0x22F70)();
    if((!g_modern_keyboard&&!g_modern_gamepad) || native==0x15 || native==0x31)return native;
    const auto pad=ReadModernFrontendPad();
    if(!ModernHasFocus()) { g_modern_confirm.Begin(true,true);return -1; }
    const auto k=ReadModernKeyboard();
    const int answer=g_modern_confirm.Read(k.confirm||(pad.buttons&ff_input::A),
        k.cancel||(pad.buttons&(ff_input::B|ff_input::View)));
    return answer>=0?answer:native;
}
static int __cdecl ModernAnyKey() {
    if(ControlInputBlocked())return -1;
    const int native=reinterpret_cast<int(__cdecl*)()>(g_base+0x22F70)();
    return native<0 && ReadModernPad().buttons ? 0x1C:native;
}
static void InstallModernInputHooks() {
    g_controls=ReadControlSettings(g_config_path);
    g_controls_custom=GetPrivateProfileIntW(L"Controls",L"BindingsVersion",0,g_config_path)!=0;
    g_modern_mouse_combat=GetPrivateProfileIntW(L"Input",L"MouseCombat",1,g_config_path)!=0;
    g_modern_keyboard=GetPrivateProfileIntW(L"Input",L"ModernKeyboard",1,g_config_path)!=0;
    g_modern_gamepad=GetPrivateProfileIntW(L"Input",L"Gamepad",1,g_config_path)!=0;
    g_modern_controller=static_cast<int>(GetPrivateProfileIntW(L"Input",L"ControllerIndex",-1,g_config_path));
    if(g_modern_controller< -1 || g_modern_controller>3)g_modern_controller=-1;
    g_modern_second_controller=static_cast<int>(GetPrivateProfileIntW(L"Input",L"Controller2Index",-1,g_config_path));
    if(g_modern_second_controller< -1 || g_modern_second_controller>3)g_modern_second_controller=-1;
    g_modern_coop_scheme=g_controls.keyboard<0?2:g_controls.keyboard==1?3:1;
    g_modern_coop_camera=g_controls.dynamic;
    g_modern_controller=g_controls.players[g_controls.keyboard==0?1:0].slot;
    g_modern_second_controller=g_controls.players[1].slot;
    g_modern_left_deadzone=static_cast<int>(GetPrivateProfileIntW(L"Input",L"LeftStickDeadzone",7849,g_config_path));
    g_modern_right_deadzone=static_cast<int>(GetPrivateProfileIntW(L"Input",L"RightStickDeadzone",8689,g_config_path));
    if(g_modern_left_deadzone<1000 || g_modern_left_deadzone>30000)g_modern_left_deadzone=7849;
    if(g_modern_right_deadzone<1000 || g_modern_right_deadzone>30000)g_modern_right_deadzone=8689;
    if(!g_modern_keyboard&&!g_modern_gamepad)return;
    struct Call { DWORD rva,target;void* replacement; };
    const Call calls[]={
        {0x4884E,0x22D70,reinterpret_cast<void*>(ModernMenuRead<true>)},
        {0x48857,0x22BE0,reinterpret_cast<void*>(ModernMenuRead<false>)},
        {0x48895,0x22D70,reinterpret_cast<void*>(ModernAssignedJoystickThunk)},
        {0x488A0,0x22BE0,reinterpret_cast<void*>(ModernAssignedKeyboardThunk)},
        {0x4CD96,0x22D70,reinterpret_cast<void*>(ModernAssignedJoystickThunk)},
        {0x4CD9D,0x22BE0,reinterpret_cast<void*>(ModernAssignedKeyboardThunk)},
        {0x4CE3C,0x22D70,reinterpret_cast<void*>(ModernAssignedJoystickThunk)},
        {0x4CE43,0x22BE0,reinterpret_cast<void*>(ModernAssignedKeyboardThunk)},
        {0x4AA6A,0x22D70,reinterpret_cast<void*>(ModernPauseRead<true,false>)},
        {0x4AA75,0x22BE0,reinterpret_cast<void*>(ModernPauseRead<false,false>)},
        {0x4AB00,0x22D70,reinterpret_cast<void*>(ModernPauseRead<true,true>)},
        {0x4AB0C,0x22BE0,reinterpret_cast<void*>(ModernPauseRead<false,true>)},
        {0x42319,0x22F70,reinterpret_cast<void*>(ModernQuitKey)},
        {0x4245D,0x22F70,reinterpret_cast<void*>(ModernQuitAnswer)},
        {0x41BC5,0x22F70,reinterpret_cast<void*>(ModernAnyKey)},
        {0x486DC,0x22F70,reinterpret_cast<void*>(ModernAnyKey)}
    };
    for(const auto& call:calls) {
        auto code=reinterpret_cast<BYTE*>(g_base+call.rva);LONG offset;memcpy(&offset,code+1,4);
        if(code[0]!=0xE8 || reinterpret_cast<uintptr_t>(code+5)+offset!=g_base+call.target) {
            g_modern_keyboard=g_modern_gamepad=false;Log("Modern input signatures differ; native controls retained.\r\n");return;
        }
    }
    // Validate the complete set before making any call-site changes.
    auto start=reinterpret_cast<BYTE*>(g_base+0x41BC5);const SIZE_T length=0x4CE48-0x41BC5;
    DWORD old;if(!VirtualProtect(start,length,PAGE_EXECUTE_READWRITE,&old)) {g_modern_keyboard=g_modern_gamepad=false;return;}
    for(const auto& call:calls) {
        auto code=reinterpret_cast<BYTE*>(g_base+call.rva);
        LONG offset=static_cast<LONG>(reinterpret_cast<BYTE*>(call.replacement)-(code+5));memcpy(code+1,&offset,4);
    }
    DWORD ignored;VirtualProtect(start,length,old,&ignored);FlushInstructionCache(GetCurrentProcess(),start,length);
    Log("Modern gameplay, menu and pause controls enabled in process memory.\r\n");
    g_modern_coop_input=true;
    InstallAnalogMovementHook();
}
