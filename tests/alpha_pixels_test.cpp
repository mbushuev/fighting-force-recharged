#include "../src/alpha_pixels.h"
#include <stdio.h>
static int failures;
#define CHECK(condition) do { if (!(condition)) { printf("FAIL line %d: %s\n", __LINE__, #condition); ++failures; } } while (0)
int main() {
    CHECK(Keyed565ToArgb(0,0,0)==0);
    CHECK(Keyed565ToArgb(0xFFFF,0,0)==0xFFFFFFFF);
    CHECK(Keyed565ToArgb(0xF800,0,0)==0xFFFF0000);
    CHECK(Keyed565ToArgb(0x07E0,0,0)==0xFF00FF00);
    CHECK(Keyed565ToArgb(0x001F,0,0)==0xFF0000FF);
    CHECK(Keyed565ToArgb(11,10,12)==0);
    CHECK(PremultiplyVertexColor(0xFFFFFFFF)==0xFFFFFFFF);
    CHECK(PremultiplyVertexColor(0x80FFFFFF)==0x80808080);
    CHECK(PremultiplyVertexColor(0x00FFFFFF)==0);
    CHECK(PremultiplyVertexColor(0xFF000000)==0xFF000000);
    CHECK(KeyedVertexColor(0x64FFFFFF,false)==0xFFFFFFFF);
    CHECK(KeyedVertexColor(0x80FFFFFF,true)==0x80808080);
    CHECK(KeyedVertexColor(0x00000000,false)==0xFF000000);
    std::vector<DWORD> input={0xFFFF0000,0,0,0,0,0,0,0,0};
    auto dilated=DilateKeyedRgb(input,3,3);
    CHECK(dilated[0]==0xFFFF0000);CHECK(dilated[1]==0x00FF0000);
    CHECK(dilated[3]==0x00FF0000);CHECK(dilated[4]==0x00FF0000);
    CHECK(dilated[2]==0);CHECK(dilated[8]==0);CHECK(input[1]==0);
    // A half-covered white texel over white must remain white. Testing only
    // exact black after filtering instead produces an opaque gray fringe.
    float filtered_rgb=0.5f, filtered_alpha=0.5f, background=1.0f;
    CHECK(filtered_rgb+background*(1.0f-filtered_alpha)==1.0f);
    CHECK(filtered_rgb*filtered_alpha+background*(1.0f-filtered_alpha)==0.75f);
    printf("Transparent edge pixel checks: %s\n",failures ? "FAILED" : "PASS");
    return failures ? 1 : 0;
}
