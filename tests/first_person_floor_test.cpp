#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include "../src/orbit_camera.h"
static uintptr_t g_base;
static bool active=true,expect_copy=true;
template<class T> static T& CameraField(DWORD rva) {return *reinterpret_cast<T*>(g_base+rva);}
static bool FirstPersonActive() {return active;}
static bool CameraReadable(const void* p,SIZE_T n) {return p && n<1024*1024;}
#include "../src/first_person_props.h"
#include "../src/first_person_floor.h"
static int failures,forwarded,fragments,mixed;
static bool covered;
#define CHECK(x) do {if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);++failures;}}while(0)
static bool Inside(double x,double y,double ax,double ay,double bx,double by,double cx,double cy) {
    const double a=(bx-ax)*(y-ay)-(by-ay)*(x-ax);
    const double b=(cx-bx)*(y-by)-(cy-by)*(x-bx);
    const double c=(ax-cx)*(y-cy)-(ay-cy)*(x-cx);
    return (a>=-1e-5 && b>=-1e-5 && c>=-1e-5) || (a<=1e-5 && b<=1e-5 && c<=1e-5);
}
static int __cdecl FloorFaces(const int16_t* faces,unsigned count,int flags) {
    ++forwarded;CHECK(flags==0);
    CHECK((faces!=reinterpret_cast<const int16_t*>(g_base+0x9A170))==expect_copy);
#ifdef FF_RETAIL_FLOOR
    struct Vertex {float x,y,z,unused,rhw;DWORD color,fog,clip;};
    const auto vertices=reinterpret_cast<const Vertex*>(g_base+0x9D208);
    const auto view=reinterpret_cast<const float*(__cdecl*)()>(g_base+0x1980)();
    for(unsigned i=0;i<count;++i) {
        const auto f=faces+i*6;if(f[5]<0)continue;
        unsigned behind=0,eye_behind=0;
        for(int j=0;j<4;++j) {
            behind+=bool(vertices[f[j]].clip&16);
            const double x=(f[j]%33-16)*256,z=(f[j]/33-16)*256;
            eye_behind+=(view[8]*x+view[10]*z+view[11])<=0;
        }
        // The native bit also marks the far limit; preserve that rejection.
        if(eye_behind && eye_behind<4)++mixed;
        if(behind)continue;
        const auto a=vertices[f[0]],b=vertices[f[1]],c=vertices[f[2]],d=vertices[f[3]];
        covered|=Inside(160,220,a.x,a.y,b.x,b.y,c.x,c.y) || Inside(160,220,a.x,a.y,c.x,c.y,d.x,d.y);
    }
#else
    CHECK(count==1024);
    if(expect_copy)for(unsigned i=0;i<16*32;++i)CHECK(faces[i*6+5]==-1);
#endif
    return 37;
}
static void __cdecl Fragment(FirstPersonPropPolygon* polygon,const FirstPersonPropMaterial* material,int flags) {
    ++fragments;CHECK(flags==0 && polygon->count>=3 && polygon->count<=8);
    CHECK(material==reinterpret_cast<void*>(g_base+0x451DA8) && polygon->texture==16);
    for(unsigned i=0;i<polygon->count;++i) {
        const auto& v=polygon->vertices[i];DWORD depth;memcpy(&depth,&v.z,4);
        CHECK(depth==0x3F7FFF58 && v.rhw>0 && v.rhw<=1.00001f);
        CHECK(v.x>=0 && v.x<=320 && v.y>=0 && v.y<=240 && v.color==0xFFFFFFFF);
        CHECK(v.u>=-0.00001 && v.u<=1.00001 && v.v>=-0.00001 && v.v<=1.00001);
    }
    const auto a=polygon->vertices[0];
    for(unsigned i=2;i<polygon->count;++i) {
        const auto b=polygon->vertices[i-1],c=polygon->vertices[i];
        covered|=Inside(160,220,a.x,a.y,b.x,b.y,c.x,c.y);
    }
}
static void Branch(DWORD rva,BYTE op,uintptr_t target) {
    auto code=reinterpret_cast<BYTE*>(g_base+rva);code[0]=op;
    const LONG relative=LONG(target-uintptr_t(code+5));memcpy(code+1,&relative,4);
}
static void Faces() {
    auto faces=reinterpret_cast<int16_t*>(g_base+0x9A170);
    for(int row=0;row<32;++row)for(int column=0;column<32;++column) {
        auto f=faces+(row*32+column)*6;const int a=row*33+column;
        f[0]=int16_t(a+33);f[1]=int16_t(a+34);f[2]=int16_t(a+1);f[3]=int16_t(a);f[4]=1;f[5]=0;
    }
}
#ifndef FF_RETAIL_FLOOR
static float matrix[12]={1,0,0,0,0,1,0,8,0,0,1,32};
static const float* __cdecl Matrix() {return matrix;}
#endif
#ifdef FF_RETAIL_FLOOR
static void* retail_blocks[7]={};
static bool reservation_failed;
static void ReleaseRetail() {for(auto& block:retail_blocks)if(block){VirtualFree(block,0,MEM_RELEASE);block=nullptr;}}
static bool LoadRetail(const wchar_t* path) {
    // Reserve only code/data touched by the isolated ground path, leaving
    // unrelated image BSS available to the test process's randomized heaps.
    const uintptr_t regions[][2]={{0x400000,0x80000},{0x490000,0x20000},{0x4C0000,0x20000},
        {0x5E0000,0x10000},{0x600000,0x10000},{0x850000,0x10000},{0xC50000,0x30000}};
    for(unsigned i=0;i<7;++i) {
        retail_blocks[i]=VirtualAlloc(reinterpret_cast<void*>(regions[i][0]),regions[i][1],MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);
        if(uintptr_t(retail_blocks[i])!=regions[i][0]) {
            printf("Isolated retail address region %u unavailable.\n",i);
            reservation_failed=true;ReleaseRetail();return false;
        }
    }
    g_base=0x400000;
    FILE* file=nullptr;if(_wfopen_s(&file,path,L"rb") || !file)return false;
    fseek(file,0,SEEK_END);const long size=ftell(file);rewind(file);
    if(size<=0 || size>16*1024*1024) {fclose(file);return false;}
    std::vector<BYTE> bytes(size);const size_t read=fread(bytes.data(),1,bytes.size(),file);fclose(file);
    if(read!=bytes.size() || bytes.size()<sizeof(IMAGE_DOS_HEADER))return false;
    const auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(bytes.data());
    if(dos->e_magic!=IMAGE_DOS_SIGNATURE || dos->e_lfanew<0 || size_t(dos->e_lfanew)+sizeof(IMAGE_NT_HEADERS32)>bytes.size())return false;
    const auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(bytes.data()+dos->e_lfanew);
    if(nt->Signature!=IMAGE_NT_SIGNATURE || nt->OptionalHeader.ImageBase!=0x400000 || nt->OptionalHeader.SizeOfImage>0x900000)return false;
    const auto sections=IMAGE_FIRST_SECTION(nt);
    for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i) {
        const auto& s=sections[i];
        if(size_t(s.PointerToRawData)+s.SizeOfRawData>bytes.size() || size_t(s.VirtualAddress)+s.SizeOfRawData>0x900000)return false;
        memcpy(reinterpret_cast<void*>(g_base+s.VirtualAddress),bytes.data()+s.PointerToRawData,s.SizeOfRawData);
    }
    return true;
}
#endif
int wmain(int argc,wchar_t** argv) {
#ifdef FF_RETAIL_FLOOR
    if(argc!=2 || !LoadRetail(argv[1])) {puts("Cannot map the supported retail image for isolated floor execution.");return reservation_failed?3:2;}
    static float stack[64][12]={};CameraField<float*>(0x60044)=stack[0];CameraField<int>(0x861F68)=1;
#else
    (void)argc;(void)argv;
    g_base=uintptr_t(VirtualAlloc(nullptr,0x900000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));
    if(!g_base)return 2;Branch(0x1980,0xE9,uintptr_t(Matrix));
#endif
    Branch(0x25C00,0xE9,uintptr_t(FloorFaces));Branch(0x265B0,0xE9,uintptr_t(Fragment));
    CameraField<float>(0xAD214)=CameraField<float>(0xAD234)=100;
    CameraField<float>(0x20C50C)=160;CameraField<float>(0x20C508)=120;
    CameraField<float>(0x9D170)=320;CameraField<float>(0x9D174)=240;CameraField<float>(0x5E8A4)=0.0001f;
    CameraField<int>(0x9D188)=0;CameraField<int>(0x8689EC)=80;
    CameraField<int>(0x6BC60)=CameraField<int>(0x6BC70)=0;CameraField<float>(0x6BC64)=8000;
    CameraField<FirstPersonPropMaterial>(0x451DA8)={0,1,0,1,0,0,16,{0,0}};
    Faces();
#ifdef FF_RETAIL_FLOOR
    auto floor=reinterpret_cast<void(__cdecl*)(float)>(g_base+0x23EA0);
    active=false;expect_copy=false;CameraField<int32_t>(0x8677B4)=ff_camera::Fixed(-8);
    floor(2304);CHECK(mixed>0 && !covered && fragments==0);
    printf("Retail floor baseline: %d crossing cells rejected; lower view uncovered.\n",mixed);
    Branch(0x24236,0xE8,uintptr_t(FirstPersonFloorFaces));active=true;expect_copy=true;
    for(int height:{8,16,32,160})for(int yaw=0;yaw<65536;yaw+=8192) {
        mixed=fragments=0;covered=false;CameraField<unsigned>(0x1EC4D0)=0;
        CameraField<int16_t>(0x873CE0)=static_cast<int16_t>(yaw);
        CameraField<int32_t>(0x8677B4)=ff_camera::Fixed(-height);floor(2304);
        if(mixed || !fragments || !covered)printf("Ground case h=%d yaw=%d: mixed=%d fragments=%d coverage=%d\n",height,yaw,mixed,fragments,covered);
        CHECK(mixed==0 && fragments>0 && covered);
        CHECK(CameraField<float*>(0x60044)==stack[0]);
    }
    for(int i=0;i<4096;++i)CameraField<int16_t>(0x875060+i*2)=-1;
    fragments=0;covered=false;floor(2304);CHECK(fragments==0 && !covered);
#else
    auto faces=reinterpret_cast<int16_t*>(g_base+0x9A170);
    CHECK(FirstPersonFloorFaces(faces,1024,0)==37 && fragments>0 && forwarded==1 && covered);
    CHECK(faces[15*32*6+5]==0);const int emitted=fragments;
    for(int i=0;i<1024;++i)faces[i*6+5]=-1;
    FirstPersonFloorFaces(faces,1024,0);CHECK(fragments==emitted);
    active=false;expect_copy=false;FirstPersonFloorFaces(faces,1024,0);CHECK(fragments==emitted && forwarded==3);
#endif
#ifdef FF_RETAIL_FLOOR
    ReleaseRetail();
#else
    VirtualFree(reinterpret_cast<void*>(g_base),0,MEM_RELEASE);
#endif
    printf("Dedicated floor grid near clipping: %s\n",failures?"FAIL":"PASS");return failures?1:0;
}
