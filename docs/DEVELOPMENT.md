# Development

Use Windows, Python 3, Visual Studio C++ build tools and the Windows SDK.
Build the wrapper for x86. Build scripts discover Visual Studio through its
installer and resolve project files relative to the checkout.

Build and run the deterministic checks from the repository root:

```powershell
.\build.ps1
.\tests\run.ps1
python tests/repository_policy_test.py
```

The deterministic runner keeps temporary file fixtures under ignored `build`
output and restores the caller's temporary-directory environment afterward.

The `room_completion` suite runs without game assets. Its optional WAD-directory
argument additionally reads the original cabin and both office models, checking
wall coverage and preservation of their source geometry/materials. For example,
run `build/room_completion_test.exe game/Wads` after building the suite. Repeat
against the supported RIP's WAD directory. Add the corresponding executable as
a second argument to execute its original MOD loader and environment handlers
in an isolated fixture. This checks native coordinate scaling, model registration
and completed-model dispatch while stubbing resource lookup and final projection.
No proprietary data is exported.

`python tools/audit_room_shells.py game/Wads` provides read-only per-model
coverage triage for further room work. Run `python tests/room_shell_audit_test.py`
for its synthetic parser/ray checks. See [Room survey](ROOM-SURVEY.md) for the
review queue and scene-composition limits.

GitHub source CI runs these checks without game files. Candidate builds use
reviewed downloads, source-version/release-note gates and a separate downloaded
archive verification job. See [Releasing](RELEASING.md) for manual candidates
and the future tag-to-draft process.

The GPU runner accepts `-RuntimeDirectory` for an external runtime location;
its default is the ignored `game` directory. Retail integrity checks require
the mounted source media and the original installer extraction. By default,
the extraction layout is `work/install-extracted-old/English` and
`work/install-extracted-old/Dlls`; alternate locations are script parameters.
The verifier writes its manifest under ignored `verification` by default.

The optional installer uses the built wrapper and pinned external unshield/zlib
and dgVoodoo components. Retail/RIP manifests identify supported executables
and supply optional reference diagnostics; their complete inventories are not
installation requirements.
Build it with `scripts/build_installer.ps1`. Run `tests/run_installer.ps1`
with its `MediaRoot` parameter to exercise extraction, exact-file verification,
Unicode destinations, cancellation and rollback under ignored local work.
The integration runner also applies the fix to an existing retail folder. Its
optional `GameFolder` parameter exercises the reviewed RIP in isolated fixtures,
checking native file hashes, file identities and modification times. The core
and integration fixtures also accept changed resources and absent optional
files while refusing unsupported executables or missing main data. The core
fixtures cover settings/save preservation, unmanaged DLL refusal, cancellation
after writing, replacement rollback, locked DLLs and repeat/update operations.
Run `tests/run_installer_core.ps1` for media-independent path/manifest checks;
its optional `MediaRoot` exercises disc-copy cancellation or, for a reviewed RIP
folder, the in-place transaction checks with a built payload.
Run `tests/run_installer_layout.ps1` for proportional font/geometry checks at
100-300 percent and compact/restored window sizes without changing OS settings.
The setup requires no Python or build tools on the installation machine.

The `source_runtime` suite exercises the reviewed image variants, scoped local
data paths, optional intro call boundary and missing-video-settings fallback.
For a local fallback smoke test, set `FIGHTINGFORCE_TEST_VIDEO_DEFAULTS=1` only
in the test child's environment; this avoids deleting native registry settings.

The `campaign_save` suite checks versioned checkpoint round trips, corrupt and
truncated records, player ranges, eliminated slots and atomic replacement with
a locked destination. The `campaign_runtime` suite checks lifecycle boundaries,
native spawn preservation, scalar restoration, failed writes and attract-demo
exclusion. Its optional executable argument runs the original main-menu and
character-selection bodies in isolated fixtures. It checks Continue, both
new-game actions, Options, selection wrapping and held confirmation, then
repeats solo selection, cancellation and confirmation with keyboard or XInput
across all cooperative device layouts and both native device counts. Run it
with each reviewed image variant.

The `campaign_slots` suite covers independent current/previous records, ordering,
verified backups and damaged-primary repair. `campaign_menu` checks replacement
and rewind confirmations, cancellation and held-input gates. The optional
executable argument to `campaign_pause_runtime` executes the original pause
routine with scripted inputs, checking resume, quit, restart, held confirmation
and both native device reads. The device, audio and rendering boundaries are
stubbed; run it against each reviewed executable. It also checks opening and
closing Move List, category changes and return to pause for one or two players.

