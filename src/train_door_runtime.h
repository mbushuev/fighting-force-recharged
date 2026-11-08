#pragma once

struct TrainDoorCollision {
    int16_t min_x,min_z,max_x,max_z,tag,shape,flags;
};
static_assert(sizeof(TrainDoorCollision)==14,"Train collision fields");

struct TrainDoor {
    int type,z,duplicate_type,duplicate_z;
    DWORD travel;
    int16_t leaf_min_z,leaf_max_z,rear_min_z,rear_max_z;
    int16_t near_min_z,near_max_z;
};
static constexpr TrainDoor TrainDoors[]={
    {307,-4752,313,-3040,0x863E80,-4800,-4744,-4944,-4880,-4863,-3968},
    {311,-1328,309,384,0x8788A4,-1336,-1296,-1552,-1472,-1439,-512},
    {307,2096,313,3808,0x863E80,2080,2128,1864,1952,1985,2944},
};
static constexpr int TrainDoorOpen=64;
// A negative travel leaves the initial approach to the native animation.
// Each cleared join then follows its own traffic.
static int g_train_door_travel[]={-1,-1,-1};
static bool g_train_door_near[3],g_train_native_door_sound;

static bool TrainDoorNativeNear(const TrainDoor& door) {
    for(unsigned i=0;i<8;++i) {
        const DWORD offset=i*0xF4;
        if(CameraField<int16_t>(0x86A542+offset)<=0)continue;
        const int z=CameraField<int32_t>(0x86A530+offset)>>16;
        if(z>door.near_min_z && z<door.near_max_z)return true;
    }
    return false;
}

static void __cdecl CaptureTrainDoorSound(int,int,int) {g_train_native_door_sound=true;}

static bool TrainDoorwayClear(int z) {
    const int32_t clear=(z+33)*65536; // Native rectangle collision expands by 32.
    const unsigned players=CameraField<int>(0x8745A0)==2?2:1;
    bool player=false;
    for(unsigned i=0;i<8;++i) {
        const DWORD offset=i*0xF4;
        // Unoccupied slots can still have their default health.
        if(CameraField<int16_t>(0x86A534+offset)<0)continue;
        const bool alive=CameraField<int16_t>(0x86A542+offset)>0;
        // A downed player with lives left can still respawn in the old car.
        if(!alive && (i>=players || CameraField<int>(0x878890+i*4)<=0))continue;
        if(CameraField<int32_t>(0x86A530+offset)<clear)return false;
        if(i<players && alive)player=true;
    }
    return player;
}

static void __cdecl GameTrainDoors() {
    if(CameraField<int>(0x85A6C0)==0)
        for(int& travel:g_train_door_travel)travel=-1;
    g_train_native_door_sound=false;
    reinterpret_cast<void(__cdecl*)()>(g_base+0x3B260)();
    const int section=CameraField<int>(0x85A6C0);
    bool sound=false;
    for(unsigned i=0;i<3;++i) {
        const auto& door=TrainDoors[i];
        const int native=CameraField<int>(door.travel);
        const bool in_range=TrainDoorNativeNear(door),was_near=g_train_door_near[i];
        g_train_door_near[i]=in_range;
        const auto leaf=CameraField<TrainDoorCollision*>(0x86F9A0+i*4);
        const auto rear=CameraField<TrainDoorCollision*>(0x86F988+i*4);
        if(!CameraReadable(leaf,sizeof(*leaf)) || !CameraReadable(rear,sizeof(*rear)) ||
           leaf->shape!=0 || rear->shape!=0 ||
           rear->min_z!=door.rear_min_z || rear->max_z!=door.rear_max_z ||
           !((leaf->min_z==door.leaf_min_z && leaf->max_z==door.leaf_max_z) ||
             (leaf->min_z==door.z && leaf->max_z==door.z))) {
            sound|=g_train_native_door_sound;
            continue;
        }
        // The native section change activates the rear rectangle while a
        // walker is still inside it. Keep that progression trigger, but let
        // the door itself be the sole collision plane in both directions.
        rear->flags=0;
        leaf->min_z=leaf->max_z=static_cast<int16_t>(door.z);
        int& travel=g_train_door_travel[i];
        const bool passed=section>static_cast<int>(i);
        const bool clear=passed && TrainDoorwayClear(door.z);
        if(!passed || (travel<0 && !clear)) {
            // Doors sharing native materials can be in different states.
            // Keep the native sound only for the door that triggered it.
            if(g_train_native_door_sound && ((in_range && native==2) ||
                (was_near && !in_range && native==TrainDoorOpen-2)))sound=true;
            continue;
        }
        const int before=travel<0?native:travel;
        // Later waves can run in from the previous car. Start opening as
        // soon as they spawn so the scripted runners can reach an open leaf.
        travel=clear?(std::max)(0,before-2):(std::min)(TrainDoorOpen,before+8);
        leaf->flags=travel==TrainDoorOpen?0:3;
        if((before==0 && travel>0) || (before==TrainDoorOpen && travel<TrainDoorOpen))sound=true;
    }
    if(sound)reinterpret_cast<void(__cdecl*)(int,int,int)>(g_base+0x42600)(0x7D,0xFF,0x1700);
}

static void InstallTrainDoorHook() {
    auto sound=reinterpret_cast<BYTE*>(g_base+0x3B42A);
    auto tick=reinterpret_cast<BYTE*>(g_base+0x3BFF6);
    const BYTE sound_call[]={0xE8,0xD1,0x71,0x00,0x00},tick_call[]={0xE8,0x65,0xF2,0xFF,0xFF};
    if(memcmp(sound,sound_call,sizeof(sound_call)) || memcmp(tick,tick_call,sizeof(tick_call)))return;
    const SIZE_T length=tick+5-sound;
    DWORD old;
    if(!VirtualProtect(sound,length,PAGE_EXECUTE_READWRITE,&old))return;
    const auto redirect=[](BYTE* code,uintptr_t target) {
        const LONG relative=static_cast<LONG>(target-reinterpret_cast<uintptr_t>(code+5));
        memcpy(code+1,&relative,4);
    };
    redirect(sound,reinterpret_cast<uintptr_t>(CaptureTrainDoorSound));
    redirect(tick,reinterpret_cast<uintptr_t>(GameTrainDoors));
    DWORD ignored;
    VirtualProtect(sound,length,old,&ignored);
    FlushInstructionCache(GetCurrentProcess(),sound,length);
}

class TrainDoorMaterialScope {
    float saved[6][2];
    int base;
    bool changed;
    float* V(int i) const {
        return reinterpret_cast<float*>(g_base+0x451DB0+(base+i%3*2+i/3*16)*32);
    }
public:
    TrainDoorMaterialScope(int material,int travel,int native):base(material),changed(travel!=native) {
        if(!changed)return;
        const int scale=CameraField<int>(0x20C4F8)?2:1;
        const float delta=(travel/scale-native/scale)/256.0f;
        for(int i=0;i<6;++i) {
            float* v=V(i);
            saved[i][0]=v[0];saved[i][1]=v[1];
            v[0]+=delta;v[1]+=delta;
        }
    }
    ~TrainDoorMaterialScope() {
        if(!changed)return;
        for(int i=0;i<6;++i) {
            float* v=V(i);
            v[0]=saved[i][0];v[1]=saved[i][1];
        }
    }
    TrainDoorMaterialScope(const TrainDoorMaterialScope&)=delete;
    TrainDoorMaterialScope& operator=(const TrainDoorMaterialScope&)=delete;
};
