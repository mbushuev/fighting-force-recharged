# Release preparation

Project name: **Fighting Force Recharged**.
GitHub repository name: `fighting-force-recharged`.

Repository: https://github.com/mbushuev/fighting-force-recharged.
Publication is a separate maintainer action. Local build scripts do not create
tags, releases or uploads.

The checks below apply to patch source, build inputs and release artifacts.
They are not requirements for every file in a player's game folder. Setup's
minimum executable/data checks and optional reference diagnostic are described
in [installation](INSTALLER.md).

## Main-based workflow

Work on `main` by default and avoid unnecessary branches. Integrate completed,
validated work into `main` before preparing a release, preserving its focused
commits. Remove temporary branches after their commits are integrated.
Do not retain a separate release branch for each published version.

Build candidates from a clean, committed, up-to-date `main` checkout. Start
manual release-candidate Actions runs from `main`. Create a version tag only
on the reviewed `main` commit used for the candidate; the tag workflow rebuilds
that exact commit. The version tag, source archive and `release.json` must all
identify the same source revision. Tags preserve published versions as `main`
advances; merging source changes does not publish or replace a release.

## GitHub Actions

- **Verify source** runs on `main` pushes and pull requests. It audits history,
  builds the wrapper, runs deterministic runtime tests and checks installer
  logic/layout. It needs no game installation or redistribution of original data.
- **Build release candidate** can be started manually from Actions. It fetches
  pinned dependencies into ignored build output, runs the complete candidate
  build, then downloads and verifies the actual artifacts on a separate Linux
  runner against the exact source commit. Manual runs create only Actions
  artifacts, retained for 14 days; they create no tag or GitHub release.
- An explicitly pushed `v<VERSION>` tag runs the same pipeline and,
  after verification, creates an **unpublished draft** with exactly four assets.
  Prerelease versions retain the prerelease flag. No job publishes a draft.
  Ordinary source pushes never create a release or draft.

The first stable release uses **0.0.N**, where N is `git rev-list --count HEAD`
at that release commit, including its preparation commit. Subsequent releases
increment the patch number by one, independently of the number of commits.
Stable versions use the README **Current version** label and a dated changelog
section; prereleases retain **Current candidate** and the alpha, beta or rc suffix.

`VERSION`, the README version label and
`.github/release-notes/v<VERSION>.md` must agree. Tag mismatches, detailed
diagnostic defaults and missing notes stop the build. Update these together
before creating a future version tag.

Actions use pinned commit hashes, read-only default permissions and no persisted
checkout credentials. Only the tag-triggered draft job receives release-write
permission. Do not push a version tag until draft creation is intended.

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

Use a clean, committed `main` checkout with the reviewed external dependencies in the
ignored local locations described in [installer dependencies](INSTALLER-DEPENDENCIES.md).
The installed game and its personal INI files are never packaging inputs.

```powershell
.\scripts\build_release.ps1 -Version 0.0.131
```

For a fresh source checkout without a local game installation:

```powershell
.\scripts\fetch_build_dependencies.ps1
.\scripts\build_release.ps1 -RuntimeDirectory build/dependencies/runtime -UnshieldDirectory build/dependencies/unshield
```

The dependency destination must be new. Each downloaded archive and extracted
binary is checked against its pinned size/hash. Original game data is never
requested. A local build may use `-OutputDirectory` to select a fresh ignored
candidate destination. `-Version`, if supplied, must match `VERSION`.

The script audits history, runs policy/package tests, builds the wrapper,
runs deterministic runtime suites and builds setup. It reads setup's embedded
payload without running setup and verifies an exact file allowlist, project
file contents and the pinned hashes of unmodified dependencies. It also checks
the project binaries for local build/user paths and absolute PDB paths.
Final verification checks both downloaded archives, exact source contents,
user-document allowlists, source revision, artifact sizes and checksums.

`dist/releases/<version>` is a new directory; an existing candidate is never
silently replaced. It contains:

- `FightingForceRecharged-<version>-Windows.zip`: the game-specific setup,
  quick start, user documentation, project license, dependency notices and
  changelog. No game executable, game assets, disc images, local configuration,
  saves, screenshots or raw reports are included.
- `FightingForceRecharged-<version>-Source.zip`: only the committed source tree
  exported by Git and independently audited. It contains neither `.git` nor
  ignored workspace content or third-party binaries.
- `release.json`: source commit, version, prerelease status, artifact sizes and
  SHA-256 hashes, without local paths or machine metadata.
- `SHA256SUMS.txt`: hashes of the archives and release manifest.

Repeat independent verification from the matching source checkout with:

```powershell
python scripts/verify_release.py --directory <candidate-directory> --revision <source-commit>
```

Candidate builds are not automatically code-signed. Git maintainer identity
does not imply a cryptographic Git signature or an Authenticode signature.

## Validate before publication

Run the original native menu/character-selection and pause fixtures with both
reviewed executables, plus installer core and layout tests. Test clean install
and folder update without changing original files. Check current configuration
and saves survive an update, and run a short live session covering menus,
controller return, campaign slots, loading, focus changes and both cameras.

Extended cooperative play and later campaign geometry still need broader live
coverage. Keep experimental feature labels and describe remaining coverage
limits in release notes, including for stable releases. Refer to
[validation](VALIDATION.md) for the recorded scope.
Do not infer full campaign or hardware compatibility from deterministic tests.

Before attaching artifacts, compare their hashes with `SHA256SUMS.txt` and
publish only the reviewed files. Publishing a Git branch sends its reachable
history and public author metadata; attaching the source ZIP sends no history.
Never attach the development directory, raw logs or media folders.

After the tag workflow succeeds, download the draft's four assets and verify
them against the tagged checkout. Confirm the source revision, checksums,
installer contents and stable/prerelease flag before publication. Publishing
the reviewed draft is an explicit maintainer action; the workflow itself never
publishes it. Mark a stable release as the latest release after this review.

An explicitly authorized refresh of an existing version must keep its tag,
source archive and `release.json` aligned with the reviewed source commit.
Retain the previous assets locally for rollback, verify the replacement build
and update its checksums and release notes together. If the tag pipeline finds
an already published release, it leaves that release and its assets untouched;
the verified replacement remains an Actions artifact for manual review and
upload. Ordinary updates should use a new patch version.

Approved promotional artwork can be attached as a separate release asset and
referenced by the README and release description. Keep artwork drafts and input
references outside Git. Promotional images are separate from the four verified
build artifacts; download those four files into a separate directory for
`verify_release.py`. Adding artwork must preserve the published build artifacts
and their checksums byte for byte.
