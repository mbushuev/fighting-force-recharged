#include "../src/dinput_proxy.cpp"
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"ole32.lib")
static int failures,native_calls,text_calls;
static bool highlighted[240];
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
static bool Unfocused(){return false;}
static void __cdecl NativeMenu(){++native_calls;}
static int __cdecl TextWidth(const char* text) {
    int width=0;for(const char* c=text;*c;++c)width+=*c==' '?6:11;return width;
}
static void __cdecl TextDraw(const char* text,int x,int y,const BYTE* rgb) {
    ++text_calls;CHECK(x>=0 && y>=0 && y<=226);CHECK(x+TextWidth(text)<=320);
    CHECK(rgb[0]==248 || rgb[0]==15);highlighted[y]=rgb[0]==15;
    for(int i=3;i<12;++i)CHECK(rgb[i]==rgb[i%3]);
    for(const char* c=text;*c;++c)CHECK((*c>='A' && *c<='Z') || (*c>='0' && *c<='9') || *c==' ' || *c=='-');
}
static void JumpStub(BYTE* code,void* target) {
    code[0]=0xE9;LONG offset=static_cast<LONG>(reinterpret_cast<BYTE*>(target)-(code+5));memcpy(code+1,&offset,4);
}
int main() {
    auto base=static_cast<BYTE*>(VirtualAlloc(nullptr,0x900000,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    CHECK(base);if(!base)return 1;g_base=reinterpret_cast<uintptr_t>(base);g_modern_focus=Unfocused;
    const BYTE call[]={0xFF,0x15,0x7C,0xC4,0x46,0};
    memcpy(base+0x2EBAF,call,6);InstallControlMenu();CHECK(base[0x2EBAF]==0xFF);
    memcpy(base+0x2F091,call,6);InstallControlMenu();CHECK(base[0x2EBAF]==0xE8 && base[0x2F096]==0x90);
    JumpStub(base+0x4AEB0,reinterpret_cast<void*>(TextWidth));JumpStub(base+0x44A20,reinterpret_cast<void*>(TextDraw));
    InputField<uintptr_t>(0x6C47C)=reinterpret_cast<uintptr_t>(NativeMenu);base[0x2EBB5]=0xC3;
    reinterpret_cast<void(__cdecl*)()>(base+0x2EBAF)();CHECK(native_calls==1 && !g_controls_menu_active);
    InputField<uintptr_t>(0x6C47C)=g_base+0x30800;
    reinterpret_cast<void(__cdecl*)()>(base+0x2EBAF)();CHECK(g_controls_menu_active && text_calls>=12);
    CHECK(g_control_return==g_base+0x2FC30 && InputField<uintptr_t>(0x6C47C)==reinterpret_cast<uintptr_t>(ControlMenuFrame));
    CHECK(ModernMenuRead<false>(0)==0 && ModernSecondMenuRead<true>(0)==0 && ModernQuitKey()==-1);
    for(int selected=0;selected<9;++selected) {
        g_control_menu.row=selected;DrawControlMenu();
        for(int row=0;row<9;++row)CHECK(highlighted[43+row*16]==(row==selected));
    }
    for(int player=0;player<2;++player)for(int keyboard=-1;keyboard<2;++keyboard) {
        g_control_menu.draft.keyboard=keyboard;g_control_menu.page=player;
        for(int row=0;row<g_control_menu.Rows();++row) {
            g_control_menu.row=row;DrawControlMenu();const int first=row/7*7;
            for(int item=first;item<g_control_menu.Rows() && item<first+7;++item)CHECK(highlighted[54+(item-first)*18]==(item==row));
        }
        g_control_menu.capture=true;DrawControlMenu();g_control_menu.capture=false;
    }
    for(const char* message:{"BUTTON ALREADY USED","DYNAMIC CAMERA UNAVAILABLE","SAVE FAILED  TRY AGAIN"}) {
        g_control_message=message;DrawControlMenu();
    }
    g_control_message="";
    CloseControlMenu();CHECK(!g_controls_menu_active && g_controls_wait_release && InputField<uintptr_t>(0x6C47C)==g_base+0x2FC30);
    auto settings=ff_controls::DefaultsSettings();settings.keyboard=1;settings.dynamic=false;settings.players[0].slot=3;
    ApplyControlSettings(settings);CHECK(g_modern_coop_scheme==3 && !g_modern_coop_camera && g_modern_controller==3);
    CHECK(!g_modern_pad_session.ready && !g_coop_ready);
    VirtualFree(base,0,MEM_RELEASE);
    puts(failures?"Native control menu: FAIL":"Native control menu: PASS; call signatures, entry/return, input isolation and menu pages.");return failures?1:0;
}
