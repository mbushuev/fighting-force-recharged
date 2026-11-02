#pragma once
static thread_local bool g_combat_world_drawn;
static void CombatWorldDrawn(){g_combat_world_drawn=true;}
static void (*g_combat_weapon_overlay)(void*,bool);
struct CombatActorBounds {double left=0,top=0,right=0,bottom=0;bool active=false;};
static thread_local CombatActorBounds g_combat_actor_bounds;
static void CaptureCombatBounds(const FirstPersonModel* model,void* palette) {
    const uintptr_t address=reinterpret_cast<uintptr_t>(palette),base=g_base+0x8623E0;
    if(address<base || (address-base)%0x238 || (address-base)/0x238>=8)return;
    const unsigned player=g_camera_owner==1?1:0;int target;
    AcquireSRWLockShared(&g_focus_visual_lock);target=g_focus_visual.slot[player];ReleaseSRWLockShared(&g_focus_visual_lock);
    if(target!=int((address-base)/0x238) || !CameraReadable(model,sizeof(*model)) ||
        !model->vertex_count || model->vertex_count>4096)return;
    // Native skeletal projection retains 32-byte records at this boundary:
    // screen X/Y at +0/+4, with bit 0x10 marking an unprojected vertex at +0x1C.
    const auto vertices=reinterpret_cast<const BYTE*>(g_base+0x9D208);
    CombatActorBounds box;box.left=box.top=1e12;box.right=box.bottom=-1e12;unsigned valid=0;
    for(unsigned i=0;i<model->vertex_count;++i) {
        const auto v=vertices+i*32;
        if(*reinterpret_cast<const DWORD*>(v+0x1C)&0x10)continue;
        const double x=*reinterpret_cast<const float*>(v),y=*reinterpret_cast<const float*>(v+4);
        if(!isfinite(x) || !isfinite(y) || fabs(x)>1e6 || fabs(y)>1e6)continue;
        box.left=fmin(box.left,x);box.right=fmax(box.right,x);box.top=fmin(box.top,y);box.bottom=fmax(box.bottom,y);++valid;
    }
    box.active=valid>=4 && box.right>box.left && box.bottom>box.top;
    g_combat_actor_bounds=box;
}
// Draw after the world, inside its existing scene. Every modified render state
// is queried and restored, including error paths; no game textures are loaded.
struct CombatOverlayDraw {
    void* device;RenderObserver* record;
    static constexpr D3DRENDERSTATETYPE states[]={D3DRENDERSTATE_TEXTUREHANDLE,D3DRENDERSTATE_ZENABLE,
        D3DRENDERSTATE_ZWRITEENABLE,D3DRENDERSTATE_FOGENABLE,D3DRENDERSTATE_ALPHABLENDENABLE,
        D3DRENDERSTATE_SRCBLEND,D3DRENDERSTATE_DESTBLEND,D3DRENDERSTATE_CULLMODE,
        D3DRENDERSTATE_COLORKEYENABLE,D3DRENDERSTATE_SPECULARENABLE,D3DRENDERSTATE_SHADEMODE};
    DWORD saved[11]={};unsigned prepared=0;bool ready=false;
    using Set=HRESULT(STDMETHODCALLTYPE*)(void*,D3DRENDERSTATETYPE,DWORD);
    CombatOverlayDraw(void* object):device(object),record(RenderRecord(object)) {
        if(!record)return;
        const DWORD values[]={0,FALSE,FALSE,FALSE,TRUE,D3DBLEND_SRCALPHA,D3DBLEND_INVSRCALPHA,
            D3DCULL_NONE,FALSE,FALSE,D3DSHADE_GOURAUD};
        for(unsigned i=0;i<11;++i) {
            if(FAILED(static_cast<IDirect3DDevice2*>(device)->GetRenderState(states[i],&saved[i])))return;
            ++prepared;if(FAILED(reinterpret_cast<Set>(record->original[23])(device,states[i],values[i])))return;
        }
        ready=true;
    }
    ~CombatOverlayDraw(){for(unsigned i=0;i<prepared;++i)reinterpret_cast<Set>(record->original[23])(device,states[i],saved[i]);}
    void Line(double ax,double ay,double bx,double by,double width,DWORD color) {
        if(!ready)return;const double length=hypot(bx-ax,by-ay);if(length<0.01)return;
        const float x=float(-(by-ay)*width/length*0.5),y=float((bx-ax)*width/length*0.5);
        D3DTLVERTEX v[6]={};
        const float points[6][2]={{float(ax)+x,float(ay)+y},{float(bx)+x,float(by)+y},{float(ax)-x,float(ay)-y},
            {float(ax)-x,float(ay)-y},{float(bx)+x,float(by)+y},{float(bx)-x,float(by)-y}};
        for(unsigned i=0;i<6;++i){v[i].sx=points[i][0];v[i].sy=points[i][1];v[i].sz=0.01f;v[i].rhw=1;v[i].color=color;}
        using Draw=HRESULT(STDMETHODCALLTYPE*)(void*,D3DPRIMITIVETYPE,D3DVERTEXTYPE,void*,DWORD,DWORD);
        reinterpret_cast<Draw>(record->original[29])(device,D3DPT_TRIANGLELIST,D3DVT_TLVERTEX,v,6,D3DDP_DONOTCLIP);
    }
    void Frame(double left,double top,double right,double bottom,double width,double height,double stroke) {
        const double horizontal=fmin((right-left)*0.16,14*stroke),vertical=fmin((bottom-top)*0.16,14*stroke);
        const double edges[8][4]={{left+horizontal,top,left,top},{left,top,left,top+vertical},
            {right-horizontal,top,right,top},{right,top,right,top+vertical},
            {left,bottom-vertical,left,bottom},{left,bottom,left+horizontal,bottom},
            {right,bottom-vertical,right,bottom},{right,bottom,right-horizontal,bottom}};
        for(const auto& e:edges) {
            double ax=e[0],ay=e[1],bx=e[2],by=e[3];
            if(!ff_focus::ClipLine(ax,ay,bx,by,width,height))continue;
            Line(ax,ay,bx,by,stroke+2,0xCC101820);Line(ax,ay,bx,by,stroke,0xD090D8FF);
        }
    }
};
static void DrawCombatOverlay(void* device) {
    const bool world=g_combat_world_drawn;g_combat_world_drawn=false;
    const auto bounds=g_combat_actor_bounds;g_combat_actor_bounds={};
    if(g_combat_weapon_overlay)g_combat_weapon_overlay(device,world);
    if(!world)return;
    if(!g_camera_active || !CameraMayOwnPose() || !ModernHasFocus() || g_controls_menu_active || g_campaign_menu_active)return;
    CombatFocusVisual focus;
    AcquireSRWLockShared(&g_focus_visual_lock);focus=g_focus_visual;
    ReleaseSRWLockShared(&g_focus_visual_lock);
    const unsigned player=g_camera_owner==1?1:0;
    const bool marker=g_controls.marker && focus.stage==CameraField<int>(0x85A6B8) && focus.slot[player]>=2;
    if(!marker)return;
    const double width=CameraField<float>(0x9D170),height=CameraField<float>(0x9D174);
    if(width<=0 || height<=0 || !isfinite(width) || !isfinite(height))return;
    const double sy=ff_camera::Clamp(height/720,0.75,2),sx=sy*(g_coop_render_pass>=0?2:1);
    CombatOverlayDraw draw(device);
    if(marker) {
        if(bounds.active && bounds.right>0 && bounds.left<width && bounds.bottom>0 && bounds.top<height)
            draw.Frame(bounds.left-3*sx,bounds.top-3*sy,bounds.right+3*sx,bounds.bottom+3*sy,width,height,sy);
    }

}
static void InstallCombatBoundsHooks() {
    g_camera_world_drawn=CombatWorldDrawn;
    const DWORD calls[]={0x43C1E,0x44699};
    for(DWORD rva:calls) {
        const auto code=reinterpret_cast<const BYTE*>(g_base+rva);LONG offset;memcpy(&offset,code+1,4);
        if(code[0]!=0xE8 || g_base+rva+5+offset!=g_base+0x27180) {
            Log("Enemy projection signatures differ; target bounds unavailable.\r\n");return;
        }
    }
    DWORD previous;auto start=reinterpret_cast<BYTE*>(g_base+calls[0]);const SIZE_T size=calls[1]-calls[0]+5;
    if(!VirtualProtect(start,size,PAGE_EXECUTE_READWRITE,&previous))return;
    for(DWORD rva:calls) {
        const LONG offset=static_cast<LONG>(reinterpret_cast<uintptr_t>(FirstPersonSkeleton)-(g_base+rva+5));
        memcpy(reinterpret_cast<void*>(g_base+rva+1),&offset,4);
    }
    DWORD ignored;VirtualProtect(start,size,previous,&ignored);FlushInstructionCache(GetCurrentProcess(),start,size);
    g_camera_actor_bounds=CaptureCombatBounds;
}
