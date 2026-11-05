"""Synthetic fixtures for read-only shell triage; no game data required."""
import importlib.util
import struct
import tempfile
import unittest
from pathlib import Path

spec = importlib.util.spec_from_file_location('room_audit', Path(__file__).resolve().parents[1] / 'tools/audit_room_shells.py')
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


def box(closed=False, offset=(0, 0, 0)):
    vertices = [(x + offset[0], y + offset[1], z + offset[2])
                for x, y, z in [(-1000, 0, -1000), (1000, 0, -1000), (1000, 2000, -1000), (-1000, 2000, -1000),
                                (-1000, 0, 1000), (1000, 0, 1000), (1000, 2000, 1000), (-1000, 2000, 1000)]]
    faces = [(0, 1, 2, 3), (0, 3, 7, 4), (1, 5, 6, 2), (3, 2, 6, 7), (0, 4, 5, 1)]
    if closed:
        faces.append((4, 7, 6, 5))
    data = struct.pack('<5i', 0, 16, len(vertices), 16 + len(vertices) * 8, len(faces))
    data += b''.join(struct.pack('<4h', *v, 0) for v in vertices)
    data += b''.join(struct.pack('<6h', *(v * 4 for v in f), 0, 0) for f in faces)
    return data


class RoomAuditTest(unittest.TestCase):
    def test_archive_inventory_includes_singular_and_low_poly_rooms(self):
        data = box()
        wad = struct.pack('>I', 1) + struct.pack('>12sII', b'ROOM.MOD', len(data), 24) + data
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            for name in ('MODEL33A.WAD', 'models13.wad', 'MODELSC1.WAD', 'MODELSFE.WAD'):
                (directory / name).write_bytes(wad)
            result = audit.audit(directory)
        self.assertEqual(len(result['included_archives']), 2)
        self.assertEqual(len(result['excluded_archives']), 2)
        self.assertEqual(len(result['models']), 2)
        self.assertTrue(all(r['three_sided_candidate'] and r['large_shell'] for r in result['models']))

    def test_open_side_and_translation(self):
        for offset in [(0, 0, 0), (700, -200, 1400)]:
            vertices, triangles, _ = audit.model(box(offset=offset))
            _, sides = audit.coverage(vertices, triangles)
            self.assertEqual(sides, {'-X': 1, '+X': 1, '-Z': 1, '+Z': 0})

    def test_closed_shell(self):
        vertices, triangles, _ = audit.model(box(closed=True))
        self.assertTrue(all(v == 1 for v in audit.coverage(vertices, triangles)[1].values()))

    def test_internal_furniture_does_not_close_boundary(self):
        vertices, triangles, _ = audit.model(box())
        triangles += [((-400, 0, 0), (400, 0, 0), (400, 1000, 0)),
                      ((-400, 0, 0), (400, 1000, 0), (-400, 1000, 0))]
        self.assertEqual(audit.coverage(vertices, triangles)[1]['+Z'], 0)

    def test_native_half_scale(self):
        vertices, _, _ = audit.model(box(offset=(-1, -1, -1)))
        self.assertEqual(vertices[0], (-501, -1, -501))

    def test_invalid_ranges_and_indices(self):
        for data in [b'', box()[:-1], box()[:4] + struct.pack('<i', -1) + box()[8:]]:
            with self.assertRaises(ValueError):
                audit.model(data)
        data = bytearray(box())
        struct.pack_into('<h', data, 20 + 8 * 8, 9996)
        with self.assertRaises(ValueError):
            audit.model(data)

    def test_wad_bounds_and_duplicate_names(self):
        data = struct.pack('>I', 2) + struct.pack('>12sII', b'ROOM.MOD', 1, 44) * 2 + b'x'
        self.assertEqual(list(audit.members(data)), [('ROOM.MOD', b'x'), ('ROOM.MOD', b'x')])
        with self.assertRaises(ValueError):
            list(audit.members(data[:-1]))


if __name__ == '__main__':
    unittest.main()
