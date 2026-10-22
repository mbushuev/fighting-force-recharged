#include "../src/dinput_proxy.cpp"
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"ole32.lib")
#include <fstream>
#include <iterator>
static int failures,effects,idles;static ff_input::Keyboard test_keys;static bool focus=true;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
static bool Focus(){return focus;}
static ff_input::Keyboard Keys(){return test_keys;}
static void __cdecl Effect(int id,int,int,int){CHECK(id==0x362);++effects;}
static int __cdecl Idle(void* actor,void**,int action){CHECK(action==0x45);GuardField<int>(actor,0xC)=action;++idles;return 0;}
static void Jump(DWORD rva,void* function) {
    auto code=reinterpret_cast<BYTE*>(g_base+rva);code[0]=0xE9;
    const LONG offset=static_cast<LONG>(reinterpret_cast<uintptr_t>(function)-reinterpret_cast<uintptr_t>(code+5));memcpy(code+1,&offset,4);
}
static int InvokeUpdate(void* actor,unsigned player) {
    const auto boundary=g_base+0x4EE8B;int result;
    __asm {
        push esi
        push edi
        sub esp,40h
        mov esi,actor
        mov eax,player
        mov [esp+10h],eax // call pushes another four bytes, matching the native local.
        call boundary
        add esp,40h
        pop edi
        pop esi
        mov result,eax
    }
    return result;
}
// Emulate the native contact frame, including both actor pointers and the
// original epilogues. Exercise rejected contacts through the hook as well.
static int __declspec(naked) InvokeContact(unsigned,unsigned,int) {
    __asm {
        sub esp,24h
        push ebx
        push ebp
        push esi
        push edi
        mov ebx,[esp+38h]
        mov edi,[esp+3Ch]
        mov eax,[esp+40h]
        mov [esp+14h],eax
        imul ebp,ebx,0F4h
        add ebp,dword ptr [g_base]
        add ebp,86A500h
        imul esi,edi,0F4h
        add esi,dword ptr [g_base]
        add esi,86A500h
        mov eax,dword ptr [g_base]
        add eax,2B1BBh
        jmp eax
    }
}
static int unguarded;
static void __declspec(naked) NativeHit() {
    __asm {
        inc dword ptr [unguarded]
        sub word ptr [esi+42h],10
        mov dword ptr [esi+0Ch],85h
        jmp dword ptr [g_guard_hit_success]
    }
}
int main(int argc,char** argv) {
    using namespace ff_guard;
    g_base=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,0x900000,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    CHECK(g_base);if(!g_base)return 1;
    wchar_t folder[MAX_PATH];GetTempPathW(MAX_PATH,folder);GetTempFileNameW(folder,L"ffg",0,g_config_path);
    WritePrivateProfileStringW(L"Combat",L"PlayerGuard",L"1",g_config_path);
    const BYTE update[]={0x8B,0x4C,0x24,0x28,0x8B,0x7E,0x0C},hit[]={0x8B,0x4D,0x30,0x8B,0x55,0x28};
    memcpy(reinterpret_cast<void*>(g_base+0x4EE8B),update,7);memcpy(reinterpret_cast<void*>(g_base+0x2B1BB),hit,6);
    Jump(0x411C8,reinterpret_cast<void*>(g_base+0x4E970));InputField<BYTE>(0x411C8)=0xE8;
    InputField<BYTE>(0x2B1BB)=0;CHECK(!InstallPlayerGuard());CHECK(InputField<BYTE>(0x4EE8B)==0x8B);
    memcpy(reinterpret_cast<void*>(g_base+0x2B1BB),hit,6);CHECK(InstallPlayerGuard());
    const BYTE returns[]={0xB8,11,0,0,0,0xC3},skip[]={0xB8,22,0,0,0,0xC3};
    memcpy(reinterpret_cast<void*>(g_base+0x4EE92),returns,6);memcpy(reinterpret_cast<void*>(g_base+0x51EFF),skip,6);
    Jump(0x4E320,reinterpret_cast<void*>(Idle));Jump(0x31370,reinterpret_cast<void*>(Effect));
    g_guard_assets_attempted=g_guard_assets_ready=true;g_modern_gamepad=false;g_modern_focus=Focus;g_modern_keyboard_reader=Keys;
    FirstPersonModel model={};model.joints=27;void* table[0x121]={};table[0x45]=table;
    InputField<FirstPersonModel*>(0x85A6F8+8)=&model;InputField<void**>(0x879780+8)=table;
    Pose pose;pose.angles.resize(81);for(auto& clip:g_guard_clips.clips)clip=Encode({pose,pose});
    if(argc==4) {
        auto read=[](const char* path){std::ifstream f(path,std::ios::binary);return std::vector<uint8_t>(std::istreambuf_iterator<char>(f),{});};
        auto exe=read(argv[1]),hero=read(argv[2]),donor=read(argv[3]);
        auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(exe.data()+reinterpret_cast<const IMAGE_DOS_HEADER*>(exe.data())->e_lfanew);
        auto section=IMAGE_FIRST_SECTION(nt);const BYTE* decoder=nullptr;
        for(unsigned s=0;s<nt->FileHeader.NumberOfSections;++s)if(section[s].VirtualAddress<=0x488F0 &&
            section[s].VirtualAddress+section[s].SizeOfRawData>=0x48A72)
            decoder=exe.data()+section[s].PointerToRawData+0x488F0-section[s].VirtualAddress;
        CHECK(decoder);if(!decoder)return 1;
        // This leaf decoder contains no absolute addresses or external calls.
        memcpy(reinterpret_cast<void*>(g_base+0x488F0),decoder,0x182);
        SmasherClips native_clips;native_clips.Build({hero.data(),hero.size()},{donor.data(),donor.size()});
        for(const auto& clip:native_clips.clips) {
            BYTE actor[0xF4]={},palette[0x238]={};GuardField<const void*>(actor,0)=clip.data();
            auto expected=Decode({clip.data(),clip.size()},27);
            for(size_t f=0;f<expected.size();++f) {
                reinterpret_cast<void(__cdecl*)(const void*,void*,void*)>(g_base+0x488F0)(&model,actor,palette);
                CHECK(!memcmp(actor+0x10,expected[f].root,12));CHECK(!memcmp(palette+0x118,expected[f].angles.data(),81*2));
            }
            CHECK(GuardField<int>(actor,4)==-1);
        }
        puts("Original native decoder: PASS for all converted guard frames.");
    }
    InputField<int>(0x8745A0)=1;g_guard_tick=100;g_guard_stage=0;
    // Native input mode is one during gameplay; it is not a replay flag.
    InputField<int>(0x93D20)=1;
    auto actor=GuardActor(0);GuardField<int16_t>(actor,0x34)=2;GuardField<int16_t>(actor,0x42)=100;
    GuardField<int>(actor,0xC)=0x45;GuardField<int8_t>(actor,0x7D)=GuardField<int8_t>(actor,0x7E)=-1;
    GuardField<int8_t>(actor,0x78)=GuardField<int8_t>(actor,0x7A)=-1;
    CHECK(InvokeUpdate(actor,0)==11);test_keys.block=true;CHECK(InvokeUpdate(actor,0)==22);CHECK(GuardOwns(actor));
    CHECK(GuardField<int>(actor,0xC)==Entry && table[Entry]==g_guard_clips.clips[0].data());
    GuardField<int>(actor,4)=-1;CHECK(InvokeUpdate(actor,0)==22);CHECK(GuardField<int>(actor,0xC)==Hold);
    auto attacker=GuardActor(2);GuardField<int>(attacker,0x30)=-100*65536;GuardField<int>(attacker,0x1C)=1;
    GuardField<int8_t>(attacker,0x7D)=GuardField<int8_t>(attacker,0x7E)=-1;
    const BYTE success[]={0xB8,1,0,0,0,0x5F,0x5E,0x5D,0x5B,0x83,0xC4,0x24,0xC3};
    const BYTE repeat[]={0x31,0xC0,0x5F,0x5E,0x5D,0x5B,0x83,0xC4,0x24,0xC3};
    memcpy(reinterpret_cast<void*>(g_guard_hit_success),success,sizeof(success));
    memcpy(reinterpret_cast<void*>(g_guard_hit_repeat),repeat,sizeof(repeat));
    Jump(0x2B1C1,reinterpret_cast<void*>(NativeHit));
    CHECK(InvokeContact(2,0,123)==1);CHECK(InvokeContact(2,0,123)==0);CHECK(effects==1);
    CHECK(GuardField<int16_t>(actor,0x42)==100 && GuardField<int>(actor,0xC)==Impact);
    CHECK(unguarded==0);
    GuardField<int>(attacker,0x30)=100*65536;CHECK(InvokeContact(2,0,0)==1);
    CHECK(unguarded==1 && GuardField<int16_t>(actor,0x42)==90 && GuardField<int>(actor,0xC)==0x85);
    GuardSetAction(actor,Impact);GuardField<int>(attacker,0x30)=-100*65536;GuardField<int>(attacker,0xC)=0x41;
    CHECK(InvokeContact(2,0,0)==1);CHECK(unguarded==2 && GuardField<int16_t>(actor,0x42)==80);
    GuardSetAction(actor,Impact);
    test_keys.block=false;CHECK(InvokeUpdate(actor,0)==22);CHECK(GuardField<int>(actor,0xC)==Exit);
    GuardField<int>(actor,4)=-1;CHECK(InvokeUpdate(actor,0)==11);CHECK(idles==1 && GuardField<int>(actor,0xC)==0x45);
    test_keys.block=true;InvokeUpdate(actor,0);focus=false;InvokeUpdate(actor,0);CHECK(GuardField<int>(actor,0xC)==Exit);
    // Player two's update never inherits player one's guard ownership.
    CHECK(UpdatePlayerGuard(GuardActor(1),1)==0);CHECK(g_guard_states[1].phase==None);
    DeleteFileW(g_config_path);VirtualFree(reinterpret_cast<void*>(g_base),0,MEM_RELEASE);
    puts(failures?"Player guard runtime: FAIL":"Player guard runtime: PASS");return failures?1:0;
}
