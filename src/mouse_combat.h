#pragma once
#include <stdint.h>

// Merge semantic retail actions after native input polling. No scan codes,
// keyboard bindings, button latches or synthetic window messages are changed.
inline uint32_t MergeMouseCombat(uint32_t native, bool left, bool right, bool active) {
    return native | (active && left ? 0x40u : 0u) | (active && right ? 0x20u : 0u);
}
