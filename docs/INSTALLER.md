# Fighting Force Recharged installer

The setup program targets 64-bit Windows 10 and Windows 11 using the built-in
.NET Framework 4 runtime. It installs the supported English Windows retail
edition of Fighting Force from its original disc, or adds the fix to an
existing retail or reviewed English RIP folder. Game data is supplied by the
selected source.
The retail executable requires original mounted media during play.
The reviewed RIP runs from its own folder; original media is optional for music.

Start `FightingForceSetup.exe` and choose the operation:

- **Add fix to existing game folder:** select the folder containing `fforce.exe`
  and choose **Install fix**. Compatibility files are added directly to it.
  There is no second destination and no game-data copy.
- **Install game from original disc:** select the original disc root and a new
  destination, then choose **Install game**. A complete extracted disc layout
  also works; the retail executable still needs mounted original media to play.

Mounted CD drives are detected without assuming a drive letter. Selecting an
existing game folder switches to the fix operation automatically. Both modes
offer an optional desktop shortcut. The disc installation defaults to the
current user's Programs folder without requiring administrator rights there.
Existing game folders must be writable; new installations need a writable parent.

Setup uses a 96-DPI layout baseline, wraps descriptions and status text to the
available width, and sizes buttons to their labels. If the window is shortened,
the content scrolls vertically while Install and Close remain accessible.
After success, setup hides the installation action and offers **Finish**.
Enter or Escape closes the completed window; installation inputs stay locked.
Errors and cancellation leave the installation action available for another
attempt. Start setup again to install into another folder or perform an update.
Mounted full CloneCD media has installation and launch coverage. Use its CCD
descriptor to retain the audio-track layout. Setup requires a supported retail
or reviewed RIP executable and a nonempty `Wads/ENGLISH.WAD`. Unknown executable
hashes and missing main data are rejected before any wrapper write. Resources
need not match a reference size or hash. Missing intros, readmes, icons, legacy
setup files and unused resources do not block the folder fix. Existing extra
files are retained. These minimum checks do not certify that all campaign data
is present or that modified resources will work correctly in the game.
Setup never restores an executable or fills missing assets from another source.

The reviewed RIP uses a separate preservation profile: its
executable has eight modified bytes at the four PCI BIOS probes, `Intro/logo.rpl`
is empty, and `Mfc30.dll`, `Mfc42.dll` and `Msvcrt20.dll` are absent. Its other
230 retail files match. Its executable SHA-256 is
`bf1bd02dc9b6508edd76efa36e39a6cc1b82055f7d31ccda329c1ae55796fda3`.
Existing RIP files, including its intro and supplied setup files, stay in place
unchanged. No registry import or bundled setup
script is executed. The wrapper recognizes this exact EXE, handles its BIOS
changes only in memory, resolves data locally and skips the empty movie.
Missing legacy video settings receive safe Direct3D defaults in memory.
Without a mounted original audio CD, playback stays silent and the native
track retry waits are avoided. Missing intro/music content is not reconstructed.

For disc installation, setup accepts a readable InstallShield cabinet without
requiring the exact reference cabinet size or hash. It extracts only fixed
game filenames from the English and Dlls groups using unshield's old-compression
mode, discarding archive directory components, and checks the extracted retail
executable before copying. It copies available original libraries, optional
setup/readme/icon files and the complete Wads and Intro directory trees,
including extra data files. Missing optional files are skipped. Data trees
containing symbolic links or junctions are rejected instead of followed.
Each copied file is checked against the actual selected source bytes. Existing
game folders do not run the extractor or copy game data; only separate
compatibility paths can be written. Supplied executables, DLLs and assets
remain byte-identical.
The legacy installer and its DirectX installers are never executed; no game
registry settings or system libraries are installed.

The compatibility DLL, reviewed INI files and unmodified dgVoodoo components
are added as separate files. Extractor binaries exist only in private temporary
storage. Documentation and dependency notices are placed in `Compatibility`.
See [Controls](CONTROLS.md), [Frame rate](FRAME-RATE.md) and
[Cooperative camera](COOPERATIVE.md) for the installed defaults.

Existing destinations are rejected for disc installation. That operation stages
the new installation beside its destination and publishes it after verification.
Cancellation or failure removes its private staging copy.

The folder fix preserves existing `fightingforce-compat.ini`, `dgVoodoo.conf`,
saves and unrelated files. It records compatibility hashes in
`Compatibility/wrapper-files.json`. Setup adopts identical components and
updates recorded files whose hashes still match. It also recognizes reviewed
older versions of the patch's `DINPUT.dll` by their exact SHA-256, including
copies installed manually. It leaves unrecognized or edited guides in
`Compatibility` in place and continues the update. An unrecognized runtime DLL
or a directory/link at a compatibility path stops setup before writing.
Close the game before updating a loaded wrapper DLL.
Cancellation or failure restores replaced compatibility files and removes
new additions. Game files are never part of this transaction.

There is no system-wide
application registration or automatic uninstaller. Remove a no-longer-needed
installation folder and its shortcut manually, preserving any wanted saves.

For unattended local installation and verification:

```text
FightingForceSetup.exe --inspect-source --source <disc-root-or-game-folder>
FightingForceSetup.exe --apply --target <existing-game-folder>
FightingForceSetup.exe --install --source <disc-root> --target <new-folder>
FightingForceSetup.exe --verify --target <installed-folder>
```

`--media` and `--inspect-media` remain aliases for earlier disc-only commands.
`--verify` is an optional strict reference-inventory diagnostic. It selects the
profile by executable hash and reports missing or differing files. Its full
235-file size/hash checks are not an installation or update requirement.
Add `--shortcut` to the installation command to create a current-user desktop
shortcut. CLI mode does not launch the game. Unsupported executables, missing
main data, conflicting replacement files or an existing disc-install destination
produce a nonzero exit code.

Build the wrapper, then run `scripts/build_installer.ps1`. The build takes the
reviewed external binaries from the ignored local dependency locations; optional
`RuntimeDirectory`, `UnshieldDirectory` and `OutputDirectory` parameters select
other locations. Dependency hashes must match `config/installer-dependencies.json`.
The standalone setup and SHA-256 file are written under ignored `dist`.
No game files, personal settings or raw local reports enter the setup payload.
Setup includes the project MIT license and separate dependency notices. Its
payload uses repository defaults rather than an installed player's settings.
