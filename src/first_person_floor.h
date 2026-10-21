#pragma once

struct FirstPersonFloorGrid {
    int16_t vertices[33*33*4]={};
    FirstPersonFloorGrid() {
        for(int row=0;row<33;++row)for(int column=0;column<33;++column) {
            auto v=vertices+(row*33+column)*4;
            v[0]=int16_t((column-16)*256);v[2]=int16_t((row-16)*256);
        }
    }
};
static int __cdecl FirstPersonFloorForward(const FirstPersonPropModel* model,int flags,int) {
    return reinterpret_cast<int(__cdecl*)(const int16_t*,unsigned,int)>(g_base+0x25C00)(
        model->faces,model->face_count,flags);
}
static int __cdecl FirstPersonFloorFaces(const int16_t* faces,unsigned count,int flags) {
    auto native=reinterpret_cast<int(__cdecl*)(const int16_t*,unsigned,int)>(g_base+0x25C00);
    if(!FirstPersonActive() || count!=1024 || faces!=reinterpret_cast<const int16_t*>(g_base+0x9A170))
        return native(faces,count,flags);
    // The floor has a dedicated 33x33 vertex projector, separate from models.
    // Its active matrix already includes cell origin and camera translation.
    // Reuse the native tile/material records, including deliberately absent cells.
    static const FirstPersonFloorGrid grid;
    const FirstPersonPropModel floor={grid.vertices,33*33,faces,count};
    return FirstPersonProjectModel(&floor,flags,0,false,FirstPersonFloorForward,true);
}
