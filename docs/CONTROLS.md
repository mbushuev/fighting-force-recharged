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
| Open quit confirmation | F10 |
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
| Run | Hold RB / L3 while moving |
| Look | Right stick |
| Recenter behind character | Right stick click (R3) |
| Increase / decrease camera distance | LT / RT |
| Pause / continue | Start / Menu |
| Open quit confirmation | View / Back |
| Menu navigation | Left stick or D-pad |
| Confirm menu / quit | A; Start also confirms frontend menus |
| Skip intro/movie | A, B or Start |
| Back / cancel / continue from pause | B; opens quit on the main menu |

The wrapper resolves the system XInput runtime and reconnects automatically.
`ControllerIndex=-1` chooses the first available slot, retaining it while
connected; `0..3` selects a fixed slot. The same slot supplies movement and
camera input. Left/right stick deadzones default to 7849/8689. Right-stick
speed uses the existing camera sensitivity setting. Triggers have a small
neutral threshold and adjust camera distance continuously. The default
`TriggerZoomUnitsPerSecond=720` under `[Camera]` is three times the initial
trigger zoom rate; mouse wheel steps remain unchanged.

`AnalogMovement=1` under `[Input]` gives the left stick continuous speed for
unarmed ground movement. After the radial deadzone, the full stick range scales
from a slow step to the character's normal walking speed. Running requires
holding RB/L3; stick deflection alone never sets the run action, including
during attacks. Releasing the run button immediately clears that action and
caps movement at walking speed. The wrapper reads each character's native
animation displacement
to retain its normal maximum speeds, then passes the adjusted movement proposal
through the original collision solver. Attacks, jumps, knockback, carried
objects, cooperative play and demos retain native movement. D-pad or simultaneous
keyboard movement also uses native speed. Set `AnalogMovement=0` to use normal
walking speed throughout the stick range, with RB/L3 still required to run.
Animation blending and arbitrary movement
angles are not added; the original eight direction choices remain.

Release buttons, sticks and triggers after connecting, scene changes or focus
changes. A held button cannot immediately select the next screen or answer a
new quit prompt. A connected XInput pad replaces player one's duplicate legacy
joystick actions so old button numbering and right-stick axes do not interfere.
Keyboard and mouse remain available. Set `Gamepad=0` to disable modern pad input.
F10 replaces the original Q quit shortcut when modern keyboard input is enabled.
On the main menu, press B or View, then A to exit the application; B cancels the
prompt. In gameplay, View opens the return-to-menu prompt. In submenus B goes back.
Movies poll the selected controller independently of gameplay input. Release a
held button before pressing A, B or Start to skip; native playback cleanup and
transition fades still run.

Direct USB/Bluetooth PlayStation HID input is not implemented. Such controllers
currently need to be presented to Windows as an XInput-compatible device by an
existing controller layer. The wrapper does not install drivers or remappers.
Hardware-specific verification remains separate from deterministic mapper and
native-boundary tests; a full controller playthrough is not claimed.
