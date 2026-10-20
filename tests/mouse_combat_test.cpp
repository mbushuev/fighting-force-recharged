#include "../src/mouse_combat.h"
#include <stdio.h>
static int failures;
#define CHECK(x) do { if (!(x)) { printf("FAIL line %d: %s\n", __LINE__, #x); ++failures; } } while (0)
int main() {
    CHECK(MergeMouseCombat(0, true, false, true) == 0x40); // punch
    CHECK(MergeMouseCombat(0, false, true, true) == 0x20); // kick
    CHECK(MergeMouseCombat(0x102, true, true, true) == 0x162); // preserve movement/jump
    CHECK(MergeMouseCombat(0, false, false, true) == 0); // release has no retained state
    CHECK(MergeMouseCombat(0x40, false, false, true) == 0x40); // keyboard punch survives
    CHECK(MergeMouseCombat(0x20, true, true, false) == 0x20); // focus/scene gate
    CHECK(MergeMouseCombat(0x80000000, false, true, true) == 0x80000020);
    printf("Mouse combat action merge: %s\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
