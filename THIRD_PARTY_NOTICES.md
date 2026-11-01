# Third-party notices

The MIT license in this repository covers the project-authored wrapper,
installer code, tools and documentation. It does not relicense Fighting Force,
the original game data, third-party binaries or their notices.

The game-specific Windows installer bundles these reviewed, unmodified
components. Versions, sizes and SHA-256 hashes are pinned in
`config/installer-dependencies.json`; their binaries remain outside Git.

| Component | Version | License or redistribution terms | Notice |
| --- | --- | --- | --- |
| unshield | 1.6.2 | MIT | [unshield notice](docs/licenses/unshield.txt) |
| zlib | 1.3.1 | zlib license | [zlib notice](docs/licenses/zlib.txt) |
| dgVoodoo | 2.86.2 | Upstream game/mod redistribution terms; not MIT | [dgVoodoo terms](docs/licenses/dgVoodoo.txt) |

Upstream projects:

- [unshield](https://github.com/twogood/unshield)
- [zlib](https://github.com/madler/zlib)
- [dgVoodoo](https://github.com/dege-diosg/dgVoodoo2) and its
  [redistribution terms](https://dege.freeweb.hu/dgVoodoo2/ReadmeGeneral/#1-redistribution-rights)

The source archive contains no third-party runtime binaries. The Windows
package contains the Fighting Force-specific installer, documentation and
notices. It is not a standalone or general-purpose distribution of dgVoodoo.
The installer obtains all game files from the selected original media or
existing game folder and preserves them byte for byte.

Fighting Force and its original content belong to their respective rights
holders. This is an unofficial community project; it is not affiliated with
or endorsed by the original developer or publisher. No game files, disc images,
extracted animations or music are distributed by the project.
