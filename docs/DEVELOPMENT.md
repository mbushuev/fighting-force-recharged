# Development

Use Windows, Python 3, Visual Studio C++ build tools and the Windows SDK.
Build the wrapper for x86. Build scripts discover Visual Studio through its
installer and resolve project files relative to the checkout.

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
