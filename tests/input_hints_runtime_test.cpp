#include "../src/dinput_proxy.cpp"
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"ole32.lib")
static int failures,native_pause;
static std::vector<std::pair<int,std::string>> lines;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
static int __cdecl Width(const char* text){int n=0;for(;*text;++text)n+=*text==' '?6:11;return n;}
static void __cdecl Draw(const char* text,int x,int y,const BYTE* rgb) {
    CHECK(x>=0 && x+Width(text)<=320 && y>=0 && y<=226);
    CHECK(rgb[0]==252 || rgb[0]==15 || rgb[0]==248);
    lines.emplace_back(y,text);
}
static void __cdecl NativePause(const char* text,int x,int y){CHECK(!strcmp(text,"PAUSED") && x==100 && y==115);++native_pause;}
static void Branch(DWORD rva,void* target,BYTE op=0xE9) {
    auto code=reinterpret_cast<BYTE*>(g_base+rva);code[0]=op;
    const LONG delta=static_cast<LONG>(reinterpret_cast<BYTE*>(target)-(code+5));memcpy(code+1,&delta,4);
}
static std::string Text(){std::string all;for(const auto& line:lines)all+=line.second+"\n";return all;}
int main() {
    auto base=static_cast<BYTE*>(VirtualAlloc(nullptr,0x900000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));
    CHECK(base);if(!base)return 1;g_base=reinterpret_cast<uintptr_t>(base);
    Branch(0x4AEB0,reinterpret_cast<void*>(Width));Branch(0x44A20,reinterpret_cast<void*>(Draw));
    Branch(0x44830,reinterpret_cast<void*>(NativePause));
    Branch(0x4ACC5,base+0x44830,0xE8);CHECK(!InstallInputHints() && base[0x4ACC5]==0xE8);
    Branch(0x4244B,base+0x44A20,0xE8);CHECK(InstallInputHints());
    // A tail-call fixture executes the actual installed call destination with the original arguments.
    base[0x4ACC5]=base[0x4244B]=0xE9;
    auto pause=reinterpret_cast<void(__cdecl*)(const char*,int,int)>(base+0x4ACC5);
    auto quit=reinterpret_cast<void(__cdecl*)(const char*,int,int,const BYTE*)>(base+0x4244B);
    InputField<int>(0x8745A0)=1;g_controls.guard=true;
    pause("PAUSED",100,115);CHECK(native_pause==1);
    CHECK(Text().find("PUNCH MOUSE 1")!=std::string::npos && Text().find("BLOCK Q")!=std::string::npos);
    CHECK(Text().find("ESC RESUME")!=std::string::npos);
    g_input_activity.pads[2].connected=true;g_input_activity.Use(ff_input::Device::Gamepad);
    g_controls.players[0].pad[ff_controls::Hand]=ff_controls::RT;
    g_controls.players[0].pad[ff_controls::Block]=ff_input::Y;
    lines.clear();pause("PAUSED",100,115);
    CHECK(Text().find("PUNCH RT")!=std::string::npos && Text().find("BLOCK Y")!=std::string::npos);
    CHECK(Text().find("START RESUME")!=std::string::npos && Text().find("MOUSE")==std::string::npos);
    lines.clear();quit("OLD",0,130,nullptr);CHECK(Text().find("A YES  B NO")!=std::string::npos);
    g_input_activity.pads[2]={};lines.clear();quit("OLD",0,130,nullptr);
    CHECK(Text().find("ENTER YES  ESC NO")!=std::string::npos);
    g_controls.players[0].keys[ff_controls::View]=0;g_controls.players[0].alternate[ff_controls::View]=0x39;
    CHECK(PlayerHintBinding(0,ff_controls::View,false)=="SPACE");
    g_controls.guard=false;lines.clear();pause("PAUSED",100,115);CHECK(Text().find("BLOCK")==std::string::npos);
    g_controls.guard=true;InputField<int>(0x8745A0)=2;
    for(int scheme:{1,2,3})for(unsigned key:{0x36u,0x11Du,0x17Fu,ff_controls::Mouse5}) {
        g_modern_coop_scheme=scheme;
        for(auto& player:g_controls.players)for(auto& binding:player.keys)binding=key;
        lines.clear();pause("PAUSED",100,115);
        bool first=false,second=false;
        for(const auto& line:lines) {
            CHECK((line.first>=40 && line.first<=82) || (line.first>=182 && line.first<=224));
            first|=line.second.find("P1")!=std::string::npos;second|=line.second.find("P2")!=std::string::npos;
        }
        CHECK(first && second);
    }
    VirtualFree(base,0,MEM_RELEASE);
    puts(failures?"Input hints runtime: FAIL":"Input hints runtime: PASS");return failures?1:0;
}
