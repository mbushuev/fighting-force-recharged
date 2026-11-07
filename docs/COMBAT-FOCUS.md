# Combat focus and aiming previews

These features were introduced in version 0.0.131. They require the
independent gameplay camera; Original Shared co-op and native stage cameras
retain their existing behavior.

## Player controls

Press Target Focus (R3 or middle mouse by default) to select a
living, visible enemy near the center of the view. Press again to release it.
If no eligible enemy is available, the camera stays where it is.
Flick the right stick left/right, or press Z/C, to select a visible enemy in
that direction. A stick must return to neutral before another target switch.
Keyboard target switching has separate rebindable Previous Target and Next
Target actions. Mouse movement does not switch targets.

The view follows the target's upper body with bounded angular speed, using the
local character's actual eye position in first person. Ground locomotion
remains relative to the view while the neutral body faces the selected enemy.
Attacks, jumps, damage reactions and knockdowns retain native action ownership.
Focus does not cancel hitstun, modify damage or guarantee that a shot reaches a
selected enemy. Other actors and scenery can intercept it.

A dead, absent, replaced, distant or persistently occluded enemy releases focus.
No automatic selection of another enemy follows its death. Scene changes,
loss of window focus and a long input gap reset the selection. Each cooperative
player owns a separate target; focus requests separate dynamic camera views.

## Independent visual switches

Open **Options > Gameplay Settings**. Target Focus enables the mechanic. Target
Marker controls only the short outlined corners around the selected enemy;
disabling it leaves focus active. Weapon Laser controls the first-person weapon
preview independently. Save and Back applies and saves the draft; Cancel discards
it. The matching `[Combat]` keys are `TargetFocus`, `TargetMarker` and
`WeaponLaser`, each 1 by default. Throw Trajectory is another independent visual
switch, saved as `ThrowTrajectory = 1`. Player Block remains disabled by default.

The corners follow the bounds of the current projected skeletal vertices,
including lying, jumping, interpolated and split-screen poses. The sides remain
transparent. The marker is hidden when the target is occluded or outside the
viewport. Its thin blue strokes have a dark outline and load no new assets.
Overlay drawing restores the exact device states even after a failed draw.

## Weapon laser and native firearm limits

RT fires the held firearm in either camera view. The default keyboard/mouse
fire input is the same as Punch. The original game still handles the shot.

Pistols, the shotgun and the heavy gun fire horizontal rays in the original game.
Looking up or down does not change that native shot height. The laser attaches
to the front of the rendered weapon model and follows its current barrel direction,
including a lowered running pose. It is blue with ammunition and red when empty.
It uses the current render transform, including the intermediate 60 FPS pose,
rather than a previous simulation-frame camera or a fixed wrist offset.

The dot marks a visual intersection with captured opaque scene triangles or an
approximate actor volume. A miss draws the beam without an impact dot. The
preview is consumed once in its world scene; frontend and later HUD scenes do
not redraw a stale beam. It dispatches no native damage, effects or projectiles.
The bazooka shows its rendered barrel direction with an open reticle. The
reticle indicates direction only because rockets follow the native projectile
path. Native firing animations can raise the weapon before discharge.

## Throw trajectory

Carrying a grenade or a heavy throw-only item shows a dashed flight guide in
first or third person. The small gold ring marks the first predicted contact.
Turning the character changes its direction; looking up or down does not alter
the native throw. Each player owns a separate preview in dynamic co-op, with
blue strokes for player 1 and gold for player 2 in a shared view. No extra
button is required. Disable **Options > Gameplay Settings > Throw Trajectory**
to hide it without changing the laser, focus or throwing controls.

The guide starts at the current rendered held-item origin and applies the
retail 30 Hz flight steps, including horizontal drag. It stops at captured
opaque scenery, approximate actor bounds, or the native Y=0 floor. It is an
aiming estimate: the subsequent throw animation may move the release hand,
and native collision volumes can differ from visible geometry. It does not
predict bounces, rolling, grenade fuse timing or the explosion radius.
The preview advances only private points; it never calls the native mutating
item updater, releases an item, consumes ammunition or spawns effects.

The reviewed grenade update callbacks at `0x19CF5`/`0x19CFA` identify types
`0x1F0`/`0x1F1`. Heavy pickup types above `0x209` and below `0x233` use the
heavy carry/throw path. Player throw completion at `0x515BD` releases with
speed 128 and lift 64; the heavy/right and left variants at `0x51654` and
`0x516EB` use lift 32. The common update at `0xA2E0` performs two horizontal
half-steps using the signed 32767-scale sine/cosine tables, subtracts 16 from
lift before moving Y by negative half-lift, and then subtracts 3 from speed.
Runtime signatures gate the preview. Render ownership, near-plane clipping,
scene consumption and device-state restoration use the same boundaries as
the laser. Automated tests cover native sample positions, both lifts,
cardinal/diagonal directions, nearest contact, setting persistence and separate
player ownership. Original files remain unchanged.

## Integration evidence

All addresses are RVAs in the supported English image. Native melee at
`0x39A60` scans eight actors at `0x86A500` with stride `0xF4`, rejecting absent
character types, health at or below zero, noncombatants `0x11`/`0x33` and actions
`0xCB`/`0xCD`. Selection excludes both player slots unconditionally.

Ray firing at `0xE311` obtains bone 3 through `0x38FB0`, then uses actor X/Z,
that bone's Y, body heading times 32 and the carried weapon type. Calls to
`0xDE60` try heading offsets 0, -384 and +384 until one hits. The ray performs
64 samples using the original sine/cosine tables and horizontal scenery/actor
queries `0x37690`/`0x37800`. These findings describe existing shots; preview
drawing does not invoke that dispatch. The reviewed held-item callers at
`0x4426F`/`0x44488` retain render actor + `0x62` in ESI and object type in EBX.
The laser captures their actual model matrix and front cap. Ownership/signatures
are checked; another actor's copy of the same weapon cannot replace the local
attachment. Original files remain unchanged.

Enemy projection hooks at `0x43C1E` and `0x44699` capture the selected actor's
projected bounds immediately after skeletal projection. The native temporary
vertex records at `0x9D208` have stride 32, screen X/Y at offsets 0/4 and
clipping flags at `0x1C`; flag `0x10` rejects an unprojected point. Neither the
source model nor its output records are changed by the marker.

Deterministic tests cover selection exclusions, on-screen directional switching,
button edges, occlusion grace, death/scene/focus reset, angular wrap, independent
player ownership, attack-direction preservation, barrel rotation, ammunition
colors, scene consumption, actor intersection, and device-state restoration
after a draw or state-query failure.
They do not establish comfort across every character, weapon or campaign stage;
live playtesting remains necessary.
