#pragma once
#include <ddraw.h>

static bool g_responsive_presentation=true,g_presentation_installed,g_presentation_suspended;
static void (*g_presentation_focus_lost)();
static bool (*g_presentation_skip_frame)();
static decltype(&PeekMessageA) g_presentation_peek=PeekMessageA;
static bool PresentationActive() {
    if(!g_base)return true;
    HWND window=*reinterpret_cast<HWND*>(g_base+0x1EC4E4);
    if(!window || !IsWindow(window))return true; // Native window initialization/replacement.
    DWORD process=0;GetWindowThreadProcessId(GetForegroundWindow(),&process);
    return process==GetCurrentProcessId() && IsWindowVisible(window) && !IsIconic(window);
}
static bool (*g_presentation_active)()=PresentationActive;
static void PresentationWait() {
    if(MsgWaitForMultipleObjectsEx(0,nullptr,50,QS_ALLINPUT,MWMO_INPUTAVAILABLE)==WAIT_FAILED)Sleep(10);
}
static void (*g_presentation_wait)()=PresentationWait;
static void PresentationDispatch(MSG* message){TranslateMessage(message);DispatchMessageA(message);}
static void (*g_presentation_dispatch)(MSG*)=PresentationDispatch;
static decltype(&PostQuitMessage) g_presentation_post_quit=PostQuitMessage;
static void PresentationSuspend(bool value) {
    if(value==g_presentation_suspended)return;
    g_presentation_suspended=value;
    if(value && g_presentation_focus_lost)g_presentation_focus_lost();
    Log(value?"Presentation suspended while the game window is inactive.\r\n":
        "Presentation resumed after window activation.\r\n");
}
static BOOL PresentationPeekMessage(LPMSG message,HWND window,UINT first,UINT last,UINT flags) {
    if(!g_responsive_presentation || !g_presentation_installed || window || first || last || !(flags&PM_REMOVE))
        return g_presentation_peek(message,window,first,last,flags);
    for(;;) {
        const bool inactive=!g_presentation_active();PresentationSuspend(inactive);
        // Return every pending message to the original dispatcher, including
        // activation, restore, close and WM_QUIT. Never dispatch reentrantly
        // from inside a DirectDraw call or report an idle game frame in the background.
        if(g_presentation_peek(message,window,first,last,flags))return TRUE;
        if(!inactive)return FALSE;
        g_presentation_wait();
    }
}
static int __fastcall GameResponsiveFlip(void* renderer,void*,int) {
    if(g_presentation_skip_frame && g_presentation_skip_frame())return 1;
    // Native pause/confirmation loops render without calling PeekMessage.
    // EndScene has completed at both intercepted sites, so service the same
    // window procedure here without dispatching from inside DirectDraw.
    MSG message;
    while(PresentationPeekMessage(&message,nullptr,0,0,PM_REMOVE)) {
        if(message.message==WM_QUIT){g_presentation_post_quit(static_cast<int>(message.wParam));return 1;}
        g_presentation_dispatch(&message);
    }
    if(!g_presentation_active()){PresentationSuspend(true);return 1;}
    PresentationSuspend(false);
    auto holder=*reinterpret_cast<IDirectDrawSurface***>(static_cast<BYTE*>(renderer)+0x10);
    // Retail's original IDirectDrawSurface::Flip uses DDFLIP_WAIT, which can
    // retry indefinitely when its output is occluded/minimized. This interface
    // predates Surface7: zero flags requests a nonblocking flip. Preserve the
    // native method's success convention; busy/lost frames can be retried on
    // the next frame after Windows has processed activation/restore messages.
    if(holder && *holder)(*holder)->Flip(nullptr,0);
    return 1;
}
static void InstallPresentationGuard() {
    g_responsive_presentation=GetPrivateProfileIntW(L"Compatibility",L"ResponsivePresentation",1,g_config_path)!=0;
    if(!g_responsive_presentation)return;
    const DWORD calls[]={0x25A73,0x25B2A};
    for(DWORD rva:calls) {
        auto code=reinterpret_cast<BYTE*>(g_base+rva);LONG relative;memcpy(&relative,code+1,4);
        if(code[0]!=0xE8 || reinterpret_cast<uintptr_t>(code+5)+relative!=g_base+0x237F0) {
            Log("Presentation call signatures differ; native calls retained.\r\n");return;
        }
    }
    auto start=reinterpret_cast<BYTE*>(g_base+calls[0]);
    const SIZE_T length=calls[1]-calls[0]+5;DWORD previous;
    if(!VirtualProtect(start,length,PAGE_EXECUTE_READWRITE,&previous))return;
    for(DWORD rva:calls) {
        auto code=reinterpret_cast<BYTE*>(g_base+rva);
        LONG relative=static_cast<LONG>(reinterpret_cast<uintptr_t>(GameResponsiveFlip)-reinterpret_cast<uintptr_t>(code+5));
        memcpy(code+1,&relative,4);
    }
    DWORD ignored;VirtualProtect(start,length,previous,&ignored);FlushInstructionCache(GetCurrentProcess(),start,length);
    g_presentation_installed=true;Log("Responsive native surface presentation enabled (process memory only).\r\n");
}
