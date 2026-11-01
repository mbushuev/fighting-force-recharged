"""Audit staged content or committed history without printing private contents."""
import argparse
import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
TOP_LEVEL = {'.gitignore', '.gitattributes', 'AGENTS.md', 'README.md',
             'CHANGELOG.md', 'build.ps1', 'LICENSE', 'THIRD_PARTY_NOTICES.md', 'VERSION'}
DIRECTORIES = {'src', 'tests', 'tools', 'scripts', 'config', 'docs', '.githooks', '.github'}
PRIVATE_DIRECTORIES = {'tools/python', 'tools/unshield'}
PRIVATE_COMPONENTS = {'.git', '.agents', '.codex', '.aws', '.ssh', '.azure',
                      '.gnupg', '.venv', '__pycache__', 'node_modules'}
BINARY_SUFFIXES = {'.exe', '.dll', '.obj', '.pdb', '.lib', '.exp', '.ilk',
                   '.png', '.jpg', '.jpeg', '.mp4', '.zip', '.7z', '.iso',
                   '.bin', '.cue', '.img', '.sub', '.ccd', '.mdf', '.mds',
                   '.nrg', '.rar', '.tar', '.gz', '.bz2', '.xz', '.wad',
                   '.rpl', '.tga', '.sav', '.bak', '.log', '.dmp', '.bmp',
                   '.ico', '.gif', '.webp', '.wav', '.mp3', '.flac', '.ogg',
                   '.avi', '.webm', '.tmp', '.pyc', '.pyo'}
ABSOLUTE_PATH = re.compile(r'(?<![A-Za-z0-9+.-])[A-Za-z]:[\\/]|/(?:Users|home)/')
CYRILLIC = re.compile(r'[\u0400-\u04ff]')
CHAT_MARKERS = re.compile('|'.join(map(re.escape, (
    'question' + 'ItemId', 'send_user_message_' + 'question_reply',
    '<environment_' + 'context', '<app-' + 'context', '<external_' + 'codex_apps',
    'User' + 'Confirmation', 'User' + 'Feedback', 'User' + 'Confirmed'))))
SECRET_PATTERNS = (
    re.compile(r'gh[pousr]_[A-Za-z0-9]{24,}|github_pat_[A-Za-z0-9_]{24,}'),
    re.compile(r'sk-(?:proj-|svcacct-)?[A-Za-z0-9_-]{24,}'),
    re.compile(r'AKIA[A-Z0-9]{16}|xox[baprs]-[A-Za-z0-9-]{20,}'),
    re.compile(r'-----BEGIN (?:RSA |EC |OPENSSH |DSA )?PRIVATE KEY-----'),
    re.compile(r'https?://[^/\s:@]+:[^/\s@]+@'),
)


def git(*args):
    return subprocess.check_output(['git', *args], cwd=ROOT)


def audit_file(path, content):
    issues = []
    pure = pathlib.PurePosixPath(path)
    lowered = path.casefold()
    if pure.is_absolute() or '\\' in path or '..' in pure.parts:
        issues.append('unsafe repository path')
    if path not in TOP_LEVEL and (len(pure.parts) < 2 or pure.parts[0] not in DIRECTORIES):
        issues.append('outside the reviewed source/documentation layout')
    if (any(lowered == item or lowered.startswith(item + '/') for item in PRIVATE_DIRECTORIES)
            or any(part.casefold() in PRIVATE_COMPONENTS for part in pure.parts)):
        issues.append('local dependency directory')
    if pure.suffix.lower() in BINARY_SUFFIXES:
        issues.append('binary, capture, proprietary data or local evidence')
    if pure.name.startswith('.env') or pure.suffix.lower() in {'.pem', '.key', '.pfx', '.p12'}:
        issues.append('local credentials or signing material')
    if len(content) > 1024 * 1024:
        issues.append('exceeds the reviewed source file size limit')
    try:
        text = content.decode('utf-8-sig')
    except UnicodeDecodeError:
        return issues + ['not UTF-8 text']
    if '\0' in text:
        issues.append('contains binary data')
    if ABSOLUTE_PATH.search(text):
        issues.append('contains a machine-specific absolute path')
    # Policy definitions and their negative fixtures intentionally name the
    # markers. Their complete contents still receive the other checks.
    if path not in {'scripts/check_repository.py', 'tests/repository_policy_test.py'} and CHAT_MARKERS.search(text):
        issues.append('contains conversation-derived metadata')
    if any(pattern.search(text) for pattern in SECRET_PATTERNS):
        issues.append('contains a credential-like value or authenticated URL')
    if pure.suffix.lower() in {'.md', '.txt'}:
        if CYRILLIC.search(text):
            issues.append('documentation must be in English')
    return issues


