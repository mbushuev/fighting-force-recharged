#pragma once
#include <d3d.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <memory>
#include <vector>

struct LoadingImage {
    DWORD width=0,height=0;
    std::vector<DWORD> pixels;
};
static bool g_loading_capture_requested;
static std::shared_ptr<const LoadingImage> g_loading_capture,g_transition_loading_image;
static DWORD LoadingChannel(DWORD pixel,DWORD mask) {
    if(!mask)return 0;
    unsigned shift=0;while(!(mask&1)){mask>>=1;++shift;}
    return static_cast<DWORD>((uint64_t((pixel>>shift)&mask)*255+mask/2)/mask);
}
static std::shared_ptr<const LoadingImage> CaptureLoadingImage(IDirectDrawSurface* surface) {
    DDSURFACEDESC desc={};desc.dwSize=sizeof(desc);
    if(!surface || FAILED(surface->Lock(nullptr,&desc,DDLOCK_WAIT|DDLOCK_READONLY,nullptr)))return {};
    struct Unlock {IDirectDrawSurface* surface;~Unlock(){surface->Unlock(nullptr);}} unlock={surface};
    const auto& format=desc.ddpfPixelFormat;
    DWORD bytes=format.dwRGBBitCount/8;
    if(!desc.lpSurface || !desc.dwWidth || !desc.dwHeight || desc.dwWidth>8192 || desc.dwHeight>4096 ||
        !(format.dwFlags&DDPF_RGB) || (bytes!=2 && bytes!=3 && bytes!=4) ||
        !format.dwRBitMask || !format.dwGBitMask || !format.dwBBitMask)return {};
    auto image=std::make_shared<LoadingImage>();image->width=desc.dwWidth;image->height=desc.dwHeight;
    image->pixels.resize(size_t(image->width)*image->height);
    for(DWORD y=0;y<image->height;++y) {
        auto row=static_cast<const BYTE*>(desc.lpSurface)+ptrdiff_t(y)*desc.lPitch;
        for(DWORD x=0;x<image->width;++x) {
            DWORD pixel=0;memcpy(&pixel,row+size_t(x)*bytes,bytes);
            image->pixels[size_t(y)*image->width+x]=
                (LoadingChannel(pixel,format.dwRBitMask)<<16)|(LoadingChannel(pixel,format.dwGBitMask)<<8)|
                LoadingChannel(pixel,format.dwBBitMask);
        }
    }
    return image;
}
static void PaintLoadingImage(HDC dc,const RECT& area,const LoadingImage* image) {
    FillRect(dc,&area,static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
    if(!image || !image->width || !image->height || image->pixels.empty())return;
    LONG width=area.right-area.left,height=area.bottom-area.top;
    if(width<=0 || height<=0)return;
    if(int64_t(width)*image->height>int64_t(height)*image->width)
        width=static_cast<LONG>(int64_t(height)*image->width/image->height);
    else height=static_cast<LONG>(int64_t(width)*image->height/image->width);
    BITMAPINFO info={};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth=image->width;info.bmiHeader.biHeight=-static_cast<LONG>(image->height);
    info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
    SetStretchBltMode(dc,HALFTONE);SetBrushOrgEx(dc,0,0,nullptr);
    StretchDIBits(dc,area.left+(area.right-area.left-width)/2,area.top+(area.bottom-area.top-height)/2,
        width,height,0,0,image->width,image->height,image->pixels.data(),&info,DIB_RGB_COLORS,SRCCOPY);
}
