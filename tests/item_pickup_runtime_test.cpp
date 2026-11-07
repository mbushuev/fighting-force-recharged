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
static int failures,chosen=-1,flushes,drops,created=-1,creations;
static int pin_effects,pin_owner,pin_type;
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
static int __cdecl CreateItem(int type,int,int,int,int){created=type;++creations;return 0;}
static void __cdecl NoSound(int,int,int){}
static void __cdecl GrenadePin(unsigned player,int type){++pin_effects;pin_owner=player;pin_type=type;}
static void __cdecl Fire(unsigned){}
static void Jump(DWORD rva,void* to){auto p=reinterpret_cast<BYTE*>(g_base+rva);p[0]=0xE9;LONG delta=LONG(reinterpret_cast<uintptr_t>(to)-(g_base+rva+5));memcpy(p+1,&delta,4);}
static void Call(DWORD rva,DWORD target){auto p=reinterpret_cast<BYTE*>(g_base+rva);p[0]=0xE8;LONG delta=target-rva-5;memcpy(p+1,&delta,4);}
static void Setup() {
    g_controls=ff_controls::DefaultsSettings();g_modern_focus=Focus;g_modern_keyboard_reader=Keys;
    g_modern_keyboard=g_modern_gamepad=true;g_modern_coop_input=false;g_modern_xinput=Pads;
    g_modern_pad_session={};g_modern_second_session={};g_modern_pad_pair={};g_modern_controller=g_modern_second_controller=-1;
    g_controls_menu_active=g_campaign_menu_active=g_controls_wait_release=false;
    pickup_keys[0]=pickup_keys[1]={};controllers[0]=controllers[1]={};focused=true;chosen=-1;flushes=drops=0;
    InputField<int>(0x85A6B8)=0;InputField<int>(0x8745A0)=1;InputField<int>(0x85A668)=0;
    InputField<int>(0x85B560)=100;InputField<int>(0x85DE60)=0;InputField<int>(0x85A6BC)=0;
    InputField<int>(0x93D20)=1; // Native game entry disables frontend readers, not live gameplay.
    InputField<int>(0x8689E8)=0;created=-1;creations=0;pin_effects=0;pin_owner=pin_type=-1;
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
    Jump(0x31290,reinterpret_cast<void*>(CreateItem));Jump(0x42600,reinterpret_cast<void*>(NoSound));
    Jump(0xEFD0,reinterpret_cast<void*>(GrenadePin));Jump(0xE290,reinterpret_cast<void*>(Fire));
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
        mov [esp+18h],edi
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
    const DWORD ranges[][2]={{0x32430,0x32674},{0x4FB1A,0x4FF1F},{0x4F638,0x4FA08},
        {0x50AFA,0x50B93},{0x51F5C,0x51FBD},{0xE790,0xE88C},{0x11BB0,0x11D52},{0x37B80,0x37B90}};
    const DWORD globals[]={0xC70624,0xC6A534,0xC6A500,0x467F50,0x468350,0xC776E0,0xC70620,
        0xC71850,0xC71851,0xC71852,0xC71853,0xC6A57D,0xC6A57E,0xC77694,0xC6E9CC,0xC5A660,
        0xC6A538,0xC6A56C,0xC677BC};
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
    const DWORD native_jump_input=0xC71851,native_kick_input=0xC71852,native_item_type=0xC70620;
    memcpy(base+0x4F832,&native_jump_input,4);
    memcpy(base+0x4F8EF,&native_kick_input,4);
    memcpy(base+0x4FC22,&native_item_type,4);
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
    memcpy(base+0x4F823,kick_entry,5);
    Jump(0x4FA08,reinterpret_cast<void*>(NativeFinish));
    Jump(0x5119C,reinterpret_cast<void*>(NativeFinish));
    for(unsigned player=0;player<2;++player)for(bool separate:{false,true}) {
        actor=GuardActor(player);g_controls.separate_pickup=separate;
        for(bool left:{false,true})for(int type:{0x1F3,0x1FB,0x210}) {
            for(unsigned input:{2,3}) {
                GuardField<int8_t>(actor,0x7D)=left?0:-1;GuardField<int8_t>(actor,0x7E)=left?-1:0;
                GuardField<int>(actor,0xC)=type>0x209?0x46:0x48;InputField<int>(0x870620)=type;
                memset(base+0x871850,0,12);InputField<BYTE>(0x871850+player*6+input)=12;
                InputField<int>(0x877694)=0;chosen=-1;drops=0;
                native_start=g_base+0x4F638;NativeThrow(actor,tables[0],player,0);
                CHECK(input==2?chosen==(type>0x209?-1:0x1A):chosen!=0x9F && chosen!=0xA1 && chosen!=0xA3);
                CHECK(GuardField<int8_t>(actor,left?0x7D:0x7E)==0 && !drops);
            }
        }
        for(bool left:{false,true}) {
            GuardField<int8_t>(actor,0x7D)=left?0:-1;GuardField<int8_t>(actor,0x7E)=left?-1:0;
            InputField<int>(0x870620)=0x210;
            memset(base+0x871850,0,12);InputField<BYTE>(0x871851+player*6)=12;
            chosen=-1;drops=0;native_start=g_base+0x4F638;NativeThrow(actor,tables[0],player,0);
            CHECK(chosen==-1 && !drops && GuardField<int8_t>(actor,left?0x7D:0x7E)==0);
        }
        memset(base+0x871850,0,12);
        GuardField<int8_t>(actor,0x7D)=-1;GuardField<int8_t>(actor,0x7E)=0;
        for(int type:{0x1F0,0x1F1,0x201,0x210,0x1F3,0x1E6}) {
            InputField<int>(0x870620)=type;InputField<int16_t>(0x870636)=type==0x1F3?2:0;
            GuardField<int>(actor,0xC)=0x45;GuardField<BYTE>(actor,0x66)=0;
            chosen=-1;pin_effects=0;native_start=g_base+0x4FB1A;NativeSelect(actor,tables[0],player,0);
            CHECK(chosen==(type==0x1F3?0xC0:type==0x1E6?0x2B:-1));
            CHECK(InputField<int16_t>(0x870636)==(type==0x1F3?1:0) && !pin_effects);
        }
        for(DWORD entry:{0x4F638u,0x50AFAu})for(bool armed:{true,false}) {
            GuardField<int8_t>(actor,0x7E)=armed?0:-1;
            GuardField<int16_t>(actor,0x42)=100;InputField<int>(0x870620)=0x1F3;
            memset(base+0x871850,0,12);InputField<BYTE>(0x871850+player*6)=12;InputField<BYTE>(0x871851+player*6)=12;
            chosen=-1;drops=0;native_start=g_base+entry;NativeThrow(actor,tables[0],player,0);
            CHECK(chosen==(armed?-1:0x21) && drops==(armed?0:1));
            CHECK(GuardField<int16_t>(actor,0x42)==(armed?100:68));
        }
    }
    auto fixture=base+0x86DD20,object=base+0x80000;InputField<int>(0x8689E8)=1;
    for(unsigned player=0;player<2;++player)for(int hero=0;hero<4;++hero)
    for(int source:{0x273,0x297,0x32D})for(int callback:{7,8,9,10}) {
        actor=GuardActor(player);GuardField<int16_t>(actor,0x34)=int16_t(hero);
        GuardField<int>(actor,0xC)=0x45;GuardField<int>(actor,0x1C)=0;
        GuardField<int8_t>(actor,0x7D)=GuardField<int8_t>(actor,0x7E)=-1;GuardField<BYTE>(actor,0x66)=0;
        GuardField<int16_t>(actor,0x38)=int16_t(callback==7?512:callback==8?1536:callback==9?0:1024);
        GuardField<int16_t>(fixture,8)=int16_t(callback);GuardField<void*>(fixture,0x14)=object;GuardField<int>(object,0)=source;
        const DWORD provider=callback==7?0xE810:callback==8?0xE850:callback==9?0xE790:0xE7D0;
        reinterpret_cast<void(__cdecl*)(void*,unsigned)>(g_base+provider)(fixture,player);
        CHECK(FindWorldPickup(actor)==fixture);
        InputField<int>(0x870620)=-1;native_start=g_base+0x4FB1A;
        for(bool separate:{false,true}) {
            g_controls.separate_pickup=separate;chosen=-1;NativeSelect(actor,tables[hero],player,0);
            CHECK(chosen==(separate?0:0x82));CHECK(GuardField<int>(object,0)==source);
            CHECK(GuardField<BYTE>(actor,0x66)&ff_pickup::WorldSide(callback));GuardField<int>(actor,0xC)=0x45;
        }
        GuardField<int8_t>(actor,0x7D)=0;InputField<int>(0x870620)=0x203;
        g_controls.separate_pickup=false;chosen=-1;drops=0;NativeSelect(actor,tables[hero],player,0);
        CHECK(chosen!=0x82 && !drops && GuardField<int8_t>(actor,0x7D)==0);
        g_controls.separate_pickup=true;GuardField<int>(actor,0xC)=0x45;
        g_modern_coop_input=true;g_modern_coop_scheme=player?3:1;InputField<int>(0x8745A0)=2;
        pickup_keys[player]={};ResetItemPickup();Tick(player);CHECK(g_item_pickup_visual.slots[player]==-1);
        pickup_keys[player].pickup=true;CHECK(!Tick(player) && !drops);
        pickup_keys[player]={};Tick(player);
        pickup_keys[player].throw_item=true;CHECK(Tick(player)==1 && chosen==0x45 && drops==1);
        pickup_keys[player]={};Tick(player);CHECK(g_item_pickup_visual.slots[player]==-2);
        pickup_keys[player].pickup=true;chosen=-1;CHECK(Tick(player)==1 && chosen==0x82);
        CHECK(drops==1);
        CHECK(GuardField<int8_t>(actor,0x7E)==-1 && GuardField<int>(object,0)==source);
        GuardField<int>(actor,0x1C)=0x800;const int before=creations;
        reinterpret_cast<void(__cdecl*)(unsigned)>(g_base+0x11BB0)(player);
        CHECK(GuardField<int>(object,0)==source+1 && created==ff_pickup::WorldWeapon(source) && creations==before+1);
        CHECK(GuardField<int8_t>(actor,0x7E)==0 && !WorldPickupObject(fixture));
        reinterpret_cast<void(__cdecl*)(unsigned)>(g_base+0x11BB0)(player);CHECK(creations==before+1);
    }
    native_start=g_base+0x4FB8F;
}
int main(int argc,char** argv) {
    auto base=static_cast<BYTE*>(VirtualAlloc(nullptr,0x900000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));
    CHECK(base);if(!base)return 1;g_base=reinterpret_cast<uintptr_t>(base);Setup();
    native_start=g_base+0x4FB8F;Jump(0x4FF1F,reinterpret_cast<void*>(NativeFinish));
    const BYTE scanner[]={0x83,0xEC,0x24,0x8B,0x4C,0x24,0x28,0x53};memcpy(base+0x32430,scanner,sizeof(scanner));
    const BYTE hand[]={0x8A,0x46,0x7E,0x3C,0xFF},heavy[]={0x3D,9,2,0,0,0x7E,4,0x6A,0x79};
    memcpy(base+0x4FB67,hand,sizeof(hand));memcpy(base+0x4FBB1,heavy,sizeof(heavy));Call(0x4FB21,0x32430);
    const BYTE drop[]={0x8B,0x7C,0x24,0x2C,0x83,0xFF,0xFF};memcpy(base+0x4F638,drop,sizeof(drop));
    const BYTE jump[]={0x8A,0x82,0x51,0x18,0xC7,0};memcpy(base+0x4F830,jump,sizeof(jump));
    const BYTE kick[]={0x8B,0x44,0x24,0x24,0x8A,0x88,0x52,0x18,0xC7,0};memcpy(base+0x4F8E9,kick,sizeof(kick));
    const BYTE pull[]={0x8A,0x46,0x66,0x83,0xC4,0x08},detach[]={0x8B,0x44,0x24,0x04,0x83,0xEC,0x10};
    memcpy(base+0x4FB26,pull,sizeof(pull));memcpy(base+0x11BB0,detach,sizeof(detach));
    const BYTE punch[]={0x66,0x3D,9,2,0x8D,0x3C,0x8D,0x20,6,0xC7,0};memcpy(base+0x4FC1B,punch,sizeof(punch));
    const BYTE air_spin[]={0x0F,0x8E,0x89,6,0,0},ground_spin[]={0x0F,0x8E,0x74,0xD9,0xFF,0xFF};
    memcpy(base+0x50B0D,air_spin,sizeof(air_spin));memcpy(base+0x51F6B,ground_spin,sizeof(ground_spin));
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
    // Exercise the default controller LB through the actual throw/drop
    // handler for every native pickup type and both carrying hands.
    for(int type=0x1DD;type<0x233;++type)for(bool left:{false,true})for(bool separate:{false,true}) {
        Setup();const unsigned owner=left?1:0;
        g_controls.separate_pickup=separate;
        g_modern_coop_input=true;g_modern_coop_scheme=2;InputField<int>(0x8745A0)=2;
        GuardField<int8_t>(GuardActor(owner),left?0x7D:0x7E)=0;InputField<int>(0x870620)=type;
        Tick(0);Tick(1);
        controllers[owner].Gamepad.wButtons=ff_input::LeftShoulder;
        const int expected=left?(type==0x1F8?0xA3:0x45):(type>0x209?0xA1:0x9F);
        CHECK(Tick(owner)==1 && chosen==expected && drops==(expected==0x45?1:0));
        const bool grenade=!left && (type==0x1F0 || type==0x1F1);
        CHECK(pin_effects==int(grenade) && InputField<int16_t>(0x870636)==(grenade?270:0));
        if(grenade)CHECK(pin_owner==int(owner) && pin_type==0x241);
        CHECK(!Tick(1-owner));
    }
    Setup();GuardField<int8_t>(GuardActor(0),0x7D)=0;InputField<int>(0x870620)=0x203;
    Tick();pickup_keys[0].throw_item=true;CHECK(Tick()==1 && chosen==0x45 && drops==1);
    Setup();GuardField<int8_t>(GuardActor(0),0x7E)=0;Tick();pickup_keys[0].throw_item=true;
    GuardField<int>(GuardActor(0),0xC)=0x85;CHECK(!Tick());GuardField<int>(GuardActor(0),0xC)=0x48;CHECK(!Tick());
    pickup_keys[0].throw_item=false;Tick();focused=false;pickup_keys[0].throw_item=true;CHECK(!Tick());
    focused=true;CHECK(!Tick());pickup_keys[0].throw_item=false;Tick();
    g_controls.separate_pickup=false;pickup_keys[0].throw_item=true;CHECK(Tick()==1 && chosen==0x9F);
    Setup();auto fixture=base+0x86DD20,object=base+0x80000;InputField<int>(0x8689E8)=1;
    GuardField<int16_t>(fixture,8)=9;GuardField<void*>(fixture,0x14)=object;GuardField<int>(object,0)=0x273;
    auto actor=GuardActor(0);GuardField<void*>(actor,0x6C)=fixture;GuardField<BYTE>(actor,0x66)=1;
    CHECK(FindWorldPickup(actor)==fixture);Tick();pickup_keys[0].pickup=true;
    CHECK(Tick()==1 && chosen==0x82 && GuardField<int>(object,0)==0x273);
    GuardField<int16_t>(fixture,8)=17;CHECK(!FindWorldPickup(actor));GuardField<int16_t>(fixture,8)=9;
    GuardField<int>(object,0)=0x274;CHECK(!FindWorldPickup(actor));GuardField<int>(object,0)=0x273;
    GuardField<BYTE>(actor,0x66)=2;CHECK(!FindWorldPickup(actor));GuardField<BYTE>(actor,0x66)=1;
    GuardField<void*>(actor,0x6C)=fixture+1;CHECK(!FindWorldPickup(actor));
    for(int i=1;i<argc;++i)NativeFixture(argv[i]);
    VirtualFree(base,0,MEM_RELEASE);puts(failures?"Item pickup runtime: FAIL":"Item pickup runtime: PASS");return failures?1:0;
}
