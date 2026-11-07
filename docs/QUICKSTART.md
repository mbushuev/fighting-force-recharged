# Fighting Force Recharged

An unofficial compatibility and gameplay mod for the original Windows version
of Fighting Force (1997). You must supply a supported English PC copy. This
package does not contain the game, disc images, movies or music.

## Install

1. Extract the Windows release ZIP and run `FightingForceSetup.exe`.
2. Choose **Add fix to existing game folder** for an existing supported game
   folder, or **Install game from original disc** for a new installation.
3. Start `fforce.exe` or use the optional desktop shortcut.

Setup targets 64-bit Windows 10/11. The game and compatibility wrapper remain
32-bit. No system libraries or legacy DirectX installers are installed.
Close the game before updating the compatibility files.

Setup requires a supported executable and nonempty `Wads/ENGLISH.WAD`.
Resources do not have to match an exact reference inventory. Missing optional
intro, readme, icon or legacy setup files do not block installation.
The optional strict `--verify` diagnostic is not required to install or update.

The full English retail version needs its original mounted disc for gameplay
and CD music. The reviewed English RIP runs from its own folder and has no
intro or CD music unless original audio media is mounted. Missing content is
not reconstructed. Other editions are rejected by setup.

## Play

Use F/RB to pick up and Q/LB to throw or drop. Use E/B for Backfist or
enemy grabs. D-pad up/down zooms and left/right turns the camera.
Toggle **Separate Pickup** in Gameplay Settings to restore pickup with Punch.
See [controls](CONTROLS.md#separate-item-pickup) for saved-layout migration and remapping.
Open **Pause > Move List** for hero-specific
commands and current button assignments for each player. Preview characters
before play in **Options > Move List**.

| Action | Keyboard/mouse | Xbox-compatible controller |
| --- | --- | --- |
| Move | WASD | Left stick |
| Look | Mouse | Right stick |
| Punch / kick | Left / right mouse | X / Y |
| Jump / run | Space / hold Shift | A / push the left stick near its edge |
| Backfist / native enemy grab | E | B |
| Pick up an item or detachable scenery | F | RB |
| Throw / drop a held item | Q | LB |
| Fire a held firearm | Left mouse | RT |
| Block (when enabled) | Hold R | Hold LT |
| Camera distance | Mouse wheel | D-pad up / down |
| Additional horizontal camera turn | Remappable | D-pad left / right |
| First-person view | Tab | L3 |
| Focus / release target | Middle mouse | R3 |
| Switch target while focused | Z / C | Flick right stick left / right |
| Pause | Esc | Start |
| Menus | Arrows/WASD, Enter, Esc | D-pad/stick, A, B |

Open **Options > Controller Settings** to assign devices, remap buttons,
configure vibration and choose original shared or dynamic split cameras.
Single-player accepts both keyboard/mouse and a controller. The optional
**Player Block** setting is in **Options > Gameplay Settings** and defaults to OFF.
That screen also controls target focus and its optional marker, the
first-person pistol/shotgun laser, a grenade/heavy-item flight guide and
**Complete Rooms** for reconstructed boundaries in selected areas.
See [Combat focus and aiming previews](COMBAT-FOCUS.md) for controls and native
firing/throwing limits.

Campaign progress saves automatically at level entry in one of three slots.
Use **Continue** or **Load Game** on the main menu. These are level-start saves,
not mid-fight snapshots. Existing single-slot progress migrates automatically.
Back up `fightingforce-save-*.sav` and their `.sav.bak` files before moving an
installation. Keep any existing configuration and saves when updating.

## Details and limitations

Read [installation](INSTALLER.md), [controls](CONTROLS.md),
[saves](SAVES.md), [cooperative camera](COOPERATIVE.md) and
[frame rate](FRAME-RATE.md) for complete instructions.

Free orbit, first-person view, dynamic split screen and interpolated 60 FPS
are experimental. Full-campaign and extended two-controller coverage remains
incomplete. Use `FreeOrbit=0` or `GameplayFPS=30` in the wrapper INI to restore
the corresponding native behavior after restarting the game.

Project code is MIT-licensed. See the included project license and
third-party notices for the bundled components' separate terms.
