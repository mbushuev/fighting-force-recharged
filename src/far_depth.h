#pragma once
#include <d3d.h>
#include <string.h>
#include <vector>

// Retail uses this exact value as its painter-ordered far-plane sentinel.
// dgVoodoo reconstructs clip Z as sz * clip W from TL vertices. A value just
// below one can round to different depths after that multiply/divide, exposing
// the black background between otherwise coplanar triangles. At exactly one,
// clip Z equals clip W. Preserve every ordinary depth and every texture field.
inline bool IsRetailFarDepth(float depth) {
    DWORD bits; memcpy(&bits, &depth, sizeof(bits));
    return bits == 0x3F7FFF58;
}
inline void* StableFarDepthVertices(D3DVERTEXTYPE type, void* vertices, DWORD count,
    std::vector<D3DTLVERTEX>& storage, DWORD* changed) {
    *changed = 0;
    if (type != D3DVT_TLVERTEX || !vertices || !count || count > 65535) return vertices;
    auto points = static_cast<D3DTLVERTEX*>(vertices);
    for (DWORD index=0; index<count; ++index) if (IsRetailFarDepth(points[index].sz)) ++*changed;
    if (!*changed) return vertices;
    storage.resize(count);
    memcpy(storage.data(), points, count*sizeof(D3DTLVERTEX));
    for (auto& vertex:storage) if (IsRetailFarDepth(vertex.sz)) vertex.sz=1.0f;
    return storage.data();
}
