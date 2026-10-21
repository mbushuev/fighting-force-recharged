# Validation

The current development baseline was checked on Windows with the supported
English retail executable. These results cover the implemented compatibility
and presentation changes; they are not a full-game completion claim.

## Automated coverage

- First-person skeleton parsing, head/hand subtree separation, malformed-stream
  rejection, angle conventions, pitch limits and held-toggle neutral boundaries.
- First-person native call signature rejection before writes, balanced matrix
  stack restoration, animated root/jump translation, temporary local head-face
  exclusion, retained body faces and unchanged source model data. Confirm other
  actors and ineligible scenes use the original draw/visibility functions.
- Reserve L3 for camera selection: the gameplay mapper and analog movement
  boundary never synthesize Run from it; RB still requests running.
- First-person backward/sideways movement for keyboard and XInput direction
  words, diagonal step length, alternate view yaws, ground-gait variants and
  unchanged attack/airborne/carrying proposals. Verify corrections from the
  native collision boundary survive and keyboard-only hook setup is idempotent.
- Held-firearm diagonals in both hands, all native firearm stance types and
  alternate idle poses. Verify keyboard and XInput diagonals keep the native
  weapon step length and collision corrections, while firing/throwing actions,
  unsupported carried items and invalid held-object slots remain unchanged.
- Keyboard loss and reacquisition, including failed reads and stale state.
- Mounted-disc CD audio reuse, disc changes and failed MCI commands. Preserve
  playback through the retail open/play/close and open/status/close sequences;
  forward explicit stop commands without synthesizing a stop on close.
- Exact far-plane depth correction without changing other vertex fields.
- Transparent texture pixels, opaque cutouts and native translucent passes.
- Transition readiness, stale generations and renderer/movie lifetimes.
- Native flip call signatures and thiscall stack cleanup, nonblocking busy-frame
  handling, inactive-window waits, pause-loop message dispatch after EndScene,
  activation/quit message ownership and held
  controller suppression after focus return.
- Widescreen mode selection, uniform UI scaling and unchanged world polygons
  when reused records retain a normalized-screen flag.
- Eight real D3D2 device cycles covering point/linear filtering, fog off/on,
  later geometry through cutouts, render-state restoration and COM cleanup.
- Offscreen transition windows covering fade, non-activating styles, destruction
  and worker-thread cleanup.
- Separate one-frame loading-image readiness from gameplay, movies and copyright
  images; cover the loading renderer before subsequent replacement.
- Read back completed 16-bit texture and 32-bit D3D render surfaces, retain CPU
  pixels independently of renderer destruction, and paint proportional loading
  images with black margins. Hold the loading picture across immediate native
  teardown and fade it only after two replacement game frames.
- Execute all three patched loading-entry boundaries with their original media
  argument, reject partial signature matches, and run the native disc-wait
  instruction sequence against a stationary clock to confirm immediate exit.
- Orbit angle wrap, pitch/distance limits, fixed-point coordinates, mouse
  displacement versus stick velocity, and native scene/demo ownership gates.
- Local ground clearance across the zoom range, including negative angle
  quantization and changes in the followed actor's height.
- Mouse punch/kick action merging, release without a retained latch, scene/focus
  suppression and simultaneous keyboard actions.
- Two-sided continuous sphere contact with triangle faces, edge capsules and
  vertex spheres, including high-speed motion, degenerate geometry and initial
  overlap. Invert native view bases and reconstruct world-space model vertices.
- Execute patched model/environment/object calls against a synthetic native
  image, retain the original model projector, exclude alpha material faces,
  contract at a wall, and recover distance only at the input phase.
- Preserve the last complete collision frame during environment/actor rendering
  until solid objects are available. Check a hood corner that overlapped the
  old pivot sphere, continuously growing probe clearance, nearby thin walls
  and full-radius contacts beyond the pivot region.
- Compare variable-radius obstruction contacts with sampled triangle distances
  across deterministic oblique-face and edge cases.
- Execute the patched solid-object dispatch exactly once per object; retain
  collision geometry across a missing render model, invalidate changed/carried
  objects, and reject alternating clear/blocked boom samples without oscillation.
- Execute the native projector trampoline with temporary two-sided opaque
  environment faces; retain original buffers, prop flags, cutouts and demos.
- Retain environment-record collision geometry across a missing native model,
  invalidate transformed records and respect removal through the published
  environment count. Execute unmodified native dispatch handlers exactly once.
- Reproduce the second stage's hall section transition at patched environment
  boundaries in a synthetic image: retain the doorway wall across the reduced
  count and changing yaw, include it in camera contacts, balance matrix calls
  and preserve record/count state. Avoid duplicate draws, restoration of broken
  partitions, demo/cooperative/native-camera paths and unrelated stages.
