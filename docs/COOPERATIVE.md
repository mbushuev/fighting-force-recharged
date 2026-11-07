# Cooperative controls and camera

Choose **Dynamic Split** in **Options > Controller Settings** to enable the
experimental camera for the
original two-player game. It requires `FreeOrbit=1`, available first-person
hooks and responsive presentation. One-player gameplay retains its existing
camera and input paths. Original game files are unchanged.

Choose **Original Shared** for the native level-directed cooperative camera,
without separate views. Modern device assignment and action bindings remain
active; independent orbit/first-person views are disabled in this mode. The
choice applies after saving, without restarting. `[Camera] CooperativeCamera`
stores `0` for original or `1` for dynamic; older INIs can still use
`DynamicSplitScreen` as a fallback when the new key is absent.

## Behavior

When only one actor is alive, that player owns one full-width view. Their look,
distance and first-person controls remain available on the assigned device.
The normal shared/split policy resumes when both actors are alive again.

The common third-person camera follows the midpoint between the two living
players. Screen occupancy includes each actor's standing height and a horizontal
margin. The camera separates when either actor exceeds the safe frame bounds
or an opaque captured obstacle blocks the common view. It merges only after
both actors have stayed comfortably inside those bounds for a short interval.
Different split and merge thresholds prevent repeated switching near a boundary.

Separation and reunion interpolate camera positions, angles and off-axis
projection. Player one occupies the left half and player two the right half.
Vertical field of view and world proportions remain unchanged. First person on
either controller requests separate views, even when the players stand together.
Each view retains its own orbit distance and first-person selection. L3 toggles
that player's view; R3 toggles target focus or recenters without a target. The
keyboard player uses Tab and middle mouse instead. Target focus also requests
separate views and is owned independently by each living player. Disable Target
Focus in Gameplay Settings to retain unconditional recentering.
Right stick/mouse and the assigned zoom controls/wheel retain their usual look and
third-person distance functions.

Both controllers can steer the common view. Once separated, each controls its
own camera. Native movement heading uses the owning player's view yaw. Combat,
collisions, level progression and simulation ticks remain native.

## Input schemes and controller assignment

The default `CooperativeInput=1` under `[Input]` assigns keyboard/mouse to player
one and the first selected XInput pad to player two. WASD, mouse combat, Space,
Shift and Tab retain their single-player meanings for the keyboard player.
`CooperativeInput=2` assigns a separate pad to each player. `CooperativeInput=3`
assigns a pad to player one and keyboard/mouse to player two. Open **Options >
Controller Settings** or press **F6** on the main menu to change assignments,
remap each player's buttons and select the camera mode. Keyboard/mouse can
belong to only one player. Changes apply with **Save and Back** and require
controls to return to neutral. See [Controls](CONTROLS.md) for rebinding details.

`ControllerIndex=-1` selects the first available XInput slot for player one.
`Controller2Index=-1` selects another available slot for player two, excluding
the first player's selected device. A slot setting is ignored for the player
assigned keyboard/mouse. Explicit values `0..3` select fixed slots, displayed as
1..4 in the menu. Selecting an occupied fixed slot makes the other player's
assignment automatic; automatic selection excludes the other player's fixed or
connected slot. Connecting, changing scenes or
returning focus requires neutral controls before new actions are accepted.

If one pad disconnects, the other keeps its player. An automatic assignment
can use a free XInput slot to replace a missing pad. Losing both assigned
pads pauses play. With two automatic assignments, reconnect a pad and press
**A for Player 1** or **X for Player 2** on the pad you want to keep in that
role. The other pad fills the vacant role when available. This confirmation
is necessary because Windows can reassign XInput slot numbers after a
disconnect. Explicit slot assignments continue to use their configured
numbers. Release the controls before resuming with **Start**.

Choose `2 PLAYER GAME` in the native menu. Each assigned input selects its own
character. The default controller layout uses the left stick to move, A to
jump, X to punch, Y to kick, B for Backfist, RT to run, RB for pickup and
**LB for all manual item throws and drops** in both Separate Pickup modes. Cooperative
walking currently uses native animation speed;
single-player analog walking scaling is unchanged. Either controller can use
Start for pause and View for the native quit confirmation by default. Esc/P and
F10 are the keyboard defaults. Native menu confirm/back remain A/B after gameplay
buttons are remapped. Shared frontend menus accept either pad; character selection
and live gameplay use each player's assigned device.

## Rendering boundary

The supported retail's `0x440BC0` callback publishes render copies and draws a
game frame. The wrapper calls this draw path twice only while views separate;
it never calls actor updates or the stage simulation a second time.

Each view renders to the existing target with an off-axis horizontal projection.
Completed images are copied to separate halves of a wrapper-owned DirectDraw
surface, then composed before the final presentation. The first intermediate
frame is not presented or reported as a completed transition frame. Native HUD
coordinates stay continuous across the composed frame. Health-display easing
and native fade counters are restored after the first pass so they advance once.
The intermediate view skips the native frame-clock measurement, pacing wait and
clock restart. The completed shared/split frame retains the original single
frame limit and simulation timing. Optional `[Diagnostics] TraceCooperative=1`
records aggregate shared/split frame times in the local wrapper log.

Temporary camera/projection fields are restored afterward. Only the selected
player's head is excluded from an eye view; the other player retains their
complete model. The second actor uses its own animation palette at stride
`0x238`, and its actor record at stride `0xF4`.

## Validation and limits

Deterministic tests cover occupancy, split/merge hysteresis, angle interpolation,
continuous off-axis reunion, distinct controller slots, per-player heading,
second-player eye/head scope, native signature rejection, render-pass count,
unchanged simulation ticks and restored presentation state. The Direct3D GPU
test verifies independent image halves, their divider and resource cleanup.
Patched native clock-call tests verify that a split pair measures and restarts
the frame clock once and that the intermediate view cannot enter its busy wait.

Initial keyboard/mouse plus controller gameplay and dynamic separation have
interactive coverage. Two-controller play, sustained performance, later-stage
authored visibility and native cooperative travel limits require further
validation. Rendering two complete
intermediates costs more than a single shared frame. This implementation retains
native cooperative stage progression and does not add independent progression,
network play or separate pause states. Camera obstruction uses the same captured
opaque geometry as the existing orbit; unavailable or transparent geometry is
outside that collision snapshot.
