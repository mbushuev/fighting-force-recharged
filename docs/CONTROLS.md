# Modern controls

## Controller vibration

**Options > Controller Settings > Vibration** enables or disables short XInput
motor pulses. Choose **Save and Back** to apply the setting; Cancel preserves
the previous value. The default is ON (`Input/Vibration=1`).

A confirmed hit on a character or destructible object gives a light pulse.
Taking damage gives a stronger pulse, and a successful optional player block gives a short sharp
pulse. Vehicle and scenery impacts include melee, held weapons, slides and
firearm hits, without requiring the object to break. Firing a weapon also gives
a separate recoil pulse, even when the shot misses. Multiple aiming rays from
one discharge do not repeat recoil. Recoil and hit pulses overlap without
adding motor strength. Missed melee attacks, merely holding Block and AI aiming
probes do not trigger feedback. Unattributed environmental
destruction does not assign a pulse to a player. Feedback does not change
damage, object durability, hit recovery or guard rules.

Recoil follows the weapon being fired: pistols give a firm short pulse,
shotguns give a heavier and longer kick, and heavy guns and projectile launchers
have their own stronger profiles. The same pistol profile covers all five
native pistol variants. New shots refresh the recoil pulse during a burst;
switching weapons replaces the previous recoil instead of stacking profiles.

Cooperative feedback follows the assigned player and controller. It stops on
pause, quit prompts, focus loss, scene transitions and device reassignment.
Pulses also expire while rendering is stalled. Controllers without working
XInput vibration remain usable for input.

## Gameplay input

The wrapper adds semantic actions to living single-player gameplay before the
native actor update. Movement retains the game's eight directions relative to
the current camera; native action buffering and combinations remain intact.
Retail files and saved binding tables are not edited. Recorded demonstrations
retain native gameplay input. Experimental two-player input and dynamic camera
behavior are described in [Cooperative camera](COOPERATIVE.md).

## In-game settings

Open **Options > Controller Settings**, or press **F6** on the main menu.
Select each player's device, optional fixed gamepad slot, buttons and cooperative
camera mode. Open **Options > Gameplay Settings** for the shared Player Block,
Target Focus, Target Marker, Weapon Laser, Throw Trajectory and Separate Pickup switches. Button
bindings belong in Controller Settings. Assigning
keyboard/mouse to one player removes that assignment from the other; two gamepads always use
distinct XInput slots. Automatic slot
selection also reserves the other player's explicit slot. Single-player keeps
keyboard/mouse and a controller available and uses player one's button layout.
In single-player character selection both devices control the same first-player
cursor; the second cursor stays inactive. Cooperative selection keeps separate
player cursors and device assignments.

Use arrows/WASD or a gamepad's left stick/D-pad to navigate. Left/right changes
a value; Enter/A selects. On a buttons page, Enter/A starts capture, then press
the desired key, mouse button, gamepad button or trigger. Keyboard actions have
primary and alternate bindings, selected with left/right. Delete/Backspace clears
the selected binding; X also clears it when not capturing a new button.
A button already assigned to another action is rejected;
clear its previous assignment first. Esc or View+Start cancels capture. The reset
row restores that player's default buttons. Movement/look stick axes and mouse
look/wheel remain fixed; gameplay action buttons and trigger zoom are rebindable.

Gamepad actions also accept two-button chords. Press both buttons together during
capture; a single button is accepted on release or after a short hold. A complete
chord suppresses its individual gameplay actions, so a modifier cannot also run
or block during that chord. Native menu buttons remain fixed.

**Save and Back** applies the draft immediately and saves only the wrapper INI.
**Cancel** or Esc/B on the settings root discards the draft. Returning to the
native menu requires controls to return to neutral. Native menu navigation and
confirm/back buttons stay available independently of gameplay remapping.
Footer instructions use a separate blue tint. Keyboard navigation and capture
poll the game's existing DirectInput keyboard, so exclusive keyboard acquisition
does not depend on Windows asynchronous key state. Shared frontend menus remain
accessible from keyboard even when both cooperative players use gamepads.

Settings and quit instructions follow the last deliberately used input device.
An idle connected pad, a held button or small stick noise does not switch the
instructions back. Menu navigation stays fixed, so remapping a gameplay action
does not change menu A/B or Enter/Esc prompts.

Pause shows the current Punch, Kick, Jump, Run, View and Focus/Center bindings, plus
Block when enabled. Single-player follows the active device; cooperative play
shows each player's assigned layout separately. Keyboard hints use the primary
binding, or the alternate if no primary is assigned. Unassigned actions display
UNBOUND. These hints do not add an overlay during active combat.

