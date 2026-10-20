# Modern controls

The wrapper adds semantic actions to living single-player gameplay before the
native actor update. Movement retains the game's eight directions relative to
the current camera; native action buffering and combinations remain intact.
Retail files and saved binding tables are not edited. Cooperative play and
recorded demonstrations retain native gameplay input.

## Keyboard and mouse

| Action | Default input |
| --- | --- |
| Move relative to camera | W / A / S / D |
| Punch | Left mouse button or J |
| Kick | Right mouse button or K |
| Jump | Space |
| Backfist | E |
| Run | Hold left Shift |
| Look | Mouse movement |
| Camera distance | Mouse wheel |
| Recenter behind character | Middle mouse button |
| Pause / continue | Esc or P |
| Open quit confirmation | Q |
| Accept quit | Enter or Y |
| Cancel quit | Esc or N |

Menus accept WASD or the original arrow keys, Enter/Space to confirm and Esc
to go back. The native pause menu uses horizontal choices; A/D changes the
choice and Enter confirms it. Esc/P continues directly. Space is reserved for
jump in gameplay and no longer opens pause. Original F1 filtering and F2 fog
shortcuts remain available.

Set `ModernKeyboard=0` under `[Input]` to restore original keyboard behavior.
`MouseCombat` is separate. Camera controls require `FreeOrbit=1`.

## Xbox / XInput-compatible controller

| Action | Default input |
| --- | --- |
| Move relative to camera | Left stick or D-pad |
| Punch | X |
| Kick | Y |
| Jump | A |
| Backfist | B |
| Run | Hold RB or left stick click (L3) |
| Look | Right stick |
| Recenter behind character | Right stick click (R3) |
| Increase / decrease camera distance | LT / RT |
| Pause / continue | Start / Menu |
| Open quit confirmation | View / Back |
| Menu navigation | Left stick or D-pad |
| Confirm menu / quit | A; Start also confirms frontend menus |
| Back / cancel / continue from pause | B |

The wrapper resolves the system XInput runtime and reconnects automatically.
`ControllerIndex=-1` chooses the first available slot, retaining it while
connected; `0..3` selects a fixed slot. The same slot supplies movement and
camera input. Left/right stick deadzones default to 7849/8689. Right-stick
speed uses the existing camera sensitivity setting. Triggers have a small
neutral threshold and adjust camera distance continuously.

Release buttons, sticks and triggers after connecting, scene changes or focus
changes. A held button cannot immediately select the next screen or answer a
new quit prompt. A connected XInput pad replaces player one's duplicate legacy
joystick actions so old button numbering and right-stick axes do not interfere.
Keyboard and mouse remain available. Set `Gamepad=0` to disable modern pad input.

Direct USB/Bluetooth PlayStation HID input is not implemented. Such controllers
currently need to be presented to Windows as an XInput-compatible device by an
existing controller layer. The wrapper does not install drivers or remappers.
Hardware-specific verification remains separate from deterministic mapper and
native-boundary tests; no physical controller playthrough is claimed.
