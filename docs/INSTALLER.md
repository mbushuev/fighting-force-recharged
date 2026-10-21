# Disc and game-folder installer

The setup program targets 64-bit Windows 10 and Windows 11 using the built-in
.NET Framework 4 runtime. It installs the supported English Windows retail
edition of Fighting Force 1 from its original disc or a verified game folder.
It also accepts the reviewed English RIP profile described below. It does not install the
PlayStation-based Fighting Force Collection and does not contain retail game
data. The retail executable requires original mounted media during play.
The reviewed RIP runs from its own folder; original media is optional for music.

Start `FightingForceSetup.exe`, select the disc root or game folder and a new destination,
and choose whether to create a desktop shortcut. Mounted CD drives are detected
without assuming a drive letter. A source directory containing the complete
disc layout is also accepted for installation; playing still requires mounted
original media. The default destination is the current user's Programs folder.
Administrator rights are not required for that location. The destination's
parent folder must exist and be writable.

Setup uses a 96-DPI layout baseline, wraps descriptions and status text to the
available width, and sizes buttons to their labels. If the window is shortened,
the content scrolls vertically while Install and Close remain accessible.
Mounted full CloneCD media has installation and launch coverage; its data files
match the supported 235-file inventory. Mount the complete image using its CCD
descriptor to retain the audio-track layout. A game folder must match either
the complete retail inventory or the reviewed RIP inventory. Setup identifies
the profile by executable hash and verifies every required file before copying.
Unknown executables and incomplete folders are rejected before extraction or
destination creation. Setup never restores an executable or fills missing assets
from another source.

The reviewed RIP uses a separate preservation profile: its
executable has eight modified bytes at the four PCI BIOS probes, `Intro/logo.rpl`
is empty, and `Mfc30.dll`, `Mfc42.dll` and `Msvcrt20.dll` are absent. Its other
230 retail files match. Its executable SHA-256 is
`bf1bd02dc9b6508edd76efa36e39a6cc1b82055f7d31ccda329c1ae55796fda3`.
Setup copies all 235 files in this profile, including the empty intro and its
three extra supplied files, unchanged. No registry import or bundled setup
script is executed. The wrapper recognizes this exact EXE, handles its BIOS
changes only in memory, resolves data locally and skips the empty movie.
Missing legacy video settings receive safe Direct3D defaults in memory.
Without a mounted original audio CD, playback stays silent and the native
track retry waits are avoided. Missing intro/music content is not reconstructed.

Setup checks the original InstallShield archive against a reviewed hash,
extracts the English executable/setup and original libraries with unshield's
old-compression mode, then copies the disc's Wads, Intro, readme and icon.
Folder sources use direct verified copying and do not run the archive extractor.
All 235 files in the selected profile are verified before and after copying and again before
completion. Original executables, DLLs and assets remain byte-identical.
The legacy installer and its DirectX installers are never executed; no game
registry settings or system libraries are installed.

The compatibility DLL, reviewed INI files and unmodified dgVoodoo components
are added as separate files. Extractor binaries exist only in private temporary
storage. Documentation and dependency notices are placed in `Compatibility`.
See [Controls](CONTROLS.md), [Frame rate](FRAME-RATE.md) and
[Cooperative camera](COOPERATIVE.md) for the installed defaults.

Existing destinations are rejected, including previously installed copies.
Setup stages the new installation beside its destination and publishes it only
after verification. Cancellation or failure removes its private staging copy;
it does not repair or merge existing installations. There is no system-wide
application registration or automatic uninstaller. Remove a no-longer-needed
installation folder and its shortcut manually, preserving any wanted saves.

For unattended local installation and verification:

```text
FightingForceSetup.exe --inspect-source --source <disc-root-or-game-folder>
FightingForceSetup.exe --install --source <disc-root-or-game-folder> --target <new-folder>
FightingForceSetup.exe --verify --target <installed-folder>
```

`--media` and `--inspect-media` remain aliases for earlier disc-only commands.
Verification selects the correct profile from the installed executable hash.
Add `--shortcut` to the installation command to create a current-user desktop
shortcut. CLI mode does not launch the game. Unsupported media, modified files
or an existing destination produce a nonzero exit code.

Build the wrapper, then run `scripts/build_installer.ps1`. The build takes the
reviewed external binaries from the ignored local dependency locations; optional
`RuntimeDirectory`, `UnshieldDirectory` and `OutputDirectory` parameters select
other locations. Dependency hashes must match `config/installer-dependencies.json`.
The standalone setup and SHA-256 file are written under ignored `dist`.
No game files, personal settings or raw local reports enter the setup payload.
