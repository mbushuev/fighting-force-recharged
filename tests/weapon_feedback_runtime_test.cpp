#include "../src/dinput_proxy.cpp"
#include <fstream>
#include <iterator>
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"ole32.lib")
static int failures,queues,trace_result;static bool focus=true,allocation=true;
static XINPUT_VIBRATION motors[4];
static const DWORD sites[]={0xE128,0xE234,0xE311};
struct Observation {
    int muzzle_calls=0,player=-1,bone=-1,sounds=0,traces=0;
    int rays[3][7]={};BYTE projectile[64]={};
    bool operator==(const Observation& b)const {
        return muzzle_calls==b.muzzle_calls && player==b.player && bone==b.bone &&
            sounds==b.sounds && traces==b.traces && !memcmp(rays,b.rays,sizeof(rays)) &&
            !memcmp(projectile,b.projectile,sizeof(projectile));
    }
};
static Observation observed;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
static bool Focus(){return focus;}
static bool ManualTimer(){++queues;return true;}
static DWORD WINAPI SetMotors(DWORD slot,XINPUT_VIBRATION* value){motors[slot]=*value;return ERROR_SUCCESS;}
static int __cdecl Muzzle(int player,int bone,void* position) {
    ++observed.muzzle_calls;observed.player=player;observed.bone=bone;
    auto xyz=static_cast<int*>(position);xyz[0]=11;xyz[1]=22;xyz[2]=33;return 0x2468;
}
static int __cdecl Sound(int,int,int){++observed.sounds;return 0;}
static void* __cdecl Allocate(){return allocation?observed.projectile:nullptr;}
static int __cdecl Trace(void* projectile,int player) {
    CHECK(observed.traces<3);if(observed.traces>=3)return trace_result;
    auto ray=static_cast<BYTE*>(projectile);auto out=observed.rays[observed.traces++];
    memcpy(out,ray,16);out[4]=*reinterpret_cast<int16_t*>(ray+16);
    out[5]=*reinterpret_cast<int16_t*>(ray+28);out[6]=player;return trace_result;
}
static void Relative(BYTE* code,BYTE opcode,void* target) {
    code[0]=opcode;const LONG delta=static_cast<LONG>(reinterpret_cast<BYTE*>(target)-code-5);memcpy(code+1,&delta,4);
}
static int __declspec(naked) __cdecl MuzzleSite(void*,int,int,void*) {
    __asm {
        mov eax,[esp+4]
        push [esp+16]
        push [esp+16]
        push [esp+16]
        jmp eax
    }
}
static void SyntheticSites(BYTE* base) {
    for(DWORD rva:sites) {
        Relative(base+rva,0xE8,base+0x38FB0);
        const BYTE epilogue[]={0x83,0xC4,12,0xC3};memcpy(base+rva+5,epilogue,sizeof(epilogue));
    }
}
static void Reset(){StopControllerFeedback();queues=0;observed={};}
static void CheckPulse(int player,bool active) {
    CHECK(queues==(active?1:0));WriteControllerFeedback(GetTickCount64(),true);
    for(unsigned slot=0;slot<4;++slot) {
        const bool expected=active && slot==static_cast<unsigned>(player?3:1);
        CHECK((motors[slot].wLeftMotorSpeed>0)==expected);
    }
}
static void NativeFiring(BYTE* base,const char* path) {
    std::ifstream stream(path,std::ios::binary);std::vector<BYTE> bytes((std::istreambuf_iterator<char>(stream)),{});
    CHECK(bytes.size()>0x40000);if(bytes.size()<0x40000)return;
    const auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(bytes.data()+reinterpret_cast<const IMAGE_DOS_HEADER*>(bytes.data())->e_lfanew);
    const auto sections=IMAGE_FIRST_SECTION(nt);bool found=false;
    for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i)if(0xE0D0>=sections[i].VirtualAddress &&
        0xE3D0<=sections[i].VirtualAddress+sections[i].SizeOfRawData) {
        memcpy(base+0xE0D0,bytes.data()+sections[i].PointerToRawData+0xE0D0-sections[i].VirtualAddress,0x300);found=true;
    }
    CHECK(found);if(!found)return;
    const DWORD symbols[]={0xC6A500,0xC6A57E,0xC70620,0xC77694,0xC6E9CC};
    for(DWORD rva=0xE0D0;rva+4<=0xE3D0;++rva) {
        DWORD value;memcpy(&value,base+rva,4);
        for(DWORD symbol:symbols)if(value==symbol) {
            value=static_cast<DWORD>(g_base)+value-0x400000;memcpy(base+rva,&value,4);rva+=3;break;
        }
    }
    Relative(base+0x42600,0xE9,reinterpret_cast<void*>(Sound));
    Relative(base+0x31130,0xE9,reinterpret_cast<void*>(Allocate));
    Relative(base+0xDE60,0xE9,reinterpret_cast<void*>(Trace));
    struct Case {DWORD rva;int player,result;bool allocated,empty;Observation native;};
    std::vector<Case> cases;
    for(DWORD rva:{0xE0D0,0xE1C0,0xE290})for(int player:{0,1,2})for(int accepted:{0,1})
        cases.push_back({rva,player,accepted,true,false,{}});
    cases.push_back({0xE0D0,0,0,false,false,{}});
    cases.push_back({0xE1C0,0,0,true,true,{}});
    auto run=[&](const Case& c) {
        Reset();allocation=c.allocated;trace_result=c.result;
        const auto actor=0x86A500+c.player*0xF4;
        InputField<int8_t>(actor+0x7E)=c.empty?-1:0;InputField<int8_t>(actor+0x7D)=-1;
        InputField<int16_t>(actor+0x34)=2;InputField<int16_t>(actor+0x38)=100;
        InputField<int>(actor+0x28)=101;InputField<int>(actor+0x30)=303;
        InputField<int>(0x870620)=c.rva==0xE0D0?0x1F9:0x203;
        CHECK(reinterpret_cast<int(__cdecl*)(int)>(base+c.rva)(c.player)==0);
    };
    for(auto& c:cases) {run(c);c.native=observed;CheckPulse(c.player,false);}
    CHECK(InstallWeaponFeedback());
    for(const auto& c:cases) {
        run(c);CHECK(observed==c.native);CheckPulse(c.player,c.player<2 && c.native.muzzle_calls==1);
        if(c.rva==0xE290)CHECK(observed.traces==(c.result?1:3));
    }
}
int main(int argc,char** argv) {
    auto base=static_cast<BYTE*>(VirtualAlloc(nullptr,0x900000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));
    CHECK(base);if(!base)return 1;g_base=reinterpret_cast<uintptr_t>(base);
    g_modern_focus=Focus;g_modern_xinput_set=SetMotors;g_feedback_start_timer=ManualTimer;
    g_modern_pad_session.selected=1;g_modern_pad_session.raw.connected=g_modern_pad_session.ready=true;
    g_modern_second_session.selected=3;g_modern_second_session.raw.connected=g_modern_second_session.ready=true;
    InputField<int>(0x8745A0)=2;g_modern_coop_scheme=2;
    Relative(base+0x38FB0,0xE9,reinterpret_cast<void*>(Muzzle));
    CHECK(!InstallWeaponFeedback());SyntheticSites(base);
    BYTE first[5];memcpy(first,base+sites[0],5);base[sites[2]]=0x90;
    CHECK(!InstallWeaponFeedback() && !memcmp(first,base+sites[0],5));SyntheticSites(base);
    CHECK(InstallWeaponFeedback());
    int position[3];
    for(DWORD rva:sites)for(int player:{0,1,2,-1}) {
        Reset();CHECK(MuzzleSite(base+rva,player,3,position)==0x2468);
        CHECK(position[0]==11 && position[1]==22 && position[2]==33);
        CHECK(observed.muzzle_calls==1 && observed.player==player && observed.bone==3);
        CheckPulse(player,player==0 || player==1);
    }
    for(int inhibit=0;inhibit<5;++inhibit) {
        Reset();g_controls.vibration=inhibit!=0;focus=inhibit!=1;g_controls_menu_active=inhibit==2;
        InputField<int>(0x85A6B8)=inhibit==3?26:0;g_modern_pad_session.ready=inhibit!=4;
        MuzzleSite(base+sites[0],0,3,position);CHECK(observed.muzzle_calls==1);CheckPulse(0,false);
    }
    g_controls.vibration=focus=g_modern_pad_session.ready=true;g_controls_menu_active=false;InputField<int>(0x85A6B8)=0;
    // Ordinary skeletal queries (including aiming) remain entirely native.
    Reset();CHECK(reinterpret_cast<int(__cdecl*)(int,int,void*)>(base+0x38FB0)(0,3,position)==0x2468);CheckPulse(0,false);
    if(argc>1)NativeFiring(base,argv[1]);
    StopControllerFeedback();VirtualFree(base,0,MEM_RELEASE);
    puts(failures?"Weapon feedback runtime: FAIL":"Weapon feedback runtime: PASS");return failures?1:0;
}
