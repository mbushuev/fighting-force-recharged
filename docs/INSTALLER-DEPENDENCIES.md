# Installer dependencies

This game-specific installer contains the project's compatibility wrapper,
reviewed configuration, and unmodified third-party components listed below.
Their hashes and sizes are pinned in `config/installer-dependencies.json`.
External binaries and generated installer packages remain outside Git.

Fresh builds use `scripts/fetch_build_dependencies.ps1` and the download
inventory in `config/build-downloads.json`. The reviewed unshield/zlib Windows
archive is fetched from an immutable Scoop Binary commit and verified before
extracting only `unshield.exe` and `zlib1.dll`. The removed upstream dgVoodoo
2.86.2 archive is replaced for build retrieval by a preservation copy at an
immutable Deathtrap Native 50 commit. These are the identical, unmodified Dege
files; all five final binaries must still match this project's pinned hashes.
No binaries from that other project's wrapper are used. The downloader does
not run the fetched programs or fetch any game data.

- unshield 1.6.2, by David Eriksson and contributors: used only to extract the
  original InstallShield archive. MIT license, reproduced in
  [licenses/unshield.txt](licenses/unshield.txt).
  Source and releases: https://github.com/twogood/unshield.
- zlib 1.3.1, by Jean-loup Gailly and Mark Adler: unshield's decompression
  dependency. Its notice is reproduced in [licenses/zlib.txt](licenses/zlib.txt).
  Source and releases: https://github.com/madler/zlib.
- dgVoodoo 2.86.2, by Dege: x86 DirectDraw/Direct3D components used specifically
  with Fighting Force. The original, unmodified binaries are included alongside
  this game mod. The upstream redistribution terms are reproduced in
  [licenses/dgVoodoo.txt](licenses/dgVoodoo.txt).
  Project: https://github.com/dege-diosg/dgVoodoo2.

Retail Fighting Force files are supplied entirely by the selected original
media. They are not embedded in the installer or distributed by this repository.
The setup does not grant or replace any rights to the original game.
