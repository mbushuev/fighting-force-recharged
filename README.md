# Fighting Force Recharged

Current candidate: **0.1.0-rc.1**.

[Source repository](https://github.com/mbushuev/fighting-force-recharged)

An unofficial compatibility and gameplay mod for the original Windows
version of **Fighting Force (1997)**. Play on modern Windows with widescreen,
modern controls, independent cameras, optional first-person view, cooperative
split screen and campaign saves. Original game files remain unchanged on disk:
the project uses a separate x86 DirectInput proxy and runtime interception.

This is release-candidate software; the camera and rendering extensions remain
experimental. The project is not affiliated with the original rights holders.

## Get started

You need a supported English PC copy of the game and 64-bit Windows 10/11.
Extract the Windows release ZIP, run `FightingForceSetup.exe`, and choose
**Add fix to existing game folder** or **Install game from original disc**.
Then start `fforce.exe` or use the optional desktop shortcut.
See [Quick start](docs/QUICKSTART.md) for installation and default controls.

No GitHub release is published yet. Source pushes run automated verification;
the manual **Build release candidate** workflow produces downloadable Actions
artifacts without creating a release. See [release preparation](docs/RELEASING.md).

The installer supplies only compatibility files and reviewed dependencies.
It does not contain or download the game, disc images, movies or music.

## Supported game files

Compatibility testing used the English **ISO Version** (FULL disc image) and
**RIP Version** listed on
[MyAbandonware's Fighting Force page](https://www.myabandonware.com/game/fighting-force-bgp).
See [tested media and source](docs/VALIDATION.md#tested-media-and-source) for
the image format and reviewed file inventories.

The supported English retail executable has SHA-256
`92ab6179bf6497d471c6ab30d6df6f4f333d5bc3f641a154d6966b47fdba90a7`.
The reviewed English RIP executable has SHA-256
`bf1bd02dc9b6508edd76efa36e39a6cc1b82055f7d31ccda329c1ae55796fda3`.
Other executables and Fighting Force 2 are outside the validated scope.

This repository contains project-owned source, configuration and documentation.
It does not contain the game, game assets or third-party runtime binaries.

See [Development](docs/DEVELOPMENT.md) for the repository and validation policy.

## Implemented behavior

- Handle the retail PCI BIOS probes through the existing fallback path.
- Discover the mounted original disc independently of its drive letter.
- Resolve the reviewed RIP's data from its own folder without a mounted disc,
  skip its empty intro, and retain the supplied executable unchanged on disk.
- Supply primary Direct3D HAL defaults in memory if legacy video settings are
  absent, without running old setup or writing registry settings.
- Initialize DPI awareness early for correctly sized borderless presentation.
- Reacquire keyboard input and retry the same poll after focus/device loss.
- Keep window messages responsive during gameplay and pause; suspend updates
  while another application has focus or the game is minimized.
- Reuse the mounted-disc audio session between transitions.
- Continue reviewed RIP transitions silently when no audio CD is mounted,
  avoiding the native repeated track-start waits.
- Preserve native texture filtering while correcting far-plane cracks and
  transparent cutout coverage.
- Cover movie/window/renderer replacement with a dark fade and reveal ready
  frames, including the original copyright image. Retain the native loading
  picture in memory while its renderer is replaced.
- Expand the gameplay view horizontally while preserving vertical field of
  view, UI proportions and movie framing.
- Provide an experimental independent player-centered orbit in single-player
  gameplay, with matching camera-relative character movement.
- Add an experimental [dynamic cooperative camera](docs/COOPERATIVE.md): share
  a view while both players fit, then smoothly separate into independent views.
- Keep three independent campaigns with automatic level-entry saves, quick
  **Continue**, a **Load Game** menu and previous-checkpoint recovery. New games
  choose a slot before character selection; pause supports level restart.
  See [Campaign saves](docs/SAVES.md) for migration and backup instructions.

## Build and installation

Requirements: Windows, Visual Studio C++ build tools with the Windows SDK,
PowerShell and Python 3 for repository checks. Run from the checkout:

```powershell
.\build.ps1
.\tests\run.ps1
python scripts/check_repository.py --staged
```

The x86 DLL is written to `build/DINPUT.dll`. A compatible x86 dgVoodoo
DirectDraw/Direct3D runtime is required and is not included in Git. Place the
project DLL and `config/fightingforce-compat.ini` beside the supported
`fforce.exe`. The reviewed `config/dgVoodoo.conf` supplies the tested borderless
presentation profile. Keep a separate backup of existing wrapper configuration.
Do not replace or edit original game files. Keep the original disc mounted for
the retail executable. The reviewed RIP can run from its own folder; it has no
intro movie or CD music. A mounted original CD can supply its music.

The local `game` directory is an optional development installation and is
ignored by Git. A standalone installer can be built locally with
`scripts/build_installer.ps1`; see [Setup](docs/INSTALLER.md). Select
**Add fix to existing game folder** for a complete retail or reviewed RIP
folder. It adds compatibility files there and preserves game files, settings
and saves. **Install game from original disc** creates a new installation.
No game data is embedded. See [Release preparation](docs/RELEASING.md) for
building clean source and Windows packages without publishing them.

## Settings and controls

`config/fightingforce-compat.ini` documents the wrapper settings. The installed
INI must be beside `DINPUT.dll`.

- Open **Options > Controller Settings**, or **F6** on the main menu, for
  per-player devices, button remapping and cooperative camera selection.
  Keyboard/mouse can belong to either player, or choose two distinct gamepads.
  Select **Original Shared** for the native camera without splitting, or
  **Dynamic Split** for the updated camera. Save applies changes immediately.
- `GameplayFPS=60` under `[Rendering]` enables experimental interpolated
  gameplay presentation with the native simulation cadence. Set `30` and
  restart to restore native rendering. See [Frame rate](docs/FRAME-RATE.md).
- `WidescreenAspectX1000=1` selects the current monitor aspect; `0` retains 4:3.
  A manual value such as `1778` requests approximately 16:9. If no compatible
  DirectDraw display mode is available, native framing is retained.
- `SmoothTransparentEdges=1` preserves filtering while correcting cutouts.
- `SmoothTransitions=1` enables the dark transition cover and fade.
- `ResponsivePresentation=1` avoids blocking flips and services window messages
  while inactive, including the native pause loop.
- `StartupScreenMs=2500` controls the original copyright display time.
- `ShortLoadingWait=1` skips the native fixed three-second disc-spin delay;
  actual disc checks and file reads retain their normal behavior.
- `TraceRenderer=0` keeps detailed render diagnostics disabled.
- `FreeOrbit=1` enables the independent gameplay camera; `0` restores the
  native stage-directed camera after restart. Move the mouse or right stick
  to look, use the wheel for distance and middle button/R3 to recenter behind
  the character. Camera sensitivity, distance and inversion are configurable.
  A sphere sweep contracts the camera distance at opaque scene geometry and
  solid objects; the distance recovers smoothly after the obstruction clears.
- `FirstPerson=1` enables Tab/L3 to switch between orbit and an animated view
  from the character's eyes. Native arms/body remain visible; looking down
  reveals the torso and legs. The camera follows head translation during
  movement, jumps and combat, with independent mouse/right-stick look.
  Wheel and trigger zoom affect third person only. Requires `FreeOrbit=1`.
- `MouseCombat=1` adds left-mouse punch and right-mouse kick during live
  single-player gameplay. Keyboard/joystick bindings remain available; this
  option also works with `FreeOrbit=0`.
- `ModernKeyboard=1` adds WASD movement, Space jump, left Shift run, E backfist
  and J/K punch/kick in live single-player play. Esc/P opens the native pause
  menu; press Esc/P again to continue. Space no longer opens pause. Saved retail
  bindings remain unchanged. In menus use WASD, Enter/Space to confirm and Esc
  to go back. Esc on the main menu and F10 open the quit prompt; Enter/Y
  accepts and Esc/N cancels.
- `Gamepad=1` enables Xbox/XInput-compatible controllers with automatic
  reconnection. Left stick/D-pad moves, A jumps, X punches, Y kicks, B performs
  backfist and RB runs. L3 switches camera view. Right stick looks, R3 recenters, and LT/RT zooms
  out/in. Start pauses, View opens quit; A confirms and B goes back in menus.
  `ControllerIndex=-1` automatically selects an available slot; `0..3` selects
  a fixed slot. Deadzones are configurable. Return controls to neutral after
  connecting, changing scenes or switching back from another app.
- `AnalogMovement=1` gives the left stick continuous unarmed ground speed from
  slow walking to normal walking, with a short acceleration ramp and native
  collision checks. Hold RB to run; full stick alone never selects running.
  Carrying, combat and keyboard/D-pad movement retain native behavior.

See [Controls](docs/CONTROLS.md) for the complete default layout and device scope.

The original F1 and F2 shortcuts toggle texture filtering and fog. During
play, F10 then Enter returns to the main menu. Repeat there to exit the game.
On a controller, use View then A in gameplay, or B then A on the main menu.

## Validation and limitations

Run the focused deterministic tests with `tests/run.ps1`. Additional tests
require the local rendering runtime:

```powershell
.\tests\run_gpu.ps1
.\tests\run_transitions.ps1
```

The GPU runner loads the external runtime from the ignored `game` directory.
The transition test uses an offscreen window. The original-file verification
tool accepts a mounted-media root and optional game, extraction and output
locations as parameters; it writes only a generated verification manifest.

See [Validation](docs/VALIDATION.md) for completed checks and limits,
[Development](docs/DEVELOPMENT.md) for contribution policy, and
[Changelog](CHANGELOG.md) for user-visible changes. Current gameplay coverage
is the first level at 16:10. The orbit and obstruction handling are experimental.
Full campaign geometry coverage and hardware controller checks remain future work.

## License

Project-authored code and documentation are available under the [MIT license](LICENSE).
Game files and third-party components are excluded; see
[Third-party notices](THIRD_PARTY_NOTICES.md) for their separate terms.

[Native camera research](docs/CAMERA-RESEARCH.md) records the verified camera
data flow, integration boundaries and remaining camera work.
