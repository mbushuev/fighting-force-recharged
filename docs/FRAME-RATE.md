# Gameplay frame rate

`GameplayFPS=60` under `[Rendering]` enables experimental 60 Hz gameplay
presentation. `GameplayFPS=30` retains native rendering after restart. Both
settings retain one native input, actor, collision and stage update per game
step. Menus, movies, pause/confirmation loops and recorded attract playback
retain their original cadence. The optional native render thread is not
supported by this mode and falls back to native rendering.

The single-thread gameplay loop calls its renderer before polling input and
advancing the stage. The wrapper replaces only that render call with two
displayed samples, spaced at 60 Hz. The first sample interpolates the previous
and current simulation snapshots; the second uses current state. No actor,
animation-decoding, collision or stage-update function is repeated. This
retains the native 30-step cadence when the machine sustains the render budget.
It does not add catch-up simulation during a stall.

Interpolation covers actor position, animated root offsets, wrapped body/joint
angles, each active camera pose, loose-object transforms and effect positions.
Joint triples choose the equivalent Euler branch nearest the previous pose,
avoiding artificial pitch/roll when native rotation decomposition switches
branches. An exact half-turn of actor heading retains the current heading and
root rotation together; other joints and position continue to interpolate.
This keeps native backstep root/body handoffs coherent in the extra sample.
Actor interpolation uses the native render copies. Loose objects and effects
are read directly by retail render handlers, so their temporary spatial values
are restored before input/simulation resumes. The additional sample also
restores source actor/palette/environment records, HUD easing, fade counters
and the native random seed, so render effects cannot consume additional gameplay
randomness.
Source camera fields temporarily agree with the render pose because the native
single-thread matrix initializer reads those fields. Collision-mesh capture is
disabled for the interpolated sample; current geometry remains authoritative.

History is discarded at stage/player/view changes, nonconsecutive native ticks,
focus gaps and long stalls. Model/type changes and large position discontinuities
skip interpolation for the affected slot. Native pointers, action/frame indices,
hit state, material data and effect parameters are not averaged. There is no
motion prediction or image-based frame generation. Interpolation adds up to one
display interval of visual delay compared with the latest-state sample; gameplay
input is still consumed by the native simulation step.

Cooperative split-screen renders both viewpoints for each displayed sample,
with one wait per composed image. This requires four native view draws per game
step. The feature does not guarantee 60 FPS when the CPU/GPU cannot meet that
budget. Optional `[Diagnostics] TraceFrameRate=1` records aggregate presentation
and native-update rates in the ignored local wrapper log.

Deterministic fixtures cover interpolation overflow, wrapped angles, identity
and discontinuity rejection, timer epochs, complete hook-signature rejection,
temporary-state restoration, unchanged native ticks and shared/split pacing.
Live visual and performance coverage is recorded in [Validation](VALIDATION.md).
