# Changelog

## Unreleased

### Compatibility and input

- Start the supported original Windows executable through a separate wrapper
  without editing retail files.
- Discover the mounted original disc independently of its drive letter.
- Reacquire keyboard input and retry the same poll; clear stale state when a
  read fails.
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

- Decode inspection ranges from the requested address to avoid carrying
  instruction alignment across embedded executable data.
- Establish a source-only repository with focused rollback checkpoints,
  portable build-tool discovery and checks for private or generated content.
