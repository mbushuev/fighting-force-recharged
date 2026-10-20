#pragma once
#include <windows.h>
inline RECT RetailInitialRenderRect(unsigned width,unsigned height) {
    if(width<320 || height<240 || width>8192 || height>8192){width=800;height=600;}
    unsigned unit=width/4;if(height/3<unit)unit=height/3;
    // The retail initializer treats RECT right/bottom as inclusive and adds
    // one. Keep its 4:3 render size independent of the physical borderless HWND.
    return RECT{0,0,static_cast<LONG>(unit*4-1),static_cast<LONG>(unit*3-1)};
}
