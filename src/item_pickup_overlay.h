#pragma once
static void DrawItemPickupLabel(CombatOverlayDraw& draw,const std::string& text,double x,double y,double width,double height) {
    if(!draw.ready || text.empty() || text.size()>60)return;
    const double sy=ff_camera::Clamp(height/480,.75,3),sx=sy*(g_coop_render_pass>=0?2:1);
    const double w=OverlayLabelWidth(text,sx),h=kOverlayLabelHeight*sy;
    if(width<w+8*sx || height<h+8*sy)return;
    const double left=ff_camera::Clamp(x-w/2,4*sx,width-w-4*sx);
    const double top=ff_camera::Clamp(y-h-7*sy,4*sy,height-h-4*sy);
    DrawOverlayLabel(draw,text,left,top,sx,sy);
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
