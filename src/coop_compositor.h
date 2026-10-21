#pragma once
#include <ddraw.h>
#include <d3d.h>
static thread_local int g_coop_render_pass=-1;
static thread_local double g_coop_render_blend;
static IDirectDrawSurface* g_coop_composite;
static unsigned g_coop_surface_width,g_coop_surface_height;

static bool g_coop_composition_failed;
static void CooperativeReleaseSurface() {
    if(g_coop_composite)g_coop_composite->Release();
    g_coop_composite=nullptr;g_coop_surface_width=g_coop_surface_height=0;
}
static bool CooperativeSurface(IDirectDrawSurface* target) {
    DDSURFACEDESC info={};info.dwSize=sizeof(info);
    if(FAILED(target->GetSurfaceDesc(&info)))return false;
    if(g_coop_composite && (g_coop_surface_width!=info.dwWidth || g_coop_surface_height!=info.dwHeight))CooperativeReleaseSurface();
    if(g_coop_composite && FAILED(g_coop_composite->IsLost()))g_coop_composite->Restore();
    if(g_coop_composite)return true;
    IDirectDrawSurface2* surface=nullptr;IUnknown* parent=nullptr;IDirectDraw* draw=nullptr;
    if(SUCCEEDED(target->QueryInterface(IID_IDirectDrawSurface2,reinterpret_cast<void**>(&surface)))) {
        surface->GetDDInterface(reinterpret_cast<void**>(&parent));surface->Release();
    }
    if(parent) {parent->QueryInterface(IID_IDirectDraw,reinterpret_cast<void**>(&draw));parent->Release();}
    if(!draw)return false;
    DDSURFACEDESC desc={};desc.dwSize=sizeof(desc);
    desc.dwFlags=DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT|DDSD_PIXELFORMAT;
    desc.dwWidth=info.dwWidth;desc.dwHeight=info.dwHeight;desc.ddpfPixelFormat=info.ddpfPixelFormat;
    desc.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN|DDSCAPS_VIDEOMEMORY;
    HRESULT result=draw->CreateSurface(&desc,&g_coop_composite,nullptr);
    if(FAILED(result)) {desc.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN|DDSCAPS_SYSTEMMEMORY;result=draw->CreateSurface(&desc,&g_coop_composite,nullptr);}
    draw->Release();
    if(FAILED(result))return false;
    g_coop_surface_width=info.dwWidth;g_coop_surface_height=info.dwHeight;return true;
}
static bool CooperativeFrameFinished(void* object) {
    if(g_coop_render_pass<0)return true;
    IDirectDrawSurface* target=nullptr;
    if(FAILED(static_cast<IDirect3DDevice2*>(object)->GetRenderTarget(&target)) || !target)return g_coop_render_pass!=0;
    bool okay=CooperativeSurface(target);
    if(okay) {
        const LONG width=static_cast<LONG>(g_coop_surface_width),height=static_cast<LONG>(g_coop_surface_height);
        RECT source={0,0,width,height},destination={g_coop_render_pass?width/2:0,0,g_coop_render_pass?width:width/2,height};
        okay=SUCCEEDED(g_coop_composite->Blt(&destination,target,&source,DDBLT_WAIT,nullptr));
        if(okay && g_coop_render_pass==1) {
            if(g_coop_render_blend>0.01) {
                RECT line={width/2-1,0,width/2+1,height};DDBLTFX fill={};fill.dwSize=sizeof(fill);
                g_coop_composite->Blt(&line,nullptr,nullptr,DDBLT_COLORFILL|DDBLT_WAIT,&fill);
            }
            okay=SUCCEEDED(target->Blt(nullptr,g_coop_composite,nullptr,DDBLT_WAIT,nullptr));
        }
    }
    target->Release();
    if(!okay) {g_coop_composition_failed=true;Log("Cooperative composition failed; shared cooperative view retained.\r\n");}
    return g_coop_render_pass==1;
}
