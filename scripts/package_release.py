"""Package reviewed release inputs; never walk the local game/work folders."""
import argparse
import hashlib
import io
import json
import os
import pathlib
import re
import subprocess
import zipfile

from check_repository import audit_file

ROOT = pathlib.Path(__file__).resolve().parent.parent
VERSION = re.compile(r'[0-9]+\.[0-9]+\.[0-9]+(?:-(?:alpha|beta|rc)\.[0-9]+)?')
USER_DOCS = ('QUICKSTART.md', 'INSTALLER.md', 'CONTROLS.md', 'FRAME-RATE.md',
             'COOPERATIVE.md', 'SAVES.md', 'COMBAT-FOCUS.md', 'INSTALLER-DEPENDENCIES.md')
ROOT_DOCS = ('LICENSE', 'THIRD_PARTY_NOTICES.md', 'CHANGELOG.md')
NOTICES = ('unshield.txt', 'zlib.txt', 'dgVoodoo.txt')
WINDOWS_README = (
    'Fighting Force Recharged\n\n'
    'Read docs/QUICKSTART.md, then run FightingForceSetup.exe.\n'
    'A supported English PC copy of Fighting Force is required.\n'
    'This package contains no game data.\n').encode('utf-8')


def git(*args):
    return subprocess.check_output(['git', *args], cwd=ROOT)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def read_zip(data):
    result = {}
    seen = set()
    total = 0
    with zipfile.ZipFile(io.BytesIO(data)) as archive:
        for info in archive.infolist():
            name = info.orig_filename
            pure = pathlib.PurePosixPath(name)
            canonical = pure.as_posix() + ('/' if info.is_dir() else '')
            if (name in seen or name != info.filename or '\0' in name or pure.is_absolute() or '..' in pure.parts
                    or '\\' in name or ':' in name or name != canonical
                    or (info.external_attr >> 16) & 0o170000 == 0o120000):
                raise ValueError('Unsafe or duplicate archive member')
            seen.add(name)
            if info.is_dir():
                continue
            if info.file_size > 16 * 1024 * 1024:
                raise ValueError('Unexpectedly large release member')
            total += info.file_size
            if total > 64 * 1024 * 1024:
                raise ValueError('Unexpectedly large release archive')
            result[name] = archive.read(info)
    return result


def installer_inputs():
    expected = {'game/DINPUT.dll': (ROOT / 'build/DINPUT.dll').read_bytes()}
    for name in ('fightingforce-compat.ini', 'dgVoodoo.conf'):
        expected['game/' + name] = (ROOT / 'config' / name).read_bytes()
    for name in USER_DOCS:
        expected['support/' + name] = (ROOT / 'docs' / name).read_bytes()
    for name in ROOT_DOCS:
        data = (ROOT / name).read_bytes()
        if name == 'THIRD_PARTY_NOTICES.md':
            data = data.replace(b'(docs/licenses/', b'(licenses/')
        expected['support/' + name] = data
    for name in NOTICES:
        expected['support/licenses/' + name] = (ROOT / 'docs/licenses' / name).read_bytes()
    for name in ('retail-english.json', 'rip-english.json'):
        expected['support/' + name] = (ROOT / 'config' / name).read_bytes()
    return expected


def verify_payload(data, expected, dependencies):
    files = read_zip(data)
    allowed = set(expected)
    for item in dependencies:
        prefix = 'tools/' if item['name'] in ('unshield.exe', 'zlib1.dll') else 'game/'
        name = prefix + item['name']
        if name in allowed or '/' in item['name'] or '\\' in item['name']:
            raise ValueError('Invalid dependency inventory')
        allowed.add(name)
        content = files.get(name, b'')
        if len(content) != item['size'] or digest(content) != item['sha256']:
            raise ValueError('Installer dependency differs: ' + item['name'])
    if set(files) != allowed:
        raise ValueError('Installer payload differs from the release allowlist')
    for name, content in expected.items():
        if files[name] != content:
            raise ValueError('Installer payload is stale: ' + name)
    return files


def verify_source(data, prefix):
    files = read_zip(data)
    if not files:
        raise ValueError('Empty source archive')
    for name, content in files.items():
        if not name.startswith(prefix):
            raise ValueError('Unexpected source archive prefix')
        issues = audit_file(name[len(prefix):], content)
        if issues:
            raise ValueError('Source archive rejected: ' + name[len(prefix):] + ': ' + ', '.join(issues))
    if prefix + 'LICENSE' not in files:
        raise ValueError('Source archive has no project license')


