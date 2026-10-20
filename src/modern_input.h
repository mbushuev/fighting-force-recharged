#pragma once
#include <stdint.h>

namespace ff_input {
enum Action : uint32_t {
    Up=1, Down=2, Left=4, Right=8, Kick=0x20, Punch=0x40,
    Jump=0x80, Backfist=0x100, NativeCamera=0x200, Run=0x400,
    Confirm=0x20, Cancel=0x40, Pause=0x1000
};
struct Keyboard {
    bool up=false, down=false, left=false, right=false;
    bool punch=false, kick=false, jump=false, backfist=false, run=false;
    bool pause=false, confirm=false, cancel=false;
};
inline uint32_t Directions(bool up, bool down, bool left, bool right) {
    return (up!=down ? (up ? Up : Down) : 0) |
           (left!=right ? (left ? Left : Right) : 0);
}
inline uint32_t Gameplay(const Keyboard& k) {
    return Directions(k.up,k.down,k.left,k.right) | (k.punch?Punch:0) |
        (k.kick?Kick:0) | (k.jump?Jump:0) | (k.backfist?Backfist:0) | (k.run?Run:0);
}
inline uint32_t Menu(const Keyboard& k, bool space_confirm=true) {
    return Directions(k.up,k.down,k.left,k.right) |
        ((k.confirm || (space_confirm && k.jump)) ? Confirm:0) | (k.cancel?Cancel:0);
}
// A held attack/confirm must not answer a newly opened modal prompt. Releasing
// an input arms its next press; focus loss starts a new neutral boundary.
struct ConfirmGate {
    bool accept=false, cancel=false;
    void Begin(bool a,bool c) { accept=a;cancel=c; }
    int Read(bool a,bool c) {
        const bool yes=a&&!accept, no=c&&!cancel;accept=a;cancel=c;
        return no?0x31:yes?0x15:-1;
    }
};
struct PauseGate {
    bool held=false;
    uint32_t Read(bool down,bool inside) {
        const bool fresh=down&&!held;held=down;
        return inside ? (fresh ? Left|Confirm:0) : (down ? Pause:0);
    }
};
} // namespace ff_input
