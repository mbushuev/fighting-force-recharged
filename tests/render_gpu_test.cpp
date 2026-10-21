#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <new>
#include <vector>
static uintptr_t g_base;
static wchar_t g_config_path[MAX_PATH];
static void Log(const char* message){printf("%s",message);}
#include "../src/render_observer.h"
#include "../src/first_person_clip.h"

#define REQUIRE_OK(call) do { HRESULT result=(call); if(FAILED(result)){printf("FAIL %s: %08lx at %d\n",#call,result,__LINE__);return 1;} }while(0)
#define REQUIRE(condition) do {if(!(condition)){printf("FAIL %s at %d\n",#condition,__LINE__);return 1;}}while(0)

static void ReadPixels(IDirectDrawSurface* surface,std::vector<DWORD>& pixels) {
    DDSURFACEDESC locked={};locked.dwSize=sizeof(locked);
    HRESULT result=surface->Lock(nullptr,&locked,DDLOCK_WAIT|DDLOCK_READONLY,nullptr);
    if(FAILED(result)){printf("Readback failed %08lx\n",result);ExitProcess(2);}
    pixels.resize(64*64);
    for(int y=0;y<64;++y)memcpy(pixels.data()+64*y,static_cast<BYTE*>(locked.lpSurface)+y*locked.lPitch,64*sizeof(DWORD));
    surface->Unlock(nullptr);
}
static HRESULT Clear(IDirectDrawSurface* surface) {
    DDBLTFX effects={};effects.dwSize=sizeof(effects);effects.dwFillColor=0x00306090;
    return surface->Blt(nullptr,nullptr,nullptr,DDBLT_COLORFILL|DDBLT_WAIT,&effects);
}
int wmain(int argc,wchar_t** argv) {
    if(argc!=2)return 2;
    wchar_t directory[MAX_PATH];lstrcpyW(directory,argv[1]);
    wchar_t* separator=wcsrchr(directory,L'\\');if(!separator)return 2;*separator=0;
    SetDllDirectoryW(directory);SetCurrentDirectoryW(directory);
    HMODULE library=LoadLibraryExW(argv[1],nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);
    if(!library){printf("DDraw load failed %lu\n",GetLastError());return 2;}
    using Create=HRESULT(WINAPI*)(GUID*,IDirectDraw**,IUnknown*);
    auto create=reinterpret_cast<Create>(GetProcAddress(library,"DirectDrawCreate"));
    REQUIRE(create);
    WNDCLASSW window_class={};window_class.lpfnWndProc=DefWindowProcW;window_class.hInstance=GetModuleHandleW(nullptr);window_class.lpszClassName=L"FightingForceRenderTest";
    RegisterClassW(&window_class);
    HWND window=CreateWindowExW(0,window_class.lpszClassName,L"Offscreen rendering test",WS_POPUP,0,0,64,64,nullptr,nullptr,window_class.hInstance,nullptr);
    REQUIRE(window);
    for(int cycle=0;cycle<8;++cycle) {
        LocalCom<IDirectDraw> first;
        LocalCom<IDirectDraw2> draw;
        LocalCom<IDirect3D2> d3d;
        LocalCom<IDirectDrawSurface> target,source,depth;
        LocalCom<IDirect3DTexture2> original_texture;
        LocalCom<IDirect3DViewport2> viewport;
        REQUIRE_OK(create(nullptr,first.Put(),nullptr));
        REQUIRE_OK(first->QueryInterface(IID_IDirectDraw2,reinterpret_cast<void**>(draw.Put())));
        REQUIRE_OK(draw->SetCooperativeLevel(window,DDSCL_NORMAL));
        REQUIRE_OK(draw->QueryInterface(IID_IDirect3D2,reinterpret_cast<void**>(d3d.Put())));
        DDSURFACEDESC desc={};desc.dwSize=sizeof(desc);desc.dwFlags=DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT;
        desc.dwWidth=desc.dwHeight=64;desc.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN|DDSCAPS_3DDEVICE|DDSCAPS_VIDEOMEMORY;
        auto& format=desc.ddpfPixelFormat;format.dwSize=sizeof(format);format.dwFlags=DDPF_RGB;format.dwRGBBitCount=32;
        format.dwRBitMask=0xFF0000;format.dwGBitMask=0xFF00;format.dwBBitMask=0xFF;
        REQUIRE_OK(draw->CreateSurface(&desc,target.Put(),nullptr));
        DDSURFACEDESC depth_desc={};depth_desc.dwSize=sizeof(depth_desc);
        depth_desc.dwFlags=DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|DDSD_ZBUFFERBITDEPTH;
        depth_desc.dwWidth=depth_desc.dwHeight=64;depth_desc.dwZBufferBitDepth=16;
        depth_desc.ddsCaps.dwCaps=DDSCAPS_ZBUFFER|DDSCAPS_VIDEOMEMORY;
        REQUIRE_OK(draw->CreateSurface(&depth_desc,depth.Put(),nullptr));
        REQUIRE_OK(target->AddAttachedSurface(depth.value));
        IDirect3DDevice2* device=nullptr;
        REQUIRE_OK(d3d->CreateDevice(IID_IDirect3DHALDevice,target.value,&device));
        REQUIRE_OK(d3d->CreateViewport(viewport.Put(),nullptr));
        D3DVIEWPORT2 view={};view.dwSize=sizeof(view);view.dwWidth=view.dwHeight=64;view.dvClipX=-1;view.dvClipY=1;view.dvClipWidth=view.dvClipHeight=2;view.dvMaxZ=1;
        REQUIRE_OK(device->AddViewport(viewport.value));REQUIRE_OK(viewport->SetViewport2(&view));
        REQUIRE_OK(device->SetCurrentViewport(viewport.value));
        desc.dwWidth=desc.dwHeight=8;desc.ddsCaps.dwCaps=DDSCAPS_TEXTURE|DDSCAPS_VIDEOMEMORY;
        format.dwRGBBitCount=16;format.dwRBitMask=0xF800;format.dwGBitMask=0x07E0;format.dwBBitMask=0x1F;
        REQUIRE_OK(draw->CreateSurface(&desc,source.Put(),nullptr));
        DDCOLORKEY key={};REQUIRE_OK(source->SetColorKey(DDCKEY_SRCBLT,&key));
        DDSURFACEDESC locked={};locked.dwSize=sizeof(locked);
        REQUIRE_OK(source->Lock(nullptr,&locked,DDLOCK_WAIT|DDLOCK_WRITEONLY,nullptr));
        for(int y=0;y<8;++y){auto row=reinterpret_cast<WORD*>(static_cast<BYTE*>(locked.lpSurface)+y*locked.lPitch);for(int x=0;x<8;++x)row[x]=(x>=2&&x<6&&y>=2&&y<6)?0xFFFF:0;}
        REQUIRE_OK(source->Unlock(nullptr));
        REQUIRE_OK(source->QueryInterface(IID_IDirect3DTexture2,reinterpret_cast<void**>(original_texture.Put())));
        DWORD handle;REQUIRE_OK(original_texture->GetHandle(device,&handle));
        auto captured_565=CaptureLoadingImage(source.value);
        REQUIRE(captured_565 && captured_565->width==8 && captured_565->height==8);
        REQUIRE(captured_565->pixels[0]==0 && captured_565->pixels[3*8+3]==0xFFFFFF);
        auto record=new RenderObserver;record->original=*reinterpret_cast<void***>(device);memcpy(record->table,record->original,sizeof(record->table));
        record->table[2]=reinterpret_cast<void*>(RenderRelease);record->table[10]=reinterpret_cast<void*>(RenderBeginScene);
        record->table[11]=reinterpret_cast<void*>(RenderEndScene);record->table[23]=reinterpret_cast<void*>(RenderSetState);
        record->table[29]=reinterpret_cast<void*>(RenderDraw);record->table[30]=reinterpret_cast<void*>(RenderDrawIndexed);
        *reinterpret_cast<void***>(device)=record->table;
        auto edge=BuildEdgeTexture(device,handle,source.value);REQUIRE(edge.replacement);record->edge_textures.push_back(edge);
        const DWORD states[][2]={{1,handle},{3,D3DTADDRESS_CLAMP},{4,0},{7,0},{14,0},{15,0},{17,D3DFILTER_LINEAR},{18,D3DFILTER_LINEAR},{19,D3DBLEND_SRCALPHA},{20,D3DBLEND_INVSRCALPHA},{21,D3DTBLEND_MODULATE},{27,0},{41,1},{22,D3DCULL_NONE},{24,73},{25,D3DCMP_LESS}};
        for(const auto& state:states)REQUIRE_OK(device->SetRenderState(static_cast<D3DRENDERSTATETYPE>(state[0]),state[1]));
        D3DTLVERTEX points[4]={};
        points[0].sx=points[0].sy=0;points[1].sx=63;points[1].sy=0;points[2].sx=points[2].sy=63;points[3].sx=0;points[3].sy=63;
        points[1].tu=points[2].tu=1;points[2].tv=points[3].tv=1;
        for(auto& point:points){point.sz=0.5f;point.rhw=0.001f;point.color=0x64FFFFFF;}
        std::vector<DWORD> baseline,guarded,ui;
        g_smooth_edges=false;REQUIRE_OK(Clear(target.value));REQUIRE_OK(device->BeginScene());
        REQUIRE_OK(device->DrawPrimitive(D3DPT_TRIANGLEFAN,D3DVT_TLVERTEX,points,4,0));REQUIRE_OK(device->EndScene());ReadPixels(target.value,baseline);
        // Disabling the feature releases its cache between scenes. Rebuild here
        // before enabling it, as the production queue does before BeginScene.
        edge=BuildEdgeTexture(device,handle,source.value);REQUIRE(edge.replacement);record->edge_textures.push_back(edge);
        g_smooth_edges=true;REQUIRE_OK(Clear(target.value));REQUIRE_OK(device->BeginScene());
        REQUIRE_OK(device->DrawPrimitive(D3DPT_TRIANGLEFAN,D3DVT_TLVERTEX,points,4,0));REQUIRE_OK(device->EndScene());ReadPixels(target.value,guarded);
        REQUIRE((baseline[32*64+32]&0xFFFFFF)==0xFFFFFF);
        for(int y=24;y<40;++y)for(int x=24;x<40;++x)
            REQUIRE((guarded[y*64+x]&0xFFFFFF)==0xFFFFFF);
        REQUIRE((guarded[2*64+2]&0xFFFFFF)==0x306090);REQUIRE(record->edge_draws==1);
        for(auto& point:points){point.sz=0.0001f;point.rhw=1.0f;}
        D3DTLVERTEX unchanged[4];memcpy(unchanged,points,sizeof(points));
        REQUIRE_OK(Clear(target.value));REQUIRE_OK(device->BeginScene());
        REQUIRE_OK(device->DrawPrimitive(D3DPT_TRIANGLEFAN,D3DVT_TLVERTEX,points,4,0));REQUIRE_OK(device->EndScene());ReadPixels(target.value,ui);
        REQUIRE(record->edge_draws==2);REQUIRE(!memcmp(unchanged,points,sizeof(points)));
        REQUIRE((ui[32*64+32]&0xFFFFFF)==0xFFFFFF);REQUIRE((ui[2*64+2]&0xFFFFFF)==0x306090);
        for(const auto& state:states){DWORD actual;REQUIRE_OK(device->GetRenderState(static_cast<D3DRENDERSTATETYPE>(state[0]),&actual));REQUIRE(actual==state[1]);}
        // Fading menu passes do use vertex alpha. Preserve their opacity in
        // addition to ignoring that byte on the opaque world passes above.
        REQUIRE_OK(device->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE,TRUE));
        REQUIRE_OK(device->SetRenderState(D3DRENDERSTATE_TEXTUREMAPBLEND,D3DTBLEND_MODULATEALPHA));
        for(auto& point:points)point.color=0x80FFFFFF;
        memcpy(unchanged,points,sizeof(points));
        REQUIRE_OK(Clear(target.value));REQUIRE_OK(device->BeginScene());
        REQUIRE_OK(device->DrawPrimitive(D3DPT_TRIANGLEFAN,D3DVT_TLVERTEX,points,4,0));
        REQUIRE_OK(device->EndScene());ReadPixels(target.value,ui);
        REQUIRE(!memcmp(unchanged,points,sizeof(points)));
        DWORD center=ui[32*64+32];
        REQUIRE(((center>>16)&255)>=151 && ((center>>16)&255)<=153);
        REQUIRE(((center>>8)&255)>=175 && ((center>>8)&255)<=177);
        REQUIRE((center&255)>=199 && (center&255)<=201);
        REQUIRE((ui[2*64+2]&0xFFFFFF)==0x306090);
        REQUIRE_OK(device->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE,FALSE));
        REQUIRE_OK(device->SetRenderState(D3DRENDERSTATE_TEXTUREMAPBLEND,D3DTBLEND_MODULATE));
        // F2 enables legacy vertex fog. Both clear and fogged key cutouts
        // must leave their background visible and their opaque parts solid.
        REQUIRE_OK(device->SetRenderState(D3DRENDERSTATE_FOGENABLE,TRUE));
        REQUIRE_OK(device->SetRenderState(D3DRENDERSTATE_FOGCOLOR,0));
        REQUIRE_OK(device->SetRenderState(D3DRENDERSTATE_FOGTABLEMODE,D3DFOG_NONE));
        for(auto& point:points){point.color=0x64FFFFFF;point.specular=0x80000000;}
        REQUIRE_OK(Clear(target.value));REQUIRE_OK(device->BeginScene());
        REQUIRE_OK(device->DrawPrimitive(D3DPT_TRIANGLEFAN,D3DVT_TLVERTEX,points,4,0));
        REQUIRE_OK(device->EndScene());ReadPixels(target.value,ui);
        REQUIRE((ui[2*64+2]&0xFFFFFF)==0x306090);
        center=ui[32*64+32];
        REQUIRE(((center>>16)&255)>=127 && ((center>>16)&255)<=129);
        REQUIRE(((center>>8)&255)>=127 && ((center>>8)&255)<=129);
        REQUIRE((center&255)>=127 && (center&255)<=129);
        REQUIRE_OK(device->SetRenderState(D3DRENDERSTATE_FOGENABLE,FALSE));
        // Retail can draw a keyed foreground before the geometry behind it.
        // Invisible texels must discard depth as the original color key does.
        // Otherwise later objects disappear in a rectangular black silhouette.
        REQUIRE_OK(device->SetRenderState(D3DRENDERSTATE_ZENABLE,TRUE));
        REQUIRE_OK(device->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE,TRUE));
        REQUIRE_OK(device->SetRenderState(D3DRENDERSTATE_ZFUNC,D3DCMP_LESS));
        D3DRECT area={0,0,64,64};REQUIRE_OK(viewport->Clear(1,&area,D3DCLEAR_ZBUFFER));
        REQUIRE_OK(Clear(target.value));REQUIRE_OK(device->BeginScene());
        for(auto& point:points){point.sz=0.25f;point.color=0x64FFFFFF;}
        REQUIRE_OK(device->DrawPrimitive(D3DPT_TRIANGLEFAN,D3DVT_TLVERTEX,points,4,0));
        REQUIRE_OK(device->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE,0));
        REQUIRE_OK(device->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE,FALSE));
        for(auto& point:points){point.sz=0.75f;point.color=0xFF40C020;}
        REQUIRE_OK(device->DrawPrimitive(D3DPT_TRIANGLEFAN,D3DVT_TLVERTEX,points,4,0));
        REQUIRE_OK(device->EndScene());ReadPixels(target.value,ui);
        REQUIRE((ui[32*64+32]&0xFFFFFF)==0xFFFFFF);
        REQUIRE((ui[2*64+2]&0xFFFFFF)==0x40C020);
        // No blend with the old clear color may survive around a cutout when
        // its background is rendered later. Exercise both F1/F2 states.
        for(DWORD filter:{DWORD(D3DFILTER_NEAREST),DWORD(D3DFILTER_LINEAR)})for(DWORD fog:{0u,1u}) {
            REQUIRE_OK(viewport->Clear(1,&area,D3DCLEAR_ZBUFFER));
            REQUIRE_OK(Clear(target.value));
            REQUIRE_OK(device->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE,handle));
            REQUIRE_OK(device->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE,TRUE));
            REQUIRE_OK(device->SetRenderState(D3DRENDERSTATE_TEXTUREMAG,filter));
            REQUIRE_OK(device->SetRenderState(D3DRENDERSTATE_TEXTUREMIN,D3DFILTER_NEAREST));
            REQUIRE_OK(device->SetRenderState(D3DRENDERSTATE_FOGENABLE,fog));
            REQUIRE_OK(device->BeginScene());
            for(auto& point:points){point.sz=0.25f;point.color=0x64FFFFFF;point.specular=0x80000000;}
            REQUIRE_OK(device->DrawPrimitive(D3DPT_TRIANGLEFAN,D3DVT_TLVERTEX,points,4,0));
            REQUIRE_OK(device->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE,0));
            REQUIRE_OK(device->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE,FALSE));
            for(auto& point:points){point.sz=0.75f;point.color=0xFF40C020;point.specular=0xFF000000;}
            REQUIRE_OK(device->DrawPrimitive(D3DPT_TRIANGLEFAN,D3DVT_TLVERTEX,points,4,0));
            g_loading_capture_requested=true;REQUIRE_OK(device->EndScene());g_loading_capture_requested=false;
            ReadPixels(target.value,ui);
            REQUIRE(g_loading_capture && g_loading_capture->width==64 && g_loading_capture->height==64);
            for(size_t pixel=0;pixel<ui.size();++pixel)REQUIRE(g_loading_capture->pixels[pixel]==(ui[pixel]&0xFFFFFF));
            DWORD solid=fog ? 0x808080 : 0xFFFFFF;
            for(int x=8;x<24;++x) {
                DWORD pixel=ui[32*64+x]&0xFFFFFF;
                if(pixel!=solid && pixel!=0x40C020)printf("Fringe: filter=%lu fog=%lu x=%d RGB=%06lx\n",filter,fog,x,pixel);
                REQUIRE(pixel==solid || pixel==0x40C020);
            }
        }
        REQUIRE(g_loading_capture->pixels[2*64+2]==0x40C020); // CPU image outlives its renderer.
        // A held triangle crossing the eye plane must keep its visible area.
        // The retail projector would reject the entire triangle instead.
        const ff_camera::PropProjection prop_projection={32,32,32,32,64,64,0.000015265};
        const ff_camera::PropVertex crossing[]={{-1,-1,-2,0,0},{2,-1,4,1,0},{0,2,4,0,1}};
        ff_camera::PropVertex clipped[8];D3DTLVERTEX prop_vertices[8]={};
        const unsigned prop_count=ff_camera::ClipPropTriangle(crossing,prop_projection,clipped);
        REQUIRE(prop_count>=3);
        for(unsigned i=0;i<prop_count;++i) {
            const auto& v=clipped[i];auto& out=prop_vertices[i];
            out.sx=float(32+32*v.x/v.z);out.sy=float(32+32*v.y/v.z);
            out.sz=float(v.z*prop_projection.depth_scale);out.rhw=float(1/v.z);out.color=0xFFFFFFFF;
        }
        REQUIRE_OK(device->SetRenderState(D3DRENDERSTATE_FOGENABLE,FALSE));
        REQUIRE_OK(device->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE,0));
        REQUIRE_OK(device->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE,FALSE));
        REQUIRE_OK(device->SetRenderState(D3DRENDERSTATE_ZENABLE,FALSE));
        REQUIRE_OK(Clear(target.value));REQUIRE_OK(device->BeginScene());
        REQUIRE_OK(device->DrawPrimitive(D3DPT_TRIANGLEFAN,D3DVT_TLVERTEX,prop_vertices,prop_count,0));
        REQUIRE_OK(device->EndScene());ReadPixels(target.value,ui);
        REQUIRE((ui[32*64+32]&0xFFFFFF)==0xFFFFFF);
        REQUIRE((ui[63*64+63]&0xFFFFFF)==0x306090);
        // Low first-person views still see the floor when its rear corners lie
        // behind the eyes. Rejecting the quad would leave a clear-color hole.
        const unsigned floor_indices[2][3]={{0,3,2},{0,2,1}};
        for(double eye_height:{8.0,16.0,32.0}) {
            const ff_camera::PropVertex floor[]={{-128,eye_height,-128,0,0},{128,eye_height,-128,1,0},
                {128,eye_height,128,1,1},{-128,eye_height,128,0,1}};
            REQUIRE_OK(Clear(target.value));REQUIRE_OK(device->BeginScene());
            for(const auto& index:floor_indices) {
                const ff_camera::PropVertex triangle[]={floor[index[0]],floor[index[1]],floor[index[2]]};
                const unsigned count=ff_camera::ClipPropTriangle(triangle,prop_projection,clipped);
                REQUIRE(count>=3);
                for(unsigned i=0;i<count;++i) {
                    const auto& v=clipped[i];auto& out=prop_vertices[i];
                    out.sx=float(32+32*v.x/v.z);out.sy=float(32+32*v.y/v.z);
                    out.sz=float(v.z*prop_projection.depth_scale);out.rhw=float(1/v.z);out.color=0xFFD0A080;
                }
                REQUIRE_OK(device->DrawPrimitive(D3DPT_TRIANGLEFAN,D3DVT_TLVERTEX,prop_vertices,count,0));
            }
            REQUIRE_OK(device->EndScene());ReadPixels(target.value,ui);
            REQUIRE((ui[50*64+32]&0xFFFFFF)==0xD0A080);
            REQUIRE((ui[16*64+32]&0xFFFFFF)==0x306090);
        }
        device->DeleteViewport(viewport.value);
        REQUIRE(device->Release()==0);
        printf("GPU lifecycle cycle %d PASS; opaque interiors, transparent coverage, later geometry through cutouts and state restoration correct.\n",cycle+1);
    }
    DestroyWindow(window);
    printf("Color-key rendering and depth regression GPU test: PASS\n");
    return 0;
}
