"""Exercise read-only asset parsing with synthetic, non-retail records."""
import pathlib
import struct
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / 'tools'))
import analyze_guard as guard


def clip(frames):
    offset = len(frames) * 4
    table = []
    for flags, payload in frames:
        table.append((flags << 16) | offset)
        offset += len(payload)
    return (struct.pack('<I', len(frames)) + struct.pack('<' + 'I' * len(table), *table)
            + b''.join(payload for _, payload in frames))


class GuardAssetTests(unittest.TestCase):
    def test_wad_byte_order_and_member_bounds(self):
        data = struct.pack('>I12sII', 1, b'POSE.ADT', 3, 24) + b'abc'
        self.assertEqual(guard.read_wad(data), {'POSE.ADT': b'abc'})
        with self.assertRaises(ValueError):
            guard.read_wad(data[:-1])
        with self.assertRaises(ValueError):
            guard.read_wad(struct.pack('>I12sII', 1, b'POSE.ADT', 3, 4) + b'abc')

    def test_adt_size_includes_its_own_word_and_last_name_wins(self):
        data = (struct.pack('<8sI', b'POSE', 7) + b'abc'
                + struct.pack('<8sI', b'POSE', 5) + b'z' + b'\0')
        self.assertEqual(guard.read_animations(data), {'POSE': b'z'})
        with self.assertRaises(ValueError):
            guard.read_animations(data[:-1])

    def test_absolute_frame_and_signed_narrow_and_wide_deltas(self):
        first = struct.pack('<3ibBhh', -1000, 20, -30, 1, 0b101, 100, -20)
        second = struct.pack('<3ibBbh', 500, -40, 60, 1, 0b11001, -3, 300)
        frames = guard.decode_frames(clip([(0, first), (0x123, second)]), 1)
        self.assertEqual(frames[0]['root'], (-1000, 20, -30))
        self.assertEqual(frames[0]['angles'], (100, 0, -20))
        self.assertEqual(frames[1]['angles'], (94, 0, 580))
        self.assertEqual(frames[1]['flags'], 0x123)

    def test_delta_wrap_and_bit_mask_crossing(self):
        first = struct.pack('<3ibBhhh', 0, 0, 0, 1, 0b111, 0, 4094, 2)
        second = struct.pack('<3ibBbbb', 0, 0, 0, 1, 0b010101, -2, 2, -2)
        frames = guard.decode_frames(clip([(0, first), (0, second)]), 1)
        self.assertEqual(frames[1]['angles'], (4092, 2, 4094))
        # Nine angle components require a second mask byte.
        wide = struct.pack('<3ibBBh', 0, 0, 0, 2, 0, 1, 12)
        self.assertEqual(guard.decode_frames(clip([(0, wide)]), 3)[0]['angles'],
                         (0,) * 8 + (12,))

    def test_frame_rejects_missing_payload_and_insufficient_mask(self):
        frame = struct.pack('<3ibB', 0, 0, 0, 1, 1)
        with self.assertRaises(ValueError):
            guard.decode_frames(clip([(0, frame)]), 1)
        frame = struct.pack('<3ibB', 0, 0, 0, 1, 0)
        with self.assertRaises(ValueError):
            guard.decode_frames(clip([(0, frame)]), 3)
        with self.assertRaises(ValueError):
            guard.decode_frames(struct.pack('<II', 1, 0), 1)

    def test_ach_offsets_and_hierarchy(self):
        header = struct.pack('<9iI', -1, 88, 1, 0, 0, 0, 2, 36, 1, 1)
        commands = struct.pack('<13i', 3, 0, 0, 1, 2, 0, 0, 0, 1, 0, 0, 0, 0)
        vertices = struct.pack('<4h', 1, 2, 3, 0)
        data = header + commands + vertices
        model = guard.read_skeleton(data)
        self.assertEqual(model['joints'], 2)
        self.assertEqual(model['steps'][0]['translation'], (0, 1, 2))
        self.assertEqual(model['steps'][0]['parent'], 0)
        with self.assertRaises(ValueError):
            guard.read_skeleton(data[:-1])
        with self.assertRaises(ValueError):
            guard.read_skeleton(header + struct.pack('<i', 2) + commands[4:] + vertices)


if __name__ == '__main__':
    unittest.main()
