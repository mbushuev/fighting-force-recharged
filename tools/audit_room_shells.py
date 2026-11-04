"""Read-only model-shell triage; open boundaries are candidates, not defects."""
import argparse
import json
import struct
from pathlib import Path


def members(data):
    if len(data) < 4:
        raise ValueError('Truncated WAD header')
    count, = struct.unpack_from('>I', data)
    if count > 4096 or 4 + count * 20 > len(data):
        raise ValueError('Invalid WAD directory')
    for i in range(count):
        name, size, offset = struct.unpack_from('>12sII', data, 4 + i * 20)
        if offset + size > len(data):
            raise ValueError('WAD member outside archive')
        yield name.rstrip(b'\0').decode('ascii'), data[offset:offset + size]


def model(data):
    if len(data) < 20:
        raise ValueError('Truncated MOD header')
    _, vo, count, fo, faces = struct.unpack_from('<5i', data)
    if (vo != 16 or not 0 < count <= 16384 or not 0 <= faces <= 32768 or
            fo < vo + count * 8 or 4 + fo + faces * 12 > len(data)):
        raise ValueError('Invalid MOD ranges')
    vertices = [tuple(v >> 1 for v in struct.unpack_from('<4h', data, 4 + vo + i * 8)[:3])
                for i in range(count)]
    triangles = []
    for i in range(faces):
        *indices, flags, material = struct.unpack_from('<6h', data, 4 + fo + i * 12)
        if indices[3] == -1:
            continue
        if any(v < 0 or v % 4 or v // 4 >= count for v in indices):
            raise ValueError('Invalid MOD face index')
        if material < 0:
            continue
        points = [vertices[v // 4] for v in indices]
        triangles.extend(((points[0], points[1], points[2]), (points[0], points[2], points[3])))
    return vertices, triangles, faces


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def hit(origin, direction, triangle):
    a, b, c = triangle
    e1, e2 = sub(b, a), sub(c, a)
    p = cross(direction, e2)
    det = dot(e1, p)
    if abs(det) < 1e-9:
        return None
    delta = sub(origin, a)
    u = dot(delta, p) / det
    q = cross(delta, e1)
    v = dot(direction, q) / det
    distance = dot(e2, q) / det
    return distance if u >= -1e-6 and v >= -1e-6 and u + v <= 1.000001 and distance >= 0 else None


def coverage(vertices, triangles):
    bounds = [(min(v[k] for v in vertices), max(v[k] for v in vertices)) for k in range(3)]
    spans = [b - a for a, b in bounds]
    if min(spans) <= 0:
        return None
    result = {}
    # Multiple heights and origins distinguish broad openings from a doorway.
    # Consider only each outer quarter; furniture/internal dividers cannot
    # stand in for a model's far wall.
    for axis, label in ((0, 'X'), (2, 'Z')):
        other = 2 if axis == 0 else 0
        for sign in (-1, 1):
            covered = 0
            direction = tuple(sign if k == axis else 0 for k in range(3))
            for across in range(1, 10):
                for height in (.15, .4, .7):
                    origin = [(a + b) / 2 for a, b in bounds]
                    origin[other] = bounds[other][0] + spans[other] * across / 10
                    origin[1] = bounds[1][0] + spans[1] * height
                    for triangle in triangles:
                        distance = hit(origin, direction, triangle)
                        if distance is not None and spans[axis] * .25 <= distance <= spans[axis] * .55:
                            covered += 1
                            break
            result[('+' if sign > 0 else '-') + label] = round(covered / 27, 3)
    return bounds, result


def audit(directory):
    records = []
    for archive in sorted(directory.glob('MODELS*.WAD')):
        suffix = archive.stem[6:]
        if suffix.startswith('C') or suffix == 'FE':
            continue  # Recorded scenes and front end have different ownership.
        for name, data in members(archive.read_bytes()):
            if not name.endswith('.MOD'):
                continue
            vertices, triangles, face_count = model(data)
            spans = [max(v[k] for v in vertices) - min(v[k] for v in vertices) for k in range(3)]
            if min(spans[0], spans[2]) < 600 or spans[1] < 350 or face_count < 40:
                continue
            measured = coverage(vertices, triangles)
            if not measured:
                continue
            _, sides = measured
            ordered = sorted(sides.values())
            records.append({'archive': archive.name, 'model': name, 'vertices': len(vertices),
                            'faces': face_count, 'side_coverage': sides,
                            'three_sided_candidate': ordered[0] <= .4 and ordered[1] >= .55})
    return {'method': 'Per-model outer-quarter rays; no scene placement or material-opacity evaluation.',
            'limit': 'Open boundaries can be doors, adjoining meshes or outdoor space. Review the assembled stage before reconstruction.',
            'models': records}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('wad_directory', type=Path)
    args = parser.parse_args()
    print(json.dumps(audit(args.wad_directory), indent=2))


if __name__ == '__main__':
    main()
