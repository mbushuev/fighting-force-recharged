#pragma once
// Small asset-free bitmap labels are batched into one draw per visible player.
// This does not enqueue native text or reuse a previous interpolation sample.
static const BYTE* ItemPickupGlyph(char c) {
    static const char alphabet[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+-";
    static const BYTE rows[][7]={
        {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
        {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
        {14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
        {7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
        {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
        {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
        {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
        {17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
        {17,17,10,4,4,4,4},{31,1,2,4,8,16,31},{14,17,19,21,25,17,14},
        {4,12,4,4,4,4,14},{14,17,1,2,4,8,31},{30,1,1,14,1,1,30},
        {2,6,10,18,31,2,2},{31,16,16,30,1,1,30},{14,16,16,30,17,17,14},
        {31,1,2,4,8,8,8},{14,17,17,14,17,17,14},{14,17,17,15,1,1,14},
        {0,4,4,31,4,4,0},{0,0,0,31,0,0,0}};
    const auto found=strchr(alphabet,c);return found && c?rows[found-alphabet]:nullptr;
}
static void ItemPickupQuad(std::vector<D3DTLVERTEX>& mesh,double left,double top,double right,double bottom,DWORD color) {
    const float corners[][2]={{float(left),float(top)},{float(right),float(top)},{float(left),float(bottom)},
        {float(left),float(bottom)},{float(right),float(top)},{float(right),float(bottom)}};
    for(const auto& point:corners){D3DTLVERTEX v={};v.sx=point[0];v.sy=point[1];v.sz=.01f;v.rhw=1;v.color=color;mesh.push_back(v);}
}
static void DrawItemPickupLabel(CombatOverlayDraw& draw,const std::string& text,double x,double y,double width,double height) {
    if(!draw.ready || text.empty() || text.size()>60)return;
    const double sy=ff_camera::Clamp(height/480,.75,3),sx=sy*(g_coop_render_pass>=0?2:1);
    const double w=(text.size()*6+8)*sx,h=13*sy;
    if(width<w+8*sx || height<h+8*sy)return;
    const double left=ff_camera::Clamp(x-w/2,4*sx,width-w-4*sx);
    const double top=ff_camera::Clamp(y-h-7*sy,4*sy,height-h-4*sy);
    static thread_local std::vector<D3DTLVERTEX> mesh;mesh.clear();
    ItemPickupQuad(mesh,left,top,left+w,top+h,0xC0101820);
    ItemPickupQuad(mesh,left,top+h-sy,left+w,top+h,0xD080D0FF);
    for(unsigned i=0;i<text.size();++i)if(const auto rows=ItemPickupGlyph(text[i]))
        for(unsigned row=0;row<7;++row)for(unsigned col=0;col<5;++col)if(rows[row]&(16>>col)) {
            const double px=left+(4+i*6+col)*sx,py=top+(3+row)*sy;
            ItemPickupQuad(mesh,px,py,px+sx,py+sy,0xFFE4F6FF);
        }
    using Draw=HRESULT(STDMETHODCALLTYPE*)(void*,D3DPRIMITIVETYPE,D3DVERTEXTYPE,void*,DWORD,DWORD);
    reinterpret_cast<Draw>(draw.record->original[29])(draw.device,D3DPT_TRIANGLELIST,D3DVT_TLVERTEX,
        mesh.data(),static_cast<DWORD>(mesh.size()),D3DDP_DONOTCLIP);
}
static void DrawItemPickupHint(void* device,bool world) {
    if(!world || !SeparatePickupActive() || !ModernHasFocus() || g_controls_menu_active || g_campaign_menu_active ||
        InputField<int>(0x85A668)==1)return;
    ItemPickupVisual hint;
    AcquireSRWLockShared(&g_item_pickup_lock);hint=g_item_pickup_visual;ReleaseSRWLockShared(&g_item_pickup_lock);
    if(hint.stage!=InputField<int>(0x85A6B8))return;
    const bool coop=InputField<int>(0x8745A0)==2;
    const unsigned first=coop && g_camera_owner==1?1:0,last=coop && g_camera_owner<0?2:first+1;
    const double width=CameraField<float>(0x9D170),height=CameraField<float>(0x9D174);
    if(!isfinite(width) || !isfinite(height) || width<=0 || height<=0)return;
    const ff_camera::Pose view={CameraField<int32_t>(0x8677B0),CameraField<int32_t>(0x8677B4),CameraField<int32_t>(0x8677B8),
        CameraField<int16_t>(0x873CE0),CameraField<int16_t>(0x87459C)};
    for(unsigned p=first;p<last;++p) {
        const int slot=hint.slots[p];if(InputField<int16_t>(0x86A542+p*0xF4)<=0)continue;
        ff_camera::Point item;
        if(slot==-2) {
            auto object=WorldPickupObject(hint.fixtures[p]);
            if(!object || object!=hint.objects[p] || GuardField<int>(object,0)!=hint.types[p])continue;
            item={GuardField<int32_t>(object,4)/65536.0,GuardField<int32_t>(object,8)/65536.0-24,
                GuardField<int32_t>(object,12)/65536.0};
        } else {
            if(slot<0 || slot>=64)continue;const DWORD offset=0x870620+slot*36;
            if(InputField<int>(offset)!=hint.types[p] || InputField<int32_t>(offset+8)!=0)continue;
            item={InputField<int32_t>(offset+4)/65536.0,-24,InputField<int32_t>(offset+12)/65536.0};
        }
        ff_focus::ScreenPoint projected;
        if(!ff_focus::Project(item,view,CameraField<float>(0xAD214),CameraField<float>(0xAD234),
            CameraField<float>(0x20C50C),CameraField<float>(0x20C508),projected))
            projected={width/2,height,0}; // A close item at the feet must remain actionable in first person.
        projected.x=ff_camera::Clamp(projected.x,0,width);
        projected.y=ff_camera::Clamp(projected.y,0,height);
        const bool pad=coop?ModernKeyboardPlayer()!=int(p):InputHintsUsePad();
        const auto button=PlayerHintBinding(p,ff_controls::Pickup,pad);if(button=="UNBOUND")continue;
        const std::string prefix=coop && g_camera_owner<0?(p?"P2 ":"P1 "):"";
        const double separate=coop && g_camera_owner<0 && p==1 && hint.slots[0]==slot &&
            (slot!=-2 || hint.fixtures[0]==hint.fixtures[1])?
            (projected.y<height/2?18:-18)*height/480:0;
        CombatOverlayDraw draw(device);DrawItemPickupLabel(draw,prefix+button+"  PICK UP",projected.x,projected.y+separate,width,height);
    }
}