- Execute the patched retail partition-cleanup loop with a broken panel, an
  intact counterpart and an unrelated gate. Preserve the intact record and loop
  state during live hall orbit, retain later break-type changes, and keep native
  cleanup outside orbit ownership. Reject a changed cleanup signature before
  any hook write.
- Camera call-site signature rejection, execution of the patched indirect
  stage boundary, preservation of native stage side effects and native render
  publication, plus real system DirectInput mouse format initialization.
- Modern keyboard diagonals, opposing directions, simultaneous actions, release
  and prompt press gates. Execute native menu/pause/confirmation call boundaries
  against a synthetic image; preserve native readers and suppress modern input
  outside living single-player play, in recorded demos and without focus.
- XInput action combinations, eight-sector stick direction, radial deadzones,
  menu/confirmation mapping, all-slot discovery, fixed slots, disconnect retry
  and neutral gates after focus/scene changes. Feed simulated XInput states
  through runtime action/pause/quit boundaries and camera publication, including
  right-stick look, R3 recenter, trigger zoom and duplicate joystick suppression.
- Continuous analog walking speed, explicit run buttons, immediate run release
  and walk-speed cap, attack combinations at full stick, acceleration limits
  and per-character animation displacement decoding. Execute
  the patched actor collision call, verify its adjusted proposal and preserve
  native collision corrections. Exclude combat/airborne poses, AI actors,
  carried objects, other movement sources, focus loss, demos and cooperative play.
- Modern F10 quit remapping, B exit on the main menu versus back in submenus,
  held confirmation gates and independent movie skip polling through the native
  abort flag, including repeated clips and focus loss.
- Main-menu Esc quit, submenu Back and gameplay pause, with release gates after
  submenu return and cancellation; Enter accepts the resulting quit prompt.

## Gameplay coverage

- Startup movie, copyright image, main menu and character selection.
- First-level gameplay on a 16:10 display with a 1280x800 game render target.
- Audible CD background music during first-level gameplay with the reused
  control session retained after the native playback command.
- Expanded horizontal view with preserved object proportions and centered UI.
- No displaced texture fragments, missing side geometry or flickering shadows
  in the verified first-level run after restricting screen mapping to native
  normalized polygon kinds.
- Filtered vegetation and menu text, visible vehicle wheels, and both native
  fog states.
- Gameplay return to the main menu through Q followed by Y.
- Full mouse orbit and camera-relative movement on the first level.
- Retained fixed doorway wall after the second stage's section transition.
- Second-stage destructible partitions: break one panel and cross the section
  boundary; the intact counterpart remains visible and can still be broken by
  attacks after the transition.
- Left-mouse punch and right-mouse kick during first-level gameplay.
- WASD movement, Space jump without opening pause, Shift run and Esc
  pause/continue during first-level gameplay.
- XInput trigger zoom, continuous left-stick ground speed and rear recentering
  with R3 during first-level gameplay.
- Controller combat at full stick without automatic running or slide attacks,
  with running selected explicitly by holding RB/L3.
- Stable camera distance beside a vehicle, controller movie skipping, main-menu
  exit with B/A, and the modern F10 quit shortcut.
- Minimize/restore and screenshot-tool focus changes during gameplay and in the
  native pause menu, with responsive return to the same game state.
- Original loading pictures retained through the native renderer replacement
  before movies and first-level gameplay.
- Transitions after character selection and movie playback without the fixed
  last-frame disc-spin stall or desktop/blank-window flashes.
- Read-only capture of native environment/object geometry in an attract scene,
  with computed obstruction contacts while demo camera ownership stays native.

The original-file check compared 235 installed retail files against the source
media/extraction. All 235 matched. The supported executable hash is documented
in the README. Generated manifests and raw runtime logs remain local.

## Limits

First-person view has deterministic skeleton/input/render-boundary coverage and
isolated native replay checks of animated arms and the torso when looking down.
Held-prop tests cover eye-plane intersections, pre-projection texture-coordinate
interpolation, screen bounds, temporary face records and frame-pool lifetime.
The GPU regression also draws a triangle crossing the eye plane and verifies
its retained coverage. Carried-prop animation across every character and item
still requires gameplay verification.
Interactive Tab/L3 selection
and extended first-person play across all four characters and later levels
still require gameplay verification. Native third-person animations are reused;
no separate first-person hand models or animation blending are supplied.

Other retail builds, Fighting Force 2, other aspect ratios and later levels
have not received equivalent gameplay validation. Camera obstruction handling
uses captured opaque environment/object triangles and a local ground guard;
transparent materials, characters and transient debris are excluded. Geometry
omitted by an authored environment selector may not be available in a snapshot.
Right-stick hardware, cooperative play and later-stage camera handoffs have not
been played through. A release installer is not implemented.

The Git import splits the existing baseline into buildable feature checkpoints.
Only the final combined build has the gameplay coverage listed above; earlier
import checkpoints are build/test checkpoints, not separately played releases.
