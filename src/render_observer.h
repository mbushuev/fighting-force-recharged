#pragma once
#include <d3d.h>
#include <float.h>
#include <stddef.h>
#include "far_depth.h"
#include "transparent_edges.h"
#include "camera_diagnostics.h"
static bool g_trace_renderer;
static bool g_fix_far_depth;
static bool g_smooth_edges;
static void (*g_render_scene_complete)();
static void (*g_render_before_release)();
static void (*g_render_poll)();

// Observe only the recognized retail's IDirect3DDevice2. Each COM object gets
// its own vtable copy; dgVoodoo DLL code and the retail files stay untouched.
struct RenderObserver {
    void** original;
    void* table[33];
    DWORD states[256] = {};
    DWORD frames = 0, draws = 0, bad_vertices = 0, unusual_uv = 0;
    int stage = -1;
    DWORD examples = 0;
    DWORD depth_vertices = 0;
    bool alpha_sample = false;
    DWORD traced_features=~0u;
    DWORD edge_draws=0;
    std::vector<D3DTLVERTEX> scratch;
    std::vector<EdgeTexture> edge_textures;
    std::vector<DWORD> pending_textures;
    ~RenderObserver(){ReleaseEdgeTextures(edge_textures);}
};
static RenderObserver* RenderRecord(void* object) {
    auto table = *reinterpret_cast<void***>(object);
    return reinterpret_cast<RenderObserver*>(reinterpret_cast<BYTE*>(table) - offsetof(RenderObserver, table));
}
static void RenderTrace(RenderObserver* record, D3DVERTEXTYPE type, void* vertices, DWORD count) {
    if(!g_trace_renderer)return;
    const DWORD features=record->states[17]|(record->states[28]<<4)|(record->states[35]<<5);
    if(features!=record->traced_features) {
        record->traced_features=features;record->examples=0;record->alpha_sample=false;
        char message[192];sprintf_s(message,"Render modes: filters=%lu/%lu, fog=%lu/%lu, fog color=%08lx, stipple=%lu, color key=%lu.\r\n",record->states[17],record->states[18],record->states[28],record->states[35],record->states[34],record->states[33],record->states[41]);Log(message);
    }
    int stage = *reinterpret_cast<int*>(g_base + 0x85A6B8);
    if (stage != record->stage) {
        record->stage = stage; record->examples = 0;
        char message[160];
        sprintf_s(message, "Render stage=%d; frames=%lu, draws=%lu, bad vertices=%lu, UV outside=%lu\r\n",
            stage, record->frames, record->draws, record->bad_vertices, record->unusual_uv);
        Log(message);
    }
    ++record->draws;
    if (type != D3DVT_TLVERTEX || !vertices || !count) return;
    auto points = static_cast<D3DTLVERTEX*>(vertices);
    float min_u=points[0].tu, max_u=min_u, min_v=points[0].tv, max_v=min_v;
    bool bad=false, outside=false;
    for (DWORD index=0; index<count; ++index) {
        auto& v=points[index];
        if (!_finite(v.sx) || !_finite(v.sy) || !_finite(v.sz) || !_finite(v.rhw) ||
            !_finite(v.tu) || !_finite(v.tv) || v.rhw<=0 || v.sz<0 || v.sz>1) {
            ++record->bad_vertices; bad=true;
        }
        if (v.tu<0 || v.tu>1 || v.tv<0 || v.tv>1) { ++record->unusual_uv; outside=true; }
        if(v.tu<min_u)min_u=v.tu; if(v.tu>max_u)max_u=v.tu;
        if(v.tv<min_v)min_v=v.tv; if(v.tv>max_v)max_v=v.tv;
    }
    bool first_alpha=record->states[27] && !record->alpha_sample;
    if(first_alpha)record->alpha_sample=true;
    if (record->examples<12 && (bad || outside || record->examples<4 || first_alpha)) {
        ++record->examples;
        char message[640];
        sprintf_s(message, "Render sample: stage=%d, tex=%lu, n=%lu, UV=%.7g..%.7g/%.7g..%.7g, XYZ=%.7g/%.7g/%.7g, RHW=%.7g, diffuse=%08lx, specular=%08lx; address=%lu, persp=%lu, Z=%lu/%lu/%lu, filters=%lu/%lu, blend=%lu/%lu/%lu, alpha=%lu/%lu, bad=%d\r\n",
            stage,record->states[1],count,min_u,max_u,min_v,max_v,points[0].sx,points[0].sy,points[0].sz,points[0].rhw,
            points[0].color,points[0].specular,record->states[3],record->states[4],record->states[7],record->states[14],record->states[23],
            record->states[17],record->states[18],record->states[21],record->states[19],record->states[20],record->states[15],record->states[27],bad);
        Log(message);
    }
}
static IDirectDrawSurface* NativeTextureSurface(DWORD handle) {
    auto textures=reinterpret_cast<uintptr_t*>(g_base+0x211D60);
    for(size_t index=0;index<18;++index) {
        auto texture=reinterpret_cast<BYTE*>(textures[index]);
        if(!texture || *reinterpret_cast<DWORD*>(texture+4)!=handle)continue;
        auto surface_object=*reinterpret_cast<IDirectDrawSurface***>(texture+12);
        return surface_object ? *surface_object : nullptr;
    }
    return nullptr;
}
struct SmoothEdgeDraw {
    void* object;
    RenderObserver* record;
    bool active=false;
    using SetState=HRESULT(STDMETHODCALLTYPE*)(void*,D3DRENDERSTATETYPE,DWORD);
    void Set(DWORD state,DWORD value){reinterpret_cast<SetState>(record->original[23])(object,static_cast<D3DRENDERSTATETYPE>(state),value);}
    SmoothEdgeDraw(void* device,RenderObserver* observer,D3DVERTEXTYPE type,void*& vertices,DWORD count):object(device),record(observer) {
        if(!g_smooth_edges || type!=D3DVT_TLVERTEX || !vertices || !count || count>65535 ||
            !record->states[41] || !record->states[1] ||
            (record->states[17]==D3DFILTER_NEAREST && record->states[18]==D3DFILTER_NEAREST))return;
        if(record->states[27] && (record->states[19]!=D3DBLEND_SRCALPHA || record->states[20]!=D3DBLEND_INVSRCALPHA))return;
        const bool native_blended=record->states[27]!=0;
        DWORD handle=record->states[1],replacement=0;
        bool found=false;
        for(auto& entry:record->edge_textures)if(entry.original==handle){replacement=native_blended?entry.replacement:entry.cutout_replacement;found=true;break;}
        if(!found) {
            bool queued=false;
            for(DWORD pending:record->pending_textures)if(pending==handle)queued=true;
            if(!queued)record->pending_textures.push_back(handle);
            return; // Build between frames, never Lock/Load textures in a scene.
        }
        if(!replacement)return;
        if(vertices!=record->scratch.data()) {
            record->scratch.resize(count);
            memcpy(record->scratch.data(),vertices,count*sizeof(D3DTLVERTEX));
        }
        const bool native_vertex_alpha=record->states[27]!=0 && record->states[21]==D3DTBLEND_MODULATEALPHA;
        for(auto& vertex:record->scratch)vertex.color=KeyedVertexColor(vertex.color,native_vertex_alpha);
        vertices=record->scratch.data();
        Set(1,replacement);Set(41,FALSE);
        // Opaque keyed geometry must remain an opaque cutout: blending an
        // edge against the clear color before the background is drawn leaves
        // a fringe and writes depth over that background. Filter the extended
        // RGB border, test coverage at half a texel, and keep blending off.
        // Only passes already translucent in the game use premultiplied alpha.
        Set(24,native_blended?0:127);Set(25,D3DCMP_GREATER);Set(15,TRUE);Set(27,native_blended);
        Set(21,D3DTBLEND_MODULATEALPHA);Set(19,D3DBLEND_ONE);Set(20,D3DBLEND_INVSRCALPHA);
        active=true;++record->edge_draws;
    }
    ~SmoothEdgeDraw(){if(active)for(DWORD state:{1u,41u,24u,25u,15u,27u,21u,19u,20u})Set(state,record->states[state]);}
};
static void DiagnoseTextureFormats() {
    if(!g_trace_renderer)return;
    auto textures=reinterpret_cast<uintptr_t*>(g_base+0x211D60);
    for(size_t index=0;index<18;++index) {
        auto texture=reinterpret_cast<BYTE*>(textures[index]);
        if(!texture)continue;
        DWORD handle=*reinterpret_cast<DWORD*>(texture+4);
        IDirectDrawSurface* surface=NativeTextureSurface(handle);
        if(!surface)continue;
        DDSURFACEDESC desc={};desc.dwSize=sizeof(desc);
        if(FAILED(surface->GetSurfaceDesc(&desc)))continue;
        char message[256];
        sprintf_s(message,"Texture page=%zu, handle=%lu, size=%lux%lu, bpp=%lu, flags=%08lx, RGBA=%08lx/%08lx/%08lx/%08lx\r\n",index,handle,desc.dwWidth,desc.dwHeight,desc.ddpfPixelFormat.dwRGBBitCount,desc.ddpfPixelFormat.dwFlags,desc.ddpfPixelFormat.dwRBitMask,desc.ddpfPixelFormat.dwGBitMask,desc.ddpfPixelFormat.dwBBitMask,desc.ddpfPixelFormat.dwRGBAlphaBitMask);
        Log(message);
    }
}
static ULONG STDMETHODCALLTYPE RenderRelease(void* object) {
    auto record=RenderRecord(object);
    if(g_render_before_release)g_render_before_release();
    ReleaseEdgeTextures(record->edge_textures);
    record->pending_textures.clear();
    using Method=ULONG(STDMETHODCALLTYPE*)(void*);
    ULONG count=reinterpret_cast<Method>(record->original[2])(object);
    if(!count) { Log("Renderer released cleanly.\r\n");delete record; }
    return count;
}
static HRESULT STDMETHODCALLTYPE RenderBeginScene(void* object) {
    CameraTiming timing(CameraRenderPrepareTime);
    auto record=RenderRecord(object);
    if(!g_smooth_edges)ReleaseEdgeTextures(record->edge_textures);
    std::vector<DWORD> pending;pending.swap(record->pending_textures);
    if(g_smooth_edges)for(DWORD handle:pending)
        record->edge_textures.push_back(BuildEdgeTexture(object,handle,NativeTextureSurface(handle)));
    using Method=HRESULT(STDMETHODCALLTYPE*)(void*);
    return reinterpret_cast<Method>(record->original[10])(object);
}
static HRESULT STDMETHODCALLTYPE RenderEndScene(void* object) {
    CameraTiming timing(CameraRenderEndTime);
    auto record=RenderRecord(object); ++record->frames;
    if(record->frames==60 && record->depth_vertices) {
        char message[160];
        sprintf_s(message,"Renderer: stabilized %lu far-plane vertices in 60 frames; texture sampling unchanged.\r\n",record->depth_vertices);
        Log(message);
    }
    if(record->frames==60 && record->edge_draws) {
        char message[160];sprintf_s(message,"Renderer: %lu transparent filtered draws used alpha coverage in 60 frames.\r\n",record->edge_draws);Log(message);
    }
    using Method=HRESULT(STDMETHODCALLTYPE*)(void*);
    HRESULT result=reinterpret_cast<Method>(record->original[11])(object);
    if(SUCCEEDED(result) && g_render_scene_complete)g_render_scene_complete();
    return result;
}
static HRESULT STDMETHODCALLTYPE RenderSetState(void* object,D3DRENDERSTATETYPE state,DWORD value) {
    auto record=RenderRecord(object);
    using Method=HRESULT(STDMETHODCALLTYPE*)(void*,D3DRENDERSTATETYPE,DWORD);
    HRESULT result=reinterpret_cast<Method>(record->original[23])(object,state,value);
    if(SUCCEEDED(result) && static_cast<DWORD>(state)<256)record->states[state]=value;
    return result;
}
static HRESULT STDMETHODCALLTYPE RenderDraw(void* object,D3DPRIMITIVETYPE primitive,D3DVERTEXTYPE type,void* vertices,DWORD count,DWORD flags) {
    CameraTiming timing(CameraRenderSubmitTime);
    auto record=RenderRecord(object); RenderTrace(record,type,vertices,count);
    DWORD changed=0;
    if(g_fix_far_depth)vertices=StableFarDepthVertices(type,vertices,count,record->scratch,&changed);
    record->depth_vertices+=changed;
    SmoothEdgeDraw edges(object,record,type,vertices,count);
    using Method=HRESULT(STDMETHODCALLTYPE*)(void*,D3DPRIMITIVETYPE,D3DVERTEXTYPE,void*,DWORD,DWORD);
    HRESULT result=reinterpret_cast<Method>(record->original[29])(object,primitive,type,vertices,count,flags);
    return result;
}
static HRESULT STDMETHODCALLTYPE RenderDrawIndexed(void* object,D3DPRIMITIVETYPE primitive,D3DVERTEXTYPE type,void* vertices,DWORD count,WORD* indices,DWORD index_count,DWORD flags) {
    CameraTiming timing(CameraRenderSubmitTime);
    auto record=RenderRecord(object); RenderTrace(record,type,vertices,count);
    DWORD changed=0;
    if(g_fix_far_depth)vertices=StableFarDepthVertices(type,vertices,count,record->scratch,&changed);
    record->depth_vertices+=changed;
    SmoothEdgeDraw edges(object,record,type,vertices,count);
    using Method=HRESULT(STDMETHODCALLTYPE*)(void*,D3DPRIMITIVETYPE,D3DVERTEXTYPE,void*,DWORD,WORD*,DWORD,DWORD);
    return reinterpret_cast<Method>(record->original[30])(object,primitive,type,vertices,count,indices,index_count,flags);
}
static void ObserveGameRenderer() {
    g_trace_renderer=GetPrivateProfileIntW(L"Diagnostics", L"TraceRenderer", 0, g_config_path)!=0;
    g_fix_far_depth=GetPrivateProfileIntW(L"Compatibility", L"StableFarDepth", 0, g_config_path)!=0;
    g_smooth_edges=GetPrivateProfileIntW(L"Compatibility", L"SmoothTransparentEdges", 0, g_config_path)!=0;
    static const bool test_ui_edges=GetEnvironmentVariableW(L"FIGHTINGFORCE_TEST_UI_EDGES",nullptr,0)!=0;
    if(test_ui_edges)g_smooth_edges=true;
    if(!g_trace_renderer && !g_fix_far_depth && !g_smooth_edges)return;
    void* object=*reinterpret_cast<void**>(g_base+0xAD210);
    if(!object) return;
    auto table=*reinterpret_cast<void***>(object);
    if(table[29]==reinterpret_cast<void*>(RenderDraw))return;
    auto record=new(std::nothrow) RenderObserver;
    if(!record)return;
    record->original=table; memcpy(record->table,table,sizeof(record->table));
    const DWORD states[]={1,3,4,7,14,15,17,18,19,20,21,23,24,25,27,28,29,33,34,35,41,144};
    using GetState=HRESULT(STDMETHODCALLTYPE*)(void*,D3DRENDERSTATETYPE,DWORD*);
    for(DWORD state:states)reinterpret_cast<GetState>(table[22])(object,static_cast<D3DRENDERSTATETYPE>(state),&record->states[state]);
    record->table[2]=reinterpret_cast<void*>(RenderRelease);
    record->table[10]=reinterpret_cast<void*>(RenderBeginScene);
    record->table[11]=reinterpret_cast<void*>(RenderEndScene);
    record->table[23]=reinterpret_cast<void*>(RenderSetState);
    record->table[29]=reinterpret_cast<void*>(RenderDraw);
    record->table[30]=reinterpret_cast<void*>(RenderDrawIndexed);
    *reinterpret_cast<void***>(object)=record->table;
    IDirectDrawSurface* target=nullptr;
    if(SUCCEEDED(static_cast<IDirect3DDevice2*>(object)->GetRenderTarget(&target)) && target) {
        DDSURFACEDESC info={};info.dwSize=sizeof(info);
        if(SUCCEEDED(target->GetSurfaceDesc(&info))) {
            char message[128];sprintf_s(message,"Game render target: %lux%lu.\r\n",info.dwWidth,info.dwHeight);Log(message);
        }
        target->Release();
    }
    DiagnoseTextureFormats();
    Log("Renderer interception enabled; native filtering and UV coordinates retained.\r\n");
}
static BOOL WINAPI GamePeekMessage(LPMSG message,HWND window,UINT first,UINT last,UINT flags) {
    ObserveGameRenderer();
    if(g_render_poll)g_render_poll();
    return PeekMessageA(message,window,first,last,flags);
}
