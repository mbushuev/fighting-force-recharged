"""Exercise release version gates, provenance and downloaded archive rejection."""
import json
import pathlib
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'scripts'))
import package_release as package
import release_metadata
import verify_release


class ReleaseMetadataTests(unittest.TestCase):
    def test_source_version_tag_and_diagnostic_gates(self):
        with tempfile.TemporaryDirectory() as folder:
            root = pathlib.Path(folder)
            (root / '.github/release-notes').mkdir(parents=True)
            (root / 'config').mkdir()
            (root / 'VERSION').write_text('1.2.3-rc.1\n')
            (root / 'README.md').write_text('Current candidate: **1.2.3-rc.1**')
            (root / 'CHANGELOG.md').write_text('## Unreleased\n')
            (root / '.github/release-notes/v1.2.3-rc.1.md').write_text('# Fighting Force Recharged 1.2.3-rc.1\n')
            config = root / 'config/fightingforce-compat.ini'
            safe = '[Diagnostics]\nTraceRenderer=0\nTraceCooperative=0\nTraceFrameRate=0\n[Combat]\nPlayerGuard=0\n'
            config.write_text(safe)
            self.assertEqual(release_metadata.validate(root, 'v1.2.3-rc.1'), '1.2.3-rc.1')
            with self.assertRaises(ValueError):
                release_metadata.validate(root, 'v1.2.3')
            for old, new in [('TraceRenderer=0', 'TraceRenderer=1'), ('PlayerGuard=0', 'PlayerGuard=1')]:
                config.write_text(safe.replace(old, new))
                with self.assertRaises(ValueError):
                    release_metadata.validate(root)

    def test_downloaded_candidates_reject_tampering_and_extra_files(self):
        version, revision = '1.2.3-rc.1', 'a' * 40
        prefix = 'fighting-force-recharged-' + version + '/'
        source = package.make_zip({prefix + 'LICENSE': b'MIT', prefix + 'src/ok.h': b'code'})
        windows = package.make_zip(package.windows_files(b'MZ fixture installer'))
        stem = 'FightingForceRecharged-' + version
        archives = {stem + '-Source.zip': source, stem + '-Windows.zip': windows}
        meta = {'schema': 1, 'project': 'Fighting Force Recharged', 'version': version,
                'source_commit': revision, 'prerelease': True,
                'artifacts': [{'name': n, 'size': len(b), 'sha256': package.digest(b)} for n, b in archives.items()]}
        files = dict(archives, **{'release.json': json.dumps(meta).encode()})
        files['SHA256SUMS.txt'] = ''.join(package.digest(b) + '  ' + n + '\n' for n, b in files.items()).encode()
        with tempfile.TemporaryDirectory() as folder:
            root = pathlib.Path(folder)
            for name, content in files.items():
                (root / name).write_bytes(content)
            verify_release.verify(root, revision, version, source)
            with self.assertRaises(ValueError):
                verify_release.verify(root, 'b' * 40, version, source)
            with self.assertRaises(ValueError):
                verify_release.verify(root, revision, version, package.make_zip({prefix + 'LICENSE': b'changed'}))
            (root / 'raw.log').write_text('private')
            with self.assertRaises(ValueError):
                verify_release.verify(root, revision, version, source)
            (root / 'raw.log').unlink()
            (root / (stem + '-Windows.zip')).write_bytes(windows + b'changed')
            with self.assertRaises(ValueError):
                verify_release.verify(root, revision, version, source)

    def test_stable_release_labels_and_changelog(self):
        with tempfile.TemporaryDirectory() as folder:
            root = pathlib.Path(folder)
            (root / '.github/release-notes').mkdir(parents=True)
            (root / 'config').mkdir()
            (root / 'VERSION').write_text('0.0.130\n')
            readme = root / 'README.md'
            readme.write_text('Current version: **0.0.130**')
            changelog = root / 'CHANGELOG.md'
            changelog.write_text('## Unreleased\n\n## 0.0.130 - 2026-11-01\n')
            (root / '.github/release-notes/v0.0.130.md').write_text('# Fighting Force Recharged 0.0.130\n')
            (root / 'config/fightingforce-compat.ini').write_text(
                '[Diagnostics]\nTraceRenderer=0\nTraceCooperative=0\nTraceFrameRate=0\n[Combat]\nPlayerGuard=0\n')
            self.assertEqual(release_metadata.validate(root, 'v0.0.130'), '0.0.130')
            with self.assertRaises(ValueError):
                release_metadata.validate(root, 'v0.0.131')
            readme.write_text('Current candidate: **0.0.130**')
            with self.assertRaises(ValueError):
                release_metadata.validate(root)
            readme.write_text('Current version: **0.0.130**')
            changelog.write_text('## Unreleased\n')
            with self.assertRaises(ValueError):
                release_metadata.validate(root)
            # Later releases increment the patch version independently of Git history.
            (root / 'VERSION').write_text('0.0.131\n')
            readme.write_text('Current version: **0.0.131**')
            changelog.write_text('## Unreleased\n\n## 0.0.131 - 2026-11-02\n')
            (root / '.github/release-notes/v0.0.131.md').write_text('# Fighting Force Recharged 0.0.131\n')
            self.assertEqual(release_metadata.validate(root, 'v0.0.131'), '0.0.131')


if __name__ == '__main__':
    unittest.main()
