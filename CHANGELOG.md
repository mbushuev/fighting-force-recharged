# Changelog

## Unreleased

### Gameplay camera

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

### Compatibility and input

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
