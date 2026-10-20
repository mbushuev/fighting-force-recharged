# Native camera research

This records analysis of the supported English Fighting Force 1 executable and
the independent single-player orbit built from it. Full orbit and movement
have been checked on the first level. Experimental geometry obstruction handling
is implemented; full campaign coverage remains separate work.

All addresses are executable-relative RVAs. The supported image hash is in
the README. Reproduce the reference checks with:

```powershell
python tools/analyze_camera.py
python tools/inspect_retail.py range 0x404200 0x4042eb
python tools/inspect_retail.py range 0x44f142 0x44f22e
```

The generic inspection tool accepts virtual addresses; its examples include
the retail image base. The camera analyzer reports RVAs. Both are read-only.

## Verified ownership and data flow

| Responsibility | RVA | Evidence and implication |
| --- | --- | --- |
| Player focus | `0x42f0` | Selects the living player or an X/Z midpoint when both players are alive; also computes their separation. |
| Stage policy dispatch | `0x4120f` | Calls the stage-indexed table at `0x8736e0` after focus selection in the gameplay loop. |
| First-stage policy | `0x7aa0` | Chooses yaw using stage-section state, then calls shared framing and follow functions. |
| Shared framing | `0x43d0` | Builds a desired horizontal position from focus, yaw and separation; changes height and pitch based on nearby character conditions. |
| Follow dynamics | `0x4200` | Integrates desired position into current position and eases yaw/pitch. |
| Recorded camera pose | `0x45a0` | Reads position and orientation from indexed records. Calls exist in the scene playback path and stage logic. |
| Render pose publication | `0x40bc0` | Copies camera pose and object state into rendering buffers before drawing. |
| Render orientation | `0x44540` | Builds the drawing orientation from the published pitch/yaw. |
| Movement heading | `0x4f197` | Adds a saved camera yaw to the direction selected by the player input mask. |
| View-dependent environment | `0x9600` | Selects environment model entries based on camera yaw and stage-section state. |

The stage table is initialized explicitly: stage 0 uses `0x7aa0`, stage 1
uses `0x7bf0`, and other stages use additional handlers. For example, stage 1
clamps one desired horizontal coordinate before following it. Stage 4 includes
timed camera targets and temporarily suppresses ordinary player framing.
Changing the common follow function alone would not replace these policies.

### Pose fields

| Data | RVA | Representation |
| --- | --- | --- |
| Current X/Y/Z | `0x866220`, `0x866224`, `0x866228` | Signed 32-bit fixed point; native matrix translation converts with `1/65536`. |
| Current yaw/pitch | `0x87888c`, `0x87888e` | 16-bit angles; full turn is 65536 units, native trigonometry uses 2048 samples. |
| Desired X/Y/Z | `0x86d31c`, `0x86d324`, `0x86d328` | Inputs to common follow dynamics. |
| Desired yaw/pitch | `0x86d318`, `0x874cf2` | Inputs to common angular easing. |
| X/Y/Z dynamics | `0x8744a8`, `0x8651a4`, `0x8689f8` | Retained velocity-like state. |
| Render X/Y/Z | `0x8677b0`, `0x8677b4`, `0x8677b8` | Published copies consumed by world/background rendering. |
| Render yaw/pitch | `0x873ce0`, `0x87459c` | Published orientation copies. |
| Input yaw snapshot | `0x85a68c` | Saved before the current gameplay update; consumed by direction-to-heading conversion. |

These are verified references, not an exhaustive inventory of every writer.
Additional pose writers exist in menu and scene paths. Runtime ownership is
limited to the gameplay update boundaries described below.

## Follow behavior

Ignoring fixed-point rounding, the shared position update is approximately:

```text
X/Z: velocity += (desired - position - velocity) / 2
     position += velocity / 4

Y:   velocity += (desired - position - velocity) / 16
     position += velocity / 32
```

Yaw advances by a signed wrapped difference divided by 8. Pitch advances by
approximately one thirty-second of its difference, with integer rounding.
Vertical recovery therefore has a substantially different response from
horizontal recovery. These are coefficients per native update, not seconds;
the live update cadence has not been measured in this research stage.

