# Modern controls

The wrapper adds semantic actions to living single-player gameplay before the
native actor update. Movement retains the game's eight directions relative to
the current camera; native action buffering and combinations remain intact.
Retail files and saved binding tables are not edited. Recorded demonstrations
retain native gameplay input. Experimental two-player input and dynamic camera
behavior are described in [Cooperative camera](COOPERATIVE.md).

## In-game settings

Open **Options > Controller Settings**, or press **F6** on the main menu.
Select each player's device, optional fixed gamepad slot, buttons and cooperative
camera mode. Assigning keyboard/mouse to one player removes that assignment
from the other; two gamepads always use distinct XInput slots. Automatic slot
selection also reserves the other player's explicit slot. Single-player keeps
keyboard/mouse and a controller available and uses player one's button layout.

Use arrows/WASD or a gamepad's left stick/D-pad to navigate. Left/right changes
a value; Enter/A selects. On a buttons page, Enter/A starts capture, then press
the desired key, mouse button, gamepad button or trigger. Keyboard actions have
primary and alternate bindings, selected with left/right. Delete/Backspace clears
the selected binding. A button already assigned to another action is rejected;
clear its previous assignment first. Esc or View+Start cancels capture. The reset
row restores that player's default buttons. Movement/look stick axes and mouse
look/wheel remain fixed; gameplay action buttons and trigger zoom are rebindable.

**Save and Back** applies the draft immediately and saves only the wrapper INI.
**Cancel** or Esc/B on the settings root discards the draft. Returning to the
native menu requires controls to return to neutral. Native menu navigation and
confirm/back buttons stay available independently of gameplay remapping.
Footer instructions use a separate blue tint. Keyboard navigation and capture
poll the game's existing DirectInput keyboard, so exclusive keyboard acquisition
does not depend on Windows asynchronous key state. Shared frontend menus remain
accessible from keyboard even when both cooperative players use gamepads.

Saved layouts use `[Player1Bindings]` and `[Player2Bindings]` with `Key`, `Alt`
and `Pad` prefixes and the action names `Forward`, `Backward`, `Left`, `Right`,
`Punch`, `Kick`, `Jump`, `Backfist`, `Run`, `View`, `Recenter`, `Pause`, `Quit`,
`ZoomIn`, `ZoomOut` and `Block`. Keyboard values are physical scan codes, with `0x100`
marking extended keys; mouse buttons are `0x1001..0x1005`. Pad values use XInput
button masks, `0x10000` for LT and `0x20000` for RT. Zero means unbound.
`[Controls] BindingsVersion=1` marks the saved format. Existing settings without
this marker retain their previous first-selected-controller slot interpretation
until saved. Original game binding tables and save files remain untouched.

## Keyboard and mouse

| Action | Default input |
| --- | --- |
| Move relative to camera | W / A / S / D |
| Punch | Left mouse button or J |
| Kick | Right mouse button or K |
| Jump | Space |
| Backfist | E |
| Experimental block (when enabled) | Hold Q |
| Run | Hold left Shift |
| Look | Mouse movement |
| Toggle first-person / third-person view | Tab |
| Camera distance | Mouse wheel |
| Recenter behind character | Middle mouse button |
| Pause / continue | Esc or P |
| Open quit confirmation | F10; Esc also opens it on the main menu |
| Accept quit | Enter or Y |
| Cancel quit | Esc or N |

Menus accept WASD or the original arrow keys, Enter/Space to confirm and Esc
to go back. Esc on the main menu opens quit confirmation; Enter/Y accepts and
Esc/N cancels. Release Esc after returning from a submenu before pressing it
again to quit. The native pause menu uses horizontal choices; A/D changes the
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
| Experimental block (when enabled) | Hold LB |
| Run | Hold RB while moving |
| Toggle first-person / third-person view | L3 |
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
Automatic assignment chooses the first available slot, retaining it while
connected; the menu's slots 1..4 correspond to INI values `0..3`. See the
cooperative device settings for per-player slot ownership. The same slot supplies movement and
camera input. Left/right stick deadzones default to 7849/8689. Right-stick
speed uses the existing camera sensitivity setting. Triggers have a small
neutral threshold and adjust camera distance continuously. The default
`TriggerZoomUnitsPerSecond=720` under `[Camera]` is three times the initial
trigger zoom rate; mouse wheel steps remain unchanged.

