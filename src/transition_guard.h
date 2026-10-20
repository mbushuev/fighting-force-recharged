#pragma once
#include <atomic>
#include <intrin.h>
#include "transition_gate.h"
#include "render_size.h"
#include "loading_image.h"

static bool g_smooth_transitions;
static void (*g_movie_input_poll)(bool begin);
static void (*g_loading_renderer_observe)();
static HINSTANCE g_transition_instance;
static HWND g_transition_game_window;
static unsigned g_requested_render_width=800,g_requested_render_height=600;
static decltype(&CreateWindowExA) g_native_create_window=CreateWindowExA;
static decltype(&DestroyWindow) g_native_destroy_window=DestroyWindow;
static decltype(&RegisterClassExA) g_native_register_class=RegisterClassExA;
static decltype(&GetClientRect) g_native_get_client_rect=GetClientRect;
static TransitionGate g_transition_gate;
static std::atomic<HWND> g_transition_curtain;
static INIT_ONCE g_transition_once=INIT_ONCE_STATIC_INIT;
static HANDLE g_transition_ready;
static HANDLE g_transition_thread;
static constexpr UINT kTransitionHold=WM_APP+70;
static constexpr UINT kTransitionReveal=WM_APP+71;
static constexpr UINT kTransitionBind=WM_APP+72;
static constexpr UINT kTransitionLoadingImage=WM_APP+73;
// A fully opaque covering HWND can make DXGI stop presenting as occluded.
// Retain 1/255 transmission so frames continue composing under an effectively
// black curtain and are ready before the reveal begins.
static constexpr BYTE kTransitionDarkAlpha=254;
#ifdef FIGHTINGFORCE_TRANSITION_TESTS
static bool g_transition_offscreen_test;
#endif

