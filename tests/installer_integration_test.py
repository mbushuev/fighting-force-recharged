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


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--media', type=pathlib.Path, required=True)
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
