#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../src/first_person.h"
#include "../src/camera_collision.h"
static uintptr_t g_base;
static wchar_t g_config_path[]=L"first-person-runtime-test-missing.ini";
static bool g_free_orbit=true,g_camera_active=true,ownership=true;
static ff_camera::FirstPersonView g_first_person;
static ff_camera::Orbit g_camera_orbit;
static bool CombatFocusActive(){return false;}
static double CombatFocusBodyYaw(){return 0;}
static SRWLOCK g_camera_geometry_lock=SRWLOCK_INIT;
static int g_camera_geometry_stage=0;
static std::vector<ff_camera::Triangle> g_camera_triangles;
template<class T> static T& CameraField(DWORD rva) {return *reinterpret_cast<T*>(g_base+rva);}
static bool CameraMayOwnPose() {return ownership;}
static bool CameraReadable(const void* p,SIZE_T n) {
    MEMORY_BASIC_INFORMATION page={};
    return p && VirtualQuery(p,&page,sizeof(page)) && page.State==MEM_COMMIT &&
        !(page.Protect&(PAGE_NOACCESS|PAGE_GUARD)) &&
        uintptr_t(p)+n>=uintptr_t(p) && uintptr_t(p)+n<=uintptr_t(page.BaseAddress)+page.RegionSize;
}
static void Log(const char*) {}
#include "../src/first_person_runtime.h"
static int failures,draws,native_origins,native_origin_result=7;
#define CHECK(x) do {if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);++failures;}}while(0)
static float matrices[40][12];static unsigned depth;
static void __cdecl Identity() {
    memset(matrices[depth],0,48);matrices[depth][0]=matrices[depth][5]=matrices[depth][10]=1;
}
static void __cdecl Push() {memcpy(matrices[depth+1],matrices[depth],48);++depth;}
static void __cdecl Pop() {CHECK(depth>0);--depth;}
static const float* __cdecl Matrix() {return matrices[depth];}
static void __cdecl Translate(int32_t x,int32_t y,int32_t z) {
    auto m=matrices[depth];
    for(int row=0;row<3;++row)m[row*4+3]+=float((m[row*4]*double(x)+m[row*4+1]*double(y)+m[row*4+2]*double(z))/65536);
}
static void Rotate(int a,int b,int angle) {
    auto m=matrices[depth];const double r=ff_camera::Radians(static_cast<int16_t>(angle)),c=cos(r),s=sin(r);
    for(int row=0;row<3;++row) {
        const float x=m[row*4+a],y=m[row*4+b];
        m[row*4+a]=float(x*c+y*s);m[row*4+b]=float(y*c-x*s);
    }
}
static void __cdecl Rz(int a) {Rotate(0,1,a);}
static void __cdecl Ry(int a) {Rotate(2,0,a);}
static void __cdecl Rx(int a) {Rotate(1,2,a);}
static int __cdecl Origin() {++native_origins;return native_origin_result;}
static const void* action_table;
static void* action_actor;
static int requested_action;
static int __cdecl StartAction(void* actor,const void* animations,int action) {
    action_actor=actor;action_table=animations;requested_action=action;return 81;
}
static const FirstPersonModel* passed;
static int16_t received_faces[12];
static int __cdecl Draw(const FirstPersonModel* m,void*,int,int,int,int) {
    ++draws;passed=m;memcpy(received_faces,m->faces,24);
    CHECK(reinterpret_cast<const unsigned*>(m+1)[1]==4);return 42;
}
static void Branch(DWORD rva,BYTE op,uintptr_t target) {
    auto code=reinterpret_cast<BYTE*>(g_base+rva);code[0]=op;
    const LONG displacement=static_cast<LONG>(target-(g_base+rva+5));memcpy(code+1,&displacement,4);
}
int main() {
    g_base=uintptr_t(VirtualAlloc(nullptr,0x900000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));
    if(!g_base)return 1;
    Branch(0x1000,0xE9,uintptr_t(Matrix));Branch(0x10F0,0xE9,uintptr_t(Identity));
    Branch(0x1170,0xE9,uintptr_t(Push));Branch(0x11F0,0xE9,uintptr_t(Pop));
    Branch(0x12F0,0xE9,uintptr_t(Rz));Branch(0x13F0,0xE9,uintptr_t(Ry));
    Branch(0x14F0,0xE9,uintptr_t(Rx));Branch(0x15F0,0xE9,uintptr_t(Translate));
    Branch(0x16A0,0xE9,uintptr_t(Origin));Branch(0x20E0,0xE9,uintptr_t(Origin));
    Branch(0x27180,0xE9,uintptr_t(Draw));
    Branch(0x440BC,0xE8,g_base+0x27180);Branch(0x44056,0xE8,g_base+0x20E0);
    Branch(0x43B8F,0xE8,g_base+0x20E0);CameraField<BYTE>(0x43B94)=0xC3;
    Branch(0x27D22,0xE8,g_base+0x16A0);
    Branch(0x4426F,0xE8,g_base+0x26DE0);Branch(0x44488,0xE8,g_base+0x26DE0);
    Branch(0x4F4E3,0xE8,g_base+0x4E320);Branch(0x4E320,0xE9,uintptr_t(StartAction));
    CameraField<BYTE>(0x27D22)=0x90;CHECK(!InstallFirstPersonHooks());
    LONG offset;memcpy(&offset,reinterpret_cast<void*>(g_base+0x440BD),4);
    CHECK(g_base+0x440C1+offset==g_base+0x27180);
    CameraField<BYTE>(0x27D22)=0xE8;
    CHECK(InstallFirstPersonHooks());g_first_person_available=true;
    const int32_t commands[]={1,3,0,0,0,0,0,0,0,0,0,0,0,1,3,1,0,-160*65536,0,0,0,0,1,0,0,0,2,2};
    // The command terminator lives in the zero-initialized gap before vertices.
    struct ModelData {
        FirstPersonModel model;unsigned counts[2];int32_t commands[29];int16_t vertices[12*4];int16_t faces[12];
    } model={};
    memcpy(model.commands,commands,sizeof(commands));model.counts[0]=8;model.counts[1]=4;
    const int16_t faces[]={0,1,2,3,0,5,8,9,10,11,0,6};memcpy(model.faces,faces,sizeof(faces));
    model.model={model.vertices,12,model.faces,2,nullptr,2,model.commands,2};
    for(int i=0;i<4;++i)CameraField<const FirstPersonModel*>(0x85A6F8+i*4)=&model.model;
    Identity();matrices[0][3]=123;g_first_person.selected=true;g_first_person.Begin(0);
    CameraField<int8_t>(0x86A57D)=CameraField<int8_t>(0x86A57E)=-1;
    CameraField<int32_t>(0x86A528)=ff_camera::Fixed(100);CameraField<int32_t>(0x86A530)=ff_camera::Fixed(200);
    for(int character=0;character<4;++character) {
        CameraField<int16_t>(0x86A534)=int16_t(character);
        ff_camera::Point head;CHECK(FirstPersonHead(head));
        CHECK(fabs(head.x-100)<0.001 && fabs(head.y+160)<0.001 && fabs(head.z-200)<0.001);
        CHECK(depth==0 && matrices[0][3]==123);
    }
    ff_camera::Pose ground,jump;CHECK(FirstPersonPose(ground));
    CameraField<int32_t>(0x86A52C)=ff_camera::Fixed(-100);CHECK(FirstPersonPose(jump));
    CHECK(jump.y-ground.y==ff_camera::Fixed(-100));
    CameraField<int16_t>(0x868A40+0x118)=512;ff_camera::Point combat;
    CHECK(FirstPersonHead(combat) && fabs(combat.x-100)>20); // Animated root rotation moves the eyes.
    CHECK(depth==0 && matrices[0][3]==123);
    // A slide can lower the animated skull to foot level. Keep the eyes above
    // it without fixing normal head height or removing jump translation.
    CameraField<int16_t>(0x868A40+0x118)=0;
    for(int foot:{-100,0,100})for(int root=130;root<=220;root+=5) {
        CameraField<int32_t>(0x86A52C)=ff_camera::Fixed(foot);
        CameraField<int32_t>(0x86A510)=ff_camera::Fixed(root);
        ff_camera::Pose low;CHECK(FirstPersonPose(low));
        CHECK(low.y==ff_camera::Fixed(fmin(foot+root-166.0,foot-32.0)));
        CHECK(depth==0 && matrices[0][3]==123);
    }
    const ff_camera::Triangle wall={{-200,-300,0},{200,-300,0},{0,200,0}};
    g_camera_triangles={wall};
    const auto against_wall=FirstPersonConstrainEye({0,-60,-1},{0,-60,12},{0,0,1});
    CHECK(fabs(against_wall.z+24)<0.01);
    const auto on_wall=FirstPersonConstrainEye({0,-60,0},{0,-60,12},{0,0,1});
    CHECK(fabs(on_wall.z+24)<0.01);
    g_camera_triangles.clear();
    CameraField<int32_t>(0x86A52C)=CameraField<int32_t>(0x86A510)=0;
    CHECK(FirstPersonSkeleton(&model.model,reinterpret_cast<void*>(g_base+0x8623E0),0,1,2,3)==42);
    CHECK(passed!=&model.model && received_faces[5]==5 && received_faces[11]==-1);
    CHECK(!memcmp(model.faces,faces,sizeof(faces)));
    FirstPersonSkeleton(&model.model,reinterpret_cast<void*>(g_base+0x8623E0+0x238),0,1,2,3);
    CHECK(passed==&model.model && received_faces[11]==6);
    CHECK(FirstPersonRenderOrigin()==1 && FirstPersonPoseOrigin()==1 && native_origins==0);
    ownership=false;CHECK(FirstPersonRenderOrigin()==7 && FirstPersonPoseOrigin()==7);
    FirstPersonSkeleton(&model.model,reinterpret_cast<void*>(g_base+0x8623E0),0,1,2,3);
    CHECK(passed==&model.model && native_origins==2);
    ownership=true;g_first_person.yaw=0;CameraField<int>(0x86A50C)=0x45;
    FirstPersonAlignBody();CHECK(CameraField<int16_t>(0x86A538)==1024);
    CameraField<int>(0x86A50C)=0x62;CameraField<int16_t>(0x86A538)=123;
    FirstPersonAlignBody();CHECK(CameraField<int16_t>(0x86A538)==123);
    // A forward native root step must become a backward/sideways proposal
    // without rotating the body away from the view or changing step length.
    CameraField<int>(0x86A50C)=0x52;
    CameraField<int8_t>(0x86A57D)=CameraField<int8_t>(0x86A57E)=-1;
    struct Move {uint32_t input;double x,z;};
    const Move moves[]={{ff_input::Up,0,10},{ff_input::Down,0,-10},
        {ff_input::Left,-10,0},{ff_input::Right,10,0},
        {ff_input::Up|ff_input::Right,sqrt(50.0),sqrt(50.0)},
        {ff_input::Down|ff_input::Left,-sqrt(50.0),-sqrt(50.0)}};
    auto actor=reinterpret_cast<void*>(g_base+0x86A500);
    for(const auto& move:moves) {
        CameraField<uint32_t>(0x5523E0)=move.input;
        int32_t x=ff_camera::Fixed(100),z=ff_camera::Fixed(210);
        FirstPersonMovement(actor,&x,&z);
        CHECK(fabs(x/65536.0-100-move.x)<0.00002 && fabs(z/65536.0-200-move.z)<0.00002);
        CHECK(fabs(hypot(x/65536.0-100,z/65536.0-200)-10)<0.00002);
    }
    CameraField<uint32_t>(0x5523E0)=ff_input::Down;
    for(int gait=0x52;gait<=0x61;++gait) {
        CameraField<int>(0x86A50C)=gait;
        int32_t gx=ff_camera::Fixed(100),gz=ff_camera::Fixed(210);
        FirstPersonMovement(actor,&gx,&gz);
        CHECK(gx==ff_camera::Fixed(100) && gz==ff_camera::Fixed(gait==0x5B?210:190));
    }
    CameraField<int>(0x86A50C)=0x52;
    g_first_person.Begin(0); // View now looks toward -Z; back must become +Z.
    int32_t x=ff_camera::Fixed(110),z=ff_camera::Fixed(200);
    FirstPersonMovement(actor,&x,&z);CHECK(x==ff_camera::Fixed(100) && z==ff_camera::Fixed(210));
    g_first_person.yaw=ff_camera::kPi/2;x=ff_camera::Fixed(110);z=ff_camera::Fixed(200);
    FirstPersonMovement(actor,&x,&z);CHECK(x==ff_camera::Fixed(110) && z==ff_camera::Fixed(200));
    // Attacks, airborne/carrying states, other actors and orbit retain native proposals.
    const int actions[]={0x62,0x6A,0x80};
    for(int action:actions) {
        CameraField<int>(0x86A50C)=action;x=1;z=2;FirstPersonMovement(actor,&x,&z);CHECK(x==1 && z==2);
    }
    CameraField<int>(0x86A50C)=0x52;CameraField<int8_t>(0x86A57D)=0;
    x=1;z=2;FirstPersonMovement(actor,&x,&z);CHECK(x==1 && z==2);
    CameraField<int8_t>(0x86A57D)=-1;CameraField<int16_t>(0x86A550)=1;
    FirstPersonMovement(actor,&x,&z);CHECK(x==1 && z==2);CameraField<int16_t>(0x86A550)=0;
    FirstPersonMovement(reinterpret_cast<BYTE*>(actor)+0xF4,&x,&z);CHECK(x==1 && z==2);
    // Native firearm stances must retain all four view-relative diagonals in
    // either hand. Other props/throwables and invalid slots remain native.
    const int firearms[]={0x1F3,0x1F4,0x1F5,0x1F6,0x1F7,0x1F9,0x1FA,0x1FB};
    const int stances[]={0x45,0x46,0x47,0x48,0x49,0x4C,0x4E,0xC6};
    const uint32_t diagonals[]={ff_input::Up|ff_input::Right,ff_input::Up|ff_input::Left,
        ff_input::Down|ff_input::Right,ff_input::Down|ff_input::Left};
    g_first_person.yaw=0;
    for(DWORD hand=0x86A57D;hand<=0x86A57E;++hand)for(int gun:firearms) {
        CameraField<int8_t>(hand)=5;CameraField<int>(0x870620+5*36)=gun;
        CHECK(FirstPersonGroundLoad());
        for(int stance:stances) {
            CameraField<int>(0x86A50C)=stance;CameraField<int16_t>(0x86A538)=123;
            FirstPersonAlignBody();CHECK(CameraField<int16_t>(0x86A538)==1024);
        }
        CameraField<int>(0x86A50C)=0x52;
        for(auto direction:diagonals) {
            CameraField<uint32_t>(0x5523E0)=direction;x=ff_camera::Fixed(100);z=ff_camera::Fixed(210);
            FirstPersonMovement(actor,&x,&z);
            CHECK((x>ff_camera::Fixed(100))==bool(direction&ff_input::Right));
            CHECK((z>ff_camera::Fixed(200))==bool(direction&ff_input::Up));
            CHECK(fabs(hypot(x/65536.0-100,z/65536.0-200)-10)<0.00002);
            CHECK(fabs(fabs(x/65536.0-100)-fabs(z/65536.0-200))<0.00002);
        }
        // Native fire/throw/recoil actions must not inherit movement or heading edits.
        const int protected_weapon_actions[]={0x9F,0xA1,0xA3,0xC3};
        for(int firing:protected_weapon_actions) {
            CameraField<int>(0x86A50C)=firing;CameraField<int16_t>(0x86A538)=123;
            x=1;z=2;FirstPersonMovement(actor,&x,&z);FirstPersonAlignBody();
            CHECK(x==1 && z==2 && CameraField<int16_t>(0x86A538)==123);
        }
        CameraField<int8_t>(hand)=-1;
    }
    CameraField<int8_t>(0x86A57E)=5;CameraField<int>(0x86A50C)=0x52;
    const int excluded_loads[]={-1,0x1DD,0x1F2,0x1F8,0x203};
    for(int type:excluded_loads) {
        CameraField<int>(0x870620+5*36)=type;CameraField<int16_t>(0x86A538)=123;
        x=1;z=2;FirstPersonMovement(actor,&x,&z);FirstPersonAlignBody();
        CHECK(x==1 && z==2 && CameraField<int16_t>(0x86A538)==123);
    }
    CameraField<int8_t>(0x86A57E)=112;CHECK(!FirstPersonGroundLoad());
    CameraField<int8_t>(0x86A57E)=-1;
    // Convert only the idle-to-backstep request, before native animation setup.
    // Retreat can then continue through walk loops without an end-action turn.
    const auto table=reinterpret_cast<void*>(g_base+0x700000);
    for(int type:firearms) {
        CameraField<int8_t>(0x86A57E)=5;CameraField<int>(0x870620+5*36)=type;
        CHECK(FirstPersonStartBackstep(actor,table,0x5C)==81);
        CHECK(requested_action==0x52 && action_table==table && action_actor==actor);
    }
    CameraField<int8_t>(0x86A57E)=-1;
    FirstPersonStartBackstep(actor,table,0x5C);CHECK(requested_action==0x52);
    FirstPersonStartBackstep(actor,table,0x9F);CHECK(requested_action==0x9F);
    FirstPersonStartBackstep(reinterpret_cast<BYTE*>(actor)+0xF4,table,0x5C);CHECK(requested_action==0x5C);
    CameraField<int8_t>(0x86A57E)=5;CameraField<int>(0x870620+5*36)=0x210;
    FirstPersonStartBackstep(actor,table,0x5C);CHECK(requested_action==0x5C);
    CameraField<int8_t>(0x86A57E)=-1;CameraField<int16_t>(0x86A550)=1;
    FirstPersonStartBackstep(actor,table,0x5C);CHECK(requested_action==0x5C);
    CameraField<int16_t>(0x86A550)=0;g_first_person.selected=false;
    FirstPersonStartBackstep(actor,table,0x5C);CHECK(requested_action==0x5C);
    g_first_person.selected=false;FirstPersonMovement(actor,&x,&z);CHECK(x==1 && z==2);
    // A second-player eye pose reads that actor and animation palette only.
    ff_camera::FirstPersonView local_views[2];local_views[1].selected=true;local_views[1].yaw=0;
    g_camera_cooperative=true;g_camera_player_views[0]=&local_views[0];g_camera_player_views[1]=&local_views[1];
    memcpy(reinterpret_cast<void*>(g_base+0x86A5F4),actor,0xF4);
    CameraField<int32_t>(0x86A61C)=ff_camera::Fixed(500);CameraField<int32_t>(0x86A624)=ff_camera::Fixed(600);
    CHECK(!FirstPersonActive());
    {
        CameraPlayerScope scope(1);ff_camera::Point head;CHECK(FirstPersonActive() && FirstPersonHead(head));
        CHECK(fabs(head.x-500)<0.001 && fabs(head.z-600)<0.001);
        FirstPersonSkeleton(&model.model,reinterpret_cast<void*>(g_base+0x8623E0+0x238),0,1,2,3);
        CHECK(passed!=&model.model && received_faces[11]==-1);
        FirstPersonSkeleton(&model.model,reinterpret_cast<void*>(g_base+0x8623E0),0,1,2,3);
        CHECK(passed==&model.model && received_faces[11]==6); // Partner keeps their head.
    }
    CHECK(g_camera_owner==-1 && !FirstPersonActive());
    CameraField<int>(0x86A600)=0x52;CameraField<uint32_t>(0x5523E4)=ff_input::Down;
    x=ff_camera::Fixed(500);z=ff_camera::Fixed(610);
    FirstPersonMovement(reinterpret_cast<void*>(g_base+0x86A5F4),&x,&z);
    CHECK(x==ff_camera::Fixed(500) && z==ff_camera::Fixed(590));
    g_camera_cooperative=false;
    // Actor preparation and simulation-origin checks stay native. The local
    // render-origin exception still belongs only to an eligible eye view.
    native_origin_result=0;g_first_person.selected=false;CHECK(FirstPersonRenderOrigin()==0);
    g_first_person.selected=true;CHECK(FirstPersonRenderOrigin()==1);
    CHECK(reinterpret_cast<int(__cdecl*)()>(g_base+0x43B8F)()==0);
    CHECK(reinterpret_cast<int(__cdecl*)()>(g_base+0x16A0)()==0);
    VirtualFree(reinterpret_cast<void*>(g_base),0,MEM_RELEASE);
    printf("First-person native skeleton, publication and draw scope: %s\n",failures?"FAIL":"PASS");return failures?1:0;
}
