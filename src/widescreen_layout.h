#pragma once
#include <d3d.h>
#include <stdint.h>
#include <string.h>

struct WideLayout {
    unsigned width=0,height=0;
    float ui_scale=1,ui_left=0;
    bool active=false;
};
struct WideModeChoice {
    unsigned aspect_width,aspect_height,min_height;
    unsigned width=0,height=0;
};
inline void ConsiderWideMode(WideModeChoice& choice,unsigned width,unsigned height) {
    if(!choice.aspect_width || !choice.aspect_height || width<320 || width>8192 ||
        height<choice.min_height || height>4096)return;
    uint64_t actual=uint64_t(width)*choice.aspect_height,requested=uint64_t(height)*choice.aspect_width;
    uint64_t difference=actual>requested?actual-requested:requested-actual;
    if(difference*500>requested)return; // <= 0.2% aspect error (e.g. 1366x768)
    if(!choice.width || uint64_t(width)*height<uint64_t(choice.width)*choice.height){choice.width=width;choice.height=height;}
}
inline WideLayout MakeWideLayout(unsigned native_width,unsigned height,unsigned aspect_width,unsigned aspect_height) {
    WideLayout layout;layout.width=native_width;layout.height=height;
    if(native_width<320 || height<240 || height>4096 || !aspect_height ||
        uint64_t(aspect_width)*3<=uint64_t(aspect_height)*4 ||
        uint64_t(aspect_width)>uint64_t(aspect_height)*4)return layout;
    unsigned width=static_cast<unsigned>((uint64_t(height)*aspect_width+aspect_height/2)/aspect_height);
    if(width>8192)return layout;
    layout.width=width;layout.ui_scale=float(height)/240;
    layout.ui_left=(float(width)-320*layout.ui_scale)*0.5f;
    layout.active=layout.ui_left>0.25f;
    return layout;
}

// Retail 0x424ed0 consumes this polygon immediately, including the native CPU
// clipping path. ONLY kinds 5..8 read bit 0 of flags to normalize 320x240
// coordinates. Kinds 0..2 ignore that field; their pooled records may retain
// an old UI bit while holding already projected world geometry or shadows.
struct RetailScreenPolygon {
    DWORD next;
    D3DTLVERTEX vertices[8];
    float sort_depth;
    DWORD count,texture,kind,blend,flags;
};
static_assert(sizeof(RetailScreenPolygon)==0x11c,"Retail polygon layout");
inline bool CenterWideScreenPolygon(const WideLayout& layout,const RetailScreenPolygon* source,RetailScreenPolygon& copy,bool background=false) {
    if(!layout.active || !source || source->kind<5 || source->kind>8 ||
        !(source->flags&1) || !source->count || source->count>8)return false;
    copy=*source;copy.flags&=~1u;
    if(source->kind==5 && source->count==4) {
        float left=source->vertices[0].sx,right=left,top=source->vertices[0].sy,bottom=top;
        for(unsigned i=1;i<4;++i) {
            float x=source->vertices[i].sx,y=source->vertices[i].sy;
            if(x<left)left=x;if(x>right)right=x;if(y<top)top=y;if(y>bottom)bottom=y;
        }
        if(left<=0 && right>=320 && top<=0 && bottom>=240)background=true;
    }
    for(unsigned i=0;i<copy.count;++i) {
        copy.vertices[i].sx=source->vertices[i].sx*(background?float(layout.width)/320:layout.ui_scale)+(background?0:layout.ui_left);
        copy.vertices[i].sy=source->vertices[i].sy*layout.ui_scale;
    }
    return true;
}
