#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>
#include "../src/move_list.h"
static int failures,chosen;
static BYTE* base;
static uintptr_t native_start;
static void* selected_actor;
template<typename T> static T& Field(void* actor,unsigned offset){return *reinterpret_cast<T*>(static_cast<BYTE*>(actor)+offset);}
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
static int __cdecl Action(void* actor,void**,int action){Field<int>(actor,0xC)=action;if(actor==selected_actor)chosen=action;return 1;}
static void __cdecl Flush(unsigned player){memset(base+0x871850+player*6,0,6);}
static void __cdecl Scan(unsigned player,int){*reinterpret_cast<int*>(base+0x8776E0+player*4)=-1;}
static int __cdecl Zero(int,int){return 0;}
static int __cdecl Available(void**,int){return 1;}
static void __cdecl Noop(){}
static void Branch(unsigned rva,void* target){auto p=base+rva;p[0]=0xE9;LONG delta=LONG(reinterpret_cast<BYTE*>(target)-p-5);memcpy(p+1,&delta,4);}
static LONG CALLBACK Exception(EXCEPTION_POINTERS* e) {
    printf("Move selector exception %08lx at RVA %08lx\n",e->ExceptionRecord->ExceptionCode,e->ContextRecord->Eip-DWORD(base));fflush(stdout);ExitProcess(1);
}
static void __declspec(naked) Finish(){
    __asm {
        add esp,40h
        pop ebp
        pop edi
        pop esi
        pop ebx
        ret
    }
}
// Reproduce the native selector's local frame without executing world/audio setup.
static void __declspec(naked) __cdecl Select(void*,void*,void**,unsigned,int,int,int) {
    __asm {
        push ebx
        push esi
        push edi
        push ebp
        mov esi,[esp+20]
        mov ebx,[esp+24]
        mov edi,[esp+28]
        mov eax,[esp+32]
        mov ebp,[esp+36]
        mov ecx,[esp+40]
        mov edx,[esp+44]
        sub esp,40h
        mov [esp+14h],eax
        mov [esp+18h],edi
        mov [esp+20h],ecx
        lea eax,[eax+eax*2]
        add eax,eax
        mov [esp+24h],eax
        mov [esp+12h],dl
        mov byte ptr [esp+13h],0
        mov dword ptr [esp+1Ch],2
        mov eax,ebp
        jmp native_start
    }
}
static void Load(const char* executable) {
    std::ifstream stream(executable,std::ios::binary);std::vector<BYTE> bytes((std::istreambuf_iterator<char>(stream)),{});
    CHECK(bytes.size()>0x40000);if(bytes.size()<0x40000)return;
    const auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(bytes.data()+reinterpret_cast<const IMAGE_DOS_HEADER*>(bytes.data())->e_lfanew);
    const auto sections=IMAGE_FIRST_SECTION(nt);
    const DWORD ranges[][2]={{0x4F823,0x4FF69},{0x51F5C,0x52049},{0x50A52,0x50A76},
        {0x50B73,0x50B93},{0x50BA5,0x5119C}};
    const DWORD globals[]={0xC71850,0xC71851,0xC71852,0xC71853,0xC6A57D,0xC6A57E,0xC70620,
        0xC77694,0xC6E9CC,0xC6A50C,0x467F50,0x468350,0xC5A660,0xC677BC,0xC776E0,
        0xC6A500,0xC79780,0xC5B560};
    for(const auto& range:ranges) {
        bool copied=false;
        for(unsigned s=0;s<nt->FileHeader.NumberOfSections;++s)if(range[0]>=sections[s].VirtualAddress && range[1]<=sections[s].VirtualAddress+sections[s].SizeOfRawData) {
            memcpy(base+range[0],bytes.data()+sections[s].PointerToRawData+range[0]-sections[s].VirtualAddress,range[1]-range[0]);copied=true;
        }
        CHECK(copied);if(!copied)return;
        for(DWORD rva=range[0];rva+4<=range[1];++rva) {
            DWORD value;memcpy(&value,base+rva,4);for(DWORD global:globals)if(value==global) {
                value=DWORD(base)+value-0x400000;memcpy(base+rva,&value,4);rva+=3;break;
            }
        }
    }
    Branch(0x4FF69,reinterpret_cast<void*>(Finish));Branch(0x5119C,reinterpret_cast<void*>(Finish));
    Branch(0x4E320,reinterpret_cast<void*>(Action));Branch(0x49600,reinterpret_cast<void*>(Flush));
    Branch(0x32430,reinterpret_cast<void*>(Scan));Branch(0x37480,reinterpret_cast<void*>(Zero));
    Branch(0x37B80,reinterpret_cast<void*>(Available));Branch(0x32330,reinterpret_cast<void*>(Noop));
    Branch(0x42600,reinterpret_cast<void*>(Noop));
    for(unsigned player=0;player<2;++player)for(int hero=0;hero<4;++hero) {
        auto actor=base+0x86A500+player*0xF4;auto enemy=base+0x86A500+2*0xF4;selected_actor=actor;
        const auto run=[&](DWORD start,int state,unsigned commands,int health=100,int away=0) {
            memset(actor,0,0xF4);memset(enemy,0,0xF4);memset(base+0x871850,0,12);
            Field<int>(actor,0xC)=state;Field<int>(actor,0x1C)=0x200;
            Field<short>(actor,0x34)=short(hero);Field<short>(actor,0x42)=short(health);
            Field<short>(actor,0x40)=0x57;Field<signed char>(actor,0x7D)=Field<signed char>(actor,0x7E)=-1;
            Field<signed char>(actor,0x7A)=2;
            const unsigned masks[]={ff_input::Punch,ff_input::Jump,ff_input::Kick,ff_input::Backfist};
            for(unsigned i=0;i<4;++i)base[0x871850+player*6+i]=(commands&masks[i])?12:0;
            chosen=-1;native_start=uintptr_t(base+start);Select(actor,enemy,nullptr,player,state,hero,away);return chosen;
        };
        using namespace ff_input;
        CHECK(run(0x4F823,0x45,Punch)==0);
        CHECK(run(0x4F823,0,Punch)==1 && run(0x4F823,1,Punch)==2);
        CHECK(run(0x4F823,0x45,Kick)==0x1A && run(0x4F823,0x1A,Kick)==0x1B);
        CHECK(run(0x4F823,0x45,Backfist)==0x18 && run(0x4F823,0x18,Backfist)==0x19);
        CHECK(run(0x4F823,0x45,Punch|Kick)==0x1C);
        CHECK(run(0x4F823,0x45,Jump)==0x9E);
        CHECK(run(0x4F823,0x45,Jump|Kick)==0x27);
        CHECK(run(0x4F823,0x45,Jump|Punch)==0x21 && Field<short>(actor,0x42)==68);
        CHECK(run(0x4F823,0x45,Jump|Punch,32)!=0x21 && Field<short>(actor,0x42)==32);
        CHECK(run(0x4F823,0x57,Punch)==0x43);
        CHECK(run(0x50A52,0x57,Kick)==0x42);
        const int grab_punch[]={0xA,0x13,3,0xA8},grab_kick[]={0x28,0x29,4,0x29},grab_back[]={0x22,0xB2,0xA7,0xB2};
        CHECK(run(0x50D04,0x4A,Punch)==grab_punch[hero]);
        CHECK(run(0x50D04,0x4A,Kick)==grab_kick[hero]);
        CHECK(run(0x50D04,0x4A,Jump)==0xB1);
        CHECK(run(0x50D04,0x4A,Backfist)==grab_back[hero]);
        CHECK(run(0x50D04,0x4A,0,100,1)==0x45 && Field<signed char>(actor,0x7A)==-1);
        if(hero==2)CHECK(run(0x50BA5,0xA9,Punch)==0xB2);
    }
}
int main(int argc,char** argv) {
    if(argc<2){puts("Move list native: optional original-executable fixture not requested");return 0;}
    base=static_cast<BYTE*>(VirtualAlloc(nullptr,0x900000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));
    CHECK(base);if(!base)return 1;AddVectoredExceptionHandler(1,Exception);Load(argv[1]);VirtualFree(base,0,MEM_RELEASE);
    puts(failures?"Move list native: FAIL":"Move list native: PASS");return failures?1:0;
}
