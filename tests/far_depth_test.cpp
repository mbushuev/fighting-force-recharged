#include "../src/far_depth.h"
#include <stdio.h>
static int failures;
#define CHECK(condition) do { if (!(condition)) { printf("FAIL line %d: %s\n", __LINE__, #condition); ++failures; } } while (0)
int main() {
    D3DTLVERTEX original[4] = {};
    for (int i=0;i<4;++i) {
        original[i].sx=17.25f+i; original[i].sy=23.75f-i;
        original[i].sz=i<2 ? 0.99999f : (i==2 ? 0.125f : 0.0001f);
        original[i].rhw=0.00015f*(i+1); original[i].color=0xFFAABBCC;
        original[i].specular=0x66000000; original[i].tu=0.001953125f+i*0.125f;
        original[i].tv=0.501953125f-i*0.0625f;
    }
    D3DTLVERTEX expected[4], before[4];
    memcpy(expected,original,sizeof(original)); memcpy(before,original,sizeof(original));
    expected[0].sz=expected[1].sz=1.0f;
    std::vector<D3DTLVERTEX> storage;
    DWORD changed;
    void* result=StableFarDepthVertices(D3DVT_TLVERTEX,original,4,storage,&changed);
    CHECK(changed==2 && result!=original);
    CHECK(!memcmp(original,before,sizeof(original)));
    CHECK(!memcmp(result,expected,sizeof(expected)));
    original[0].sz=original[1].sz=0.5f;
    result=StableFarDepthVertices(D3DVT_TLVERTEX,original,4,storage,&changed);
    CHECK(changed==0 && result==original);
    original[0].sz=0.99999f;
    result=StableFarDepthVertices(D3DVT_VERTEX,original,4,storage,&changed);
    CHECK(changed==0 && result==original);
    CHECK(StableFarDepthVertices(D3DVT_TLVERTEX,nullptr,4,storage,&changed)==nullptr && changed==0);
    CHECK(!IsRetailFarDepth(0.99998f) && !IsRetailFarDepth(1.0f) && IsRetailFarDepth(0.99999f));
    printf("Far depth isolation checks: %s\n",failures ? "FAILED" : "PASS");
    return failures ? 1 : 0;
}
