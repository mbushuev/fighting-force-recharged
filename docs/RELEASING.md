# Release preparation

Project name: **Fighting Force Recharged**.
GitHub repository name: `fighting-force-recharged`.

Publication is a separate maintainer action. The local scripts do not create
repositories, remotes, tags, releases or uploads.

## Review the repository

- Keep game installations, CD images, dependency binaries, saves, captures,
  logs, build output and local tool state outside tracked files.
- Run `python tests/repository_policy_test.py` and
  `python scripts/check_repository.py --history`. The history audit checks
  every distinct reachable file version, tree checkpoint, commit and tag message.
- Review the staged file list and run the staged audit before each commit.
  Commit author and committer metadata are public when Git history is published;
  use the intentionally configured repository-local maintainer identity.
- Inspect references with `git for-each-ref`. Only intended branches and tags
  should be published. Unreachable objects and reflogs do not belong in a
  release. Never distribute a copy of the development workspace or its `.git`.
  Do not mirror internal tool references to a public remote.
- Retain relevant technical research and tests in the source repository;
  end-user packages contain only setup, instructions, licenses and changelog.

The checks detect known credential formats, private keys, local paths,
conversation metadata, binary/media files and private tool directories.
They supplement review and cannot prove that arbitrary sensitive text is absent.

## Build a candidate

Use a clean, committed checkout with the reviewed external dependencies in the
ignored local locations described in [installer dependencies](INSTALLER-DEPENDENCIES.md).
The installed game and its personal INI files are never packaging inputs.

```powershell
.\scripts\build_release.ps1 -Version 0.1.0-rc.1
```

The script audits history, runs policy/package tests, builds the wrapper,
runs deterministic runtime suites and builds setup. It reads setup's embedded
payload without running setup and verifies an exact file allowlist, project
file contents and the pinned hashes of unmodified dependencies. It also checks
the project binaries for the local build/user path.

`dist/releases/<version>` is a new directory; an existing candidate is never
silently replaced. It contains:

- `FightingForceRecharged-<version>-Windows.zip`: the game-specific setup,
  quick start, user documentation, project license, dependency notices and
  changelog. No game executable, game assets, disc images, local configuration,
  saves, screenshots or raw reports are included.
- `FightingForceRecharged-<version>-Source.zip`: only the committed source tree
  exported by Git and independently audited. It contains neither `.git` nor
  ignored workspace content or third-party binaries.
- `release.json`: source commit, version, candidate status, artifact sizes and
  SHA-256 hashes, without local paths or machine metadata.
- `SHA256SUMS.txt`: hashes of the archives and release manifest.

Candidate builds are not automatically code-signed. Git maintainer identity
does not imply a cryptographic Git signature or an Authenticode signature.

## Validate before publication

Run the original native menu/character-selection and pause fixtures with both
reviewed executables, plus installer core and layout tests. Test clean install
and folder update without changing original files. Check current configuration
and saves survive an update, and run a short live session covering menus,
controller return, campaign slots, loading, focus changes and both cameras.

Expanded save/pause screens, extended cooperative play and later campaign
geometry still need broader live coverage; retain the prerelease label while
these limits apply. Refer to [validation](VALIDATION.md) for the recorded scope.
Do not infer full campaign or hardware compatibility from deterministic tests.

Before attaching artifacts, compare their hashes with `SHA256SUMS.txt` and
publish only the reviewed files. Publishing a Git branch sends its reachable
history and public author metadata; attaching the source ZIP sends no history.
Never attach the development directory, raw logs or media folders.
