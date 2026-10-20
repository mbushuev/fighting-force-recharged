#pragma once
#include <windows.h>
#include <d3d.h>
#include <vector>

// RGB565 color-key images have binary coverage. Their filtered RGB is already
// weighted by coverage when transparent texels are black. Preserve this as
// premultiplied alpha instead of testing the filtered RGB against the key.
inline DWORD Keyed565ToArgb(WORD pixel, WORD key_low, WORD key_high) {
    if(pixel>=key_low && pixel<=key_high)return 0;
    DWORD r=(pixel>>11)&31, g=(pixel>>5)&63, b=pixel&31;
    return 0xFF000000 | (((r<<3)|(r>>2))<<16) | (((g<<2)|(g>>4))<<8) | ((b<<3)|(b>>2));
}
inline DWORD PremultiplyVertexColor(DWORD color) {
    DWORD alpha=color>>24;
    if(alpha==255)return color;
    DWORD r=(((color>>16)&255)*alpha+127)/255;
    DWORD g=(((color>>8)&255)*alpha+127)/255;
    DWORD b=((color&255)*alpha+127)/255;
    return (alpha<<24)|(r<<16)|(g<<8)|b;
}
inline DWORD KeyedVertexColor(DWORD color,bool native_vertex_alpha) {
    // RGB color-key draws with blending disabled ignore the diffuse alpha.
    // Do not turn that unused byte into opacity when changing texture format.
    return native_vertex_alpha ? PremultiplyVertexColor(color) : color|0xFF000000;
}

inline std::vector<DWORD> DilateKeyedRgb(const std::vector<DWORD>& pixels,DWORD width,DWORD height) {
    std::vector<DWORD> result=pixels;
    // Bilinear sampling reaches one texel across a cutout boundary. Extend
    // neighboring solid colors into that transparent border, retaining alpha
    // zero. Never change solid texels or the original game texture.
    for(DWORD y=0;y<height;++y)for(DWORD x=0;x<width;++x) {
        if(pixels[y*width+x]>>24)continue;
        DWORD r=0,g=0,b=0,count=0;
        for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx) {
            int nx=static_cast<int>(x)+dx,ny=static_cast<int>(y)+dy;
            if(nx<0 || ny<0 || nx>=static_cast<int>(width) || ny>=static_cast<int>(height))continue;
            DWORD sample=pixels[ny*width+nx];if(!(sample>>24))continue;
            r+=(sample>>16)&255;g+=(sample>>8)&255;b+=sample&255;++count;
        }
        if(count)result[y*width+x]=((r/count)<<16)|((g/count)<<8)|(b/count);
    }
    return result;
}
