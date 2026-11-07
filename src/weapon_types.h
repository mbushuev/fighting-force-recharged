#pragma once

namespace ff_weapon {
// BAZOOK.MOD is registered as 0x1FA. The native player weapon selector
// also fires the 0x1FB variant through the same projectile routine.
inline bool Launcher(int type) {return type>=0x1F9 && type<=0x1FB;}
inline bool Firearm(int type) {return (type>=0x1F3 && type<=0x1F8) || Launcher(type) || type==0x203;}
}
