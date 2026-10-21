# Installer dependencies

This game-specific installer contains the project's compatibility wrapper,
reviewed configuration, and unmodified third-party components listed below.
Their hashes and sizes are pinned in `config/installer-dependencies.json`.
External binaries and generated installer packages remain outside Git.

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
