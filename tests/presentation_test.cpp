#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static uintptr_t g_base;
static wchar_t g_config_path[]=L"presentation-test-missing.ini";
static void Log(const char*) {}
#include "../src/presentation_guard.h"
#define CHECK(x) do {if(!(x)){printf("FAIL %s at %d\n",#x,__LINE__);return 1;}}while(0)
static bool active=true,queued=false;
static int waits,losses,flips,dispatched,quit_code=-1;
static bool resume_after_message;
static UINT pending_message=WM_ACTIVATEAPP;
static DWORD flip_flags=~0u;
static HRESULT flip_result=DD_OK;
static bool Active(){return active;}
static void Lost(){++losses;}
static void Wait(){++waits;if(waits==1)queued=true;else active=true;}
static void Dispatch(MSG*){++dispatched;if(resume_after_message)active=true;}
static void WINAPI PostQuit(int code){quit_code=code;}
static BOOL WINAPI Peek(LPMSG message,HWND,UINT,UINT,UINT) {
    if(!queued)return FALSE;queued=false;message->message=pending_message;message->wParam=7;return TRUE;
}
static HRESULT STDMETHODCALLTYPE Flip(void*,void* target,DWORD flags) {
    if(target)ExitProcess(2);++flips;flip_flags=flags;return flip_result;
}
static void Branch(DWORD rva,DWORD target) {
    auto code=reinterpret_cast<BYTE*>(g_base+rva);code[0]=0xE8;
    LONG relative=static_cast<LONG>(target-rva-5);memcpy(code+1,&relative,4);code[5]=0xC3;
}
// Enter at the original CALL instruction with its existing thiscall argument.
// RET after that call also checks the replacement's native stack cleanup.
__declspec(naked) static int InvokeFlip(void*,void*) {
    __asm {
        mov ecx,[esp+8]
        push 1234h
        jmp dword ptr [esp+8]
    }
}
int main() {
    g_base=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,0x200000,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    CHECK(g_base);Branch(0x25A73,0x237F0);Branch(0x25B2A,0x237F1);
    BYTE unchanged[6];memcpy(unchanged,reinterpret_cast<void*>(g_base+0x25A73),6);
    InstallPresentationGuard();CHECK(!g_presentation_installed);
    CHECK(!memcmp(unchanged,reinterpret_cast<void*>(g_base+0x25A73),6));
    Branch(0x25B2A,0x237F0);InstallPresentationGuard();CHECK(g_presentation_installed);
    g_presentation_active=Active;g_presentation_peek=Peek;g_presentation_wait=Wait;g_presentation_focus_lost=Lost;
    g_presentation_dispatch=Dispatch;g_presentation_post_quit=PostQuit;
    void* table[36]={};table[11]=reinterpret_cast<void*>(Flip);void** surface=table;
    auto surface_pointer=reinterpret_cast<IDirectDrawSurface*>(&surface);
    void* renderer[6]={};renderer[4]=&surface_pointer;
    CHECK(InvokeFlip(reinterpret_cast<void*>(g_base+0x25A73),renderer)==1);
    CHECK(flips==1 && flip_flags==0);
    flip_result=DDERR_WASSTILLDRAWING;
    CHECK(InvokeFlip(reinterpret_cast<void*>(g_base+0x25B2A),renderer)==1);CHECK(flips==2);
    active=false;
    MSG message={};CHECK(PresentationPeekMessage(&message,nullptr,0,0,PM_REMOVE));
    CHECK(message.message==WM_ACTIVATEAPP && waits==1 && losses==1);
    CHECK(!PresentationPeekMessage(&message,nullptr,0,0,PM_REMOVE));
    CHECK(active && waits==2 && !g_presentation_suspended);
    // The pause loop has no native message pump. A flip must dispatch its
    // restore message, wait without advancing, then present exactly once.
    active=false;queued=true;resume_after_message=true;
    CHECK(InvokeFlip(reinterpret_cast<void*>(g_base+0x25A73),renderer)==1);
    CHECK(flips==3 && dispatched==1 && losses==2 && active);
    // Leave WM_QUIT and filtered/nonremoving polls under native ownership.
    active=false;queued=true;pending_message=WM_QUIT;
    CHECK(PresentationPeekMessage(&message,nullptr,0,0,PM_REMOVE));CHECK(message.message==WM_QUIT);
    CHECK(waits==2 && losses==3);
    queued=true;CHECK(InvokeFlip(reinterpret_cast<void*>(g_base+0x25B2A),renderer)==1);
    CHECK(quit_code==7 && flips==3 && dispatched==1);
    CHECK(!PresentationPeekMessage(&message,nullptr,0,0,PM_NOREMOVE));CHECK(waits==2);
    CHECK(!PresentationPeekMessage(&message,nullptr,WM_KEYFIRST,WM_KEYLAST,PM_REMOVE));CHECK(waits==2);
    g_responsive_presentation=false;
    CHECK(!PresentationPeekMessage(&message,nullptr,0,0,PM_REMOVE));CHECK(waits==2);
    VirtualFree(reinterpret_cast<void*>(g_base),0,MEM_RELEASE);
    puts("Presentation: PASS; native flip boundaries, busy frames, focus wait and message ownership.");return 0;
}
