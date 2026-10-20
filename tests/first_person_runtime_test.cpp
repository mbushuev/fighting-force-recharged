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
static int failures,draws,native_origins;
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
static int __cdecl Origin() {++native_origins;return 7;}
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
    Branch(0x27D22,0xE8,g_base+0x16A0);
    CameraField<BYTE>(0x27D22)=0x90;CHECK(!InstallFirstPersonHooks());
    LONG offset;memcpy(&offset,reinterpret_cast<void*>(g_base+0x440BD),4);
    CHECK(g_base+0x440C1+offset==g_base+0x27180);
    CameraField<BYTE>(0x27D22)=0xE8;CHECK(InstallFirstPersonHooks());g_first_person_available=true;
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
    CameraField<int>(0x86A50C)=0x60;CameraField<int16_t>(0x86A538)=123;
    FirstPersonAlignBody();CHECK(CameraField<int16_t>(0x86A538)==123);
    VirtualFree(reinterpret_cast<void*>(g_base),0,MEM_RELEASE);
    printf("First-person native skeleton, publication and draw scope: %s\n",failures?"FAIL":"PASS");return failures?1:0;
}
