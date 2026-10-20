#pragma once
#include "alpha_pixels.h"

template<typename T> struct LocalCom {
    T* value=nullptr;
    ~LocalCom(){if(value)value->Release();}
    T* operator->(){return value;}
    T** Put(){return &value;}
    T* Detach(){T* result=value;value=nullptr;return result;}
};
struct EdgeTexture {
    DWORD original=0;
    DWORD replacement=0;
    IDirect3DTexture2* texture=nullptr;
    DWORD cutout_replacement=0;
    IDirect3DTexture2* cutout_texture=nullptr;
};
static void ReleaseEdgeTextures(std::vector<EdgeTexture>& textures) {
    // Clear ownership before Release in case a driver reenters COM cleanup.
    std::vector<EdgeTexture> owned;
    owned.swap(textures);
    for(auto& entry:owned) {
        if(entry.texture)entry.texture->Release();
        if(entry.cutout_texture)entry.cutout_texture->Release();
    }
}
static IDirect3DTexture2* UploadEdgeTexture(IDirectDraw2* draw,IDirect3DDevice2* device,
    DWORD width,DWORD height,const std::vector<DWORD>& pixels,DWORD* handle) {
    DDSURFACEDESC desc={};desc.dwSize=sizeof(desc);
    desc.dwFlags=DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT|DDSD_PIXELFORMAT;
    desc.dwWidth=width;desc.dwHeight=height;
    desc.ddpfPixelFormat.dwSize=sizeof(DDPIXELFORMAT);
    desc.ddpfPixelFormat.dwFlags=DDPF_RGB|DDPF_ALPHAPIXELS;
    desc.ddpfPixelFormat.dwRGBBitCount=32;
    desc.ddpfPixelFormat.dwRBitMask=0x00FF0000;desc.ddpfPixelFormat.dwGBitMask=0x0000FF00;
    desc.ddpfPixelFormat.dwBBitMask=0x000000FF;desc.ddpfPixelFormat.dwRGBAlphaBitMask=0xFF000000;
    desc.ddsCaps.dwCaps=DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY;
    LocalCom<IDirectDrawSurface> upload,target;
    LocalCom<IDirect3DTexture2> source_texture,target_texture;
    if(FAILED(draw->CreateSurface(&desc,upload.Put(),nullptr)))return nullptr;
    DDSURFACEDESC locked={};locked.dwSize=sizeof(locked);
    if(FAILED(upload->Lock(nullptr,&locked,DDLOCK_WAIT|DDLOCK_WRITEONLY,nullptr)))return nullptr;
    for(DWORD y=0;y<height;++y)
        memcpy(static_cast<BYTE*>(locked.lpSurface)+static_cast<ptrdiff_t>(y)*locked.lPitch,pixels.data()+y*width,width*sizeof(DWORD));
    upload->Unlock(nullptr);
    desc.ddsCaps.dwCaps=DDSCAPS_TEXTURE|DDSCAPS_VIDEOMEMORY|DDSCAPS_ALLOCONLOAD;
    if(FAILED(draw->CreateSurface(&desc,target.Put(),nullptr)) ||
        FAILED(upload->QueryInterface(IID_IDirect3DTexture2,reinterpret_cast<void**>(source_texture.Put()))) ||
        FAILED(target->QueryInterface(IID_IDirect3DTexture2,reinterpret_cast<void**>(target_texture.Put()))) ||
        FAILED(target_texture->Load(source_texture.value)) ||
        FAILED(target_texture->GetHandle(device,handle)))return nullptr;
    return target_texture.Detach();
}
static EdgeTexture BuildEdgeTexture(void* device,DWORD handle,IDirectDrawSurface* original) {
    EdgeTexture result;result.original=handle;
    if(!original)return result;
    DDSURFACEDESC info={};info.dwSize=sizeof(info);
    DDCOLORKEY key={};
    if(FAILED(original->GetSurfaceDesc(&info)) || FAILED(original->GetColorKey(DDCKEY_SRCBLT,&key)) ||
        info.ddpfPixelFormat.dwRGBBitCount!=16 || info.ddpfPixelFormat.dwRBitMask!=0xF800 ||
        info.ddpfPixelFormat.dwGBitMask!=0x07E0 || info.ddpfPixelFormat.dwBBitMask!=0x001F ||
        !info.dwWidth || !info.dwHeight || info.dwWidth>1024 || info.dwHeight>1024)return result;
    std::vector<DWORD> pixels(info.dwWidth*info.dwHeight);
    DDSURFACEDESC locked={};locked.dwSize=sizeof(locked);
    if(FAILED(original->Lock(nullptr,&locked,DDLOCK_WAIT|DDLOCK_READONLY,nullptr)))return result;
    for(DWORD y=0;y<info.dwHeight;++y) {
        auto row=reinterpret_cast<WORD*>(static_cast<BYTE*>(locked.lpSurface)+static_cast<ptrdiff_t>(y)*locked.lPitch);
        for(DWORD x=0;x<info.dwWidth;++x)
            pixels[y*info.dwWidth+x]=Keyed565ToArgb(row[x],static_cast<WORD>(key.dwColorSpaceLowValue),static_cast<WORD>(key.dwColorSpaceHighValue));
    }
    original->Unlock(nullptr);
    LocalCom<IDirectDrawSurface2> source2;
    LocalCom<IUnknown> parent;
    LocalCom<IDirectDraw2> draw;
    if(FAILED(original->QueryInterface(IID_IDirectDrawSurface2,reinterpret_cast<void**>(source2.Put()))) ||
        FAILED(source2->GetDDInterface(reinterpret_cast<void**>(parent.Put()))) ||
        FAILED(parent->QueryInterface(IID_IDirectDraw2,reinterpret_cast<void**>(draw.Put()))))return result;
    result.texture=UploadEdgeTexture(draw.value,static_cast<IDirect3DDevice2*>(device),info.dwWidth,info.dwHeight,pixels,&result.replacement);
    auto cutout=DilateKeyedRgb(pixels,info.dwWidth,info.dwHeight);
    result.cutout_texture=UploadEdgeTexture(draw.value,static_cast<IDirect3DDevice2*>(device),info.dwWidth,info.dwHeight,cutout,&result.cutout_replacement);
    char message[160];
    sprintf_s(message,"Transparent texture %lu -> blend %lu / cutout %lu (%lux%lu): separate filtered coverage and native opacity.\r\n",handle,result.replacement,result.cutout_replacement,info.dwWidth,info.dwHeight);
    Log(message);
    return result;
}
