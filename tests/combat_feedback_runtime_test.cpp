#include "../src/dinput_proxy.cpp"
#include <fstream>
#include <iterator>
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"ole32.lib")
static int failures,writes[4];static bool focus=true,connected=true;
static XINPUT_VIBRATION motors[4];
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
static bool Focus(){return focus;}
static bool ManualTimer(){return true;}
static DWORD WINAPI SetMotors(DWORD slot,XINPUT_VIBRATION* value) {
    CHECK(slot<4);++writes[slot];motors[slot]=*value;return connected?ERROR_SUCCESS:ERROR_DEVICE_NOT_CONNECTED;
}
static int __cdecl NativeDamage(int attacker,int target,int factor) {
    if(target>=0 && target<=8 && factor)InputField<int16_t>(0x86A542+target*0xF4)-=10;
    return attacker*7+target*5+factor;
}
static void Jump(BYTE* code,void* target) {
    code[0]=0xE9;const LONG delta=static_cast<LONG>(reinterpret_cast<BYTE*>(target)-(code+5));memcpy(code+1,&delta,4);
}
int main(int argc,char** argv) {
    auto base=static_cast<BYTE*>(VirtualAlloc(nullptr,0x900000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));
    CHECK(base);if(!base)return 1;g_base=reinterpret_cast<uintptr_t>(base);
    g_modern_focus=Focus;g_modern_xinput_set=SetMotors;g_feedback_start_timer=ManualTimer;
    g_modern_pad_session.selected=1;g_modern_pad_session.raw.connected=g_modern_pad_session.ready=true;
    g_modern_second_session.selected=3;g_modern_second_session.raw.connected=g_modern_second_session.ready=true;
    InputField<int>(0x8745A0)=1;
    const BYTE prologue[]={0x53,0x8B,0x5C,0x24,0x0C};
    CHECK(!InstallCombatFeedback());
    if(argc>1) {
        std::ifstream stream(argv[1],std::ios::binary);std::vector<BYTE> bytes((std::istreambuf_iterator<char>(stream)),{});
        CHECK(bytes.size()>0x40000);if(bytes.size()<0x40000)return 1;
        const auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(bytes.data()+reinterpret_cast<const IMAGE_DOS_HEADER*>(bytes.data())->e_lfanew);
        const auto sections=IMAGE_FIRST_SECTION(nt);bool found=false;
        for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i)if(0x3C380>=sections[i].VirtualAddress && 0x3C4D5<=sections[i].VirtualAddress+sections[i].SizeOfRawData) {
            const auto offset=sections[i].PointerToRawData+0x3C380-sections[i].VirtualAddress;
            memcpy(base+0x3C380,bytes.data()+offset,0x155);found=true;
        }
        CHECK(found);if(!found)return 1;
        // Relocate the reviewed helper's absolute data operands in this isolated image.
        for(unsigned i=0;i+4<=0x155;++i) {
            DWORD value;memcpy(&value,base+0x3C380+i,4);
            if(value==0xC6A500 || value==0xC6A578 || value==0xC5A6BC || value==0xC5A6C8 || value==0x46F008) {
                value=static_cast<DWORD>(g_base)+value-0x400000;memcpy(base+0x3C380+i,&value,4);i+=3;
            }
        }
        InputField<int>(0x85A6BC)=2;
    } else {
        memcpy(base+0x3C380,prologue,5);base[0x3C385]=0x5B;Jump(base+0x3C386,reinterpret_cast<void*>(NativeDamage));
    }
    auto damage=reinterpret_cast<int(__cdecl*)(int,int,int)>(base+0x3C380);
    InputField<int16_t>(0x86A542+2*0xF4)=100;const int native_result=damage(0,2,256);
    const int native_health=InputField<int16_t>(0x86A542+2*0xF4);
    InputField<int16_t>(0x86A542+2*0xF4)=100;
    CHECK(InstallCombatFeedback());CHECK(damage(0,2,256)==native_result);
    CHECK(InputField<int16_t>(0x86A542+2*0xF4)==native_health && native_health<100);
    WriteControllerFeedback(GetTickCount64(),true);CHECK(motors[1].wRightMotorSpeed>0 && !writes[3]);
    StopControllerFeedback();CHECK(!motors[1].wRightMotorSpeed);
    for(int scheme:{1,2,3}) {
        InputField<int>(0x8745A0)=2;g_modern_coop_scheme=scheme;
        for(unsigned player=0;player<2;++player) {
            StopControllerFeedback();const int slot=FeedbackControllerSlot(player);
            CHECK(slot==(scheme==1?(player?1:-1):scheme==3?(player?-1:1):(player?3:1)));
            InputField<int16_t>(0x86A542+player*0xF4)=100;
            damage(2,player,256);WriteControllerFeedback(GetTickCount64(),true);
            for(unsigned s=0;s<4;++s)CHECK((motors[s].wLeftMotorSpeed>0)==(static_cast<int>(s)==slot));
        }
    }
    StopControllerFeedback();g_modern_coop_scheme=2;
    PlayerCombatFeedback(1,ff_feedback::Block);WriteControllerFeedback(GetTickCount64(),true);
    CHECK(motors[3].wRightMotorSpeed==22000 && !motors[1].wRightMotorSpeed);
    WriteControllerFeedback(GetTickCount64()+1000,true);CHECK(!motors[3].wRightMotorSpeed);
    StopControllerFeedback(); // End the synthetic future interval before returning to the real clock.
    for(int inhibit=0;inhibit<5;++inhibit) {
        if(inhibit==0)g_controls.vibration=false;
        if(inhibit==1)focus=false;
        if(inhibit==2)g_controls_menu_active=true;
        if(inhibit==3)InputField<int>(0x85A6B8)=26;
        if(inhibit==4)g_modern_second_session.ready=false;
        PlayerCombatFeedback(1,ff_feedback::Block);WriteControllerFeedback(GetTickCount64(),true);
        CHECK(!motors[3].wRightMotorSpeed);
        g_controls.vibration=true;focus=true;g_controls_menu_active=false;
        InputField<int>(0x85A6B8)=0;g_modern_second_session.ready=true;
    }
    PlayerCombatFeedback(1,ff_feedback::Block);WriteControllerFeedback(GetTickCount64(),true);
    GameFocusLost();CHECK(!motors[3].wRightMotorSpeed);
    g_modern_second_session.ready=true;connected=false;PlayerCombatFeedback(1,ff_feedback::Block);
    WriteControllerFeedback(GetTickCount64(),true);connected=true;motors[3]={};
    WriteControllerFeedback(GetTickCount64(),true);CHECK(!motors[3].wRightMotorSpeed);
    // The actual timer must expire pulses even when the game thread stops rendering.
    StopControllerFeedback();g_feedback_start_timer=StartControllerFeedbackTimer;
    QueueControllerFeedback(1,ff_feedback::Hurt);CHECK(g_feedback_timer);
    Sleep(240);ShutdownControllerFeedback();CHECK(!motors[1].wLeftMotorSpeed && !g_feedback_timer);
    VirtualFree(g_feedback_damage_trampoline,0,MEM_RELEASE);VirtualFree(base,0,MEM_RELEASE);
    puts(failures?"Combat feedback runtime: FAIL":"Combat feedback runtime: PASS");return failures?1:0;
}
