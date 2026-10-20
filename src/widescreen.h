#pragma once
#include "widescreen_layout.h"

static WideLayout g_wide_layout;
static bool g_widescreen_installed;
static bool g_wide_background;
static bool g_wide_probe_frame;
static unsigned g_wide_probe_draw;

static int __cdecl GameWideFrameStart() {
    using Function=int(__cdecl*)();
    int result=reinterpret_cast<Function>(g_base+0x28A40)();
    static const bool trace=GetEnvironmentVariableW(L"FIGHTINGFORCE_TRACE_WIDE",nullptr,0)!=0;
    static int last_stage=-999;static unsigned frame=0;
    int stage=*reinterpret_cast<int*>(g_base+0x85A6B8);
    if(stage!=last_stage){last_stage=stage;frame=0;}
    ++frame;g_wide_probe_frame=trace && stage==0 && (frame==30 || frame==120);g_wide_probe_draw=0;
    if(g_wide_probe_frame) {
        char message[320];sprintf_s(message,"Wide probe frame %u: stage %d, screen %.7gx%.7g, scale %.7g/%.7g, focal %.7g/%.7g, center %.7g/%.7g, clip %.7g/%.7g.\r\n",frame,stage,
            *reinterpret_cast<float*>(g_base+0x9D170),*reinterpret_cast<float*>(g_base+0x9D174),
            *reinterpret_cast<float*>(g_base+0x9D178),*reinterpret_cast<float*>(g_base+0x9A160),
            *reinterpret_cast<float*>(g_base+0xAD214),*reinterpret_cast<float*>(g_base+0xAD234),
            *reinterpret_cast<float*>(g_base+0x20C50C),*reinterpret_cast<float*>(g_base+0x20C508),
            *reinterpret_cast<float*>(g_base+0x731A0),*reinterpret_cast<float*>(g_base+0x731A4));Log(message);
    }
    if(result && g_wide_layout.active) {
        auto system=*reinterpret_cast<BYTE**>(g_base+0xAD22C);
        if(system) {
            auto surface_object=*reinterpret_cast<IDirectDrawSurface***>(system+0x14);
            if(surface_object && *surface_object) {
                DDBLTFX fill={};fill.dwSize=sizeof(fill);fill.dwFillColor=0;
                HRESULT cleared=(*surface_object)->Blt(nullptr,nullptr,nullptr,DDBLT_COLORFILL|DDBLT_WAIT,&fill);
                static bool logged=false;
                if(!logged){char message[128];sprintf_s(message,"Wide frame full-target clear: %08lx; before native background and BeginScene.\r\n",cleared);Log(message);logged=true;}
            }
        }
    }
    return result;
}
static void __cdecl GameWideBackground() {
    using Function=void(__cdecl*)();
    bool previous=g_wide_background;g_wide_background=true;
    reinterpret_cast<Function>(g_base+0x47A10)();g_wide_background=previous;
}
struct WideModeEnumeration {WideModeChoice choice;DWORD bits;};
static HRESULT WINAPI WideDisplayMode(DDSURFACEDESC* mode,void* context) {
    auto selection=static_cast<WideModeEnumeration*>(context);
    if(mode->ddpfPixelFormat.dwRGBBitCount==selection->bits)
        ConsiderWideMode(selection->choice,mode->dwWidth,mode->dwHeight);
    return DDENUMRET_OK;
}
static bool SelectWideFullscreenMode(unsigned aspect_width,unsigned aspect_height,unsigned min_height,unsigned& width,unsigned& height) {
    using Create=HRESULT(WINAPI*)(GUID*,IDirectDraw**,IUnknown*);
    auto create=reinterpret_cast<Create>(GetProcAddress(GetModuleHandleW(L"DDraw.dll"),"DirectDrawCreate"));
    IDirectDraw* draw=nullptr;
    if(!create || FAILED(create(nullptr,&draw,nullptr)))return false;
    WideModeEnumeration selection={{aspect_width,aspect_height,min_height},*reinterpret_cast<DWORD*>(g_base+0x9D1F8)};
    HRESULT result=draw->EnumDisplayModes(0,nullptr,&selection,WideDisplayMode);draw->Release();
    if(FAILED(result) || !selection.choice.width)return false;
    width=selection.choice.width;height=selection.choice.height;return true;
}