`AnalogMovement=1` under `[Input]` gives the left stick continuous speed for
unarmed ground movement. After the radial deadzone, the full stick range scales
from a slow step to the character's normal walking speed. Running requires
holding RB; stick deflection alone never sets the run action, including
during attacks. Releasing the run button immediately clears that action and
caps movement at walking speed. The wrapper reads each character's native
animation displacement
to retain its normal maximum speeds, then passes the adjusted movement proposal
through the original collision solver. Attacks, jumps, knockback, carried
objects, cooperative play and demos retain native movement. D-pad or simultaneous
keyboard movement also uses native speed. Set `AnalogMovement=0` to use normal
walking speed throughout the stick range, with RB still required to run.
Animation blending and arbitrary movement
angles are not added; the original eight direction choices remain.

`FirstPerson=1` under `[Camera]` enables Tab/L3 during living single-player
gameplay. The camera follows the animated skull position, including jump and
combat movement, while mouse/right-stick yaw and pitch remain under player
control. The local head is excluded; the original arms, torso, legs and held
objects remain. Looking down reveals the body. Neutral walking/idle poses face
the view; attack, knockdown and jump animations retain native direction.
Wheel/LT/RT change distance only in third person. Switching back preserves
that distance and uses the current viewing direction. R3/middle mouse recenters
either view. Disable `FirstPerson` to retain orbit without the view toggle.

First-person look has separate `[Camera]` settings:
`FirstPersonMouseHorizontalMilliDegrees=188`,
`FirstPersonMouseVerticalMilliDegrees=150` and
`FirstPersonStickDegreesPerSecond=150`. These defaults turn approximately 25%
faster than orbit defaults. `InvertY` applies to both views.

Release buttons, sticks and triggers after connecting, scene changes or focus
changes. A held button cannot immediately select the next screen or answer a
new quit prompt. A connected XInput pad replaces duplicate legacy joystick actions
so old button numbering and right-stick axes do not interfere. Saving the controls
menu also replaces legacy keyboard gameplay bindings with the chosen layout.
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

## Experimental player guard

Set `PlayerGuard=1` under `[Combat]` and restart to enable the experimental
unarmed guard for Hawk, Mace, Smasher and Alana. It is disabled by default.
Hold the remappable **Block** action (Q/LB) while idle or walking/running
to enter a stationary guard. Release it to lower the arms. Attacks, jumps,
slides, grabs and damage reactions cannot be cancelled into guard. Release and
press Block again after an interruption or scene/focus change.

The guard stops ordinary unarmed melee within a 135-degree frontal sector,
using the character's body direction. A blocked hit causes a guard impact pose
and effect without damage or knockback. Side/rear hits, weapons, projectiles,
grabs, throws and special attack paths retain native behavior. In first person,
the guarding body follows the view direction. Guard input follows each player's
existing device assignment; solo play accepts keyboard and gamepad together.

Private entry, hold, impact and exit clips are built in memory from the reviewed
original hero and generic enemy archives. Hawk, Mace and Alana retain their
native idle root, legs and accessory angles while adapting the donor upper
body to the destination skeleton. Smasher uses the matching large-male mapping.
Unknown archives disable the affected hero's guard. No original game files or
animation tables on disk are changed. Q/LB
defaults do not take over an existing saved assignment to another action; bind
Block explicitly in that case. Smasher's frontal protection and rear damage
have been verified in gameplay. Native decoder and isolated runtime checks cover
all four heroes; a complete visual, first-person and level/co-op acceptance pass
remains pending. Guarding with weapons is not supported.
