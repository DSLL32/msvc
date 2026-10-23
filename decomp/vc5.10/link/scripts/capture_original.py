#!/usr/bin/env python3
"""Explicit, unmasked original-only groundwork probes; never a fixture runner."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture', action='store_true', required=True,
                        help='explicitly permit Wine original probes')
    args = parser.parse_args()
    binary = Path(__file__).resolve().parents[1]
    repo = binary.parents[2]
    prefix = Path.home() / '.ai-tmp/vc1.0-capture/wine'
    if not prefix.is_dir():
        raise RuntimeError('shared prefix absent; refusing to create it')
    for drive, target in [('r', repo), ('s', Path.home() / '.ai-tmp/msvc2')]:
        mapping = prefix / 'dosdevices' / (drive + ':')
        if not mapping.is_symlink() or mapping.resolve() != target.resolve():
            raise RuntimeError('unexpected shared drive mapping: ' + drive)
    scratch = Path.home() / '.ai-tmp/msvc2/vc5.10/link' / time.strftime('%Y%m%d-%H%M%S', time.gmtime())
    scratch.mkdir(parents=True, exist_ok=False)
    env = os.environ.copy()
    env.update(WINEPREFIX=str(prefix),
               WINEDEBUG='+loaddll,+process', WINEPATH=r'R:\toolchains\5.10\bin',
               WINEDLLOVERRIDES='msvcrt,mspdb50,msdis100,imagehlp=n',
               INCLUDE=r'R:\toolchains\5.10\include', LIB=r'R:\toolchains\5.10\lib')
    for name in ('CL', '_CL_', 'LINK', '_LINK_'):
        env.pop(name, None)
    records = []

    def run(label, argv, work):
        work.mkdir(exist_ok=True)
        start = time.monotonic()
        try:
            proc = subprocess.run(['wine'] + argv, cwd=work, env=env,
                                  stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=45)
            status, stdout, stderr = proc.returncode, proc.stdout, proc.stderr
        except subprocess.TimeoutExpired as error:
            status, stdout, stderr = 'timeout', error.stdout or b'', error.stderr or b''
        elapsed = time.monotonic() - start
        (scratch / (label + '.stdout')).write_bytes(stdout)
        (scratch / (label + '.stderr')).write_bytes(stderr)
        records.append(dict(label=label, argv=argv, cwd=str(work), status=status,
                            elapsed_seconds=elapsed, stdout_size=len(stdout),
                            stdout_sha256=hashlib.sha256(stdout).hexdigest(),
                            stderr_size=len(stderr),
                            stderr_sha256=hashlib.sha256(stderr).hexdigest(),
                            artifacts={p.name: dict(size=p.stat().st_size,
                              sha256=hashlib.sha256(p.read_bytes()).hexdigest())
                              for p in work.iterdir() if p.is_file()}))
        (scratch / 'receipt.json').write_text(json.dumps(dict(kind='unmasked-original-groundwork',
            reconstruction_started=False, scratch=str(scratch),
            environment={k:env[k] for k in ('WINEPREFIX','WINEDEBUG','WINEPATH','WINEDLLOVERRIDES','INCLUDE','LIB')},
            records=records), indent=2) + '\n')
        print(label, status, '%.3fs' % elapsed, flush=True)
        if status == 'timeout':
            raise RuntimeError('original probe timed out; receipt retained')
        return status

    link = r'R:\toolchains\5.10\bin\LINK.EXE'
    cl = r'R:\toolchains\5.10\bin\CL.EXE'
    run('help', [link, '/?'], scratch / 'help')
    for case, source in [('hello-win32', 'hello.c'), ('hello-dll', 'hello-dll.c'),
                         ('hello-debug-dll', 'hello-debug.c')]:
        work = scratch / case
        work.mkdir()
        shutil.copyfile(binary / 'cases' / case / 'source' / source, work / source)
        flags = ['/nologo', '/c', '/O2', '/G5']
        if case == 'hello-debug-dll':
            flags += ['/Z7']
        status = run(case + '-compile', [cl] + flags + [source], work)
        if status:
            continue
        obj = Path(source).with_suffix('.obj').name
        options = ['/nologo', '/incremental:no', '/out:hello.exe' if case == 'hello-win32' else '/out:hello.dll']
        if case != 'hello-win32':
            options += ['/dll']
        run(case + '-link', [link] + options + [obj], work)
        if case == 'hello-debug-dll':
            run('debug-cv-no-pdb', [link] + options + ['/debug', '/debugtype:cv', '/pdb:none', obj], work)
            run('debug-pdb', [link] + options + ['/debug', '/pdb:hello.pdb', obj], work)
    print(scratch / 'receipt.json')


if __name__ == '__main__':
    main()
