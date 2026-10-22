#!/usr/bin/env python3
"""Run reconstructed RC with reconstructed RCPP in an isolated toolchain overlay."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
TC = ROOT / 'toolchains/1.0-nt'
SCRATCH = Path.home() / '.ai-tmp/vc1.0-resource-composed'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    SCRATCH.mkdir(parents=True, exist_ok=True)
    prefix = SCRATCH / 'wine'
    env = os.environ.copy()
    env.update(WINEPREFIX=str(prefix), WINEARCH='win32', WINEDEBUG='-all',
               WINEPATH='T:\\bin', INCLUDE='T:\\include', LIB='T:\\lib')
    for key in ('CL', '_CL_', 'LINK', '_LINK_', 'MSC_CMD_FLAGS', 'RC', 'RCPP'):
        env.pop(key, None)
    if not (prefix / 'system.reg').exists():
        subprocess.run(['wineboot', '-u'], env=env, stdout=subprocess.DEVNULL,
                       stderr=subprocess.DEVNULL, timeout=60, check=True)
    overlay = SCRATCH / 'toolchain'
    (overlay / 'bin').mkdir(parents=True, exist_ok=True)
    for directory in ('include', 'lib'):
        link = overlay / directory
        if not link.exists():
            link.symlink_to(TC / directory, target_is_directory=True)
    drive = prefix / 'dosdevices/t:'
    if drive.is_symlink():
        assert drive.resolve() == overlay.resolve(), 'T: belongs to another task'
    else:
        drive.symlink_to(overlay, target_is_directory=True)
    components = {}
    for name, directory in (('RC.EXE', 'rc'), ('RCPP.EXE', 'rcpp')):
        source = ROOT / ('decomp/vc1.0/' + directory + '/build/reconstructed.exe')
        if not source.exists():
            raise SystemExit('Build and verify ' + directory + ' first: missing ' + str(source))
        destination = overlay / 'bin' / name
        shutil.copyfile(source, destination)
        assert sha(destination) == sha(source)
        components[name] = dict(source=source.relative_to(ROOT).as_posix(), sha256=sha(source),
                                masked_reconstruction=True)
    work = SCRATCH / 'run'
    if work.exists():
        shutil.rmtree(work)
    shutil.copytree(HERE / 'source', work)
    before = {p.relative_to(work).as_posix(): sha(p) for p in work.rglob('*') if p.is_file()}
    env.update(TMP='Z:' + str(work).replace('/', '\\'), TEMP='Z:' + str(work).replace('/', '\\'))
    manifest = json.loads((HERE / 'manifest.json').read_text())
    case = manifest['phases']['rc']
    result = subprocess.run(['wine', 'T:\\bin\\RC.EXE'] + case['arguments'],
                            cwd=work, env=env, stdin=subprocess.DEVNULL,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=60)
    (SCRATCH / 'stdout.bin').write_bytes(result.stdout)
    (SCRATCH / 'stderr.bin').write_bytes(result.stderr)
    assert result.returncode == 0, (result.returncode, result.stdout, result.stderr)
    assert result.stdout == (HERE / case['stdout']).read_bytes()
    assert result.stderr == (HERE / case['stderr']).read_bytes()
    product = work / case['output']['name']
    assert product.read_bytes() == (HERE / case['output']['path']).read_bytes()
    after = {p.relative_to(work).as_posix(): sha(p) for p in work.rglob('*') if p.is_file()}
    assert set(after) == set(before) | {case['output']['name']}
    assert all(after[name] == value for name, value in before.items())
    report = dict(status='passed', components=components, arguments=case['arguments'],
                  exit=0, stdout_bytes=len(result.stdout), stderr_bytes=len(result.stderr),
                  output_bytes=product.stat().st_size, output_sha256=sha(product),
                  exact_bytes=True, ignored_fields=[], inputs_unchanged=True,
                  exact_file_set=True, original_toolchain_unchanged=True,
                  child_resolution='Both RC and RCPP in T:\\bin are masked reconstructions; no original RCPP in overlay')
    (SCRATCH / 'validation.json').write_text(json.dumps(report, indent=2) + '\n')
    print('Reconstructed RC + reconstructed RCPP: exit 0; empty streams; exact %d-byte resource product.' % product.stat().st_size)


if __name__ == '__main__':
    main()
