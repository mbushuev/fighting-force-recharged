# Native camera research

This is static analysis of the supported English Fighting Force 1 executable.
No runtime camera hooks, input mappings or configuration changes are enabled.
The installed compatibility wrapper is unchanged. Gameplay feasibility remains
separate from the verified instruction references below.

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
Additional pose writers exist in menu and scene paths. Their precise ownership
boundaries need live observation before a camera override is introduced.

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

## Options

| Option | Result | Main work and limits |
| --- | --- | --- |
| Refine automatic framing | Adjust follow response, height/distance policy and recentering while retaining authored directions. | Smallest initial scope. Still needs transition and two-player checks; global smoothing coefficients should not affect scripted shots indiscriminately. |
| Bounded manual adjustment | Add limited yaw/pitch and distance control around the native framing, with deliberate recentering and handoff to authored views. | Recommended first manual prototype. Requires coordinated input/render publication, view-selection checks and conservative movement limits near obstacles. |
| Full player-centered orbit | Freely rotate through 360 degrees with an independent follow camera. | Largest scope: collision/obstruction handling, geometry coverage, all stage policies, scene ownership, cooperative behavior and control consistency. Feasibility is not yet gameplay-verified. |

The recommended progression is read-only live tracing, then a single-player
bounded manual prototype, followed by broader orbit only after geometry and
obstruction tests pass. Cooperative play can retain native framing during the
first prototype if its camera policy is explicitly separated.

## Next evidence to collect

1. Observe stage/section, current and desired pose, dynamics, render copies and
   the input yaw snapshot at actual update boundaries. Keep traces local.
2. Identify ownership transitions during level entry, recorded shots, menus,
   player death and return to gameplay.
3. Audit the first level's environment selectors at intended yaw limits before
   writing camera state.
4. Locate a validated obstacle query or design a separate camera-volume sweep;
   verify that it cannot alter character movement or scene state.
5. Test movement and combat headings, fog, sky, shadows, transparency and
   widescreen framing under the same camera pose.

This stage adds reproducible analysis and design options only. It makes no
claim of an implemented or tested manual camera.
