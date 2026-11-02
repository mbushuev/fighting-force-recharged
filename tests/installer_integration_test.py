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


def preserved_files(folder, names):
    return {name: (hashlib.sha256((folder / name).read_bytes()).hexdigest(),
                   (folder / name).stat().st_mtime_ns, (folder / name).stat().st_ino)
            for name in names if (folder / name).is_file()}


def relaxed_folder(setup, target, manifest):
    # Modify only this disposable fixture; supplied game folders remain read-only.
    wad = target / 'Wads/ENGLISH.WAD'
    with wad.open('ab') as output:
        output.write(b'non-reference resource fixture')
    absent = ('INTRO/LOGO.RPL', 'README.TXT', 'FFORCE.ICO', 'FFSETUP.EXE',
              'Wads/AI_OLD.DAT', 'Wads/000.DEM', 'Wads/FRENCH.WAD')
    for name in absent:
        (target / name).unlink(missing_ok=True)
    extra = target / 'Wads/custom-fixture.dat'
    extra.write_bytes(b'extra resource fixture')
    settings = target / 'fightingforce-compat.ini'
    settings.write_bytes(b'[Rendering]\nGameplayFPS=30\n')
    save = target / 'campaign-fixture.sav'
    save.write_bytes(b'preserve this save')
    names = [record['path'] for record in manifest['files']] + [
        str(extra.relative_to(target)), settings.name, save.name]
    before = preserved_files(target, names)
    run(setup, '--inspect-source', '--source', target)
    # Exact reference verification remains an explicitly requested diagnostic.
    run(setup, '--verify', '--target', target, success=False)
    for _ in range(2):
        run(setup, '--apply', '--target', target)
        assert preserved_files(target, names) == before
        assert all(not (target / name).exists() for name in absent)
    missing = wad.read_bytes()
    wad.write_bytes(b'')
    invalid_before = inventory(target)
    result = run(setup, '--apply', '--target', target, success=False)
    assert 'Required game data' in result.stderr
    assert inventory(target) == invalid_before
    wad.write_bytes(missing)


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
    relaxed_folder(setup, target, manifest)
    # A known EXE with incomplete data must fail before any compatibility write.
    incomplete = work / ('incomplete-' + label)
    incomplete.mkdir()
    shutil.copyfile(folder / 'FFORCE.EXE', incomplete / 'FFORCE.EXE')
    incomplete_before = inventory(incomplete)
    result = run(setup, '--apply', '--target', incomplete, success=False)
    assert 'Required game data' in result.stderr
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
    assert 'Required game data' in result.stderr
    apply_game_folder(setup, target, work, 'retail-folder-target', manifest)
    if args.game_folder:
        rip_manifest = json.loads((ROOT / 'config/rip-english.json').read_text())
        apply_game_folder(setup, args.game_folder, work, 'rip-folder-target', rip_manifest, rip=True)
        print('Reviewed RIP fix in place, native file identities and incomplete-source rejection: PASS')
    # Missing required data is still refused before destination publication.
    partial = work / 'partial-disc'
    (partial / 'disk1').mkdir(parents=True)
    (partial / 'Wads').mkdir()
    (partial / 'Intro').mkdir()
    shutil.copyfile(args.media / 'disk1/DATA1.CAB', partial / 'disk1/DATA1.CAB')
    (partial / 'Wads/ENGLISH.WAD').write_bytes(b'')
    (partial / 'Intro/logo.rpl').write_bytes(b'invalid')
    run(setup, '--install', '--media', partial, '--target', failed, success=False)
    assert not failed.exists()
    bad_cabinet = work / 'unreadable-cabinet'
    (bad_cabinet / 'disk1').mkdir(parents=True)
    (bad_cabinet / 'Wads').mkdir()
    (bad_cabinet / 'disk1/DATA1.CAB').write_bytes(b'not an InstallShield cabinet')
    (bad_cabinet / 'Wads/ENGLISH.WAD').write_bytes(b'nonempty main data fixture')
    bad_before = inventory(bad_cabinet)
    run(setup, '--install', '--source', bad_cabinet, '--target', failed, success=False)
    assert not failed.exists() and inventory(bad_cabinet) == bad_before
    # Repacking/padding a readable CAB and changing resources does not require
    # an exact disc inventory. Preserve selected source bytes in the new copy.
    variant = work / 'compatible-disc-variant'
    shutil.copytree(args.media, variant)
    with (variant / 'disk1/DATA1.CAB').open('ab') as output:
        output.write(b'compatible cabinet padding fixture')
    with (variant / 'Wads/ENGLISH.WAD').open('ab') as output:
        output.write(b'non-reference resource fixture')
    for name in ('INTRO/LOGO.RPL', 'README.TXT', 'FFORCE.ICO', 'Wads/FRENCH.WAD'):
        (variant / name).unlink(missing_ok=True)
    (variant / 'Wads/custom-fixture').mkdir()
    (variant / 'Wads/custom-fixture/extra.dat').write_bytes(b'custom source data')
    variant_before = inventory(variant)
    variant_target = work / 'variant-game'
    run(setup, '--inspect-source', '--source', variant)
    run(setup, '--install', '--source', variant, '--target', variant_target)
    assert inventory(variant) == variant_before
    for name, digest in variant_before.items():
        if name.lower().startswith(('wads', 'intro')):
            assert hashlib.sha256((variant_target / name).read_bytes()).hexdigest() == digest
    for name in ('INTRO/LOGO.RPL', 'README.TXT', 'FFORCE.ICO', 'Wads/FRENCH.WAD'):
        assert not (variant_target / name).exists()
    assert hashlib.sha256((variant_target / 'fforce.exe').read_bytes()).hexdigest() == before
    run(setup, '--verify', '--target', variant_target, success=False)
    # Corrupt synthetic verification input; do not edit any retail game copy.
    synthetic = work / 'synthetic-invalid-install'
    synthetic.mkdir()
    (synthetic / 'fforce.exe').write_bytes(b'invalid')
    run(setup, '--verify', '--target', synthetic, success=False)
    assert not list(work.glob('.ff-install-*'))
    print('Original-disc installation: PASS; 235 exact retail hashes, Unicode paths,')
    print('Non-reference FULL/RIP resources, absent optional files, source-based disc')
    print('copy verification, wrapper payload, preservation and required-input rejection: PASS.')
    print('Isolated installation retained under ignored work/installer-tests.')


if __name__ == '__main__':
    main()