The common follow function contains no obstacle-query call. Stage-specific
coordinate restrictions are present, but a reusable general camera collision
solver has not been identified. That does not establish that none exists
elsewhere in the executable.

## Consequences for a manual camera

- Input is already camera-relative at its heading boundary. A new camera must
  publish the correct yaw at the expected input phase. A visual-only rotation
  could leave movement relative to a different orientation.
- CPU projection and visibility preparation occur before Direct3D receives
  transformed vertices. Rotating the final drawing output alone cannot restore
  geometry rejected earlier in those paths.
- Environment selection also depends on yaw and section state. Full orbit
  requires checking those selectors and authored model coverage on each stage.
  Missing reverse-side geometry is a risk to test, not an established defect.
- Recorded shots, timed stage views, menus and cooperative framing need explicit
  camera ownership rules. A single global angle offset is insufficient.
- Position, orientation, render copies and input snapshots form one transaction.
  Updating only one copy risks inconsistent culling, backgrounds or controls.

## Runtime integration

The wrapper validates all three camera call sites before replacing them in
process memory. The gameplay-entry call at `0x40d6c` resets orbit/input state.
The input call at `0x41177` first runs native polling, then supplies the orbit
pose before the engine snapshots yaw and prepares view-dependent objects.
The stage dispatch at `0x4120f` always calls the original stage handler, then
republishes the orbit around the updated character position. Stage handlers
include level progression and object triggers; bypassing them would break play.

Native render-pose publication remains intact, including its separate buffers
for the optional rendering thread. The wrapper updates current/desired pose,
clears native follow dynamics and refreshes derived position fields. It does
not rotate already-transformed screen vertices.

Ownership is restricted to living single-player gameplay on stage indices
0 through 24. Recorded input mode 1 is an attract demo and retains native
framing. Menus and recorded scene playback use separate paths. Stage 4's timed
opening retains its native view for the first 660 updates. Cooperative play
retains the original shared camera.

A separate nonexclusive relative DirectInput mouse supplies look, zoom and
buttons. Initial/reacquired samples are discarded to avoid accumulated movement
after loading or focus changes. Optional XInput right-stick look is resolved
from the system runtime outside the loader lock. Mouse displacement is applied
once; stick angular velocity is scaled by elapsed time with a bounded gap.

The native single-player action word is at `0x5523e0`. Mouse attacks merge
semantic punch (`0x40`) and kick (`0x20`) bits after native polling and before
actor processing. Native keyboard/joystick actions and combinations survive;
no window messages or physical key events are synthesized.

## Obstruction handling

The orbit has a local ground guard relative to the followed character's feet.
Its lowest angle depends on target height, distance and clearance, and is
rounded upward to the native 2048-sample angle grid. Zooming out cannot lower
the camera below that guard. This is not a terrain query or a wall sweep.

The environment pass (`0x43990`) and solid-object pass (`0x3c100`) define the
capture scope. The wrapper records the native camera basis at `0x43993`, then
observes model input at `0x26de0` before CPU visibility rejection. It transforms
model vertices back into world space without altering the original buffers or
projector. Opaque quad faces form two triangles; disabled/line primitives and
alpha material rectangles are excluded. Character and debris passes are outside
the scope. Stage changes invalidate the mesh generation, and completed snapshots
are published under a lock for the optional native render thread.

The camera uses a continuous sphere sweep against triangle faces, edge capsules
and vertices. The nearest contact shortens the requested boom immediately with
a small stand-off. Distance returns exponentially during the input phase;
post-actor pose publication can shorten it further without advancing recovery
twice. The selected user zoom remains the unobstructed target distance.

The capture has a bounded triangle count and rejects unreadable model records.
If the retail interception signatures differ, independent orbit is disabled
and native framing remains available. Camera tracing can observe attract-demo
geometry and run read-only radial probes while leaving demo camera/input native.

First-level full orbit and movement have been checked, but later-stage geometry
coverage, reverse-side models, hardware right-stick input, cooperative play and
camera handoffs have not received equivalent live validation. Authored selectors
may omit geometry before the observed model entry, so complete wall/terrain
coverage cannot be inferred solely from the sphere-sweep tests.