Saved layouts use `[Player1Bindings]` and `[Player2Bindings]` with `Key`, `Alt`
and `Pad` prefixes and the action names `Forward`, `Backward`, `Left`, `Right`,
`Punch`, `Kick`, `Jump`, `Backfist`, `Run`, `View`, `Recenter`, `Pause`, `Quit`,
`ZoomIn`, `ZoomOut`, `Block`, `PreviousTarget`, `NextTarget` and `Pickup`. The saved
`Recenter` action also toggles target focus when it is enabled. New target-switch
defaults do not override a button already used in a saved layout.
Keyboard values are physical scan codes, with `0x100`
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
| Pick up a nearby item (Separate Pickup enabled) | F |
| Experimental block (when enabled) | Hold Q |
| Run | Hold left Shift |
| Look | Mouse movement |
| Toggle first-person / third-person view | Tab |
| Camera distance | Mouse wheel |
| Toggle target focus / recenter without a target | Middle mouse button |
| Previous / next target | Z / C |
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
| Backfist | RB+B |
| Pick up a nearby item (Separate Pickup enabled) | B |
| Experimental block (when enabled) | Hold LB |
| Run | Hold RB while moving |
| Toggle first-person / third-person view | L3 |
| Look | Right stick |
| Toggle target focus / recenter without a target | Right stick click (R3) |
| Previous / next target while focused | Flick right stick left / right |
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
that distance and uses the current viewing direction. R3/middle mouse toggles
target focus, or recenters either view when no eligible enemy is available.
Disable Target Focus to retain unconditional recentering. Disable `FirstPerson`
to retain orbit without the view toggle.

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

## Separate item pickup

The current development build adds **Options > Gameplay Settings > Separate
Pickup**, enabled by default (`Combat/SeparatePickup=1`). The published 0.0.131
package retains pickup with Punch and Backfist on B.

With Separate Pickup ON, Punch no longer bends down for a weapon or throwable
object. Press the remappable **Pick Up** action (F/B) with empty hands near a
reachable item. An empty press does not attack. A press during an attack, hit
reaction, jump, guard, occupied-hand stance or scripted control lock does not
cancel that action or queue a later pickup. Release and press again to retry;
holding the button never restarts a pickup interrupted by damage.

A small label beside the reachable item shows the actual assigned key or button.
Solo follows the most recently used device; cooperative views follow their
assigned player. Unbound actions do not show a misleading default button.
Off-screen labels stay inside the viewport; items at the feet or behind the near
plane retain a lower-edge prompt, including in first person.

The original reach tests, heavy-item hero restrictions, pickup animations and
attachment/ownership frames remain in charge. Health and score consumables
retain automatic collection. Turning Separate Pickup OFF restores pickup with
Punch without changing button assignments.

Default controller Backfist moves to **RB+B**, leaving B for pickup. RB alone
still runs; the complete chord does not run and pick up at the same time.
Unchanged older default pad layouts migrate together. Customized layouts are
retained; if their B or F is already assigned, Pick Up is left unbound on that
device until it is assigned in Controller Settings. No native binding table,
game executable or asset is edited on disk.

## Experimental player guard

Open **Options > Gameplay Settings** and select **Player Block**. Left/right or
Enter/A toggles the draft value. **Save and Back** saves and applies it to both
players without restarting, as `PlayerGuard=1` or `0` under `[Combat]`.
Cancel or Esc/B discards the draft. A failed save keeps the previous value
and displays an error. Existing INI preferences are retained, and new
installations default to **OFF**. The setting covers Hawk, Mace, Smasher and Alana.

**OFF** restores native combat without the added player guard: Block bindings
cannot start a protective pose or stop incoming damage. Native enemy guarding
is unchanged. Disabling the option releases any active player guard without
dropping held items; enabling it requires releasing and pressing Block again.

Hold the remappable **Block** action (Q/LB) while idle or walking/running
to enter a stationary guard. The arms rise in about 0.13 seconds; protection
starts on entry. Release it to lower the arms. Attacks, jumps,
slides, grabs and damage reactions cannot be cancelled into guard. Holding Block
during a standing hit reaction queues protection for the moment native recovery
finishes, without an extra idle tick. A standing hit that interrupts guard also
preserves this request. Releasing Block cancels it. Other interruptions and
scene/focus changes require releasing and pressing Block again.
An in-progress backstep turn also finishes before guard starts, preserving its
new facing direction. Hold Block through the turn to guard immediately afterward.
After releasing Block, movement, attack or jump input takes over immediately;
without another command the lowering animation finishes normally. Pressing Block
again during lowering starts a new entry. Holding Block takes priority over
movement and attacks, so turning in place does not accidentally leave protection.

Guard is available with empty hands and light items that allow native running,
including firearms and melee weapons. Heavy carried objects, including vehicle
wheels, prevent guard in either hand. Carrying and throwing them remain native.
Items stay attached to their native hand joints; release restores the appropriate
held-item stance without changing ammunition or durability. Shooting, swinging,
pickup and throw actions must finish before entering guard.

The guard stops ordinary unarmed melee within a 135-degree frontal sector,
using the character's body direction. A blocked hit causes a guard impact pose
and effect without damage or knockback. Side/rear hits, incoming weapons, projectiles,
grabs, throws and special attack paths retain native behavior. In first person,
the guarding body follows the view direction. In third person, movement keys or
the left stick turn the character in place relative to that player's camera.
Guard input follows each player's
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
remains pending, including held-item appearance and subsequent attacks in gameplay.
Faster guard entry and camera-relative turning have been verified in gameplay.
Gameplay checks also confirm that a carried wheel prevents guard while a pistol
or melee stick still allows it.
