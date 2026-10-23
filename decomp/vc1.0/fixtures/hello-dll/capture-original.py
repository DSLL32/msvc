#!/usr/bin/env python3
"""Replay the original hello DLL phases and exercise the exported GUI entry."""
import argparse
import fcntl
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time

sys.dont_write_bytecode = True
from artifacts import normalized, pe_info, timestamp_fields

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
TC = ROOT / 'toolchains/1.0-nt'
PREFIX = Path.home() / '.ai-tmp/vc1.0-capture/wine'
SHARED_SCRATCH = Path.home() / '.ai-tmp/msvc2'
SCRATCH = SHARED_SCRATCH / 'fixtures/hello-dll'


def windows(path):
    path = Path(path).resolve()
    for drive, root in (('R:', ROOT), ('S:', SHARED_SCRATCH)):
        if path.is_relative_to(root):
            return drive + '\\' + str(path.relative_to(root)).replace('/', '\\')
    raise ValueError('path is outside shared R:/S: mappings: ' + str(path))


def digest(data):
    return hashlib.sha256(data).hexdigest()


def invoke(arguments, env, cwd):
    return subprocess.run(['wine', *map(str, arguments)], env=env, cwd=cwd,
                          stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE, timeout=120)


def build_helper(source, output, env, libraries=()):
    obj = output.with_suffix('.obj')
    commands = ([windows(TC / 'bin/CL.EXE'), '/nologo', '/c', '/Od', '/G3',
                 '/Fo' + windows(obj), windows(source)],
                [windows(TC / 'bin/LINK.EXE'), '/nologo', '/subsystem:console',
                 '/out:' + windows(output), windows(obj),
                 *[windows(TC / 'lib' / name) for name in libraries]])
    for command in commands:
        result = invoke(command, env, output.parent)
        assert result.returncode == 0, (result.stdout, result.stderr)