def audit_message(message):
    subject = message.splitlines()[0] if message.splitlines() else ''
    issues = []
    if not subject or len(subject) > 72:
        issues.append('commit subject must contain 1 to 72 characters')
    if (CYRILLIC.search(message) or ABSOLUTE_PATH.search(message)
            or CHAT_MARKERS.search(message) or any(p.search(message) for p in SECRET_PATTERNS)):
        issues.append('commit message contains non-English or private context')
    return issues


def history_revisions():
    commits = set(git('rev-list', '--all').decode().splitlines())
    revisions = set(commits)
    issues = []
    # Tools may retain tree checkpoints outside branches/tags. They are Git
    # objects too, even though rev-list does not list them as commits.
    for name in git('for-each-ref', '--format=%(refname)').decode().splitlines():
        kind = git('cat-file', '-t', name).decode().strip()
        if kind == 'tag':
            issues.extend(audit_message(git('for-each-ref', '--format=%(contents)', name).decode('utf-8')))
        target = git('rev-parse', name + '^{}').decode().strip()
        kind = git('cat-file', '-t', target).decode().strip()
        if kind == 'tree':
            revisions.add(target)
        elif kind != 'commit':
            issues.append('unsupported object reference type')
    return sorted(revisions), commits, issues


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    group = parser.add_mutually_exclusive_group()
    group.add_argument('--staged', action='store_true')
    group.add_argument('--history', action='store_true')
    group.add_argument('--message', type=pathlib.Path)
    args = parser.parse_args()
    failures = []
    checked = 0
    # The same blob/path appears in many commits. Audit every distinct version
    # once, while still validating each tree entry and every commit message.
    versions = set()
    if args.message:
        failures.extend(audit_message(args.message.read_text(encoding='utf-8-sig')))
    else:
        if args.history:
            revisions, commits, reference_issues = history_revisions()
            failures.extend(reference_issues)
        else:
            revisions, commits = [None], set()
        for revision in revisions:
            listing = git('ls-tree', '-rz', revision) if revision else git('ls-files', '--stage', '-z')
            for entry in listing.split(b'\0'):
                if not entry:
                    continue
                metadata, encoded_path = entry.split(b'\t', 1)
                fields = metadata.decode().split()
                if fields[0] != '100644' and fields[0] != '100755':
                    failures.append('unsupported index entry type')
                    continue
                path = encoded_path.decode('utf-8')
                version = (path, fields[2] if revision else fields[1])
                if version in versions:
                    continue
                versions.add(version)
                content = git('cat-file', 'blob', version[1])
                checked += 1
                failures.extend(f'{path}: {issue}' for issue in audit_file(path, content))
            if revision in commits:
                failures.extend(audit_message(git('show', '-s', '--format=%B', revision).decode('utf-8')))
    if failures:
        for issue in sorted(set(failures)):
            print('FAIL:', issue)
        return 1
    print(f'Repository privacy/content checks: PASS ({checked} file versions checked)')
    return 0


if __name__ == '__main__':
    sys.exit(main())
