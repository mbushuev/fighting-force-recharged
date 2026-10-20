"""Audit staged content or committed history without printing private contents."""
import argparse
import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
TOP_LEVEL = {'.gitignore', '.gitattributes', 'AGENTS.md', 'README.md',
             'CHANGELOG.md', 'build.ps1', 'LICENSE', 'THIRD_PARTY_NOTICES.md'}
DIRECTORIES = {'src', 'tests', 'tools', 'scripts', 'config', 'docs', '.githooks', '.github'}
PRIVATE_DIRECTORIES = {'tools/python', 'tools/unshield'}
BINARY_SUFFIXES = {'.exe', '.dll', '.obj', '.pdb', '.lib', '.exp', '.ilk',
                   '.png', '.jpg', '.jpeg', '.mp4', '.zip', '.7z', '.iso',
                   '.bin', '.cue', '.wad', '.rpl', '.tga', '.sav', '.log', '.dmp'}
ABSOLUTE_PATH = re.compile(r'[A-Za-z]:[\\/]|/(?:Users|home)/')
CYRILLIC = re.compile(r'[\u0400-\u04ff]')
CHAT_MARKERS = re.compile(r'questionItemId|send_user_message_question_reply|'
                          r'<(?:environment_context|app-context|external_codex_apps)|'
                          r'UserConfirmation|UserFeedback|UserConfirmed')


def git(*args):
    return subprocess.check_output(['git', *args], cwd=ROOT)


def audit_file(path, content):
    issues = []
    pure = pathlib.PurePosixPath(path)
    if path not in TOP_LEVEL and (len(pure.parts) < 2 or pure.parts[0] not in DIRECTORIES):
        issues.append('outside the reviewed source/documentation layout')
    if any(path == item or path.startswith(item + '/') for item in PRIVATE_DIRECTORIES):
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
    if pure.suffix.lower() in {'.md', '.txt'}:
        if CYRILLIC.search(text):
            issues.append('documentation must be in English')
        if CHAT_MARKERS.search(text):
            issues.append('contains conversation-derived metadata')
    return issues


def audit_message(message):
    subject = message.splitlines()[0] if message.splitlines() else ''
    issues = []
    if not subject or len(subject) > 72:
        issues.append('commit subject must contain 1 to 72 characters')
    if CYRILLIC.search(message) or ABSOLUTE_PATH.search(message) or CHAT_MARKERS.search(message):
        issues.append('commit message contains non-English or private context')
    return issues


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    group = parser.add_mutually_exclusive_group()
    group.add_argument('--staged', action='store_true')
    group.add_argument('--history', action='store_true')
    group.add_argument('--message', type=pathlib.Path)
    args = parser.parse_args()
    failures = []
    checked = 0
    if args.message:
        failures.extend(audit_message(args.message.read_text(encoding='utf-8-sig')))
    else:
        revisions = git('rev-list', '--all').decode().splitlines() if args.history else [None]
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
                content = git('cat-file', 'blob', fields[2] if revision else fields[1])
                checked += 1
                failures.extend(f'{path}: {issue}' for issue in audit_file(path, content))
            if revision:
                failures.extend(audit_message(git('show', '-s', '--format=%B', revision).decode('utf-8')))
    if failures:
        for issue in sorted(set(failures)):
            print('FAIL:', issue)
        return 1
    print(f'Repository privacy/content checks: PASS ({checked} file versions checked)')
    return 0


if __name__ == '__main__':
    sys.exit(main())
