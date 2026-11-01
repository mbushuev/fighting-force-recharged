"""Reject accidental release extras, stale payloads and unsafe archives."""
import importlib.util
import io
import pathlib
import sys
import unittest
import warnings
import zipfile

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'scripts'))
SPEC = importlib.util.spec_from_file_location('release_package', ROOT / 'scripts/package_release.py')
PACKAGE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(PACKAGE)


class ReleasePackageTests(unittest.TestCase):
    def test_payload_has_exact_allowlist_and_hashes(self):
        expected = {'game/DINPUT.dll': b'wrapper', 'support/LICENSE': b'license'}
        dependency = {'name': 'DDraw.dll', 'size': 7, 'sha256': PACKAGE.digest(b'runtime')}
        files = dict(expected, **{'game/DDraw.dll': b'runtime'})
        PACKAGE.verify_payload(PACKAGE.make_zip(files), expected, [dependency])
        for name, data in [('game/fforce.exe', b'game'), ('game/player.sav', b'save'),
                           ('support/raw.log', b'log'), ('game/personal.ini', b'local')]:
            with self.subTest(name=name):
                contaminated = dict(files, **{name: data})
                with self.assertRaises(ValueError):
                    PACKAGE.verify_payload(PACKAGE.make_zip(contaminated), expected, [dependency])
        files['game/DINPUT.dll'] = b'stale'
        with self.assertRaises(ValueError):
            PACKAGE.verify_payload(PACKAGE.make_zip(files), expected, [dependency])
        files['game/DINPUT.dll'] = expected['game/DINPUT.dll']
        files['game/DDraw.dll'] = b'changed'
        with self.assertRaises(ValueError):
            PACKAGE.verify_payload(PACKAGE.make_zip(files), expected, [dependency])

    def test_source_archive_rejects_media_metadata_and_secrets(self):
        prefix = 'source/'
        clean = {prefix + 'LICENSE': b'MIT', prefix + 'src/example.h': b'#pragma once\n'}
        PACKAGE.verify_source(PACKAGE.make_zip(clean), prefix)
        for name, data in [('CDs/image.img', b'image'), ('.git/config', b'metadata'),
                           ('src/config.h', ('gh' + 'p_' + 'A' * 36).encode())]:
            with self.subTest(name=name):
                with self.assertRaises(ValueError):
                    PACKAGE.verify_source(PACKAGE.make_zip(dict(clean, **{prefix + name: data})), prefix)

    def test_duplicate_and_escaping_zip_names_are_rejected(self):
        for name in ('../outside', '/absolute', 'folder/../outside', 'folder\\private', 'X:' + '/private'):
            with self.assertRaises(ValueError):
                PACKAGE.read_zip(PACKAGE.make_zip({name: b'data'}))
        output = io.BytesIO()
        with warnings.catch_warnings():
            warnings.simplefilter('ignore', UserWarning)
            with zipfile.ZipFile(output, 'w') as archive:
                archive.writestr('one', b'one')
                archive.writestr('one', b'two')
        with self.assertRaises(ValueError):
            PACKAGE.read_zip(output.getvalue())

    def test_archive_is_deterministic_and_binary_paths_are_rejected(self):
        self.assertEqual(PACKAGE.make_zip({'b': b'2', 'a': b'1'}),
                         PACKAGE.make_zip({'a': b'1', 'b': b'2'}))
        for encoding in ('utf-8', 'utf-16le'):
            with self.assertRaises(ValueError):
                PACKAGE.verify_own_binary(str(ROOT).encode(encoding))
            with self.assertRaises(ValueError):
                PACKAGE.verify_own_binary(('X:' + chr(92) + 'fixture' + chr(92) + 'private.pdb').encode(encoding))


if __name__ == '__main__':
    unittest.main()
