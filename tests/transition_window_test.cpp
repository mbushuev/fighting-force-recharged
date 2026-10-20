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
    PostMessageW(window,WM_CLOSE,0,0);REQUIRE(WaitForSingleObject(g_transition_thread,2000)==WAIT_OBJECT_0);
    REQUIRE(!g_transition_curtain.load());
    CloseHandle(g_transition_thread);CloseHandle(g_transition_ready);
    printf("Offscreen transition window: PASS; dark hold, separate movie/game gates, fade and cleanup.\n");
    return 0;
}