static int __cdecl GameWideScreenPolygon(const RetailScreenPolygon* polygon) {
    using Function=int(__cdecl*)(const RetailScreenPolygon*);
    RetailScreenPolygon copy;
    const auto input=CenterWideScreenPolygon(g_wide_layout,polygon,copy,g_wide_background)?&copy:polygon;
    if(g_wide_probe_frame && polygon && polygon->count && polygon->count<=8 && g_wide_probe_draw++<400) {
        char message[768];sprintf_s(message,"Wide polygon from %lx: source %lx flags %lu kind %lu tex %lu count %lu bg %d; XY %.7g,%.7g %.7g,%.7g %.7g,%.7g %.7g,%.7g; output %.7g,%.7g; Z/RHW %.7g/%.7g.\r\n",
            static_cast<unsigned long>(reinterpret_cast<uintptr_t>(_ReturnAddress())-g_base),
            static_cast<unsigned long>(reinterpret_cast<uintptr_t>(polygon)-g_base),polygon->flags,polygon->kind,polygon->texture,polygon->count,g_wide_background,
            polygon->vertices[0].sx,polygon->vertices[0].sy,polygon->vertices[1].sx,polygon->vertices[1].sy,
            polygon->vertices[2].sx,polygon->vertices[2].sy,polygon->vertices[3].sx,polygon->vertices[3].sy,
            input->vertices[0].sx,input->vertices[0].sy,polygon->vertices[0].sz,polygon->vertices[0].rhw);Log(message);
    }
    return reinterpret_cast<Function>(g_base+0x24ED0)(input);
}
static int __cdecl GameWideRendererInit(RECT* rectangle) {
    using Function=int(__cdecl*)(RECT*);
    auto native=reinterpret_cast<Function>(g_base+0x28600);
    unsigned setting=GetPrivateProfileIntW(L"Rendering",L"WidescreenAspectX1000",1,g_config_path);
    g_wide_layout={};
    if(!g_widescreen_installed || !setting)return native(rectangle);
    auto windowed=reinterpret_cast<DWORD*>(g_base+0x9D1A4);
    auto fullscreen_width=reinterpret_cast<DWORD*>(g_base+0x9D1B0);
    auto fullscreen_height=reinterpret_cast<DWORD*>(g_base+0x9D1B4);
    unsigned width=*fullscreen_width,height=*fullscreen_height;
    if(*windowed && rectangle) {
        width=rectangle->right-rectangle->left+1;
        height=rectangle->bottom-rectangle->top+1;
    }
    unsigned aspect_width=setting,aspect_height=1000;
    if(setting==1) {
        HWND window=*reinterpret_cast<HWND*>(g_base+0x1EC4E4);
        MONITORINFO monitor={};monitor.cbSize=sizeof(monitor);
        if(!GetMonitorInfoW(MonitorFromWindow(window,MONITOR_DEFAULTTOPRIMARY),&monitor))return native(rectangle);
        aspect_width=monitor.rcMonitor.right-monitor.rcMonitor.left;
        aspect_height=monitor.rcMonitor.bottom-monitor.rcMonitor.top;
    }
    const unsigned native_width=width,native_height=height;
    if(!*windowed) {
        if(!SelectWideFullscreenMode(aspect_width,aspect_height,height,width,height)) {
            Log("No supported wide DirectDraw display mode; native 4:3 retained.\r\n");return native(rectangle);
        }
        aspect_width=width;aspect_height=height;
    }
    WideLayout layout=MakeWideLayout(native_width,height,aspect_width,aspect_height);
    if(!layout.active)return native(rectangle);
    RECT expanded={0,0,static_cast<LONG>(layout.width-1),static_cast<LONG>(layout.height-1)};
    DWORD previous_width=*fullscreen_width,previous_height=*fullscreen_height;
    if(!*windowed){*fullscreen_width=layout.width;*fullscreen_height=layout.height;}
    int result=native(*windowed?&expanded:rectangle);
    *fullscreen_width=previous_width;*fullscreen_height=previous_height;
    // The engine derives its horizontal focal length from render width. Retain
    // the original vertical focal length instead, before any CPU projection or
    // culling occurs. 3D billboards use the same normalized horizontal scale.
    *reinterpret_cast<float*>(g_base+0xAD214)=*reinterpret_cast<float*>(g_base+0xAD234);
    *reinterpret_cast<float*>(g_base+0x9D178)=*reinterpret_cast<float*>(g_base+0x9A160);
    g_wide_layout=layout;
    char message[256];sprintf_s(message,"Widescreen: %ux%u -> %ux%u; horizontal view expanded, vertical focal length %.7g retained; UI scale %.7g, left %.7g.\r\n",
        native_width,native_height,layout.width,layout.height,*reinterpret_cast<float*>(g_base+0xAD214),layout.ui_scale,layout.ui_left);Log(message);
    return result;
}
static void InstallWidescreenHooks() {
    if(!GetPrivateProfileIntW(L"Rendering",L"WidescreenAspectX1000",1,g_config_path))return;
    // All direct calls to the immediate polygon consumer in this exact retail.
    // Validate the complete set before replacing any operand. No retail file is
    // written, and neither imported dgVoodoo code nor movie rendering is hooked.
    const DWORD polygon_calls[]={0x24AB3,0x24B79,0x24DDA,0x24E8C,0x25BF3,0x46631,0x4670B,
        0x46994,0x46A6E,0x46DAE,0x46EA1,0x46F7F,0x47EEF,0x4B081};
    struct Call {DWORD rva,target;void* hook;};
    Call calls[24]={{0x28545,0x28600,reinterpret_cast<void*>(GameWideRendererInit)}};
    for(size_t i=0;i<14;++i)calls[i+1]={polygon_calls[i],0x24ED0,reinterpret_cast<void*>(GameWideScreenPolygon)};
    const DWORD background_calls[]={0x2EBA1,0x2F084,0x40C42,0x416E2,0x41B3F,0x42355,0x4AB28,0x4CCDE};
    for(size_t i=0;i<8;++i)calls[i+15]={background_calls[i],0x47A10,reinterpret_cast<void*>(GameWideBackground)};
    calls[23]={0x256E1,0x28A40,reinterpret_cast<void*>(GameWideFrameStart)};
    for(const auto& call:calls) {
        auto code=reinterpret_cast<BYTE*>(g_base+call.rva);LONG relative;memcpy(&relative,code+1,4);
        if(code[0]!=0xE8 || reinterpret_cast<uintptr_t>(code+5)+relative!=g_base+call.target) {
            Log("Widescreen call signature differs; native 4:3 retained.\r\n");return;
        }
    }
    auto first=reinterpret_cast<BYTE*>(g_base+polygon_calls[0]);
    const SIZE_T length=background_calls[7]+5-polygon_calls[0];DWORD previous;
    if(!VirtualProtect(first,length,PAGE_EXECUTE_READWRITE,&previous))return;
    for(const auto& call:calls) {
        auto code=reinterpret_cast<BYTE*>(g_base+call.rva);
        DWORD relative=static_cast<DWORD>(reinterpret_cast<uintptr_t>(call.hook)-reinterpret_cast<uintptr_t>(code+5));
        memcpy(code+1,&relative,4);
    }
    DWORD ignored;VirtualProtect(first,length,previous,&ignored);FlushInstructionCache(GetCurrentProcess(),first,length);
    g_widescreen_installed=true;
    Log("Widescreen projection and explicit screen-polygon hooks enabled (process memory only).\r\n");
}
