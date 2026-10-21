#pragma once
#include <limits.h>

// The native finish routine also runs for the hidden intermediate view. Only
// the final view may measure, limit and restart the game's frame clock.
static void __cdecl CooperativeClockStop() {
    if(g_coop_render_pass!=0)reinterpret_cast<void(__cdecl*)()>(g_base+0x23060)();
}
static int __cdecl CooperativeClockElapsed() {
    return g_coop_render_pass==0?INT_MAX:reinterpret_cast<int(__cdecl*)()>(g_base+0x23090)();
}
static void __cdecl CooperativeClockReset() {
    if(g_coop_render_pass!=0)reinterpret_cast<void(__cdecl*)()>(g_base+0x23030)();
}
static void __cdecl CooperativeClockStart() {
    if(g_coop_render_pass!=0)reinterpret_cast<void(__cdecl*)()>(g_base+0x23040)();
}
