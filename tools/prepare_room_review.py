"""Create an isolated installed-game copy and scalar stage-start review saves."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import zlib

# Native campaign index -> STAGE resource, not sorted MODEL archive order.
# The loader selects resource index stage+5 from its initialized STAGE table.
STAGES = ('11', '12', '13', '14', '15', '21', '22', '31', '32A', '32B',
          '33', '33A', '41', '42', '51', '61', '62', '63', '64', '65',
          '66', '67', '71', '72', '73')
PRIORITY = (
    ('01 - Lab lift, laboratory, mall', (23, 22, 7)),
    ('02 - Ship interior, train, alternate bridge', (19, 14, 11)),
    ('03 - Completed lift, office, adjoining interior', (2, 4, 3)),
)


def save(stage, order=3):
    if not 0 <= stage < len(STAGES) or not 0 < order < 2**63 - 1:
        raise ValueError('Invalid stage or save order')
    payload = struct.pack('<16i', stage, 1, 0, 1,
                          3, 384, 384, 9, 0, 100000,
                          -1, 0, 384, 0, 0, 100000)
    point = b'FFCP' + struct.pack('<III', 1, 80, zlib.crc32(payload)) + payload
    record = struct.pack('<QI', order, 0) + point + bytes(80)
    return b'FFCS' + struct.pack('<III', 2, 188, zlib.crc32(record)) + record


def packs():
    result = [{'name': name, 'stages': list(stages)} for name, stages in PRIORITY]
    for first in range(0, len(STAGES), 3):
        last = min(first + 3, len(STAGES))
        result.append({'name': f'Campaign {first + 1:02d}-{last:02d}', 'stages': list(range(first, last))})
    return result


def write_presets(root):
    presets = root / 'presets'
    presets.mkdir(parents=True)
    for stage, resource in enumerate(STAGES):
        (presets / f'stage-{stage + 1:02d}.sav').write_bytes(save(stage))
    manifest = {'format': 'ff-room-review-1', 'stages': list(STAGES), 'packs': packs()}
    (root / 'room-review.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    return manifest


def prepare(source, destination):
    source, destination = source.resolve(), destination.resolve()
    if destination.exists() or source in destination.parents or destination in source.parents:
        raise ValueError('Choose a new review directory outside the source installation')
    if not (source / 'fforce.exe').is_file() or not (source / 'DINPUT.dll').is_file():
        raise ValueError('Source must be an installed game with the Recharged wrapper')
    # Never edit the source installation or its campaign saves. All changes
    # below belong to the new review copy; native game data is copied verbatim.
    destination.mkdir(parents=True)
    game = destination / 'Game'
    shutil.copytree(source, game, ignore=shutil.ignore_patterns('fightingforce-save*', '*.log'))
    manifest = write_presets(destination)
    for slot, stage in enumerate(manifest['packs'][0]['stages'], 1):
        (game / f'fightingforce-save-{slot}.sav').write_bytes(save(stage, 4 - slot))
    shutil.copyfile(Path(__file__).with_name('room_review_launcher.ps1'), destination / 'Review-Rooms.ps1')
    (destination / 'Review-Rooms.cmd').write_text(
        '@echo off\npowershell.exe -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -STA '
        '-File "%~dp0Review-Rooms.ps1"\n', encoding='ascii')
    lines = ['Room review saves', '', 'Run Review-Rooms.cmd to choose a three-slot set.',
             'Close the review game before switching sets. Then use Load Game.',
             'Only this isolated copy is changed. Earlier test progress is copied to history.',
             'Presets start at native stage entry: Alana, normal difficulty, nine lives.',
             'Reach later sections normally; enemies, scripts and doors are not bypassed.',
             'The area labels identify review candidates, not confirmed defects.', '',
             'Default set:']
    for i, stage in enumerate(manifest['packs'][0]['stages'], 1):
        lines.append(f'Slot {i}: Stage {stage + 1} / STAGE{STAGES[stage]}')
    lines += ['', 'Inspect full turns in both camera views and each section/doorway.',
              'Compare Complete Rooms on/off for the first lift and office.',
              'Open shafts, entrances, carriage joins and outdoor edges may be intentional.']
    (destination / 'README.txt').write_text('\n'.join(lines) + '\n', encoding='utf-8')
    # A local copy-integrity check covers every copied native and wrapper file,
    # excluding only the generated test saves. Do not export file contents.
    files = 0
    for original in source.rglob('*'):
        if not original.is_file() or original.name.lower().startswith('fightingforce-save') or original.suffix.lower() == '.log':
            continue
        copied = game / original.relative_to(source)
        if hashlib.sha256(original.read_bytes()).digest() != hashlib.sha256(copied.read_bytes()).digest():
            raise ValueError('Review copy verification failed')
        files += 1
    return files


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game', type=Path, help='Existing patched installation to copy')
    parser.add_argument('--output', type=Path, required=True, help='New local review directory')
    parser.add_argument('--presets-only', action='store_true', help='Generate scalar fixtures without copying a game')
    args = parser.parse_args()
    if args.presets_only:
        if args.output.exists():
            parser.error('Output directory already exists')
        write_presets(args.output)
    elif args.game:
        print(f'Copied and verified {prepare(args.game, args.output)} files')
    else:
        parser.error('--game is required unless --presets-only is selected')
    print('Prepared 25 stage-start presets and 12 review sets')


if __name__ == '__main__':
    main()
