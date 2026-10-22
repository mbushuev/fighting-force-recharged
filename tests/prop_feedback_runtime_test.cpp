#include "../src/dinput_proxy.cpp"
#include <fstream>
#include <iterator>
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"ole32.lib")
static int failures,contacts,contact_result=1,shot_owner=-1;
static XINPUT_VIBRATION motors[4];
static BYTE obstacle[24],object[72],point[16];
static BYTE* received_actor;
static void* received_object;
static void* received_point;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
static bool Focus(){return true;}
static bool ManualTimer(){return true;}
static DWORD WINAPI SetMotors(DWORD slot,XINPUT_VIBRATION* value){motors[slot]=*value;return ERROR_SUCCESS;}
static int __cdecl ObjectHandler(BYTE* box,void* prop,void* impact,BYTE* actor) {
    CHECK(box==obstacle);received_actor=actor;received_object=prop;received_point=impact;++contacts;
    if(contact_result>0)--*reinterpret_cast<int16_t*>(box+16);return contact_result;
}
static int __declspec(naked) __cdecl ObjectSite(void*,BYTE*,void*,void*,BYTE*) {
    __asm {
        push ebx
        mov eax,[esp+8]
        mov ecx,[esp+12]
        movsx edx,word ptr [ecx+12]
        mov ebx,edx
        push [esp+24]
        push [esp+24]
        push [esp+24]
        push [esp+24]
        jmp eax
    }
}
static int __declspec(naked) __cdecl ShotSite(void*,void*,int) {
    __asm {
        mov eax,[esp+4]
        push [esp+12]
        push [esp+12]
        jmp eax
    }
}
static int __cdecl NativeShot(void* projectile,int owner) {
    CHECK(projectile==point);shot_owner=owner;
    return ObjectSite(reinterpret_cast<void*>(g_base+0x3C22D),obstacle,object,point,nullptr);
}
static void Relative(BYTE* code,BYTE opcode,void* target) {
    code[0]=opcode;const LONG delta=static_cast<LONG>(reinterpret_cast<BYTE*>(target)-code-5);memcpy(code+1,&delta,4);
}
static const DWORD sites[]={0x3846E,0x393C9,0x393ED,0x3C22D};
static const DWORD shots[]={0xE270,0xE371,0xE398,0xE3BF};
static void SyntheticSites(BYTE* base) {
    for(unsigned i=0;i<4;++i) {
        const BYTE code[]={0xFF,0x14,BYTE(i?0x95:0x9D)};memcpy(base+sites[i],code,3);
        const DWORD table=static_cast<DWORD>(g_base+0x86D240);memcpy(base+sites[i]+3,&table,4);
        const BYTE epilogue[]={0x83,0xC4,0x10,0x5B,0xC3};memcpy(base+sites[i]+7,epilogue,sizeof(epilogue));
    }
    for(DWORD rva:shots){Relative(base+rva,0xE8,base+0xDE60);const BYTE epilogue[]={0x83,0xC4,8,0xC3};memcpy(base+rva+5,epilogue,sizeof(epilogue));}
    Relative(base+0xDE60,0xE9,reinterpret_cast<void*>(NativeShot));
}
static bool Buzz(unsigned slot){WriteControllerFeedback(GetTickCount64(),true);return motors[slot].wRightMotorSpeed>0;}
static void NativeContacts(BYTE* base,const char* path) {
    std::ifstream stream(path,std::ios::binary);std::vector<BYTE> bytes((std::istreambuf_iterator<char>(stream)),{});
    CHECK(bytes.size()>0x40000);if(bytes.size()<0x40000)return;
    const auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(bytes.data()+reinterpret_cast<const IMAGE_DOS_HEADER*>(bytes.data())->e_lfanew);
    const auto sections=IMAGE_FIRST_SECTION(nt);
    SyntheticSites(base);
    const DWORD symbols[]={0x467F50,0x468350,0xC689E8,0xC6A500,0xC6D240,0xC6DD20,0xC735E4,
        0xC5A6B8,0xC6BEA0,0xC6D2AC,0xC70620,0xC70632};
    const DWORD ranges[][2]={{0x391C0,0x39434},{0x3CEB0,0x3D4A0},{0x3C5C0,0x3C85B},
        {0x3846E,0x38475},{0x3C22D,0x3C234},{0xE270,0xE275},{0xE371,0xE376},
        {0xE398,0xE39D},{0xE3BF,0xE3C4}};
    for(const auto& range:ranges) {
        bool found=false;
        for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i)if(range[0]>=sections[i].VirtualAddress &&
            range[1]<=sections[i].VirtualAddress+sections[i].SizeOfRawData) {
            memcpy(base+range[0],bytes.data()+sections[i].PointerToRawData+range[0]-sections[i].VirtualAddress,range[1]-range[0]);found=true;
        }
        CHECK(found);if(!found)return;
        for(DWORD rva=range[0];rva+4<=range[1];++rva) {
            DWORD value;memcpy(&value,base+rva,4);
            for(DWORD symbol:symbols)if(value==symbol) {
                value=static_cast<DWORD>(g_base)+value-0x400000;memcpy(base+rva,&value,4);rva+=3;break;
            }
        }
    }
    // Native impact sounds/sparks are outside this isolated contact fixture.
    for(DWORD rva:{0x31370,0x42600,0xEEB0,0x30FF0}){base[rva]=0x31;base[rva+1]=0xC0;base[rva+2]=0xC3;}
    InputField<void*>(0x86D240+5*4)=base+0x3C5C0;
    InputField<void*>(0x86D240+10*4)=base+0x3CEB0;
    InputField<int>(0x8689E8)=1;
    auto box=reinterpret_cast<int16_t*>(base+0x86DD20);
    box[0]=-100;box[1]=-100;box[2]=100;box[3]=100;box[5]=0;box[6]=5;box[8]=2;
    *reinterpret_cast<void**>(box+10)=object;*reinterpret_cast<int*>(object)=0x100;
    auto contact=reinterpret_cast<int(__cdecl*)(void*,int)>(base+0x391C0);
    memset(point,0,sizeof(point));InputField<WORD>(0x86A552)=0;
    CHECK(contact(point,0)==1);const int native_durability=box[8];
    CHECK(native_durability==1);CHECK(!Buzz(1));
    box[8]=2;box[6]=10;memset(object,0,sizeof(object));*reinterpret_cast<int*>(object)=0x5F;
    InputField<WORD>(0x86A552)=0;const int car_result=contact(point,0);
    BYTE car_after[sizeof(object)];memcpy(car_after,object,sizeof(object));const int car_durability=box[8];
    CHECK(car_result==2 && car_durability==1 && !Buzz(1));
    CHECK(InstallPropFeedback());
    box[8]=2;box[6]=5;*reinterpret_cast<int*>(object)=0x100;InputField<WORD>(0x86A552)=0;
    CHECK(contact(point,0)==1 && box[8]==native_durability && Buzz(1));StopControllerFeedback();
    // The native per-attack contact bit prevents another pulse after expiry.
    CHECK(contact(point,0)==0 && box[8]==1 && !Buzz(1));
    InputField<WORD>(0x86A552)=0;*reinterpret_cast<int*>(point)=200<<16;
    CHECK(contact(point,0)==0 && box[8]==1 && !Buzz(1));
    memset(point,0,sizeof(point));box[8]=-1;
    CHECK(contact(point,0)==0 && !Buzz(1));
    // Exercise the separate rotated-object dispatch using native bounds tests.
    box[0]=100;box[1]=100;box[5]=1;box[8]=2;memset(object,0,sizeof(object));
    *reinterpret_cast<int*>(object)=0x100;InputField<int16_t>(0x68350)=32767;
    CHECK(contact(point,0)==1 && box[8]==1 && Buzz(1));StopControllerFeedback();
    InputField<WORD>(0x86A552)=0;*reinterpret_cast<int*>(point)=200<<16;
    CHECK(contact(point,0)==0 && box[8]==1 && !Buzz(1));
    memset(point,0,sizeof(point));box[0]=-100;box[1]=-100;box[5]=0;
    box[8]=2;box[6]=10;memset(object,0,sizeof(object));*reinterpret_cast<int*>(object)=0x5F;
    InputField<WORD>(0x86A552)=0;InputField<int>(0x8735E4)=0;
    CHECK(contact(point,0)==car_result && Buzz(1));StopControllerFeedback();
    CHECK(box[8]==car_durability && !memcmp(object,car_after,sizeof(object)));
}
int main(int argc,char** argv) {
    auto base=static_cast<BYTE*>(VirtualAlloc(nullptr,0x900000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));
    CHECK(base);if(!base)return 1;g_base=reinterpret_cast<uintptr_t>(base);
    g_modern_focus=Focus;g_modern_xinput_set=SetMotors;g_feedback_start_timer=ManualTimer;
    g_modern_pad_session.selected=1;g_modern_pad_session.raw.connected=g_modern_pad_session.ready=true;
    g_modern_second_session.selected=3;g_modern_second_session.raw.connected=g_modern_second_session.ready=true;
    InputField<int>(0x8745A0)=2;g_modern_coop_scheme=2;
    CHECK(!InstallPropFeedback());SyntheticSites(base);
    base[shots[3]]=0x90;CHECK(!InstallPropFeedback());CHECK(base[sites[0]]==0xFF);SyntheticSites(base);
    *reinterpret_cast<int16_t*>(obstacle+12)=10;
    InputField<void*>(0x86D240+40)=reinterpret_cast<void*>(ObjectHandler);
    CHECK(InstallPropFeedback());
    for(DWORD rva:sites)for(int player:{0,1,2,-1})for(int accepted:{0,1,2}) {
        StopControllerFeedback();contact_result=accepted;contacts=0;
        auto actor=player<0?nullptr:base+0x86A500+player*0xF4;
        *reinterpret_cast<int16_t*>(obstacle+16)=5;
        CHECK(ObjectSite(base+rva,obstacle,object,point,actor)==accepted);
        CHECK(contacts==1 && received_actor==actor && received_object==object && received_point==point);
        CHECK(*reinterpret_cast<int16_t*>(obstacle+16)==(accepted?4:5));
        CHECK(Buzz(1)==(accepted && player==0));CHECK(Buzz(3)==(accepted && player==1));
    }
    for(DWORD rva:shots)for(int player:{0,1,2})for(int accepted:{0,2}) {
        StopControllerFeedback();contact_result=accepted;
        CHECK(ShotSite(base+rva,point,player)==accepted && shot_owner==player && g_feedback_shooter==-1);
        CHECK(received_actor==nullptr); // Never change native attribution arguments.
        CHECK(Buzz(1)==(accepted && player==0));CHECK(Buzz(3)==(accepted && player==1));
    }
    StopControllerFeedback();contact_result=1;
    CHECK(NativeShot(point,0)==1 && !Buzz(1) && !Buzz(3)); // A query is not a fired shot.
    for(int disabled=0;disabled<2;++disabled) {
        g_controls.vibration=disabled!=0;g_controls_menu_active=disabled!=0;
        ObjectSite(base+sites[1],obstacle,object,point,base+0x86A500);CHECK(!Buzz(1));
    }
    g_controls.vibration=true;g_controls_menu_active=false;
    if(argc>1)NativeContacts(base,argv[1]);
    StopControllerFeedback();VirtualFree(base,0,MEM_RELEASE);
    puts(failures?"Object feedback runtime: FAIL":"Object feedback runtime: PASS");return failures?1:0;
}
