#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddraw.h>
#include <stdio.h>
static HRESULT WINAPI PrintMode(DDSURFACEDESC* mode,void*) {
    if(mode->ddpfPixelFormat.dwRGBBitCount==16)printf("%lux%lu %lu bpp\n",mode->dwWidth,mode->dwHeight,mode->ddpfPixelFormat.dwRGBBitCount);
    return DDENUMRET_OK;
}
int wmain(int argc,wchar_t** argv) {
    if(argc!=2)return 2;
    HMODULE module=LoadLibraryW(argv[1]);if(!module)return 3;
    using Create=HRESULT(WINAPI*)(GUID*,IDirectDraw**,IUnknown*);
    auto create=reinterpret_cast<Create>(GetProcAddress(module,"DirectDrawCreate"));
    IDirectDraw* draw=nullptr;HRESULT result=create?create(nullptr,&draw,nullptr):E_FAIL;
    printf("DirectDrawCreate: %08lx\n",result);
    if(SUCCEEDED(result)){result=draw->EnumDisplayModes(0,nullptr,nullptr,PrintMode);draw->Release();}
    printf("EnumDisplayModes: %08lx\n",result);FreeLibrary(module);return FAILED(result)?1:0;
}
