"""Review-set coverage and native stage-resource mapping checks."""
import importlib.util
from pathlib import Path
import struct
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('review', ROOT / 'tools/prepare_room_review.py')
review = importlib.util.module_from_spec(spec)
spec.loader.exec_module(review)


class RoomReviewTest(unittest.TestCase):
    def test_campaign_sets_cover_every_stage_once(self):
        stages = [s for p in review.packs()[3:] for s in p['stages']]
        self.assertEqual(stages, list(range(25)))
        self.assertTrue(all(1 <= len(p['stages']) <= 3 for p in review.packs()))
        self.assertEqual(review.packs()[0]['stages'], [23, 22, 7])

    def test_creation_never_overwrites_an_installation(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            marker = root / 'original.sav'
            marker.write_bytes(b'untouched')
            for output in (root, root / 'child'):
                with self.assertRaises(ValueError):
                    review.prepare(root, output)
            self.assertEqual(marker.read_bytes(), b'untouched')
            self.assertFalse((root / 'child').exists())

    def test_presets_reject_invalid_stage(self):
        for stage in (-1, 25):
            with self.assertRaises(ValueError):
                review.save(stage)


def verify_native_mapping(path):
    # Read initialized STAGE table assignments in either supported PE. No code
    # execution or optional disassembler dependency is needed for this check.
    data = path.read_bytes()
    pe, = struct.unpack_from('<I', data, 0x3c)
    sections, = struct.unpack_from('<H', data, pe + 6)
    optional, = struct.unpack_from('<H', data, pe + 20)
    base, = struct.unpack_from('<I', data, pe + 24 + 28)
    section_table = pe + 24 + optional

    def string(va):
        rva = va - base
        for i in range(sections):
            size, start, raw_size, offset = struct.unpack_from('<4I', data, section_table + i * 40 + 8)
            if start <= rva < start + min(size, raw_size):
                pos = offset + rva - start
                return data[pos:data.index(b'\0', pos)].decode('ascii').replace('\\', '/').upper()
        raise AssertionError('Stage string outside PE sections')

    for stage, resource in enumerate(review.STAGES):
        pattern = b'\xc7\x05' + struct.pack('<I', 0xc74d14 + stage * 4)
        expected = f'WADS/STAGE{resource}.WAD'
        values = []
        offset = data.find(pattern)
        while offset != -1:
            va, = struct.unpack_from('<I', data, offset + 6)
            try:
                values.append(string(va))
            except (AssertionError, UnicodeDecodeError, ValueError):
                pass
            offset = data.find(pattern, offset + 1)
        if expected not in values:
            raise AssertionError(f'Stage {stage + 1} does not map to {expected}')
    print('Native campaign resource mapping: PASS (25 stages)')


if __name__ == '__main__':
    if len(sys.argv) == 3 and sys.argv[1] == '--game-exe':
        verify_native_mapping(Path(sys.argv[2]))
    else:
        unittest.main()
