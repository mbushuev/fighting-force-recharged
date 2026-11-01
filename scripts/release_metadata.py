"""Validate one source version, release notes and safe default configuration."""
import argparse
import configparser
import pathlib

from package_release import ROOT, VERSION


def validate(root=ROOT, tag=None):
    version = (root / 'VERSION').read_text(encoding='utf-8').strip()
    if not VERSION.fullmatch(version):
        raise ValueError('Invalid VERSION')
    if tag is not None and tag != 'v' + version:
        raise ValueError('Tag does not match VERSION')
    readme = (root / 'README.md').read_text(encoding='utf-8')
    prerelease = '-' in version
    label = 'Current candidate' if prerelease else 'Current version'
    if label + ': **' + version + '**' not in readme:
        raise ValueError('README version label does not match VERSION')
    notes = root / '.github/release-notes' / ('v' + version + '.md')
    if notes.read_text(encoding='utf-8').splitlines()[0] != '# Fighting Force Recharged ' + version:
        raise ValueError('Release-notes heading does not match VERSION')
    changelog = (root / 'CHANGELOG.md').read_text(encoding='utf-8')
    if '## Unreleased' not in changelog:
        raise ValueError('Missing changelog section')
    if not prerelease:
        if '## ' + version + ' - ' not in changelog:
            raise ValueError('Missing versioned release changelog section')
    config = configparser.ConfigParser()
    config.read(root / 'config/fightingforce-compat.ini', encoding='utf-8')
    for key in ('TraceRenderer', 'TraceCooperative', 'TraceFrameRate'):
        if config.getint('Diagnostics', key) != 0:
            raise ValueError('Detailed diagnostics must be disabled in release defaults')
    if config.getint('Combat', 'PlayerGuard') != 0:
        raise ValueError('Optional player guard must remain disabled by default')
    return version


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--tag')
    args = parser.parse_args()
    print(validate(tag=args.tag))
