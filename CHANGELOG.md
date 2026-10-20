# Changelog

## Unreleased

### Gameplay camera

- Add an animated first-person view for live single-player gameplay, toggled
  with Tab or L3. Follow the animated skull position through movement, jumps
  and combat while retaining user-controlled look and the original arms/body.
  Exclude only local head faces using temporary render records. Preserve the
  third-person distance when switching back; reserve L3 for view selection
  and retain RB as the controller run button.
- Preserve intact destructible hall partitions when crossing into the next
  section during free orbit. Suppress only the transition's bulk removal;
  retain native hit/breakage paths and the state of already broken partitions.
- Retain the second stage's hall doorway wall across its section boundary and
  native view-yaw gate during free orbit. Keep it in camera obstruction geometry
  without restoring broken partitions or changing native progression records.
- Recenter R3 and the middle mouse button behind the actor by converting
  actor heading to the opposite-facing native view yaw.
- Preserve reverse-side opaque level walls during free orbit using temporary
  two-sided face records at the native CPU projector. Retain native material
  rules and leave original model buffers, props and attract scenes unchanged.
- Increase the default trigger zoom rate threefold and expose
  `TriggerZoomUnitsPerSecond` without changing mouse wheel steps.
- Retain collision geometry for unchanged live solid objects when their render
  model is temporarily omitted. Invalidate it on movement, carrying or removal,
  and delay outward boom recovery until clearance is stable.
- Retain environment-record vehicle contacts across the native origin visibility
  gate as well as the separate object pass. Invalidate removed, moved or replaced
  records without publishing a temporarily empty obstacle.
- Add an experimental independent 360-degree player-centered orbit for live
  single-player gameplay, with mouse/right-stick look, wheel distance and
  middle-button/R3 recentering. Preserve stage progression, camera-relative
  movement, native scene playback and cooperative framing.
- Retain native render-pose publication and discard mouse movement after
  focus changes.
- Bound the lowest orbit angle by distance and local ground clearance, rounded
  conservatively to the native angle grid, to prevent below-feet viewpoints.
- Sweep a camera sphere against opaque environment and destructible-object
  triangles captured before native visibility clipping. Contract the camera
  boom immediately at obstructions and restore distance smoothly afterward.
  Exclude transparent material rectangles, characters and transient debris.
- Publish environment and solid-object collision geometry as one complete
  frame so vehicles cannot disappear between native rendering passes.
- Grow the obstruction probe near the player so a hood touching the pivot's
  comfort radius cannot collapse an otherwise clear camera view. Keep full
  sphere clearance farther along the boom and test nearby walls continuously.

### Compatibility and input

- Keep CD music playing when the retail closes its control session immediately
  after playback or status commands. Retain native explicit stop commands and
  report successful playback requests in the local runtime log.
- Open main-menu quit confirmation with Esc. Retain Esc as submenu Back and
  gameplay pause, and require release after returning from a submenu or cancelling.
- Poll the selected controller during native movie playback. A, B or Start
  requests the native skip/cleanup path, with focus and release gates between clips.
- Replace Q with F10 for the modern keyboard quit shortcut. Add B to the main
  menu quit prompt while preserving B as back/cancel in submenus and gameplay.
- Require RB/L3 for controller running. Full stick deflection stays within walking
  speed and cannot synthesize run attacks; releasing run immediately clears its action.
- Add continuous left-stick walking speed with button-controlled running, using each
  character's native animation displacement and a short acceleration ramp.
  Apply speed before the original actor collision solver; retain native combat,
  airborne, carrying, keyboard/D-pad, cooperative and demo movement.
- Add single-player WASD movement, Space jump, Shift run, E backfist and J/K
  attacks alongside mouse combat. Move pause from Space to Esc/P and support
  WASD navigation and Enter/Esc quit confirmation without changing saved bindings.
- Map XInput controllers to gameplay actions, frontend menus, pause and quit.
  Share automatic controller selection between movement and right-stick orbit,
  add trigger zoom, radial deadzones and reconnect/focus/scene neutral gates,
  and suppress duplicate legacy joystick actions from the selected input path.
- Start the supported original Windows executable through a separate wrapper
  without editing retail files.
- Discover the mounted original disc independently of its drive letter.
- Reacquire keyboard input and retry the same poll; clear stale state when a
  read fails.
- Add left-mouse punch and right-mouse kick to live single-player gameplay at
  the native action boundary, retaining keyboard/joystick bindings and combos.
  Keep mouse attacks inactive in menus, demos and while another app has focus.
- Reuse the original CD audio session across scene transitions while handling
  remounted discs and failed commands.

### Rendering and presentation

- Cover loading at its native entry and skip the fixed three-second disc-spin
  busy wait. Preserve actual media/file reads and expose `ShortLoadingWait`.
- Use nonblocking native surface flips and suspend background game updates while
  continuing the original Windows message dispatcher, including native pause
  loops that omit message polling. Reset relative mouse,
  controller release gates and camera timing after focus loss.
- Retain an in-memory copy of the completed native loading image across its
  immediate renderer teardown. Display it with preserved proportions until
  replacement game frames are ready, then fade into the next scene.
- Initialize DPI awareness early for correctly sized borderless presentation.
- Stabilize the native far-plane depth marker without changing texture
  filtering, texture coordinates or original vertex buffers.
- Correct filtered transparent edges while preserving opaque geometry,
  vehicle wheels and native translucent passes.
- Fade across movie, window and renderer replacement without desktop flashes;
  preserve the original copyright image and proportional movie framing.
- Expand the horizontal gameplay view using supported widescreen display
  modes while keeping vertical field of view and UI proportions.
- Keep world polygons and shadows outside normalized UI mapping, avoiding
  displaced texture fragments, missing side geometry and flickering shadows.

### Development

- Document native camera ownership, input coupling and implementation boundaries.
- Add a read-only, hash-checked camera reference analysis tool.
- Decode inspection ranges from the requested address to avoid carrying
  instruction alignment across embedded executable data.
- Establish a source-only repository with focused rollback checkpoints,
  portable build-tool discovery and checks for private or generated content.
