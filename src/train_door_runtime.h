#pragma once

struct TrainDoorCollision {
    int16_t min_x,min_z,max_x,max_z,tag,shape,flags;
};
static_assert(sizeof(TrainDoorCollision)==14,"Train collision fields");

struct TrainDoor {
    int type,z,duplicate_type,duplicate_z;
    DWORD travel;
    int16_t leaf_min_z,leaf_max_z,rear_min_z,rear_max_z;
};
static constexpr TrainDoor TrainDoors[]={
    {307,-4752,313,-3040,0x863E80,-4800,-4744,-4944,-4880},
    {311,-1328,309,384,0x8788A4,-1336,-1296,-1552,-1472},
    {307,2096,313,3808,0x863E80,2080,2128,1864,1952},
};
static constexpr int TrainDoorOpen=64;
// A negative travel leaves the approach and enemy entrances to the native
// animation. Each cleared join can then close independently of the others.
static int g_train_door_closing[]={-1,-1,-1};

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
        for(int& travel:g_train_door_closing)travel=-1;
    reinterpret_cast<void(__cdecl*)()>(g_base+0x3B260)();
    const int section=CameraField<int>(0x85A6C0);
    for(unsigned i=0;i<3;++i) {
        const auto& door=TrainDoors[i];
        const auto leaf=CameraField<TrainDoorCollision*>(0x86F9A0+i*4);
        const auto rear=CameraField<TrainDoorCollision*>(0x86F988+i*4);
        if(!CameraReadable(leaf,sizeof(*leaf)) || !CameraReadable(rear,sizeof(*rear)) ||
           leaf->shape!=0 || rear->shape!=0 ||
           rear->min_z!=door.rear_min_z || rear->max_z!=door.rear_max_z ||
           !((leaf->min_z==door.leaf_min_z && leaf->max_z==door.leaf_max_z) ||
             (leaf->min_z==door.z && leaf->max_z==door.z)))continue;
        // The native section change activates the rear rectangle while a
        // walker is still inside it. Keep that progression trigger, but let
        // the door itself be the sole collision plane in both directions.
        rear->flags=0;
        leaf->min_z=leaf->max_z=static_cast<int16_t>(door.z);
        if(section<=static_cast<int>(i))continue;
        int& travel=g_train_door_closing[i];
        if(travel<0 && TrainDoorwayClear(door.z))travel=CameraField<int>(door.travel);
        if(travel>=0) {
            travel=(std::max)(0,travel-2);
            leaf->flags=3;
        }
    }
}

static void InstallTrainDoorHook() {
    auto code=reinterpret_cast<BYTE*>(g_base+0x3BFF6);
    const BYTE call[]={0xE8,0x65,0xF2,0xFF,0xFF};
    if(memcmp(code,call,sizeof(call)))return;
    DWORD old;
    if(!VirtualProtect(code,sizeof(call),PAGE_EXECUTE_READWRITE,&old))return;
    const LONG relative=static_cast<LONG>(reinterpret_cast<uintptr_t>(GameTrainDoors)-
        reinterpret_cast<uintptr_t>(code+sizeof(call)));
    memcpy(code+1,&relative,4);
    DWORD ignored;
    VirtualProtect(code,sizeof(call),old,&ignored);
    FlushInstructionCache(GetCurrentProcess(),code,sizeof(call));
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
