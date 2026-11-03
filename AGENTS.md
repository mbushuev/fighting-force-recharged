# Project rules

- Preserve original game executables, DLLs and assets byte for byte. Never patch
  retail files on disk. Add separate wrappers, their configuration and source.
- Implement compatibility and gameplay fixes through a separate runtime DLL,
  following the Deathtrap Native 50 approach. Runtime interception is allowed;
  original files on disk must remain unchanged.
- The supported English retail `fforce.exe` has SHA-256
  92ab6179bf6497d471c6ab30d6df6f4f333d5bc3f641a154d6966b47fdba90a7.
- The reviewed English RIP `fforce.exe` has SHA-256
  bf1bd02dc9b6508edd76efa36e39a6cc1b82055f7d31ccda329c1ae55796fda3.
  Preserve its supplied files too; compatibility belongs in runtime interception.
- Scope is the first Fighting Force, original Windows version.

## Repository and commits

- Work on `main` by default; do not create unnecessary branches. Integrate
  completed, validated changes into `main` and remove finished temporary branches.
- Build release candidates from clean, committed `main`. Create version tags
  only for reviewed commits already integrated into `main`.
- If a temporary branch is needed, use a descriptive project name. Never include
  assistant or tool names in branch names.
- Keep commits small, independently buildable and focused on one behavior.
- Use concise English imperative subjects that describe the resulting change.
- Use the repository-local maintainer identity for both author and committer.
  Never invent an identity or add unrelated attribution trailers.
- Do not claim cryptographic signing unless a configured key actually signs
  the commit. Keep identity and signing settings local to this repository.
- Update the English user-facing changelog for behavior changes. Record only
  implementation facts, supported behavior, validation and material limits.
- Never include conversation transcripts, quoted feedback, personal details,
  machine-specific absolute paths, screenshots, raw logs or local test reports.
- Keep retail files, third-party binaries, generated output, local dependencies
  and rollback copies outside Git. Review the staged file list before committing.
- Run `python scripts/check_repository.py --staged` before each commit and the
  checks appropriate to the changed behavior. Do not commit a broken snapshot.
- Do not publish or create a remote repository unless explicitly requested.
