"""Check source allowlisting and rejection of private repository content."""
import importlib.util
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parent.parent
SPEC = importlib.util.spec_from_file_location('repository_policy', ROOT / 'scripts' / 'check_repository.py')
POLICY = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(POLICY)


class RepositoryPolicyTests(unittest.TestCase):
    def test_reviewed_source_is_allowed(self):
        self.assertEqual(POLICY.audit_file('src/example.h', b'#pragma once\n'), [])
        self.assertEqual(POLICY.audit_file('README.md', b'# Wrapper\n'), [])

    def test_local_and_binary_files_are_rejected(self):
        for path in ('game/runtime.dll', 'verification/result.json',
                     'tools/python/vendor.py', 'tools/unshield/README.md',
                     'config/.env', 'config/signing.key', 'src/image.png'):
            with self.subTest(path=path):
                self.assertTrue(POLICY.audit_file(path, b'fixture'))
        self.assertTrue(POLICY.audit_file('src/data.h', b'\0binary'))
        self.assertTrue(POLICY.audit_file('src/data.h', b'\xff'))

    def test_disc_images_backups_and_nested_metadata_are_rejected(self):
        for path in ('config/disc.img', 'config/disc.sub', 'config/disc.ccd',
                     'src/copy.sav.bak', 'docs/.codex/state.txt',
                     'config/.AWS/credentials', 'src/__pycache__/module.pyc',
                     'Tools/Python/vendor.py', 'src/../private.txt'):
            with self.subTest(path=path):
                self.assertTrue(POLICY.audit_file(path, b'fixture'))

    def test_credentials_are_rejected_without_echoing_values(self):
        values = ('gh' + 'p_' + 'A' * 36, 'github_' + 'pat_' + 'B' * 60,
                  'sk-' + 'proj-' + 'C' * 48, 'AK' + 'IA' + 'D' * 16,
                  '-----BEGIN ' + 'OPENSSH PRIVATE KEY-----',
                  'https://' + 'user:secret@example.org/path')
        for value in values:
            with self.subTest(length=len(value)):
                issues = POLICY.audit_file('src/example.h', value.encode())
                self.assertTrue(issues)
                self.assertNotIn(value, '\n'.join(issues))
                self.assertTrue(POLICY.audit_message('Add ' + value))

    def test_conversation_metadata_is_rejected_in_code_too(self):
        self.assertTrue(POLICY.audit_file('src/example.h', ('question' + 'ItemId').encode()))

    def test_machine_paths_are_rejected(self):
        windows = 'X:' + chr(92) + 'fixture' + chr(92) + 'file'
        posix = '/' + 'home/' + 'fixture/file'
        forward = 'X:' + '/' + 'fixture/file'
        for path in (windows, forward, posix, 'https://example.org/' + forward):
            self.assertTrue(POLICY.audit_file('README.md', path.encode()))
            self.assertTrue(POLICY.audit_message('Document ' + path))

    def test_web_urls_are_allowed(self):
        for url in ('https://example.org/release', 'http://example.org/docs',
                    'https://github.com/example/project', 'urn:example:release'):
            with self.subTest(url=url):
                self.assertEqual(POLICY.audit_file('docs/dependencies.md', url.encode()), [])
                self.assertEqual(POLICY.audit_file('config/dependencies.json', ('["' + url + '"]').encode()), [])
                self.assertEqual(POLICY.audit_message('Document ' + url), [])

    def test_documentation_and_commit_privacy(self):
        self.assertTrue(POLICY.audit_file('docs/check.md', ('question' + 'ItemId').encode()))
        self.assertTrue(POLICY.audit_file('docs/check.md', chr(0x0410).encode()))
        self.assertTrue(POLICY.audit_message(''))
        self.assertTrue(POLICY.audit_message('A' * 73))
        self.assertEqual(POLICY.audit_message('Preserve native texture filtering'), [])


if __name__ == '__main__':
    unittest.main()
