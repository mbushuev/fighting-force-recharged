"""Inspect reviewed guard assets without modifying or exporting game data."""
import argparse
import hashlib
import json
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parent.parent
HEROES = ('HAWK', 'MACE', 'SMASHER', 'ALANA')
DONORS = (('CHARS11', 'GENMALE', 'SUIT'),
          ('CHARS11', 'GENBIGMA', 'BIGBLACK'),
          ('CHARS15', 'GENFMALE', 'VAMP'))
GUARD_NAMES = ('BLOCKIN1', 'BLOCKSB1', 'BLOCKHT1', 'BLOCKOT1')


def unpack(fmt, data, offset=0):
    if offset < 0 or offset + struct.calcsize(fmt) > len(data):
        raise ValueError('Truncated asset record')
    return struct.unpack_from(fmt, data, offset)


def read_wad(data):
    count, = unpack('>I', data)
    end = 4 + count * 20
    if count > 4096 or end > len(data):
        raise ValueError('Unsupported indexed WAD')
    members = {}
    for index in range(count):
        name, size, offset = unpack('>12sII', data, 4 + index * 20)
        name = name.rstrip(b'\0').decode('ascii')
        if not name or name in members or offset < end or offset + size > len(data):
            raise ValueError('Invalid WAD member')
        members[name] = data[offset:offset + size]
    return members


def read_animations(data):
    # Duplicate names are legal: the native loader overwrites earlier entries.
    offset = 0
    clips = {}
    while offset < len(data) and data[offset]:
        name, size = unpack('<8sI', data, offset)
        if size < 4 or offset + 8 + size > len(data):
            raise ValueError('Invalid ADT record size')
        clips[name.rstrip(b'\0').decode('ascii')] = data[offset + 12:offset + 8 + size]
        offset += 8 + size
    if offset >= len(data):
        raise ValueError('Missing ADT terminator')
    return clips


