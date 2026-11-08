#pragma once
// Asset-free bitmap labels use one draw over the current scene.
static constexpr int kOverlayLabelHeight=13;
static double OverlayLabelWidth(const std::string& text,double scale) {return (text.size()*6+8)*scale;}

static const BYTE* OverlayGlyph(char c) {
    static const char alphabet[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+-.v";
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
        {0,4,4,31,4,4,0},{0,0,0,31,0,0,0},{0,0,0,0,0,4,4},{0,0,17,17,17,10,4}};
    if(c>='a' && c<='z' && c!='v')c=static_cast<char>(c-'a'+'A');
    const auto found=strchr(alphabet,c);return found && c?rows[found-alphabet]:nullptr;
}
static void OverlayQuad(std::vector<D3DTLVERTEX>& mesh,double left,double top,double right,double bottom,DWORD color) {
    const float corners[][2]={{float(left),float(top)},{float(right),float(top)},{float(left),float(bottom)},
        {float(left),float(bottom)},{float(right),float(top)},{float(right),float(bottom)}};
    for(const auto& point:corners){D3DTLVERTEX v={};v.sx=point[0];v.sy=point[1];v.sz=.01f;v.rhw=1;v.color=color;mesh.push_back(v);}
}
static void DrawOverlayLabel(CombatOverlayDraw& draw,const std::string& text,double left,double top,double sx,double sy) {
    if(!draw.ready)return;
    const double w=OverlayLabelWidth(text,sx),h=kOverlayLabelHeight*sy;
    static thread_local std::vector<D3DTLVERTEX> mesh;mesh.clear();
    OverlayQuad(mesh,left,top,left+w,top+h,0xC0101820);
    OverlayQuad(mesh,left,top+h-sy,left+w,top+h,0xD080D0FF);
    for(unsigned i=0;i<text.size();++i)if(const auto rows=OverlayGlyph(text[i]))
        for(unsigned row=0;row<7;++row)for(unsigned col=0;col<5;++col)if(rows[row]&(16>>col)) {
            const double px=left+(4+i*6+col)*sx,py=top+(3+row)*sy;
            OverlayQuad(mesh,px,py,px+sx,py+sy,0xFFE4F6FF);
        }
    using Draw=HRESULT(STDMETHODCALLTYPE*)(void*,D3DPRIMITIVETYPE,D3DVERTEXTYPE,void*,DWORD,DWORD);
    reinterpret_cast<Draw>(draw.record->original[29])(draw.device,D3DPT_TRIANGLELIST,D3DVT_TLVERTEX,
        mesh.data(),static_cast<DWORD>(mesh.size()),D3DDP_DONOTCLIP);
}