def verify_own_binary(data):
    # Search both ordinary strings and UTF-16 resource/debug strings. Vendor
    # binaries stay unmodified and are verified against their reviewed hashes.
    markers = [str(ROOT), str(ROOT).replace('\\', '/')]
    if os.environ.get('USERPROFILE'):
        markers.append(os.environ['USERPROFILE'])
    for marker in markers:
        for encoding in ('utf-8', 'utf-16le'):
            if marker.casefold().encode(encoding) in data.lower():
                raise ValueError('Own binary contains a local build/user path')
    for text in (data.decode('latin1'), data.decode('utf-16le', errors='ignore')):
        if re.search(r'[A-Za-z]:\\[^\x00\r\n]{0,260}\.pdb', text, re.IGNORECASE):
            raise ValueError('Own binary contains an absolute PDB path')


def windows_files(installer):
    files = {'FightingForceSetup.exe': installer, 'README.txt': WINDOWS_README}
    for name in ROOT_DOCS:
        files[name] = (ROOT / name).read_bytes().replace(b'\r\n', b'\n')
    for name in USER_DOCS:
        files['docs/' + name] = (ROOT / 'docs' / name).read_bytes().replace(b'\r\n', b'\n')
    for name in NOTICES:
        files['docs/licenses/' + name] = (ROOT / 'docs/licenses' / name).read_bytes().replace(b'\r\n', b'\n')
    return files


def make_zip(files):
    output = io.BytesIO()
    with zipfile.ZipFile(output, 'w', compression=zipfile.ZIP_DEFLATED) as archive:
        for name, content in sorted(files.items()):
            pure = pathlib.PurePosixPath(name)
            if pure.is_absolute() or '..' in pure.parts or '\\' in name or ':' in name or '\0' in name:
                raise ValueError('Unsafe archive member')
            info = zipfile.ZipInfo(name, (2026, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            archive.writestr(info, content)
    return output.getvalue()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--version', required=True)
    parser.add_argument('--revision', required=True)
    parser.add_argument('--payload', type=pathlib.Path, required=True)
    parser.add_argument('--installer', type=pathlib.Path, default=ROOT / 'dist/FightingForceSetup.exe')
    parser.add_argument('--output', type=pathlib.Path, default=ROOT / 'dist/releases')
    args = parser.parse_args()
    if not VERSION.fullmatch(args.version):
        raise ValueError('Use a numeric version with an optional alpha, beta or rc suffix')
    if args.version != (ROOT / 'VERSION').read_text(encoding='utf-8').strip():
        raise ValueError('Package version does not match VERSION')
    if git('status', '--porcelain', '--untracked-files=normal').strip():
        raise ValueError('Commit reviewed source changes before packaging')
    revision = git('rev-parse', 'HEAD').decode().strip()
    if revision != args.revision:
        raise ValueError('Source revision changed during the release build')
    stem = 'FightingForceRecharged-' + args.version
    source_prefix = 'fighting-force-recharged-' + args.version + '/'
    source = git('archive', '--format=zip', '--prefix=' + source_prefix, 'HEAD')
    verify_source(source, source_prefix)
    dependencies = json.loads((ROOT / 'config/installer-dependencies.json').read_text(encoding='utf-8'))
    verify_payload(args.payload.read_bytes(), installer_inputs(), dependencies)
    installer = args.installer.read_bytes()
    verify_own_binary(installer)
    verify_own_binary((ROOT / 'build/DINPUT.dll').read_bytes())
    windows = windows_files(installer)
    artifacts = {stem + '-Source.zip': source, stem + '-Windows.zip': make_zip(windows)}
    metadata = {'schema': 1, 'project': 'Fighting Force Recharged', 'version': args.version,
                'source_commit': revision, 'prerelease': '-' in args.version,
                'artifacts': [{'name': name, 'size': len(data), 'sha256': digest(data)}
                              for name, data in sorted(artifacts.items())]}
    artifacts['release.json'] = (json.dumps(metadata, indent=2) + '\n').encode('utf-8')
    artifacts['SHA256SUMS.txt'] = ''.join(digest(data) + '  ' + name + '\n'
                                        for name, data in sorted(artifacts.items())).encode('ascii')
    destination = args.output / args.version
    destination.mkdir(parents=True, exist_ok=False)
    for name, data in artifacts.items():
        (destination / name).write_bytes(data)
    print('Release candidate packaged: ' + args.version)
    print('Verified source archive, installer allowlist, pinned dependencies and local-path exclusion.')
    print('No remote repository, tag or publication was created.')


if __name__ == '__main__':
    main()
