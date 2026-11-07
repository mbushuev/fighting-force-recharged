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

## Move list

Open **Pause > Move List** to view the current hero's commands. In cooperative
play, choose Player 1 or Player 2; each page uses that player's character and
bindings, including a surviving second player. Before starting a game, open
**Options > Move List**, choose a player layout and then preview any of the
four heroes in the native character-selection order.

Left/right changes the category; up/down selects a move and its conditions.
Use arrows/WASD or the controller D-pad/left stick. Esc/B returns one screen;
closing the list returns to the paused game menu. Browsing never resumes play,
spends health, changes a binding or performs an attack.

The five categories cover strikes/block, jumps/running, grabs, weapons/throws
and item interactions. Commands use the current primary keyboard/mouse binding
(or its alternate if the primary is empty) or the actual gamepad assignment.
Single-player and the frontend preview follow the last used connected device.
Cooperative pause pages follow their assigned device, independently of which
player navigates the menu.
Unbound actions and conflicting simultaneous pad chords are identified instead
of displaying a default that cannot be used.

**AND** means press together; **THEN** means a sequence. For the air grab,
jump first, then press Backfist close to an eligible enemy during the native
grab window. It is not a simultaneous Jump+Backfist command and does not work
against every enemy. A single Throw/Drop entry covers carried items and grenades;
it needs one press.

The list includes each hero's grab follow-ups, running and jumping attacks,
health-consuming spin, firearm and melee commands, heavy-item/grenade throws,
pickup and detachable scenery. A combo entry includes its automatic native
follow-up poses. The spin consumes health and is unavailable at low health;
it and the jump kick also work early during a jump. Running attacks require
movement and an established run. Grabs require native range, facing and an
eligible enemy. The menu does not relax action, timing or weapon restrictions.

Player Block is labeled disabled when its gameplay switch is off. Turning
Separate Pickup off shows Punch for pickup. Throw/drop always shows its own
binding. The list reflects settings applied with **Save and Back**.

## Room completion

**Options > Gameplay Settings > Complete Rooms** closes the open side of the
elevator cabin, first boss office, later laboratory lift, laboratory, ship
interior and shopping center during custom-camera gameplay. It also completes
Subway entrance pilasters and extends the street scenery behind the Bronx area. The
default is ON (`Rendering/CompleteRooms=1`). Choose **Save and Back** to apply;
Cancel preserves the current setting. OFF restores the original room models.

The extensions reuse materials from the installed room models, including the
cabin's side glazing and trim. The office has a wooden interior wall with a
closed double door. The laboratory has a recessed metal doorway, and the
shopping center entrance atrium has a service vestibule beneath its rear balcony.
The ship bulkhead includes recessed steel double doors. At the Bronx area,
brick and concrete buildings continue beyond the rear intersection; the cross
street, central road and existing ground surface remain open and unchanged.
These added doors are scenery. The elevator entrance/corridor and office windows,
destructible objects, triggers and level progression retain native behavior.
The added walls and glass obstruct the orbit camera; native actor collision zones are
unchanged. First/third-person and dynamic cooperative views use the same walls.
Native framing, demonstrations and the office intro keep their original models.
Only the listed areas are covered; unrelated levels are not reconstructed.
Gameplay spot checks of the reviewed additions found no obvious issues.
These checks do not establish full-campaign coverage.

## In-game settings

Open **Options > Controller Settings**, or press **F6** on the main menu.
Select each player's device, optional fixed gamepad slot, buttons and cooperative
camera mode. Open **Options > Gameplay Settings** for the shared Player Block,
Target Focus, Target Marker, Weapon Laser, Throw Trajectory, Separate Pickup and
Complete Rooms switches. Button
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
look/wheel remain fixed; gameplay action buttons and pad camera controls are rebindable.

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

Pause shows the current Punch, Kick, Jump, Run, View and Target Focus bindings, plus
Block when enabled. Single-player follows the active device; cooperative play
shows each player's assigned layout separately. Keyboard hints use the primary
binding, or the alternate if no primary is assigned. Unassigned actions display
UNBOUND. These hints do not add an overlay during active combat.

Saved layouts use `[Player1Bindings]` and `[Player2Bindings]` with `Key`, `Alt`
and `Pad` prefixes and the action names `Forward`, `Backward`, `Left`, `Right`,
`Punch`, `Kick`, `Jump`, `Backfist`, `Run`, `View`, `Recenter`, `Pause`, `Quit`,
`ZoomIn`, `ZoomOut`, `Block`, `PreviousTarget`, `NextTarget`, `Pickup`, `Throw`,
`LookLeft` and `LookRight`. The gamepad also has `PadFire`. The saved name
`Recenter` is retained for compatibility and controls target focus only.
Keyboard values are physical scan codes, with `0x100`
marking extended keys; mouse buttons are `0x1001..0x1005`. Pad values use XInput
button masks, `0x10000` for LT and `0x20000` for RT. Zero means unbound.
`[Controls] BindingsVersion=3` marks the current saved format. Existing settings without
this marker retain their previous first-selected-controller slot interpretation
until saved. Original game binding tables and save files remain untouched.

## Keyboard and mouse

