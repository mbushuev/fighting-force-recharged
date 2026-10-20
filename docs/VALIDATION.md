# Validation

The current development baseline was checked on Windows with the supported
English retail executable. These results cover the implemented compatibility
and presentation changes; they are not a full-game completion claim.

## Automated coverage

- Keyboard loss and reacquisition, including failed reads and stale state.
- Mounted-disc CD audio reuse, disc changes and failed MCI commands.
- Exact far-plane depth correction without changing other vertex fields.
- Transparent texture pixels, opaque cutouts and native translucent passes.
- Transition readiness, stale generations and renderer/movie lifetimes.
- Widescreen mode selection, uniform UI scaling and unchanged world polygons
  when reused records retain a normalized-screen flag.
- Eight real D3D2 device cycles covering point/linear filtering, fog off/on,
  later geometry through cutouts, render-state restoration and COM cleanup.
- Offscreen transition windows covering fade, non-activating styles, destruction
  and worker-thread cleanup.
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
- Camera call-site signature rejection, execution of the patched indirect
  stage boundary, preservation of native stage side effects and native render
  publication, plus real system DirectInput mouse format initialization.

## Gameplay coverage

- Startup movie, copyright image, main menu and character selection.
- First-level gameplay on a 16:10 display with a 1280x800 game render target.
- Expanded horizontal view with preserved object proportions and centered UI.
- No displaced texture fragments, missing side geometry or flickering shadows
  in the verified first-level run after restricting screen mapping to native
  normalized polygon kinds.
- Filtered vegetation and menu text, visible vehicle wheels, and both native
  fog states.
- Gameplay return to the main menu through Q followed by Y.
- Full mouse orbit and camera-relative movement on the first level.
- Read-only capture of native environment/object geometry in an attract scene,
  with computed obstruction contacts while demo camera ownership stays native.

The original-file check compared 235 installed retail files against the source
media/extraction. All 235 matched. The supported executable hash is documented
in the README. Generated manifests and raw runtime logs remain local.

## Limits

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
