# Validation

## Tested media and source

The English Windows test inputs came from the
[Fighting Force listing on MyAbandonware](https://www.myabandonware.com/game/fighting-force-bgp):

| Listing option | Tested input | Reviewed inventory |
| --- | --- | --- |
| ISO Version (English) | FULL disc image in CloneCD format (`.ccd`, `.img`, `.sub`), including the intro and CD-audio tracks | [English retail, 235 files](../config/retail-english.json) |
| RIP Version (English) | Extracted game folder with an empty intro and no bundled CD music | [Reviewed English RIP, 235 files](../config/rip-english.json) |

The executable SHA-256 fingerprints in the
[README](../README.md#supported-game-files) and these inventories identify the
reviewed builds. Disc images and game files remain outside the repository and
release packages. The coverage and limitations below apply to these inputs.

These inventories record test inputs and preservation results, not mandatory
installation contents. Current setup requires a supported executable and
nonempty `Wads/ENGLISH.WAD`; exact reference checks are optional diagnostics.
See [installation requirements](INSTALLER.md) for the user-facing policy.

## Campaign checkpoints

Three-slot fixtures cover migration without replacement of existing campaigns,
independent slot writes, recent-campaign selection, replacement cancellation,
previous-stage rewind and verified backup recovery. Slot/menu data validation
checks unsupported versions, corrupt/truncated records and inaccessible files.
Runtime fixtures preserve unrelated slots during new games, failed writes and
rewinds, and prevent attract timeouts while the save selector is open.

The original retail and RIP pause routines execute in isolated fixtures with
scripted keyboard/controller commands and stubbed platform boundaries. They
cover resume, quit, confirmed restart, canceled restart, held input and paired
native device reads. Original main-menu fixtures cover the expanded table,
Load Game, both new-game slot selectors, Continue and Options.

Native retail and RIP character-selection fixtures repeat cancellation and
confirmation with keyboard or simulated XInput across all three cooperative
layouts and both native device counts. Solo confirmation selects only the
first cursor and reaches the native start condition; device ownership remains
independent during two-player selection.

Live retail startup showed Continue and Load Game and successful legacy-save
migration into slot 1. Visual review of the expanded slot/pause screens and
live cooperative restart remain separate from these deterministic checks.

Storage fixtures cover one- and two-player records, every campaign stage and
hero type, eliminated player slots, corrupt headers/payloads, unsupported
versions, truncated or oversized files and replacement failure with a locked
destination. Failed reads leave the output unchanged; failed writes retain the
previous complete checkpoint.

Runtime fixtures cover frontend cleanup before restoration, preservation of
native spawn placement, health-bar synchronization, score/extra-life values,
stage-entry-only writes, retry and attract-demo exclusion, signature failures
and retained saves after write failures. The original main-menu routine from
both reviewed executables is exercised in an isolated process with resource,
text, sound and device calls stubbed. Continue, one- and two-player new game,
Options, wrapping and held-confirm suppression retain the expected actions.

Live retail testing confirmed automatic first-level saving, progression to the
second level, and resuming that checkpoint from the main menu with character,
lives and score retained. All 235 original files in each reviewed retail and
RIP installation matched their manifests after testing. End-to-end cooperative
resume and later campaign branches still require live gameplay coverage.

## Controller feedback

Input-hint fixtures execute the patched native pause and quit text calls, check
remapped pad and alternate-only keyboard labels, and bound line placement for
both players. Activity tests cover keyboard takeover, held-pad suppression,
stick noise, deliberate look input and focus reset.

Feedback fixtures cover pulse expiry, bounded overlap, independent controller
slots, mixed-device cooperative layouts, setting persistence and focus/menu
suppression. The runtime fixture executes the damage detour and trampoline,
checks native return values and health results, and can execute the reviewed
retail or RIP damage helper from a supplied executable. A real timer with a
mock XInput sink verifies expiry without rendered frames. Physical vibration
strength remains subject to controller testing.

Object-feedback fixtures exercise all four accepted contact dispatch sites and
four actual firearm trace calls, preserving arguments, return values and player
ownership. They cover rejected contacts, native result values 1 and 2, both
controller slots, setting/menu suppression and unattributed aiming probes.
With a reviewed retail or RIP executable, the fixture runs the original object
contact routine and destructible-prop/car handlers. Durability and car state
match an unhooked baseline; native duplicate-contact bits, empty space and
disabled bounds suppress feedback. The fixture also checks the separate rotated
object bounds path; audio, particle and impact-direction calls are stubbed.

Weapon-recoil fixtures execute all three patched muzzle-position calls and
check unchanged arguments, output coordinates and return values. They cover
both players, enemy exclusion, signature failure and setting/focus/menu/device
suppression. With a reviewed retail or RIP executable, they execute the original
firing routines against an unhooked baseline, including one or three aiming
rays, missing weapons and failed projectile allocation. Each successful
discharge emits one recoil event regardless of hit result. Audio, skeleton
position, projectile allocation and collision traces are stubbed in this fixture;
live controller testing is still required to assess recoil feel.

Recoil-profile cases cover all five native pistol types, the shotgun and heavy
weapons, either held-item slot, native hand priority and invalid slot bounds.
The retail/RIP firing fixtures check profile selection alongside unchanged
projectile state and trace results. Envelope tests cover stronger weapon
profiles, rapid-fire refresh, switching from a heavy profile to a pistol,
independent controllers and bounded overlap with combat feedback.

## Player guard

Guard regression fixtures reproduce short focus switches, masked input and
scripted control locks. They check immediate protection revocation, restoration
of native idle and a fresh release/press boundary before guard can resume.
Cooperative contact fixtures execute the original target-iteration instructions
with overlapping player capsules. Repeated contacts reach the second player,
whose own guard or native damage path is evaluated independently.
Release fixtures cover all four guard phases and eight movement/combat commands,
checking that the native input survives the handoff for all heroes, both player
slots, empty hands, firearms and melee items. Lifecycle checks cover
re-pressing during lowering and prevent command cancellation under script locks.
Input fixtures run keyboard/gamepad, two-gamepad and gamepad/keyboard routing
through XInput polling and remapped Block buttons/triggers. Each player's pose
and protection remain independent. These fixtures do not replace live gameplay
acceptance of transition appearance, first-person visibility or controller feel.
Standing-hit recovery fixtures cover every supported reaction, all heroes, both
player slots and empty/light-item loads. They exercise the native completion
call site and an immediate subsequent enemy contact, preserving damage already
taken and inventory. Incomplete reactions remain vulnerable; release, focus
loss, script locks, airborne state and disabling guard prevent buffered entry.
Optional retail/RIP checks verify the original completion instructions and call
target as well as executing the native held-item stance selector.
Turn fixtures execute the original backstep completion instructions for every
variant, hero and player slot across cardinal and wrapped headings. Block leaves
unfinished poses under native control and enters after exactly one heading
handoff. Retail/RIP checks verify the original turn-completion bytes.

The Gameplay Settings checks cover the native Options row/text hooks, callback
dispatch, duplicate shared-device confirmation and discarded drafts. Configuration
tests cover independent combat/visual switches, atomic saves and failed writes.
Saving gameplay choices preserves device assignments and unrelated INI values;
native Options navigation and other rows remain in charge.

Combat focus tests cover eligible enemy selection, screen-direction switching,
release edges, range/occlusion/death transitions, angular wrapping, player
ownership and native attack-direction preservation. Weapon attachment checks
cover barrel transforms, lowered poses, ammunition colors, foreign-actor
rejection, scene consumption and approximate actor intersections. Overlay
checks restore every changed device state after successful or failed draws and
partial state-query failure. Throw preview fixtures compare native fixed-point
sample positions for grenade and heavy-item launches, test nearest scenery
contact, both player owners, current render transforms and scene consumption.
They cover the real held-item thunk, disabled settings, level/menu transitions,
the surviving second player, split-view ownership and mismatched physics
signatures without modifying original files or advancing native item state.
Gameplay Settings layout checks include every selected row and the footer,
verifying text bounds and a single selected item.
These checks do not establish exact release-animation placement, native contact
volume agreement or full-campaign throwing coverage. See
[Combat focus and aiming previews](COMBAT-FOCUS.md) for limits.

## Disc and folder setup

The setup core and standalone executable were checked against the supported
original disc. A fresh installation in a path containing spaces and non-ASCII
characters matched all 235 retail file sizes and SHA-256 hashes. Verification
also covered wrapper/runtime placement, dependency notices, refusal to overwrite
existing files, invalid-media rejection, cancellation during copying, and
rollback after injected installation errors.
The installer payload contains no retail executables or assets. The old
InstallShield NE bootstrap and its bundled DirectX installers are not executed.
The target is Windows 10/11 x64 with .NET Framework 4; no separate Windows 10
machine or alternate retail edition is claimed as validated by these checks.
The full English CloneCD image passed the same 235-file installation and
verification checks, and interactive installation and game launch were
confirmed. Its track table contains one data track and 23 audio tracks;
this check does not establish audible playback of every track.

Read-only comparison of the English RIP folder found 230 matching retail files,
one modified executable, one empty movie and three absent libraries. Its EXE
has SHA-256 `bf1bd02dc9b6508edd76efa36e39a6cc1b82055f7d31ccda329c1ae55796fda3`;
the eight changed bytes replace the four PCI BIOS interrupts. Runtime fixtures
cover both image signatures, rejection of mixed/short images, in-memory BIOS
restoration, scoped registry interception, executable-relative data paths and
empty-intro skipping with the native call convention. A standalone RIP session
without mounted original media confirmed first-level startup, modern keyboard
and controller input, orbit camera and Tab/L3 first-person switching.
Missing-video-settings fixtures retain existing settings and provide the
primary Direct3D HAL defaults only when absent. A live launch forced that
fallback without deleting registry settings and reached playable gameplay.
This is first-level coverage, not a full-game RIP compatibility audit.
Folder-fix checks exercise both the complete retail profile and the reviewed
RIP in isolated fixtures, then verify all selected sizes/hashes, file identities
and modification times. The installer leaves game data in its existing folder.
The RIP's empty movie, supplied setup files and absent libraries remain as
supplied. Checks compare its complete source inventory before and after
installation and reject missing main data before compatibility writes.
Current installation checks require a supported executable and nonempty
`Wads/ENGLISH.WAD`, without requiring the rest of the reference inventory.
Disposable retail/RIP fixtures with changed resources, missing optional
intro/readme/icon/setup files and extra data pass installation and repeat
updates while retaining source bytes, modification times, identities, settings
and saves. Empty or missing main data and unsupported executables are refused
without compatibility writes. Strict `--verify` still rejects non-reference
fixtures independently of the installation policy.
Disc fixtures with a padded readable cabinet, changed resources, missing
optional files and nested extra data copy successfully and match their actual
selected source. They validate installer behavior, not gameplay with modified
data or compatibility of unreviewed game executables.
Transaction fixtures preserve custom/read-only settings and save files, refuse
unmanaged DLLs, cancel after the first wrapper write, restore a previous owned
wrapper after an injected error, refuse a locked DLL and exercise repeat/update
operations. New disc installations record wrapper ownership for later updates.
Audio fixtures check cached absent-media detection, immediate MCI failure,
native track calls with scoped music suppression, preference restoration,
mounted-disc forwarding and atomic rejection of changed call signatures.
Live standalone transitions no longer spend about six seconds retrying absent
CD audio; menu/level transitions were confirmed without the previous delay.

Layout fixtures exercise proportional fonts and geometry at 100%, 125%, 150%,
175%, 200%, 250% and 300%, including compact/restored windows, long paths and
multiline status text in both disc and folder-fix modes. They check label and button bounds, visible actions and
vertical scrolling without horizontal overflow. These fixtures do not change
Windows display settings or establish per-monitor DPI switching support.

## In-game control settings

Control fixtures check exclusive keyboard/mouse ownership, two distinct fixed or
automatic controller slots, player-specific action routing, keyboard/mouse and
trigger/button mapping, duplicate-binding rejection and isolated menu drafts.
Real temporary INI round trips preserve unrelated settings; a failed read-only
replacement preserves the previous configuration and removes temporary output.

Native menu fixtures reject changed call signatures before writes, execute the
patched indirect-call boundary, open the settings from the native Controller
Settings callback and restore the Options callback. They verify input isolation,
neutral return, per-page text bounds with a fixture font and immediate application
of device/camera settings. Selection uses the native font renderer's signed
channel offsets, with neutral bright and dim rows; fixtures check each selected
row and capture/error text bounds. Cooperative runtime fixtures retain modern actions,
one native view and stage camera output in Original Shared mode. These fixtures
do not establish physical controller capture or complete interactive menu coverage.
Survivor fixtures defeat either actor and check the other's right-stick look,
first-person toggle, full-width render ownership and subsequent two-player return;
both actors defeated releases custom camera ownership.
An exclusive-keyboard COM fixture exercises real DirectInput state polling,
reacquisition, focus clearing, extended arrows, wrapper menu navigation and key
capture. It also checks keyboard navigation in shared two-player Options and
separate device ownership in character selection. Footer text has a distinct tint.
The shared native reader fixture passes different legacy device indices with
each logical player index through the x86 gateway. Shared frontend keyboard
navigation, neutral menu selection and original intro playback also have
interactive retail coverage. A survivor gameplay session is not claimed here.
Character-selection fixtures execute both installed reader gateways with
different legacy device and logical player indices. They check solo keyboard
and pad merging across all three cooperative assignments and four temporary
display counts, while requested two-player selection retains separate owners.

## Gameplay rendering

The frame-rate fixtures check two displayed samples per unchanged native tick,
four cooperative view draws with two completed-image pacing boundaries, and
restored actor/palette/object/environment, source camera, HUD, fade and random
seed state.
Interpolation tests cover numeric limits, wrapped angles, identity changes,
teleports, view/stage changes, tick discontinuities and long clock gaps.
Rotation fixtures check equivalent Euler branches against independently
constructed native root matrices, retain genuine pitch/roll motion, and
reproduce the sideways pose from independent component blending. Native render
boundary fixtures cover branch switches and both directions of root/body
half-turn handoff, retaining current source poses and native update ownership.
These fixtures alone do not establish live 60 FPS or a full-game timing audit.

A live 1280 by 800 session recorded sustained 60 completed gameplay images and
30 native updates per second. The session included first-person and cooperative
camera ownership. Interactive movement/combat checks retained ordinary game
speed with smoother presentation and no visible interpolation artifacts.
This does not establish every stage, split configuration or resolution. Native
pause/CD-control stalls can still interrupt presentation. Diagnostic aggregates
restart when the stage or native tick epoch changes, and after long time gaps;
they never interpret a restarted tick counter as unsigned gameplay progress.

The cooperative fixtures cover split/merge hysteresis, off-axis composition,
two distinct XInput slots, per-player movement headings and first-person model
scope. The native-boundary fixture checks that two draw passes retain one
simulation tick and restore camera/projection fields, HUD easing and fades.
GPU readback verifies both independent image halves and compositor cleanup.
Native clock-call fixtures verify one pacing/measurement boundary per completed
frame and bypass the intermediate view's native busy wait.
Interactive two-controller coverage is tracked separately; a cooperative
playthrough is not claimed.

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
- Execute all six vehicle/prop origin call boundaries with a rejected native
  origin. Verify custom orbit/eye forwarding to face clipping, original-camera
  rejection and atomic refusal of each changed signature. With eye support
  unavailable, a vehicle handler and the installed scene projector retain a
  large face at three origins behind the camera. Shared and independent
  cooperative views cover centered, wide and off-axis projections. Native
  object-state rejection, source faces and actor/simulation origin gates remain
  unchanged; held-prop and tiled-floor special handling stays scoped to eyes.
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
- Low first-person skull poses retain eye clearance at multiple actor heights
  without removing normal animation or jump translation. Scene floor faces
  crossing the eye plane retain coverage through the installed projector and
  its native trampoline. Verify winding, source-buffer preservation, recursive
  held-object forwarding and original directed-camera/demo paths. GPU floor draws
  cover several low eye heights without a clear-color gap below the horizon.
- Dedicated tiled-ground interception preserves native tile records, disabled
  cells, opaque alpha and the far-depth sentinel. The optional
  `tests/run_retail_floor.ps1` maps the hash-verified retail image into an isolated
  test process and executes its original ground-grid projector. It reproduces
  rejected cells and missing lower-view coverage before interception, then
  verifies retained coverage at four eye heights and no restored absent tiles.
  The GPU test retains the floor as a background beneath later geometry.
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
- Gameplay return to the main menu through the quit confirmation.
- Full mouse orbit and camera-relative movement on the first level.
- First-person carried-wheel visibility during movement and camera turns, and
  continuous pistol retreat without repeated backstep resets.
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
  with running selected explicitly by holding RB; L3 selects the camera view.
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
Extended first-person play across all four characters and later levels
still requires gameplay verification. Native third-person animations are reused;
no separate first-person hand models or animation blending are supplied.

Other retail builds, display aspect ratios and later levels
have not received equivalent gameplay validation. Camera obstruction handling
uses captured opaque environment/object triangles and a local ground guard;
transparent materials, characters and transient debris are excluded. Geometry
omitted by an authored environment selector may not be available in a snapshot.
Extended two-controller play and later-stage camera handoffs have not been
played through. The local original-disc installer remains a development build.

The Git import splits the existing baseline into buildable feature checkpoints.
Only the final combined build has the gameplay coverage listed above; earlier
import checkpoints are build/test checkpoints, not separately played releases.