These tables describe the current defaults. Customized saved assignments take
precedence; see [Quick start](QUICKSTART.md#play) for a compact overview.

| Action | Default input |
| --- | --- |
| Move relative to camera | W / A / S / D |
| Punch | Left mouse button or J |
| Kick | Right mouse button or K |
| Jump | Space |
| Backfist / native enemy grab | E |
| Pick up a nearby item (Separate Pickup enabled) | F |
| Throw / drop a held item | Q |
| Experimental block (when enabled) | Hold R |
| Run | Hold left Shift |
| Look | Mouse movement |
| Toggle first-person / third-person view | Tab |
| Camera distance | Mouse wheel |
| Lock / release target | Middle mouse button |
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
| Move relative to camera | Left stick |
| Punch | X |
| Kick | Y |
| Jump | A |
| Backfist / native enemy grab | B |
| Pick up a nearby item (Separate Pickup enabled) | RB |
| Throw / drop a held item | LB |
| Experimental block (when enabled) | Hold LT |
| Walk / run | Push the left stick farther to accelerate |
| Fire a held firearm | RT in either camera view |
| Toggle first-person / third-person view | L3 |
| Look | Right stick; D-pad left/right also turns horizontally |
| Lock / release target | Right stick click (R3) |
| Previous / next target while focused | Flick right stick or press D-pad left / right |
| Move camera closer / farther | D-pad up / down |
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
speed uses the existing camera sensitivity setting. LT/RT use a small neutral
threshold for Block and Fire. D-pad up/down adjust camera distance continuously and
do not also move the character. Hold D-pad left/right to turn the camera at the
current view's configured horizontal stick speed. These are remappable
**Camera Left / Camera Right** actions and work independently for each player,
including a surviving second player. Like horizontal stick input, they switch
targets while target focus is locked. Opposite turn buttons cancel; combined
stick/button input is bounded to full stick speed. Unassigned D-pad directions
never move the character either. The existing `TriggerZoomUnitsPerSecond=720`
setting controls zoom speed, including digital zoom bindings. Mouse wheel steps
remain unchanged. Menus retain raw D-pad navigation.

The left stick walks around the middle of its range and runs near the edge.
The lower threshold for returning to a walk avoids repeated switches near the
edge. RT fire takes priority over automatic running while shooting. Assign a
Run button in Controller Settings to run manually. Clearing that binding with
Delete restores automatic running.

`AnalogMovement=1` under `[Input]` smoothly scales unarmed walking and running
speed for each player. The wrapper reads native animation displacement for the
maximum speeds and sends the resulting movement through the original collision
solver. Attacks, jumps, knockback, carrying and keyboard movement use native
speed. Set `AnalogMovement=0` to keep the automatic run threshold with native
walk and run speeds. Movement retains the original eight directions.

`FirstPerson=1` under `[Camera]` enables Tab/L3 during living single-player
gameplay. The camera follows the animated skull position, including jump and
combat movement, while mouse/right-stick yaw and pitch remain under player
control. The local head is excluded; the original arms, torso, legs and held
objects remain. Looking down reveals the body. Neutral walking/idle poses face
the view; attack, knockdown and jump animations retain native direction.
Wheel or the assigned zoom controls change distance only in third person. Switching back preserves
that distance and uses the current viewing direction. R3 and middle mouse only
lock or release a target. Disable `FirstPerson`
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

**Options > Gameplay Settings > Separate Pickup** is enabled by default
(`Combat/SeparatePickup=1`) from 0.0.132. Version 0.0.131 used pickup with Punch.

With Separate Pickup ON, Punch no longer bends down for a weapon or throwable
object. Press the remappable **Pick Up** action (F/RB) with empty hands near a
reachable item. An empty press does not attack. A press during an attack, hit
reaction, jump, guard, occupied-hand stance or scripted control lock does not
cancel that action or queue a later pickup. Release and press again to retry;
holding the button never restarts a pickup interrupted by damage.

Pickup also handles the original detachable scenery, including elevator rails.
Face the object within its native interaction area with empty hands to see the
pickup prompt. Use Throw/Drop first if you are carrying an item. The original
pull animation converts the scenery and attaches its weapon at the release
frame. With Separate Pickup off, Punch starts the pull from the same area.

A small label beside the reachable item shows the actual assigned key or button.
Solo follows the most recently used device; cooperative views follow their
assigned player. Unbound actions do not show a misleading default button.
Off-screen labels stay inside the viewport; items at the feet or behind the near
plane retain a lower-edge prompt, including in first person.

The original reach tests, heavy-item hero restrictions, pickup animations and
attachment/ownership frames remain in charge. Health and score consumables
retain automatic collection. Turning Separate Pickup OFF restores pickup with
Punch without changing button assignments.

Default controller **B retains native Backfist and enemy grabbing**. Pickup
does not select enemies or become an attack. **Throw/Drop (Q/LB)** uses the
native right-hand light/heavy throw, then the native left-hand throw or drop
fallback. It does nothing with empty hands and cannot cancel hit reactions,
attacks, jumps, guard or scripted locks. Release and press again after an
interruption; holding the button does not throw a subsequently acquired item.
Original release frames retain projectile flight and damage. **Throw/Drop** is
the only manual release input in both Separate Pickup modes. You can kick
with a light item. Heavy throw-only items block kicks, and the spin attack
requires empty hands.

The previous default keyboard Q/R pair migrates to R Block and Q Throw/Drop.
The previous default RT Run migrates to automatic running and RT Fire. A
custom Run button is retained. If RT is already used by a different custom
action, assign Fire in Controller Settings. LB remains the default Throw/Drop
button and RB remains Pickup.

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

Hold the remappable **Block** action (R/LT) while idle or walking/running
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
animation tables on disk are changed. R/LT
defaults do not take over an existing saved assignment to another action; bind
Block explicitly in that case. Smasher's frontal protection and rear damage
have been verified in gameplay. Native decoder and isolated runtime checks cover
all four heroes; a complete visual, first-person and level/co-op acceptance pass
remains pending, including held-item appearance and subsequent attacks in gameplay.
Faster guard entry and camera-relative turning have been verified in gameplay.
Gameplay checks also confirm that a carried wheel prevents guard while a pistol
or melee stick still allows it.
