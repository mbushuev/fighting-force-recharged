#include "../src/widescreen_layout.h"
#include <stdio.h>
#include <math.h>
static int failures;
#define CHECK(x) do {if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);++failures;}}while(0)
static bool Near(float a,float b){return fabsf(a-b)<0.001f;}
int main() {
    WideModeChoice choice={16,10,600};
    ConsiderWideMode(choice,800,600);CHECK(!choice.width);
    ConsiderWideMode(choice,640,400);CHECK(!choice.width);
    ConsiderWideMode(choice,2880,1800);ConsiderWideMode(choice,1280,800);
    ConsiderWideMode(choice,1680,1050);CHECK(choice.width==1280 && choice.height==800);
    choice={16,9,600};ConsiderWideMode(choice,1366,768);ConsiderWideMode(choice,1280,720);
    CHECK(choice.width==1280 && choice.height==720);
    auto layout=MakeWideLayout(800,600,2880,1800);
    CHECK(layout.active && layout.width==960 && layout.height==600);
    // A square HUD glyph must stay square; centering and spacing must not alter
    // color, depth, perspective, texture coordinates or the source polygon.
    RetailScreenPolygon source={};source.count=4;source.flags=1;source.texture=17;source.kind=6;
    const float x[]={16,32,32,16},y[]={20,20,36,36};
    for(int i=0;i<4;++i){source.vertices[i].sx=x[i];source.vertices[i].sy=y[i];source.vertices[i].rhw=0.125f;source.vertices[i].sz=0.9f;source.vertices[i].tu=float(i)/4;source.vertices[i].color=0x8fedcba9;}
    auto original=source;RetailScreenPolygon copy={};
    CHECK(CenterWideScreenPolygon(layout,&source,copy));CHECK(!memcmp(&original,&source,sizeof(source)));
    CHECK(Near(copy.vertices[0].sx,120) && Near(copy.vertices[0].sy,50));
    CHECK(Near(copy.vertices[1].sx-copy.vertices[0].sx,copy.vertices[2].sy-copy.vertices[1].sy));
    for(int i=0;i<4;++i){CHECK(copy.vertices[i].rhw==source.vertices[i].rhw && copy.vertices[i].sz==source.vertices[i].sz);CHECK(copy.vertices[i].tu==source.vertices[i].tu && copy.vertices[i].color==source.vertices[i].color);}
    CHECK(copy.flags==0 && copy.texture==17 && copy.kind==6);
    // The retail never initializes the normalized bit for kind 2 world
    // polygons / kinds 0..1 shadows. A reused UI bit must not move these.
    const DWORD world_kinds[]={0,1,2,3,4,9};
    for(DWORD kind:world_kinds) {
        source.kind=kind;auto before=source;
        CHECK(!CenterWideScreenPolygon(layout,&source,copy));
        CHECK(!CenterWideScreenPolygon(layout,&source,copy,true));
        CHECK(!memcmp(&before,&source,sizeof(source)));
    }
    source.kind=6;
    CHECK(CenterWideScreenPolygon(layout,&source,copy,true));
    CHECK(Near(copy.vertices[0].sx,48)); // background canvas uses all 960 pixels
    source.kind=5;source.vertices[0].sx=source.vertices[3].sx=0;
    source.vertices[1].sx=source.vertices[2].sx=320;
    source.vertices[0].sy=source.vertices[1].sy=0;source.vertices[2].sy=source.vertices[3].sy=242;
    CHECK(CenterWideScreenPolygon(layout,&source,copy));CHECK(copy.vertices[0].sx==0 && copy.vertices[1].sx==960);
    source.flags=0;CHECK(!CenterWideScreenPolygon(layout,&source,copy)); // world and full-screen fades
    source.flags=1;source.count=9;CHECK(!CenterWideScreenPolygon(layout,&source,copy));
    CHECK(!MakeWideLayout(800,600,4,3).active);CHECK(!MakeWideLayout(800,600,3,4).active);
    CHECK(!MakeWideLayout(800,600,0,0).active);CHECK(!MakeWideLayout(800,600,10,1).active);
    // Fixed vertical focal length: an object keeps its projected size while a
    // point culled by the old right edge enters the expanded horizontal view.
    const float focal=640,z=100,xworld=70;
    float old_x=400+focal*xworld/z,wide_x=480+focal*xworld/z;
    CHECK(old_x>800 && wide_x<960);CHECK(Near((wide_x-480),(old_x-400)));
    layout=MakeWideLayout(800,600,16,9);CHECK(layout.active && layout.width==1067);
    CHECK(Near(layout.ui_left+160*layout.ui_scale,float(layout.width)/2));
    layout=MakeWideLayout(800,600,32,9);CHECK(layout.active && layout.width==2133);
    printf("Widescreen projection / UI invariants: %s\n",failures?"FAILED":"PASS");return failures?1:0;
}
