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
    for(unsigned h=0;h<4;++h)g_guard_assets_attempted[h]=g_guard_assets_ready[h]=true;g_modern_gamepad=false;g_modern_focus=Focus;g_modern_keyboard_reader=Keys;
    FirstPersonModel models[4]={};void* tables[4][0x121]={};
    for(unsigned h=0;h<4;++h) {
        models[h].joints=g_guard_joints[h];tables[h][0x45]=tables[h];
        InputField<FirstPersonModel*>(0x85A6F8+h*4)=&models[h];InputField<void**>(0x879780+h*4)=tables[h];
        Pose pose;pose.angles.resize(g_guard_joints[h]*3);for(auto& clip:g_guard_clips[h].clips)clip=Encode({pose,pose});
    }
    auto& table=tables[2];
    if(argc==3 || argc==4) {
        auto read=[](const char* path){std::ifstream f(path,std::ios::binary);return std::vector<uint8_t>(std::istreambuf_iterator<char>(f),{});};
        auto exe=read(argv[1]);
        auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(exe.data()+reinterpret_cast<const IMAGE_DOS_HEADER*>(exe.data())->e_lfanew);
        auto section=IMAGE_FIRST_SECTION(nt);const BYTE* decoder=nullptr;
        for(unsigned s=0;s<nt->FileHeader.NumberOfSections;++s)if(section[s].VirtualAddress<=0x488F0 &&
            section[s].VirtualAddress+section[s].SizeOfRawData>=0x48A72)
            decoder=exe.data()+section[s].PointerToRawData+0x488F0-section[s].VirtualAddress;
        CHECK(decoder);if(!decoder)return 1;
        // This leaf decoder contains no absolute addresses or external calls.
        memcpy(reinterpret_cast<void*>(g_base+0x488F0),decoder,0x182);
        auto verify=[&](const GuardClips& clips,unsigned joints) {
            FirstPersonModel native_model={};native_model.joints=joints;
            for(const auto& clip:clips.clips) {
                BYTE actor[0xF4]={},palette[0x238]={};GuardField<const void*>(actor,0)=clip.data();
                auto expected=Decode({clip.data(),clip.size()},joints);
                for(size_t f=0;f<expected.size();++f) {
                    reinterpret_cast<void(__cdecl*)(const void*,void*,void*)>(g_base+0x488F0)(&native_model,actor,palette);
                    CHECK(!memcmp(actor+0x10,expected[f].root,12));CHECK(!memcmp(palette+0x118,expected[f].angles.data(),joints*3*2));
                }
                CHECK(GuardField<int>(actor,4)==-1);
            }
        };
        if(argc==4) {
            auto hero=read(argv[2]),donor=read(argv[3]);SmasherClips clips;
            clips.Build({hero.data(),hero.size()},{donor.data(),donor.size()});verify(clips,27);
        } else {
            const char* names[]={"HAWK","MACE","SMASHER","ALANA"};
            for(unsigned h=0;h<4;++h) {
                auto hero=read((std::string(argv[2])+"/"+names[h]+".WAD").c_str());
                const bool female=h==1 || h==3;
                auto donor=read((std::string(argv[2])+(female?"/CHARS15.WAD":"/CHARS11.WAD")).c_str());
                const Bytes target={hero.data(),hero.size()},source={donor.data(),donor.size()};
                if(h==2){SmasherClips clips;clips.Build(target,source);verify(clips,27);}
                else {
                    UpperBodyClips clips;
                    clips.Build(target,source,(std::string(names[h])+".ACH").c_str(),(std::string(names[h])+".ADT").c_str(),
                        female?"VAMP.ACH":"SUIT.ACH",female?"GENFMALE.ADT":"GENMALE.ADT",g_guard_joints[h],female?12:8);
                    verify(clips,g_guard_joints[h]);
                }
            }
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
    CHECK(GuardField<int>(actor,0xC)==Entry && table[Entry]==g_guard_clips[2].clips[0].data());
    GuardField<int>(actor,4)=-1;CHECK(InvokeUpdate(actor,0)==22);CHECK(GuardField<int>(actor,0xC)==Hold);
    // Direction input turns a guarded actor without moving or queuing attacks.
    for(int tick=0;tick<4;++tick) {
        InputField<uint32_t>(0x5523E0)=ff_input::Right|ff_input::Punch;
        GuardField<int>(actor,0x14)=GuardField<int>(actor,0x18)=123;
        CHECK(InvokeUpdate(actor,0)==22);
        CHECK(GuardField<int16_t>(actor,0x38)==(tick+1)*128);
        CHECK(!GuardField<int>(actor,0x14) && !GuardField<int>(actor,0x18));
        CHECK(!(InputField<uint32_t>(0x5523E0)&65535) && GuardField<int>(actor,0xC)==Hold);
    }
    CHECK(GuardField<int>(actor,0x28)==0 && GuardField<int>(actor,0x30)==0);
    InvokeUpdate(actor,0);CHECK(GuardField<int16_t>(actor,0x38)==512);
    GuardField<int16_t>(actor,0x38)=0;
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
    // Every hero attaches to its own table, including reloads and player two.
    focus=true;
    auto prepare_actor=[&](unsigned slot,unsigned hero) {
        auto a=GuardActor(slot);memset(a,0,0xF4);g_guard_states[slot]={};
        GuardField<int16_t>(a,0x34)=static_cast<int16_t>(hero);GuardField<int16_t>(a,0x42)=100;
        GuardField<int>(a,0xC)=0x45;
        for(unsigned offset:{0x78u,0x7Au,0x7Du,0x7Eu})GuardField<int8_t>(a,offset)=-1;
        return a;
    };
    for(unsigned h=0;h<4;++h) {
        const unsigned slot=h%2;auto a=prepare_actor(slot,h);test_keys.block=false;
        CHECK(InvokeUpdate(a,slot)==11);test_keys.block=true;CHECK(InvokeUpdate(a,slot)==22);
        CHECK(GuardOwns(a) && tables[h][Entry]==g_guard_clips[h].clips[0].data());
        GuardField<int>(a,4)=-1;CHECK(InvokeUpdate(a,slot)==22);CHECK(GuardField<int>(a,0xC)==Hold);
        g_guard_attacks[2]={};GuardField<int>(attacker,0xC)=0;GuardField<int>(attacker,0x30)=-100*65536;
        CHECK(InvokeContact(2,slot,0)==1 && GuardField<int16_t>(a,0x42)==100);
        CHECK(GuardField<int>(a,0xC)==Impact);CHECK(InvokeContact(2,slot,0)==0);
        GuardField<int>(attacker,0x30)=100*65536;
        CHECK(InvokeContact(2,slot,0)==1 && GuardField<int16_t>(a,0x42)==90);
        GuardSetAction(a,Impact);test_keys.block=false;InvokeUpdate(a,slot);
        CHECK(GuardField<int>(a,0xC)==Exit);GuardField<int>(a,4)=-1;InvokeUpdate(a,slot);
        CHECK(GuardField<int>(a,0xC)==0x45);
        for(int action:{Entry,Hold,Impact,Exit})tables[h][action]=nullptr;
        CHECK(AttachPlayerGuard(a) && tables[h][Hold]==g_guard_clips[h].clips[1].data());
        ++models[h].joints;CHECK(!AttachPlayerGuard(a));--models[h].joints;
        tables[h][Hold]=tables[h];CHECK(!AttachPlayerGuard(a));tables[h][Hold]=g_guard_clips[h].clips[1].data();
    }
    auto one=prepare_actor(0,1),two=prepare_actor(1,3);
    GuardSetAction(one,Hold);GuardSetAction(two,Hold);g_guard_states[0].phase=g_guard_states[1].phase=Hold;
    CHECK(GuardField<void*>(one,0)!=GuardField<void*>(two,0));
    g_guard_attacks[2]={};GuardField<int>(attacker,0x30)=-100*65536;
    CHECK(InvokeContact(2,0,0)==1 && InvokeContact(2,1,0)==1);
    CHECK(InvokeContact(2,0,0)==0 && InvokeContact(2,1,0)==0);
    CHECK(GuardField<int16_t>(one,0x42)==100 && GuardField<int16_t>(two,0x42)==100);
    // Split-screen camera yaw and input belong to the guarded player only.
    g_coop_ready=g_camera_cooperative=g_camera_active=true;
    g_coop_players[0].orbit.yaw=0;g_coop_players[1].orbit.yaw=3.141592653589793;
    InputField<uint32_t>(0x5523E0)=ff_input::Up;InputField<uint32_t>(0x5523E4)=ff_input::Up;
    test_keys.block=true;InvokeUpdate(one,0);InvokeUpdate(two,1);
    CHECK(GuardField<int16_t>(one,0x38)==1920 && GuardField<int16_t>(two,0x38)==0);
    g_coop_ready=g_camera_cooperative=g_camera_active=false;
    GuardRestoreIdle(one);g_guard_states[0]={};CHECK(!GuardOwns(one) && GuardOwns(two));
    GuardField<int16_t>(one,0x34)=-1;CHECK(!GuardOwns(one) && !AttachPlayerGuard(one));
    DeleteFileW(g_config_path);VirtualFree(reinterpret_cast<void*>(g_base),0,MEM_RELEASE);
    puts(failures?"Player guard runtime: FAIL":"Player guard runtime: PASS");return failures?1:0;
}