`move_list` checks catalog coverage, command formatting and navigation.
`move_list_runtime` measures every hero/category/selected row, current-device
switching, independent assignments, remapped bindings and focus/held-input gates.
Run `build/move_list_native_test.exe` with either original executable as its
argument to execute the original combat selectors without launching the game.
That optional fixture covers native combos, jumps, running attacks, grab
continuations and spin health rules for both player slots and all four heroes.
It stubs world/audio/animation dispatch and does not replace live combat checks.

Optional read-only executable inspection requires the dependencies pinned in
`tools/requirements.txt`. Install them into an ignored virtual environment and
use `tools/inspect_retail.py`; `--executable` selects an external retail copy.
The default is `game/fforce.exe`. Neither inspection tool writes game files.
`python tools/analyze_camera.py` checks the supported image hash and the
instruction references behind the documented camera map. It reports static
evidence and unresolved questions, without attaching to a running process.

Read [COMBAT-RESEARCH.md](COMBAT-RESEARCH.md) for the native enemy guard and
candidate hero animation mappings. `python tools/analyze_guard.py` checks the
reviewed original asset hashes and decodes their animation records without
changing or exporting game data. `python tests/guard_assets_test.py` exercises
its parsers with synthetic records. This analysis does not enable player guard.
The `guard_animation` suite checks the private native-format encoder, wrapped
angle deltas, rotation conversion and malformed records. Passing an installed
WAD directory also checks Hawk, Mace and Alana upper-body conversion, target
joint counts and preservation of idle roots, feet and accessory angles. These
clips preserve destination bind/post transforms and use private memory only.
Optionally pass the original Smasher and CHARS11 archive names to its built
executable to exercise the complete 27-joint
conversion. Converted clips are retained in memory and never exported.
The `player_guard` suite checks lifecycle, interruptions, frontal direction
against the existing camera convention, and attack-window consumption.
`player_guard_runtime` exercises the real x86 update/contact hook boundaries,
including native stack cleanup, frontal protection and rear/special-hit
fallthrough. Pass the reviewed executable and an installed WAD directory to
check every converted hero frame through the original native decoder. It also
accepts the reviewed executable, Smasher archive and CHARS11 archive as three
arguments for the Smasher-only check. Decoding runs in an isolated test process.
This does not replace a live
combat, animation or cooperative acceptance pass.

## Commits

Keep each commit focused on one behavior and independently buildable. Use a
short English imperative subject. Review the diff and staged file list before
committing. Match the maintainer identity in the repository-local Git settings;
never guess an author or add unrelated attribution trailers. Cryptographic
signing requires an explicitly configured key and successful verification.

Run the checks relevant to the change. Update [CHANGELOG.md](../CHANGELOG.md)
for changes in user-visible behavior. Prefer reverting a focused commit to
rewriting shared history. This initial import records existing work as separate
feature checkpoints; it does not recreate an earlier development timeline.

To test an earlier complete feature checkpoint, use a separate checkout at
that commit and rebuild `DINPUT.dll`. Git stores source and configuration, so
switching commits does not replace a DLL already installed beside the game.
Back up the installed wrapper files before applying a rebuilt checkpoint.

Enable the repository hooks for subsequent local commits:

```powershell
git config --local core.hooksPath .githooks
```

Audit staged files and all reachable history with:

```powershell
python scripts/check_repository.py --staged
python scripts/check_repository.py --history
```

The automated guard rejects binary payloads, local evidence, known absolute
path forms and conversation metadata. It supplements manual review; it cannot
prove that arbitrary prose contains no private information.

## Content policy

All maintained documentation and commit messages are in English. Record the
problem, final behavior, validation and material limitations. Do not include
conversation transcripts, quoted feedback, personal machine paths, raw logs,
screenshots or local process identifiers.

Retail files, installed runtimes, archives, dependencies, backups and generated
reports stay in ignored local directories. Only reviewed project configuration
belongs under `config`. Do not commit a captured personal configuration.
Do not change retail files or registry settings to make a test pass.

## Publication

This is a development repository, not a packaged public release. Do not create
remotes, push commits, publish archives or assign a redistribution license
without an explicit release decision. Review third-party licensing before
bundling any external runtime.
