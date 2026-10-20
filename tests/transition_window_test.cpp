#define WIN32_LEAN_AND_MEAN
#define FIGHTINGFORCE_TRANSITION_TESTS
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static void Log(const char* message){printf("%s",message);}
static uintptr_t g_base;
#include "../src/transition_guard.h"
#define REQUIRE(value) do {if(!(value)){printf("FAIL %s at %d\n",#value,__LINE__);return 1;}}while(0)
static int loading_draws,loading_observations;
static bool loading_covered;
static bool capture_loading;
static void __cdecl NativeLoadingScreen() {
    ++loading_draws;loading_covered=g_transition_gate.pending && g_transition_gate.source==TransitionSource::Loading;
    if(capture_loading) {
        auto image=std::make_shared<LoadingImage>();image->width=4;image->height=3;
        image->pixels.assign(12,0xFF0000);
        for(size_t i=8;i<12;++i)image->pixels[i]=0x00FF00;
        g_loading_capture=image;
    }
}
static void ObserveLoadingRenderer() {++loading_observations;}
static void Branch(DWORD rva,BYTE opcode,uintptr_t target) {
    auto code=reinterpret_cast<BYTE*>(g_base+rva);code[0]=opcode;
    LONG relative=static_cast<LONG>(target-(g_base+rva+5));memcpy(code+1,&relative,4);
}
int main() {
    g_transition_instance=GetModuleHandleW(nullptr);g_transition_offscreen_test=true;g_smooth_transitions=true;
    BeginGameTransition(TransitionSource::Game);
    HWND window=g_transition_curtain.load();REQUIRE(window);
    RECT rect;GetWindowRect(window,&rect);REQUIRE(rect.right<0 && rect.bottom<0);
    DWORD extended=static_cast<DWORD>(GetWindowLongPtrW(window,GWL_EXSTYLE));
    REQUIRE((extended&(WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW))==
        (WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW));
    BYTE alpha;DWORD flags;COLORREF color;
    REQUIRE(GetLayeredWindowAttributes(window,&color,&alpha,&flags));
    REQUIRE(alpha==kTransitionDarkAlpha);REQUIRE(IsWindowVisible(window));
    TransitionSceneComplete();PollGameTransition();Sleep(30);REQUIRE(IsWindowVisible(window));
    TransitionSceneComplete();PollGameTransition();Sleep(260);REQUIRE(!IsWindowVisible(window));
    unsigned old_generation=g_transition_gate.generation;
    BeginGameTransition(TransitionSource::Movie);REQUIRE(IsWindowVisible(window));
    PostMessageW(window,kTransitionReveal,old_generation,0);Sleep(260);
    REQUIRE(IsWindowVisible(window));
    REQUIRE(GetLayeredWindowAttributes(window,&color,&alpha,&flags));REQUIRE(alpha==kTransitionDarkAlpha);
    TransitionSceneComplete();TransitionSceneComplete();PollGameTransition();Sleep(30);REQUIRE(IsWindowVisible(window));
    REQUIRE(!g_transition_gate.MovieFrame(0,1));
    REQUIRE(g_transition_gate.MovieFrame(1,2));RevealGameTransition();Sleep(260);REQUIRE(!IsWindowVisible(window));
    g_base=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,0x600000,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    REQUIRE(g_base);
    Branch(0x22818,0xE8,g_base+0x23B00);Branch(0x22866,0xE8,g_base+0x212B0);
    Branch(0x4B155,0xE8,g_base+0x4AF01);InstallCopyrightTransitions();
    LONG relative;memcpy(&relative,reinterpret_cast<void*>(g_base+0x22819),4);
    REQUIRE(g_base+0x2281D+relative==g_base+0x23B00); // A mismatch leaves all calls untouched.
    Branch(0x4B155,0xE8,g_base+0x4AF00);InstallCopyrightTransitions();
    *reinterpret_cast<BYTE*>(g_base+0x4B15A)=0xC3;
    Branch(0x4AF00,0xE9,reinterpret_cast<uintptr_t>(NativeLoadingScreen));
    *reinterpret_cast<int*>(g_base+0x531F54)=1;
    *reinterpret_cast<void**>(g_base+0xAD210)=reinterpret_cast<void*>(g_base+0x100);
    g_loading_renderer_observe=ObserveLoadingRenderer;
    reinterpret_cast<void(__cdecl*)()>(g_base+0x4B155)();
    REQUIRE(loading_draws==1 && loading_observations==1 && loading_covered);
    REQUIRE(!g_transition_gate.pending && g_transition_gate.source==TransitionSource::Loading);
    Sleep(260);REQUIRE(!IsWindowVisible(window));
    // A disabled/unavailable loading image must never reveal an empty target.
    *reinterpret_cast<int*>(g_base+0x531F54)=0;
    reinterpret_cast<void(__cdecl*)()>(g_base+0x4B155)();
    REQUIRE(loading_draws==2 && g_transition_gate.pending && IsWindowVisible(window));
    TransitionRendererRelease();REQUIRE(g_transition_gate.source==TransitionSource::Game && IsWindowVisible(window));
    REQUIRE(!g_transition_gate.LoadingFrame());
    TransitionSceneComplete();TransitionSceneComplete();PollGameTransition();Sleep(260);REQUIRE(!IsWindowVisible(window));
    capture_loading=true;*reinterpret_cast<int*>(g_base+0x531F54)=1;
    reinterpret_cast<void(__cdecl*)()>(g_base+0x4B155)();
    REQUIRE(g_transition_gate.source==TransitionSource::Game && g_transition_gate.pending);
    Sleep(260);REQUIRE(IsWindowVisible(window)); // No early fade into renderer teardown.
    TransitionRendererRelease();Sleep(30);REQUIRE(IsWindowVisible(window));
    REQUIRE(GetLayeredWindowAttributes(window,&color,&alpha,&flags));REQUIRE(alpha==kTransitionDarkAlpha);
    // Paint the retained native aspect into a wider DIB with black side margins.
    HDC dc=CreateCompatibleDC(nullptr);BITMAPINFO info={};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth=8;info.bmiHeader.biHeight=-4;info.bmiHeader.biPlanes=1;
    info.bmiHeader.biBitCount=32;void* pixels=nullptr;
    HBITMAP bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&pixels,nullptr,0);REQUIRE(bitmap);
    HGDIOBJ old=SelectObject(dc,bitmap);RECT bounds={0,0,8,4};
    PaintLoadingImage(dc,bounds,g_loading_capture.get());GdiFlush();
    auto words=static_cast<DWORD*>(pixels);REQUIRE((words[0]&0xFFFFFF)==0 && (words[7]&0xFFFFFF)==0);
    REQUIRE((words[3]&0xFFFFFF)==0xFF0000 && (words[24+3]&0xFFFFFF)==0x00FF00);
    REQUIRE(LoadingChannel(0xF800,0xF800)==255 && LoadingChannel(0x07E0,0x07E0)==255);
    REQUIRE(LoadingChannel(0x001F,0xF800)==0);
    SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);
    TransitionSceneComplete();PollGameTransition();Sleep(30);REQUIRE(IsWindowVisible(window));
    TransitionSceneComplete();PollGameTransition();Sleep(260);REQUIRE(!IsWindowVisible(window));
    VirtualFree(reinterpret_cast<void*>(g_base),0,MEM_RELEASE);g_base=0;
    PostMessageW(window,WM_CLOSE,0,0);REQUIRE(WaitForSingleObject(g_transition_thread,2000)==WAIT_OBJECT_0);
    REQUIRE(!g_transition_curtain.load());
    CloseHandle(g_transition_thread);CloseHandle(g_transition_ready);
    printf("Offscreen transition window: PASS; dark hold, movie/game/static-image gates, fade and cleanup.\n");
    return 0;
}
