#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static SHORT WINAPI NoPhysicalKeys(int){return 0;}
#define GetAsyncKeyState NoPhysicalKeys
#include "../src/dinput_proxy.cpp"
#undef GetAsyncKeyState
#include <fstream>
#include <iterator>
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"ole32.lib")
static int failures,chosen=-1,flushes,drops;
static ff_input::Keyboard pickup_keys[2];
static XINPUT_STATE controllers[2];
static bool focused=true;
static void* tables[4][0x121];
static uintptr_t native_start;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);++failures;}}while(0)
static bool Focus(){return focused;}
static ff_input::Keyboard Keys(){return pickup_keys[g_controls_keyboard_player];}
static DWORD WINAPI Pads(DWORD slot,XINPUT_STATE* out){if(slot>=2)return ERROR_DEVICE_NOT_CONNECTED;*out=controllers[slot];return ERROR_SUCCESS;}
static int __cdecl SetAction(void* actor,void**,int action){chosen=action;GuardField<int>(actor,0xC)=action;return 1;}
static void __cdecl Flush(int){++flushes;}
static void __cdecl Scan(unsigned player,int){InputField<int>(0x8776E0+player*4)=0;}
static void __cdecl Drop(unsigned player){++drops;GuardField<int8_t>(GuardActor(player),0x7D)=GuardField<int8_t>(GuardActor(player),0x7E)=-1;}
static int __cdecl NoObstacle(int,int){return 0;}
static void Jump(DWORD rva,void* to){auto p=reinterpret_cast<BYTE*>(g_base+rva);p[0]=0xE9;LONG delta=LONG(reinterpret_cast<uintptr_t>(to)-(g_base+rva+5));memcpy(p+1,&delta,4);}
static void Call(DWORD rva,DWORD target){auto p=reinterpret_cast<BYTE*>(g_base+rva);p[0]=0xE8;LONG delta=target-rva-5;memcpy(p+1,&delta,4);}
static void Setup() {
    g_controls=ff_controls::DefaultsSettings();g_modern_focus=Focus;g_modern_keyboard_reader=Keys;
    g_modern_keyboard=g_modern_gamepad=true;g_modern_coop_input=false;g_modern_xinput=Pads;
    g_modern_pad_session={};g_modern_second_session={};g_modern_controller=g_modern_second_controller=-1;
    g_controls_menu_active=g_campaign_menu_active=g_controls_wait_release=false;
    pickup_keys[0]=pickup_keys[1]={};controllers[0]=controllers[1]={};focused=true;chosen=-1;flushes=drops=0;
    InputField<int>(0x85A6B8)=0;InputField<int>(0x8745A0)=1;InputField<int>(0x85A668)=0;
    InputField<int>(0x85B560)=100;InputField<int>(0x85DE60)=0;InputField<int>(0x85A6BC)=0;
    InputField<int>(0x93D20)=1; // Native game entry disables frontend readers, not live gameplay.
    for(unsigned p=0;p<2;++p) {
        auto actor=GuardActor(p);memset(actor,0,0xF4);GuardField<int>(actor,0xC)=0x45;
        GuardField<int16_t>(actor,0x34)=int16_t(p);GuardField<int16_t>(actor,0x42)=100;
        for(unsigned offset:{0x78u,0x7Au,0x7Du,0x7Eu})GuardField<int8_t>(actor,offset)=-1;
        InputField<uint32_t>(0x5523E0+p*4)=0;
    }
    for(unsigned h=0;h<4;++h){for(auto& clip:tables[h])clip=reinterpret_cast<void*>(g_base+0x60000);InputField<void**>(0x879780+h*4)=tables[h];}
    for(unsigned i=0;i<64;++i){memset(reinterpret_cast<void*>(g_base+0x870620+i*36),0,36);InputField<int>(0x870620+i*36)=-1;}
    for(unsigned i=0;i<2560;++i)InputField<int16_t>(0x67F50+i*2)=int16_t(sin(double(i)*6.283185307179586/2048)*32767);
    InputField<int>(0x870620)=0x1F3;
    Jump(0x4E320,reinterpret_cast<void*>(SetAction));Jump(0x49600,reinterpret_cast<void*>(Flush));
    Jump(0x32330,reinterpret_cast<void*>(Drop));Jump(0x37480,reinterpret_cast<void*>(NoObstacle));ResetItemPickup();
}
static int Tick(unsigned p=0,bool guarded=false) {
    ++InputField<int>(0x85B560);g_modern_pad_session.last_poll=g_modern_second_session.last_poll=0;
    return UpdateItemPickup(GuardActor(p),p,guarded);
}
static void __declspec(naked) NativeFinish(){
    __asm {
        add esp,30h
        pop ebp
        pop edi
        pop esi
        pop ebx
        ret
    }
}
static void __declspec(naked) __cdecl NativeSelect(void*,void**,unsigned,int) {
    __asm {
        push ebx
        push esi
        push edi
        push ebp
        mov esi,[esp+20]
        mov edi,[esp+24]
        mov eax,[esp+28]
        mov ecx,[esp+32]
        sub esp,30h
        mov [esp+14h],eax
        jmp dword ptr [native_start]
    }
}
static void __declspec(naked) __cdecl NativeThrow(void*,void**,unsigned,int) {
    __asm {
        push ebx
        push esi
        push edi
        push ebp
        mov esi,[esp+20]
        mov ecx,[esp+24]
        mov eax,[esp+28]
        sub esp,30h
        mov [esp+14h],eax
        mov [esp+18h],ecx
        mov dword ptr [esp+2Ch],-1
        mov ebp,45h
        mov edi,-1
        jmp dword ptr [native_start]
    }
}
static void NativeFixture(const char* path) {
    std::ifstream stream(path,std::ios::binary);std::vector<BYTE> bytes((std::istreambuf_iterator<char>(stream)),{});
    CHECK(bytes.size()>0x40000);if(bytes.size()<0x40000)return;
    Setup();auto base=reinterpret_cast<BYTE*>(g_base);
    const auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(bytes.data()+reinterpret_cast<const IMAGE_DOS_HEADER*>(bytes.data())->e_lfanew);
    const auto sections=IMAGE_FIRST_SECTION(nt);
    const DWORD ranges[][2]={{0x32430,0x32674},{0x4FB21,0x4FB26},{0x4FB67,0x4FB6C},{0x4FB8F,0x4FC05},
        {0x4F638,0x4FA08}};
    const DWORD globals[]={0xC70624,0xC6A534,0xC6A500,0x467F50,0x468350,0xC776E0,0xC70620,
        0xC71850,0xC71851,0xC71852,0xC71853,0xC6A57D,0xC6A57E,0xC77694,0xC6E9CC,0xC5A660};
    for(const auto& range:ranges) {
        bool copied=false;
        for(unsigned s=0;s<nt->FileHeader.NumberOfSections;++s)if(range[0]>=sections[s].VirtualAddress && range[1]<=sections[s].VirtualAddress+sections[s].SizeOfRawData) {
            memcpy(base+range[0],bytes.data()+sections[s].PointerToRawData+range[0]-sections[s].VirtualAddress,range[1]-range[0]);copied=true;
        }
        CHECK(copied);if(!copied)return;
        for(DWORD rva=range[0];rva+4<=range[1];++rva) {
            DWORD value;memcpy(&value,base+rva,4);for(DWORD global:globals)if(value==global) {
                value=DWORD(g_base)+value-0x400000;memcpy(base+rva,&value,4);rva+=3;break;
            }
        }
    }
    auto actor=GuardActor(0);auto scan=reinterpret_cast<void(__cdecl*)(unsigned,int)>(g_base+0x32430);
    // Compare actual retail/RIP candidate selection and animation dispatch.
    for(int hero=0;hero<4;++hero)for(int type=0x1DD;type<0x233;++type)
    for(int heading:{0,256,512,768,1024,1280,1536,1792})for(int distance:{-100,-50,0,50,100}) {
        GuardField<int16_t>(actor,0x34)=int16_t(hero);InputField<int>(0x870620)=type;
        GuardField<int16_t>(actor,0x38)=int16_t(heading);
        InputField<int32_t>(0x87062C)=distance*65536;
        scan(0,96);CHECK(FindItemPickup(actor)==InputField<int>(0x8776E0));
        chosen=-1;NativeSelect(actor,tables[hero],0,0);CHECK(chosen==ff_pickup::Action(type));
    }
    g_player_guard_available=true;CHECK(InstallItemPickup());
    InputField<int>(0x870620)=0x1F3;InputField<int32_t>(0x87062C)=0;
    ItemPickupPunchScan(0,96);CHECK(InputField<int>(0x8776E0)==-1);
    g_controls.separate_pickup=false;ItemPickupPunchScan(0,96);CHECK(InputField<int>(0x8776E0)==0);
    g_controls.separate_pickup=true;InputField<int>(0x85A668)=1;
    ItemPickupPunchScan(0,96);CHECK(InputField<int>(0x8776E0)==0); // Recorded demonstrations stay native.
    InputField<int>(0x85A668)=0;
    BYTE kick_entry[5];memcpy(kick_entry,base+0x4F823,5);
    Jump(0x4F72D,reinterpret_cast<void*>(NativeFinish));Jump(0x4F823,reinterpret_cast<void*>(NativeFinish));
    for(int hand=0;hand<2;++hand)for(int type=0x1DD;type<0x233;++type) {
        GuardField<int8_t>(actor,0x7D)=hand==0?0:-1;GuardField<int8_t>(actor,0x7E)=hand==1?0:-1;
        InputField<int>(0x870620)=type;
        GuardField<int>(actor,0xC)=hand==1?(type>0x209?0x46:0x48):(type==0x1F8?0x4E:0x49);
        native_start=g_base+(hand==1?0x4F670:0x4F761);chosen=-1;
        NativeThrow(actor,tables[0],0,0);CHECK(chosen==ff_pickup::ThrowAction(hand==1?type:-1,hand==0?type:-1));
    }
    memcpy(base+0x4F823,kick_entry,5);native_start=g_base+0x4FB8F;
    for(unsigned player=0;player<2;++player)for(int type:{0x1F3,0x1FB,0x210})for(bool separate:{false,true}) {
        actor=GuardActor(player);GuardField<int8_t>(actor,0x7D)=-1;GuardField<int8_t>(actor,0x7E)=0;
        GuardField<int>(actor,0xC)=type>0x209?0x46:0x48;InputField<int>(0x870620)=type;
        memset(base+0x871850,0,12);InputField<BYTE>(0x871852+player*6)=12;
        InputField<int>(0x877694)=0;g_controls.separate_pickup=separate;chosen=-1;drops=0;
        native_start=g_base+0x4F638;NativeThrow(actor,tables[0],player,0);
        if(chosen!=(separate?0x1A:type>0x209?0xA1:0x9F))printf("Kick selection: player=%u type=%x separate=%d action=%x\n",player,type,separate,chosen);
        CHECK(chosen==(separate?0x1A:type>0x209?0xA1:0x9F));
        CHECK(GuardField<int8_t>(actor,0x7E)==0 && !drops);
    }
    g_controls.separate_pickup=true;native_start=g_base+0x4FB8F;
}
int main(int argc,char** argv) {
    auto base=static_cast<BYTE*>(VirtualAlloc(nullptr,0x900000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));
    CHECK(base);if(!base)return 1;g_base=reinterpret_cast<uintptr_t>(base);Setup();
    native_start=g_base+0x4FB8F;Jump(0x4FF1F,reinterpret_cast<void*>(NativeFinish));
    const BYTE scanner[]={0x83,0xEC,0x24,0x8B,0x4C,0x24,0x28,0x53};memcpy(base+0x32430,scanner,sizeof(scanner));
    const BYTE hand[]={0x8A,0x46,0x7E,0x3C,0xFF},heavy[]={0x3D,9,2,0,0,0x7E,4,0x6A,0x79};
    memcpy(base+0x4FB67,hand,sizeof(hand));memcpy(base+0x4FBB1,heavy,sizeof(heavy));Call(0x4FB21,0x32430);
    const BYTE drop[]={0x8B,0x7C,0x24,0x2C,0x83,0xFF,0xFF};memcpy(base+0x4F638,drop,sizeof(drop));
    g_player_guard_available=true;CHECK(InstallItemPickup());Jump(0x32430,reinterpret_cast<void*>(Scan));
    ItemPickupPunchScan(0,96);CHECK(InputField<int>(0x8776E0)==-1);
    g_controls.separate_pickup=false;ItemPickupPunchScan(0,96);CHECK(InputField<int>(0x8776E0)==0);
    Setup();CHECK(!Tick());pickup_keys[0].pickup=true;CHECK(Tick()==1 && chosen==0x76 && flushes==1);
    GuardField<int>(GuardActor(0),0xC)=0x45;CHECK(!Tick() && flushes==1);
    pickup_keys[0].pickup=false;Tick();pickup_keys[0].pickup=true;InputField<unsigned>(0x5523E0)=ff_input::Punch;
    CHECK(!Tick() && flushes==1);InputField<unsigned>(0x5523E0)=0;
    pickup_keys[0].pickup=false;Tick();pickup_keys[0].pickup=true;CHECK(!Tick(0,true));CHECK(flushes==1);
    pickup_keys[0].pickup=false;Tick();pickup_keys[0].pickup=true;GuardField<int>(GuardActor(0),0xC)=0x85;CHECK(!Tick());
    GuardField<int>(GuardActor(0),0xC)=0x45;CHECK(!Tick());
    pickup_keys[0].pickup=false;Tick();pickup_keys[0].pickup=true;GuardField<int8_t>(GuardActor(0),0x7E)=0;CHECK(!Tick());
    GuardField<int8_t>(GuardActor(0),0x7E)=-1;pickup_keys[0].pickup=false;Tick();focused=false;pickup_keys[0].pickup=true;CHECK(!Tick());
    focused=true;CHECK(!Tick());pickup_keys[0].pickup=false;Tick();InputField<int>(0x870620)=-1;pickup_keys[0].pickup=true;
    chosen=-1;CHECK(!Tick() && chosen==-1);
    for(int scheme=1;scheme<=3;++scheme)for(unsigned owner=0;owner<2;++owner) {
        Setup();g_modern_coop_input=true;g_modern_coop_scheme=scheme;InputField<int>(0x8745A0)=2;
        Tick(0);Tick(1);
        if(ModernKeyboardPlayer()==int(owner))pickup_keys[owner].pickup=true;
        else controllers[scheme==2?owner:0].Gamepad.wButtons=ff_input::RightShoulder;
        CHECK(Tick(owner)==1);CHECK(!Tick(1-owner));CHECK(GuardField<int>(GuardActor(1-owner),0xC)==0x45);
    }
    Setup();g_modern_coop_input=true;g_modern_coop_scheme=2;InputField<int>(0x8745A0)=2;
    GuardField<int16_t>(GuardActor(0),0x42)=0;Tick(0);Tick(1);
    controllers[1].Gamepad.wButtons=ff_input::RightShoulder;CHECK(Tick(1)==1 && !Tick(0));
    Setup();Tick();pickup_keys[0].throw_item=true;CHECK(!Tick() && chosen==-1 && !drops);
    for(int scheme=1;scheme<=3;++scheme)for(unsigned owner=0;owner<2;++owner)for(int type:{0x1FB,0x210}) {
        Setup();g_modern_coop_input=true;g_modern_coop_scheme=scheme;InputField<int>(0x8745A0)=2;
        GuardField<int8_t>(GuardActor(owner),0x7E)=0;InputField<int>(0x870620)=type;
        Tick(0);Tick(1);
        if(ModernKeyboardPlayer()==int(owner))pickup_keys[owner].throw_item=true;
        else controllers[scheme==2?owner:0].Gamepad.wButtons=ff_input::LeftShoulder;
        CHECK(Tick(owner)==1 && chosen==(type==0x210?0xA1:0x9F) && flushes==1);
        CHECK(!Tick(owner) && !Tick(1-owner) && flushes==1 && !drops);
    }
    Setup();GuardField<int8_t>(GuardActor(0),0x7D)=0;InputField<int>(0x870620)=0x203;
    Tick();pickup_keys[0].throw_item=true;CHECK(Tick()==1 && chosen==0x45 && drops==1);
    Setup();GuardField<int8_t>(GuardActor(0),0x7E)=0;Tick();pickup_keys[0].throw_item=true;
    GuardField<int>(GuardActor(0),0xC)=0x85;CHECK(!Tick());GuardField<int>(GuardActor(0),0xC)=0x48;CHECK(!Tick());
    pickup_keys[0].throw_item=false;Tick();focused=false;pickup_keys[0].throw_item=true;CHECK(!Tick());
    focused=true;CHECK(!Tick());pickup_keys[0].throw_item=false;Tick();
    g_controls.separate_pickup=false;pickup_keys[0].throw_item=true;CHECK(Tick()==1 && chosen==0x9F);
    for(int i=1;i<argc;++i)NativeFixture(argv[i]);
    VirtualFree(base,0,MEM_RELEASE);puts(failures?"Item pickup runtime: FAIL":"Item pickup runtime: PASS");return failures?1:0;
}
