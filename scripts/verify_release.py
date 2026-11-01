"""Independently verify downloaded candidate artifacts against their source."""
import argparse
import json
import pathlib
import re

import package_release as package
import release_metadata


def verify(directory, revision, version, expected_source):
    stem = 'FightingForceRecharged-' + version
    names = {stem + '-Source.zip', stem + '-Windows.zip', 'release.json', 'SHA256SUMS.txt'}
    if {p.name for p in directory.iterdir()} != names or any(not p.is_file() or p.is_symlink() for p in directory.iterdir()):
        raise ValueError('Candidate directory has unexpected or missing files')
    meta = json.loads((directory / 'release.json').read_text(encoding='utf-8'))
    if set(meta) != {'schema', 'project', 'version', 'source_commit', 'prerelease', 'artifacts'}:
        raise ValueError('Unexpected release metadata fields')
    if (meta['schema'] != 1 or meta['project'] != 'Fighting Force Recharged'
            or meta['version'] != version or meta['source_commit'] != revision
            or meta['prerelease'] is not ('-' in version)):
        raise ValueError('Candidate metadata does not match the reviewed source')
    checksums = {}
    for line in (directory / 'SHA256SUMS.txt').read_text(encoding='ascii').splitlines():
        digest, name = line.split('  ', 1)
        if name in checksums or name not in names - {'SHA256SUMS.txt'} or not re.fullmatch('[0-9a-f]{64}', digest):
            raise ValueError('Invalid checksum inventory')
        checksums[name] = digest
    if set(checksums) != names - {'SHA256SUMS.txt'}:
        raise ValueError('Incomplete checksum inventory')
    for name, digest in checksums.items():
        if package.digest((directory / name).read_bytes()) != digest:
            raise ValueError('Candidate checksum mismatch: ' + name)
    records = meta['artifacts']
    if len(records) != 2 or {r['name'] for r in records} != names - {'release.json', 'SHA256SUMS.txt'}:
        raise ValueError('Invalid artifact inventory')
    for item in records:
        if set(item) != {'name', 'size', 'sha256'}:
            raise ValueError('Unexpected artifact metadata')
        data = (directory / item['name']).read_bytes()
        if len(data) != item['size'] or package.digest(data) != item['sha256']:
            raise ValueError('Artifact metadata mismatch')
    prefix = 'fighting-force-recharged-' + version + '/'
    source = (directory / (stem + '-Source.zip')).read_bytes()
    package.verify_source(source, prefix)
    if package.read_zip(source) != package.read_zip(expected_source):
        raise ValueError('Source archive differs from the reviewed commit')
    files = package.read_zip((directory / (stem + '-Windows.zip')).read_bytes())
    installer = files.get('FightingForceSetup.exe', b'')
    if not installer.startswith(b'MZ') or files != package.windows_files(installer):
        raise ValueError('Windows package differs from its user-file allowlist')
    package.verify_own_binary(installer)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', required=True, type=pathlib.Path)
    parser.add_argument('--revision', required=True)
    args = parser.parse_args()
    version = release_metadata.validate()
    revision = package.git('rev-parse', 'HEAD').decode().strip()
    if revision != args.revision:
        raise ValueError('Checkout revision does not match the requested candidate')
    source = package.git('archive', '--format=zip', '--prefix=fighting-force-recharged-' + version + '/', 'HEAD')
    verify(args.directory, revision, version, source)
    print('Downloaded candidate, exact source tree, user files and checksums: PASS')
