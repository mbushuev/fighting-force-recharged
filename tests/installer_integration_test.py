"""Exercise the built setup against original media in isolated local folders."""
import argparse
import hashlib
import json
import pathlib
import shutil
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parent.parent


def run(setup, *args, success=True):
    result = subprocess.run([str(setup), *map(str, args)], capture_output=True, text=True)
    if (result.returncode == 0) != success:
        raise AssertionError(result.stdout + result.stderr)
    return result


def inventory(folder):
    return {str(p.relative_to(folder)): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in folder.rglob('*') if p.is_file()}


def reject_game_folder(setup, folder, work, label):
    before = inventory(folder)
    target = work / label
    for args in (('--inspect-media', '--media', folder),
                 ('--install', '--media', folder, '--target', target),
                 ('--apply', '--target', folder)):
        result = run(setup, *args, success=False)
        assert 'modified or unsupported fforce.exe' in result.stderr
    assert not target.exists()
    assert not list(work.glob('.ff-install-*'))
    assert inventory(folder) == before


def apply_game_folder(setup, folder, work, label, manifest, rip=False):
    before = inventory(folder)
    target = work / label
    result = run(setup, '--inspect-source', '--source', folder)
    assert 'mode: folder' in result.stdout
    run(setup, '--install', '--source', folder, '--target', target, success=False)
    assert not target.exists()
    # Isolate installer tests; the installer itself must not copy game files.
    shutil.copytree(folder, target)
    native_before = {record['path']: (hashlib.sha256((target / record['path']).read_bytes()).hexdigest(),
                                     (target / record['path']).stat().st_mtime_ns,
                                     (target / record['path']).stat().st_ino)
                     for record in manifest['files']}
    run(setup, '--apply', '--target', target)
    run(setup, '--verify', '--target', target)
    for record in manifest['files']:
        path = target / record['path']
        assert path.stat().st_size == record['size']
        assert hashlib.sha256(path.read_bytes()).hexdigest() == record['sha256']
        assert (record['sha256'], path.stat().st_mtime_ns, path.stat().st_ino) == native_before[record['path']]
    assert (target / 'Compatibility/rip-english.json').is_file()
    assert not (target / 'unshield.exe').exists()
    if rip:
        assert (target / 'INTRO/LOGO.RPL').stat().st_size == 0
        assert not (target / 'Mfc30.dll').exists()
    assert inventory(folder) == before
    run(setup, '--apply', '--target', target)
    assert inventory(folder) == before
    assert not list(target.glob('.ff-fix-*'))
    # A known EXE with incomplete data must fail before any compatibility write.
    incomplete = work / ('incomplete-' + label)
    incomplete.mkdir()
    shutil.copyfile(folder / 'FFORCE.EXE', incomplete / 'FFORCE.EXE')
    incomplete_before = inventory(incomplete)
    result = run(setup, '--apply', '--target', incomplete, success=False)
    assert 'Original file verification failed' in result.stderr
    assert inventory(incomplete) == incomplete_before
    assert not list(incomplete.glob('.ff-fix-*'))
    assert not list(work.glob('.ff-install-*'))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--media', type=pathlib.Path, required=True)
    parser.add_argument('--game-folder', type=pathlib.Path,
                        help='Also install and verify the reviewed RIP folder without changing its files.')
    args = parser.parse_args()
    setup = ROOT / 'dist/FightingForceSetup.exe'
    manifest = json.loads((ROOT / 'config/retail-english.json').read_text())
    parent = ROOT / 'work/installer-tests'
    parent.mkdir(parents=True, exist_ok=True)
    # Keep artifacts under ignored work for inspection; never overwrite a run.
    work = pathlib.Path(tempfile.mkdtemp(prefix='run-', dir=str(parent)))
    target = work / 'Game with spaces \u0418\u0433\u0440\u0430'
    run(setup, '--help')
    run(setup, '--inspect-media', '--media', args.media)
    run(setup, '--install', '--media', args.media, '--target', target)
    run(setup, '--verify', '--target', target)
    for record in manifest['files']:
        path = target / record['path']
        assert path.stat().st_size == record['size']
        assert hashlib.sha256(path.read_bytes()).hexdigest() == record['sha256']
    for name in ('DINPUT.dll', 'DDraw.dll', 'D3DImm.dll', 'D3D9.dll',
                 'fightingforce-compat.ini', 'dgVoodoo.conf'):
        assert (target / name).is_file(), name
    assert not (target / 'unshield.exe').exists()
    assert not (target / 'zlib1.dll').exists()
    assert not (target / 'INSTALL.EXE').exists()
    assert (target / 'Compatibility/licenses/unshield.txt').is_file()
    before = hashlib.sha256((target / 'fforce.exe').read_bytes()).hexdigest()
    run(setup, '--install', '--media', args.media, '--target', target, success=False)
    assert hashlib.sha256((target / 'fforce.exe').read_bytes()).hexdigest() == before
    untouched = work / 'existing'
    untouched.mkdir()
    (untouched / 'keep.txt').write_text('keep')
    run(setup, '--install', '--media', args.media, '--target', untouched, success=False)
    assert list(untouched.iterdir()) == [untouched / 'keep.txt']
    assert (untouched / 'keep.txt').read_text() == 'keep'
    fake = work / 'fake-disc'
    fake.mkdir()
    failed = work / 'failed-install'
    run(setup, '--install', '--media', fake, '--target', failed, success=False)
    assert not failed.exists()
    unsupported = work / 'unsupported-game-folder'
    unsupported.mkdir()
    (unsupported / 'fforce.exe').write_bytes(b'unsupported fixture')
    reject_game_folder(setup, unsupported, work, 'unsupported-folder-target')
    installed_source = work / 'installed-folder-source'
    installed_source.mkdir()
    shutil.copyfile(target / 'fforce.exe', installed_source / 'fforce.exe')
    result = run(setup, '--inspect-media', '--media', installed_source, success=False)
    assert 'Original file verification failed' in result.stderr
    apply_game_folder(setup, target, work, 'retail-folder-target', manifest)
    if args.game_folder:
        rip_manifest = json.loads((ROOT / 'config/rip-english.json').read_text())
        apply_game_folder(setup, args.game_folder, work, 'rip-folder-target', rip_manifest, rip=True)
        print('Reviewed RIP fix in place, native file identities and incomplete-source rejection: PASS')
    # A valid archive followed by missing/invalid asset files exercises rollback
    # after extraction and initial executable copies have already succeeded.
    partial = work / 'partial-disc'
    (partial / 'disk1').mkdir(parents=True)
    (partial / 'Wads').mkdir()
    (partial / 'Intro').mkdir()
    shutil.copyfile(args.media / 'disk1/DATA1.CAB', partial / 'disk1/DATA1.CAB')
    (partial / 'Wads/ENGLISH.WAD').write_bytes(b'invalid')
    (partial / 'Intro/logo.rpl').write_bytes(b'invalid')
    run(setup, '--install', '--media', partial, '--target', failed, success=False)
    assert not failed.exists()
    # Corrupt synthetic verification input; do not edit any retail game copy.
    synthetic = work / 'synthetic-invalid-install'
    synthetic.mkdir()
    (synthetic / 'fforce.exe').write_bytes(b'invalid')
    run(setup, '--verify', '--target', synthetic, success=False)
    assert not list(work.glob('.ff-install-*'))
    print('Original-disc installation: PASS; 235 exact retail hashes, Unicode paths,')
    print('wrapper payload, existing-folder refusal and invalid-media rejection.')
    print('Isolated installation retained under ignored work/installer-tests.')


if __name__ == '__main__':
    main()