def check_gui(work, tools, env, report):
    shutil.copyfile(tools / 'dll-host.exe', work / 'dll-host.exe')
    report.unlink(missing_ok=True)
    result = subprocess.run(
        ['xvfb-run', '-a', 'wine', windows(tools / 'gui-check.exe'),
         windows(work / 'dll-host.exe'), windows(report.with_suffix('.bmp')),
         windows(report)], cwd=work, env=env, stdin=subprocess.DEVNULL,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=60)
    assert result.returncode == 0, (result.returncode, result.stdout, result.stderr)
    receipt = json.loads(report.read_text())
    assert receipt['exit_status'] == 0 and receipt['dark_pixels'] >= 20
    return receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('phase', nargs='?', choices=('all', 'c1', 'c2', 'link'), default='all')
    parser.add_argument('--receipt', type=Path, default=SCRATCH / 'verification.json')
    args = parser.parse_args()
    start = time.perf_counter()
    assert (PREFIX / 'system.reg').is_file(), 'shared repo Wine prefix is not initialized'
    assert (PREFIX / 'dosdevices/r:').resolve() == ROOT
    assert (PREFIX / 'dosdevices/s:').resolve() == SHARED_SCRATCH
    SCRATCH.mkdir(parents=True, exist_ok=True)
    capture_lock = (SCRATCH / 'capture.lock').open('a')
    fcntl.flock(capture_lock, fcntl.LOCK_EX)
    build_start = time.perf_counter()
    manifest = json.loads((HERE / 'manifest.json').read_text())
    env = os.environ.copy()
    env.update(WINEPREFIX=str(PREFIX), WINEARCH='win32', WINEDEBUG='-all',
               INCLUDE=windows(TC / 'include'), LIB=windows(TC / 'lib'),
               WINEPATH=windows(TC / 'bin'))
    for key in ('CL', '_CL_', 'LINK', '_LINK_', 'MSC_CMD_FLAGS'):
        env.pop(key, None)
    tools = SCRATCH / 'verify-tools'
    tools.mkdir(parents=True, exist_ok=True)
    build_helper(HERE.parent / 'minimal-c/run-phase.c', tools / 'run-phase.exe', env)
    if args.phase in ('all', 'link'):
        build_helper(HERE / 'dll-host.c', tools / 'dll-host.exe', env, ('USER32.LIB',))
        build_helper(ROOT / 'decomp/vc1.0/cl/src/gui-check.c', tools / 'gui-check.exe',
                     env, ('USER32.LIB', 'GDI32.LIB'))
    receipt = dict(case=manifest['case'], phases={}, link_repetitions=2, prefix=str(PREFIX),
        seconds=dict(build=time.perf_counter()-build_start))
    for name, phase in manifest['phases'].items():
        if args.phase not in ('all', name):
            continue
        target = ROOT / phase['target']
        assert digest(target.read_bytes()) == phase['sha256']
        for dependency in phase.get('toolchain_dependencies', []):
            assert digest((ROOT / dependency['path']).read_bytes()) == dependency['sha256']
        repetitions = []
        for repetition in range(2 if name == 'link' else 1):
            phase_start = time.perf_counter()
            work = SCRATCH / ('verify-%s-%d' % (name, repetition))
            if work.exists():
                shutil.rmtree(work)
            work.mkdir()
            for entry in phase['inputs']:
                data = (HERE / entry['path']).read_bytes()
                assert len(data) == entry['bytes'] and digest(data) == entry['sha256']
                (work / Path(entry['path']).name).write_bytes(data)
            phase_env = env.copy()
            phase_env.update({key: value.format(bin=windows(TC / 'bin'),
                                               include=windows(TC / 'include'),
                                               lib=windows(TC / 'lib'))
                              for key, value in phase['environment'].items()})
            command = [windows(tools / 'run-phase.exe'), windows(target)]
            if phase['arguments']:
                command.append(' '.join(phase['arguments']))
            setup_seconds = time.perf_counter()-phase_start
            execution_start = time.perf_counter()
            result = invoke(command, phase_env, work)
            execution_seconds = time.perf_counter()-execution_start
            checks_start = time.perf_counter()
            assert result.returncode == phase['expected_exit'], (name, result.returncode, result.stdout, result.stderr)
            assert result.stdout == phase['expected_stdout'].encode('latin1'), (name, result.stdout)
            assert result.stderr == phase['expected_stderr'].encode('latin1'), (name, result.stderr)
            outputs = []
            for entry in phase['outputs']:
                data = (work / entry['output_name']).read_bytes()
                expected = (HERE / entry['path']).read_bytes()
                assert len(data) == entry['bytes'] and digest(expected) == entry['sha256']
                fields = entry.get('ignored_fields', [])
                if 'artifact_kind' in entry:
                    assert timestamp_fields(data, entry['artifact_kind']) == fields
                assert normalized(data, fields) == normalized(expected, fields), (name, entry['output_name'])
                outputs.append(dict(name=entry['output_name'], bytes=len(data),
                                    sha256=digest(data), normalized_sha256=digest(normalized(data, fields)),
                                    differing_bytes=[i for i, pair in enumerate(zip(data, expected)) if pair[0] != pair[1]]))
            expected_files = {Path(entry['path']).name for entry in phase['inputs']}
            expected_files.update(entry['output_name'] for entry in phase['outputs'])
            assert {path.name for path in work.iterdir()} == expected_files
            item = dict(exit=result.returncode, stdout_bytes=len(result.stdout),
                        stderr_bytes=len(result.stderr), outputs=outputs)
            if name == 'link':
                info = pe_info((work / 'hello.dll').read_bytes())
                expected_info = pe_info((HERE / 'expected/hello.dll').read_bytes())
                assert info == expected_info and info['dll'] and info['entry_rva']
                assert info['exports'] == [dict(name='_HelloMain@16', ordinal=1, rva=4240)]
                item['pe'] = info
                item['gui'] = check_gui(work, tools, env,
                                       SCRATCH / ('verify-window-%d.json' % repetition))
            for entry in phase['inputs']:
                assert digest((work / Path(entry['path']).name).read_bytes()) == entry['sha256']
            item['seconds'] = dict(setup=setup_seconds, execution=execution_seconds,
                checks=time.perf_counter()-checks_start, total=time.perf_counter()-phase_start)
            repetitions.append(item)
            print('%s[%d]: exit 0, streams and all outputs match%s' %
                  (name, repetition, '; DLL loaded/called/painted/closed/unloaded' if name == 'link' else ''), flush=True)
        receipt['phases'][name] = repetitions
    receipt['seconds']['total'] = time.perf_counter()-start
    args.receipt.parent.mkdir(parents=True, exist_ok=True)
    args.receipt.write_text(json.dumps(receipt, indent=2) + '\n')

    print(json.dumps(receipt['seconds']))


if __name__ == '__main__':
    main()