def read_skeleton(data):
    height, vertices, vertex_count, _, _, _, joints, commands, groups = unpack('<9i', data)
    if not 1 <= joints <= 128 or not 0 <= groups <= 128 or not 0 <= vertex_count <= 65535:
        raise ValueError('Invalid ACH dimensions')
    if commands < 32 + groups * 4 or vertices <= commands or (vertices - commands) % 4:
        raise ValueError('Invalid ACH command range')
    if 4 + vertices + vertex_count * 8 > len(data):
        raise ValueError('Truncated ACH vertices')
    counts = unpack('<' + 'I' * groups, data, 36)
    words = unpack('<' + 'i' * ((vertices - commands) // 4), data, 4 + commands)
    steps, stack, seen = [], [], set()
    cursor = parent = 0
    terminated = False
    while cursor < len(words):
        op = words[cursor]
        cursor += 1
        if op == 0:
            terminated = True
            break
        if op == 1:
            stack.append(parent)
        elif op == 2:
            if not stack:
                raise ValueError('ACH stack underflow')
            parent = stack.pop()
        elif op == 3:
            if cursor + 11 > len(words) or len(steps) >= groups:
                raise ValueError('Truncated ACH bone')
            fields = words[cursor:cursor + 11]
            cursor += 11
            joint = fields[7]
            if not 0 < joint < joints or joint in seen:
                raise ValueError('Unsupported ACH joint layout')
            seen.add(joint)
            steps.append(dict(joint=joint, parent=parent, special=fields[0],
                              translation=fields[1:4], bind=fields[4:7],
                              post=fields[8:11], vertices=counts[len(steps)]))
            parent = joint
        else:
            raise ValueError('Unsupported ACH command')
    if not terminated or stack or len(steps) != groups or sum(counts) != vertex_count:
        raise ValueError('Inconsistent ACH hierarchy')
    return dict(height=height, joints=joints, steps=steps)


def decode_frames(data, joints):
    """Decode the reviewed native absolute-first-frame/delta-following format."""
    if not 1 <= joints <= 128:
        raise ValueError('Invalid animation joint count')
    count, = unpack('<I', data)
    if not 1 <= count <= 65535 or 4 + count * 4 > len(data):
        raise ValueError('Invalid animation frame count')
    descriptors = unpack('<' + 'I' * count, data, 4)
    offsets = [4 + (value & 0xffff) for value in descriptors] + [len(data)]
    if offsets[0] < 4 + count * 4 or any(a >= b for a, b in zip(offsets, offsets[1:])):
        raise ValueError('Invalid animation frame offsets')
    angles = [0] * (joints * 3)
    result = []
    for index in range(count):
        frame = data[offsets[index]:offsets[index + 1]]
        root = unpack('<3i', frame)
        mask_size, = unpack('<b', frame, 12)
        if not 1 <= mask_size <= len(frame) - 13:
            raise ValueError('Invalid animation bit mask')
        mask = frame[13:13 + mask_size]
        cursor, bit = 13 + mask_size, 0

        def take_bit():
            nonlocal bit
            if bit >= len(mask) * 8:
                raise ValueError('Truncated animation bit mask')
            value = (mask[bit // 8] >> (bit % 8)) & 1
            bit += 1
            return value

        for component in range(joints * 3):
            if take_bit():
                fmt = '<h' if index == 0 or take_bit() else '<b'
                value, = unpack(fmt, frame, cursor)
                cursor += struct.calcsize(fmt)
                angles[component] = value if index == 0 else (angles[component] + value * 2) & 0xffe
            elif index == 0:
                angles[component] = 0
        result.append(dict(root=root, angles=tuple(angles), flags=descriptors[index] >> 16))
    return result


def compare_skeletons(hero, donor, mapping):
    """Compare candidate hero-to-donor joint assignments, not retarget poses."""
    target = {step['joint']: step for step in hero['steps']}
    source = {step['joint']: step for step in donor['steps']}
    differences = []
    for joint, donor_joint in mapping.items():
        if joint == 0:
            continue
        left, right = target[joint], source[donor_joint]
        fields = [field for field in ('translation', 'bind', 'post', 'special')
                  if left[field] != right[field]]
        if mapping.get(left['parent']) != right['parent']:
            fields.append('parent')
        if fields:
            differences.append(dict(hero_joint=joint, donor_joint=donor_joint, fields=fields))
    return dict(differences=differences,
                unmapped_hero_joints=[dict(joint=j, vertices=s['vertices'])
                                     for j, s in target.items() if j not in mapping])


def analyze(asset_directory):
    manifests = [json.loads((ROOT / 'config' / (edition + '-english.json')).read_text())
                 for edition in ('retail', 'rip')]
    hashes = [{row['path'].upper(): row['sha256'] for row in manifest['files']}
              for manifest in manifests]
    assets = {}
    for archive in (*HEROES, 'CHARS11', 'CHARS15'):
        key = 'WADS/' + archive + '.WAD'
        data = (asset_directory / (archive + '.WAD')).read_bytes()
        if hashes[0][key] != hashes[1][key] or hashlib.sha256(data).hexdigest() != hashes[0][key]:
            raise ValueError('Unreviewed asset: ' + archive)
        assets[archive] = read_wad(data)
    heroes, donors, models = {}, {}, {}
    for hero in HEROES:
        model = read_skeleton(assets[hero][hero + '.ACH'])
        models[hero] = model
        clips = read_animations(assets[hero][hero + '.ADT'])
        frame_count = sum(len(decode_frames(clip, model['joints'])) for clip in clips.values())
        heroes[hero] = dict(joints=model['joints'], clips=len(clips), frames=frame_count,
                            guard_names=[name for name in clips if 'BLOCK' in name])
    for archive, animation, model_name in DONORS:
        model = read_skeleton(assets[archive][model_name + '.ACH'])
        models[model_name] = model
        clips = read_animations(assets[archive][animation + '.ADT'])
        decoded = {name: decode_frames(clips[name], model['joints']) for name in GUARD_NAMES}
        donors[animation] = dict(joints=model['joints'], model=model_name,
                                 frames={name: len(frames) for name, frames in decoded.items()},
                                 frame_flags=sorted({frame['flags'] for frames in decoded.values()
                                                     for frame in frames}))
    mappings = {
        'HAWK': ('SUIT', {i: i for i in range(20)}),
        'MACE': ('VAMP', {i: i for i in range(30)}),
        'ALANA': ('VAMP', {i: i for i in range(13)}),  # Upper body only.
        'SMASHER': ('BIGBLACK', {**{i: i for i in range(7)},
                                **{i + 3: i for i in range(7, 10)},
                                **{i + 6: i for i in range(10, 21)}}),
    }
    comparisons = {hero: compare_skeletons(models[hero], models[donor], mapping)
                   for hero, (donor, mapping) in mappings.items()}
    return dict(method='Read-only asset structure and frame decoding; no runtime or visual validation',
                reviewed_assets_match_both_editions=True, heroes=heroes, donors=donors,
                candidate_joint_maps=comparisons)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--assets', type=Path, default=ROOT / 'game' / 'Wads')
    args = parser.parse_args()
    try:
        result = analyze(args.assets)
    except (ValueError, KeyError, OSError) as error:
        # Avoid exposing installation paths in generated research output.
        parser.exit(1, 'Guard asset inspection failed: ' +
                    (str(error) if isinstance(error, ValueError) else type(error).__name__) + '\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