struct TransitionUi {
    HWND target=nullptr;
    bool pending=false,seen_foreground=false;
    BYTE alpha=0,fade_from=0;
    ULONGLONG hold_tick=0,fade_tick=0;
    unsigned generation=0;
    std::shared_ptr<const LoadingImage> loading;
};
static bool TransitionMayCover(TransitionUi& ui) {
#ifdef FIGHTINGFORCE_TRANSITION_TESTS
    if(g_transition_offscreen_test)return true;
#endif
    HWND foreground=GetForegroundWindow();DWORD process=0;
    GetWindowThreadProcessId(foreground,&process);
    if(process==GetCurrentProcessId() && ui.target &&
        (foreground==ui.target || IsChild(ui.target,foreground))) {
        ui.seen_foreground=true;return true;
    }
    if(process==GetCurrentProcessId() && foreground!=ui.target)return false;
    // Protect the tiny gap between the original movie HWND's destruction and
    // its replacement. Yield to other apps and to the game's error dialogs.
    if(!ui.target || !IsWindow(ui.target) || !IsWindowVisible(ui.target))
        return GetTickCount64()-ui.hold_tick<(ui.seen_foreground?300u:1000u);
    return false;
}
static void TransitionPosition(HWND curtain,HWND target) {
    RECT bounds={};
#ifdef FIGHTINGFORCE_TRANSITION_TESTS
    if(g_transition_offscreen_test)bounds={-10000,-10000,-9936,-9936};else
#endif
    {
        MONITORINFO monitor={};monitor.cbSize=sizeof(monitor);
        HMONITOR selected=MonitorFromWindow(target?target:GetForegroundWindow(),MONITOR_DEFAULTTOPRIMARY);
        if(!GetMonitorInfoW(selected,&monitor))return;
        bounds=monitor.rcMonitor;
    }
    SetWindowPos(curtain,HWND_TOPMOST,bounds.left,bounds.top,
        bounds.right-bounds.left,bounds.bottom-bounds.top,SWP_NOACTIVATE|SWP_NOOWNERZORDER);
}
static void TransitionVisible(HWND window,TransitionUi& ui) {
    if(!TransitionMayCover(ui)){ShowWindow(window,SW_HIDE);return;}
    SetWindowPos(window,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_SHOWWINDOW);
    UpdateWindow(window);
}
static LRESULT CALLBACK TransitionWindowProc(HWND window,UINT message,WPARAM wparam,LPARAM lparam) {
    auto ui=reinterpret_cast<TransitionUi*>(GetWindowLongPtrW(window,GWLP_USERDATA));
    if(message==WM_NCCREATE) {
        ui=static_cast<TransitionUi*>(reinterpret_cast<CREATESTRUCTW*>(lparam)->lpCreateParams);
        SetWindowLongPtrW(window,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(ui));
    }
    if(message==WM_ERASEBKGND)return 1;
    if(message==WM_PAINT) {
        PAINTSTRUCT paint;HDC dc=BeginPaint(window,&paint);
        RECT area;GetClientRect(window,&area);PaintLoadingImage(dc,area,ui?ui->loading.get():nullptr);
        EndPaint(window,&paint);return 0;
    }
    if(message==WM_MOUSEACTIVATE)return MA_NOACTIVATE;
    if(message==WM_NCHITTEST)return HTTRANSPARENT;
    if(message==WM_DESTROY){KillTimer(window,1);PostQuitMessage(0);return 0;}
    if(!ui)return DefWindowProcW(window,message,wparam,lparam);
    if(message==kTransitionBind) {
        ui->target=reinterpret_cast<HWND>(lparam);TransitionPosition(window,ui->target);
        if(ui->pending || ui->fade_tick)TransitionVisible(window,*ui);
        return 0;
    }
    if(message==kTransitionHold) {
        ui->generation=static_cast<unsigned>(wparam)>>3;
        if((static_cast<unsigned>(wparam)&7)!=static_cast<unsigned>(TransitionSource::Game)) {
            ui->loading.reset();InvalidateRect(window,nullptr,FALSE);
        }
        ui->pending=true;ui->fade_tick=0;ui->hold_tick=GetTickCount64();
        if(lparam)ui->target=reinterpret_cast<HWND>(lparam);
        TransitionPosition(window,ui->target);
        TransitionVisible(window,*ui);
        BYTE start=ui->alpha;
        // The caller still owns its previous frame while this short fade runs.
        // Only after the curtain is opaque may native teardown continue.
        for(unsigned step=1;step<=6 && start<kTransitionDarkAlpha;++step) {
            ui->alpha=static_cast<BYTE>(start+(kTransitionDarkAlpha-start)*step/6);
            SetLayeredWindowAttributes(window,0,ui->alpha,LWA_ALPHA);
            if(IsWindowVisible(window))Sleep(16);
        }
        return 0;
    }
    if(message==kTransitionLoadingImage) {
        if(static_cast<unsigned>(wparam)!=ui->generation)return 0;
        ui->loading=std::atomic_load(&g_transition_loading_image);
        if(!ui->loading)return 0;
        ui->pending=true;ui->fade_tick=0;ui->alpha=kTransitionDarkAlpha;
        SetLayeredWindowAttributes(window,0,ui->alpha,LWA_ALPHA);
        InvalidateRect(window,nullptr,FALSE);TransitionVisible(window,*ui);return 1;
    }
    if(message==kTransitionReveal) {
        // A posted reveal from the previous scene can arrive after a newer
        // synchronous hold. It must not uncover a replacement still loading.
        if(static_cast<unsigned>(wparam)!=ui->generation)return 0;
        ui->pending=false;ui->fade_from=ui->alpha;ui->fade_tick=GetTickCount64();return 0;
    }
    if(message==WM_TIMER) {
        if(ui->fade_tick) {
            ULONGLONG elapsed=GetTickCount64()-ui->fade_tick;
            ui->alpha=elapsed>=180?0:static_cast<BYTE>(ui->fade_from*(180-elapsed)/180);
            SetLayeredWindowAttributes(window,0,ui->alpha,LWA_ALPHA);
            if(!ui->alpha){ui->fade_tick=0;ShowWindow(window,SW_HIDE);ui->loading.reset();}
        }
        if(ui->pending || ui->fade_tick)TransitionVisible(window,*ui);
        return 0;
    }
    return DefWindowProcW(window,message,wparam,lparam);
}
static DWORD WINAPI TransitionThread(void*) {
    WNDCLASSW cls={};cls.hInstance=g_transition_instance;cls.lpfnWndProc=TransitionWindowProc;
    cls.lpszClassName=L"FightingForce.TransitionCurtain";
    cls.hbrBackground=static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    RegisterClassW(&cls);
    TransitionUi ui;
    HWND window=CreateWindowExW(WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW,
        cls.lpszClassName,L"Fighting Force transition",WS_POPUP,0,0,1,1,nullptr,nullptr,cls.hInstance,&ui);
    if(window) {
        SetLayeredWindowAttributes(window,0,0,LWA_ALPHA);SetTimer(window,1,16,nullptr);
        g_transition_curtain.store(window);
    }
    SetEvent(g_transition_ready);
    if(!window)return 1;
    MSG message;
    while(GetMessageW(&message,nullptr,0,0)>0){TranslateMessage(&message);DispatchMessageW(&message);}
    g_transition_curtain.store(nullptr);return 0;
}
static BOOL CALLBACK StartTransitionThread(PINIT_ONCE,PVOID,PVOID*) {
    g_transition_ready=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    if(!g_transition_ready)return FALSE;
    g_transition_thread=CreateThread(nullptr,0,TransitionThread,nullptr,0,nullptr);
    if(!g_transition_thread)return FALSE;
    return WaitForSingleObject(g_transition_ready,2000)==WAIT_OBJECT_0 && g_transition_curtain.load();
}
static void LogTransitionMode(const char* action) {
    DEVMODEW mode={};mode.dmSize=sizeof(mode);EnumDisplaySettingsW(nullptr,ENUM_CURRENT_SETTINGS,&mode);
    char message[192];sprintf_s(message,"Transition %s; desktop=%lux%lu, %lu bpp, %lu Hz.\r\n",
        action,mode.dmPelsWidth,mode.dmPelsHeight,mode.dmBitsPerPel,mode.dmDisplayFrequency);Log(message);
}
static void BeginGameTransition(TransitionSource source) {
    if(!g_smooth_transitions || !g_transition_gate.Begin(source))return;
    if(!InitOnceExecuteOnce(&g_transition_once,StartTransitionThread,nullptr,nullptr)) {
        Log("Transition curtain unavailable; native rendering retained.\r\n");return;
    }
    DWORD_PTR result=0;
    SendMessageTimeoutW(g_transition_curtain.load(),kTransitionHold,
        (static_cast<WPARAM>(g_transition_gate.generation)<<3)|static_cast<unsigned>(source),
        reinterpret_cast<LPARAM>(g_transition_game_window),SMTO_ABORTIFHUNG,750,&result);
    LogTransitionMode(source==TransitionSource::Movie?"covered for movie":
        source==TransitionSource::Splash?"covered for copyright image":
        source==TransitionSource::Loading?"covered for loading image":"covered for game");
}
static void RevealGameTransition() {
    if(HWND curtain=g_transition_curtain.load())PostMessageW(curtain,kTransitionReveal,g_transition_gate.generation,0);
    LogTransitionMode("ready; fading in");
}
static void TransitionSceneComplete(){if(g_smooth_transitions)g_transition_gate.SceneComplete();}
static void TransitionRendererRelease(){
    if(g_transition_gate.source!=TransitionSource::Movie)BeginGameTransition(TransitionSource::Game);
}
static void PollGameTransition() {
    // Retail 0x4259dd ends its scene, then presents at 0x425aXX. The next
    // native message poll occurs after that presentation. Wait for two such
    // frames so the curtain never exposes a newly allocated swap chain.
    if(g_smooth_transitions && g_transition_gate.PollGame())RevealGameTransition();
}
static bool IsRetailWindowClass(LPCSTR name) {
    return reinterpret_cast<uintptr_t>(name)>65535 && !_stricmp(name,"fforce");
}
static ATOM WINAPI GameRegisterClassExA(const WNDCLASSEXA* value) {
    if(!g_smooth_transitions || !value || !IsRetailWindowClass(value->lpszClassName))return g_native_register_class(value);
    WNDCLASSEXA copy=*value;copy.hbrBackground=static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    return g_native_register_class(&copy);
}
static void BindTransitionWindow(HWND window) {
    g_transition_game_window=window;
    if(HWND curtain=g_transition_curtain.load()) {
        DWORD_PTR result=0;SendMessageTimeoutW(curtain,kTransitionBind,0,reinterpret_cast<LPARAM>(window),SMTO_ABORTIFHUNG,500,&result);
    }
}
static HWND WINAPI GameCreateWindowExA(DWORD extended,LPCSTR cls,LPCSTR title,DWORD style,
    int x,int y,int width,int height,HWND parent,HMENU menu,HINSTANCE instance,void* param) {
    if(!g_smooth_transitions || !IsRetailWindowClass(cls))
        return g_native_create_window(extended,cls,title,style,x,y,width,height,parent,menu,instance,param);
    BeginGameTransition(TransitionSource::Game);
    g_requested_render_width=static_cast<unsigned>(width);g_requested_render_height=static_cast<unsigned>(height);
    HWND window=g_native_create_window(extended&~WS_EX_TOPMOST,cls,title,style&~WS_VISIBLE,
        x,y,width,height,parent,menu,instance,param);
    if(window) {
        RedrawWindow(window,nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_UPDATENOW);
        BindTransitionWindow(window);
        if(style&WS_VISIBLE)ShowWindow(window,SW_SHOW);
        BindTransitionWindow(window);
    }
    return window;
}
static BOOL WINAPI GameDestroyWindow(HWND window) {
    if(g_smooth_transitions && window==g_transition_game_window) {
        BeginGameTransition(TransitionSource::Game);
        BOOL result=g_native_destroy_window(window);if(result)BindTransitionWindow(nullptr);return result;
    }
    return g_native_destroy_window(window);
}
static BOOL WINAPI GameGetClientRect(HWND window,LPRECT area) {
    BOOL result=g_native_get_client_rect(window,area);
    if(result && g_smooth_transitions && g_base && window==g_transition_game_window &&
        reinterpret_cast<uintptr_t>(_ReturnAddress())==g_base+0x2E1FB &&
        *reinterpret_cast<DWORD*>(g_base+0x9D1A4)) {
        RECT physical=*area;*area=RetailInitialRenderRect(g_requested_render_width,g_requested_render_height);
        char message[192];sprintf_s(message,"Native game render size: %ldx%ld; physical client=%ldx%ld; 4:3 retained.\r\n",
            area->right+1,area->bottom+1,physical.right-physical.left,physical.bottom-physical.top);Log(message);
    }
    return result;
}
static int __cdecl GameMovieInitVideoSystem(HWND window,int fullscreen) {
    using Function=int(__cdecl*)(HWND,int);
    static auto real=reinterpret_cast<Function>(GetProcAddress(GetModuleHandleW(L"winplay.dll"),"Player_InitVideoSystem"));
    BeginGameTransition(TransitionSource::Movie);BindTransitionWindow(window);
    if(g_movie_input_poll)g_movie_input_poll(true);
    return real?real(window,fullscreen):-1;
}
static int __cdecl GameMovieReturnPlaybackMode() {
    using Function=int(__cdecl*)();
    static auto real=reinterpret_cast<Function>(GetProcAddress(GetModuleHandleW(L"winplay.dll"),"Player_ReturnPlaybackMode"));
    BeginGameTransition(TransitionSource::Game);return real?real():-1;
}
static int __cdecl GameMoviePlayFrame(void* movie,void* video,void* sound,DWORD a,RECT* area,DWORD b,DWORD c,DWORD d) {
    using Function=int(__cdecl*)(void*,void*,void*,DWORD,RECT*,DWORD,DWORD,DWORD);
    using Frame=unsigned(__cdecl*)(void*);
    static HMODULE player=GetModuleHandleW(L"winplay.dll");
    static auto real=reinterpret_cast<Function>(GetProcAddress(player,"Player_PlayFrame"));
    static auto frame=reinterpret_cast<Frame>(GetProcAddress(player,"Movie_GetCurrentFrame"));
    unsigned before=frame?frame(movie):0;
    int result=real?real(movie,video,sound,a,area,b,c,d):-1;
    if(g_movie_input_poll)g_movie_input_poll(false);
    if(g_smooth_transitions && frame && g_transition_gate.MovieFrame(before,frame(movie)))RevealGameTransition();
    return result;
}
static int __fastcall GameCopyrightBlt(void* surface,void*,RECT* destination,void* source,RECT* source_area,DWORD flags,void* effects) {
    using Function=int(__thiscall*)(void*,RECT*,void*,RECT*,DWORD,void*);
    BeginGameTransition(TransitionSource::Splash);
    int result=reinterpret_cast<Function>(g_base+0x23B00)(surface,destination,source,source_area,flags,effects);
    if(result && g_transition_gate.SplashFrame())RevealGameTransition();
    // Reset Windows' responsiveness timer without dispatching messages into
    // the retail's temporary renderer while its bounded static wait runs.
    MSG message;PeekMessageA(&message,nullptr,0,0,PM_NOREMOVE);
    return result;
}
static void __fastcall GameCopyrightRendererDestroy(void* renderer,void*) {
    using Function=void(__thiscall*)(void*);
    BeginGameTransition(TransitionSource::Game);
    MSG message;PeekMessageA(&message,nullptr,0,0,PM_NOREMOVE);
    reinterpret_cast<Function>(g_base+0x212B0)(renderer);
}
static void __cdecl GameLoadingScreen() {
    BeginGameTransition(TransitionSource::Loading);
    // A loading image presents once before blocking asset work. It cannot
    // satisfy the normal two-frame gameplay gate. Observe its device now so
    // subsequent destruction is covered even if no message poll occurs.
    g_loading_capture.reset();g_loading_capture_requested=g_smooth_transitions;
    if(g_loading_renderer_observe)g_loading_renderer_observe();
    const bool ready=*reinterpret_cast<int*>(g_base+0x531F54)!=0 &&
        *reinterpret_cast<void**>(g_base+0xAD210)!=nullptr;
    reinterpret_cast<void(__cdecl*)()>(g_base+0x4AF00)();
    g_loading_capture_requested=false;
    if(g_smooth_transitions && ready && g_transition_gate.LoadingFrame()) {
        DWORD_PTR retained=0;
        if(g_loading_capture) {
            std::atomic_store(&g_transition_loading_image,g_loading_capture);
            SendMessageTimeoutW(g_transition_curtain.load(),kTransitionLoadingImage,g_transition_gate.generation,
                0,SMTO_ABORTIFHUNG,750,&retained);
        }
        if(retained) {
            // Native code destroys this renderer immediately after its one
            // loading frame. Keep an owned pixel copy until new game frames
            // are ready, then fade it out without exposing the desktop.
            BeginGameTransition(TransitionSource::Game);
            Log("Native loading image retained across renderer replacement.\r\n");
        } else RevealGameTransition();
    }
}
static void InstallCopyrightTransitions() {
    if(!g_smooth_transitions)return;
    struct Call {DWORD rva,target;void* hook;};
    const Call calls[]={{0x22818,0x23B00,reinterpret_cast<void*>(GameCopyrightBlt)},
        {0x22866,0x212B0,reinterpret_cast<void*>(GameCopyrightRendererDestroy)},
        {0x4B155,0x4AF00,reinterpret_cast<void*>(GameLoadingScreen)}};
    for(const auto& call:calls) {
        auto code=reinterpret_cast<BYTE*>(g_base+call.rva);
        LONG relative;memcpy(&relative,code+1,sizeof(relative));
        if(code[0]!=0xE8 || reinterpret_cast<uintptr_t>(code+5)+relative!=g_base+call.target) {
            Log("Static-image transition call signature differs; native code retained.\r\n");return;
        }
    }
    for(const auto& call:calls) {
        auto code=reinterpret_cast<BYTE*>(g_base+call.rva);DWORD previous;
        if(!VirtualProtect(code,5,PAGE_EXECUTE_READWRITE,&previous))continue;
        DWORD relative=static_cast<DWORD>(reinterpret_cast<uintptr_t>(call.hook)-reinterpret_cast<uintptr_t>(code+5));
        memcpy(code+1,&relative,sizeof(relative));DWORD ignored;
        VirtualProtect(code,5,previous,&ignored);FlushInstructionCache(GetCurrentProcess(),code,5);
    }
    Log("Copyright and loading image transitions enabled (process memory only).\r\n");
}
