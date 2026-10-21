#!/usr/bin/env python3
"""Run captured compiler/linker cases independently with the original toolchain."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
TC = ROOT / 'toolchains/1.0-nt'
SCRATCH = Path.home() / '.ai-tmp/vc1.0-c1xx/replay'


def windows(path):
    return 'Z:' + str(Path(path).resolve()).replace('/', '\\')


def invoke(arguments, env, **kwargs):
    return subprocess.run(['wine'] + list(map(str, arguments)), env=env,
                          stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE, timeout=90, **kwargs)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('phase', nargs='?', choices=['c1', 'c2', 'link', 'all'], default='all')
    parser.add_argument('--prefix', type=Path, default=SCRATCH / 'wine')
    parser.add_argument('--fixture', type=Path, default=HERE)
    args = parser.parse_args()
    fixture = args.fixture.resolve()
    manifest = json.loads((fixture / 'manifest.json').read_text())
    case_scratch = SCRATCH / fixture.name
    case_scratch.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    env.update(WINEPREFIX=str(args.prefix.resolve()), WINEARCH='win32', WINEDEBUG='-all',
               INCLUDE=windows(TC / 'include'), LIB=windows(TC / 'lib'), WINEPATH=windows(TC / 'bin'))
    env.pop('CL', None)
    env.pop('_CL_', None)
    env.pop('LINK', None)
    env.pop('_LINK_', None)
    env.pop('MSC_CMD_FLAGS', None)
    SCRATCH.mkdir(parents=True, exist_ok=True)
    if not (args.prefix / 'system.reg').exists():
        args.prefix.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run(['wineboot', '-u'], env=env, stdin=subprocess.DEVNULL,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
    launcher = SCRATCH / 'run-phase.exe'
    obj = SCRATCH / 'run-phase.obj'
    for command in ([TC / 'bin/CL.EXE', '/nologo', '/c', '/Od', '/G3', '/Fo'+windows(obj), windows(HERE/'run-phase.c')],
                    [TC / 'bin/LINK.EXE', '/nologo', '/subsystem:console', '/out:'+windows(launcher), windows(obj)]):
        result = invoke(command, env, cwd=SCRATCH)
        if result.returncode:
            raise RuntimeError(result.stdout.decode('latin1') + result.stderr.decode('latin1'))
    gui_checker = None
    if any(case.get('generated_program', {}).get('kind') == 'win32-gui'
           for name, case in manifest['phases'].items() if args.phase in ('all', name)):
        gui_checker = SCRATCH / 'gui-check.exe'
        gui_obj = SCRATCH / 'gui-check.obj'
        for command in ([TC / 'bin/CL.EXE', '/nologo', '/c', '/Od', '/G3', '/Fo'+windows(gui_obj), windows(ROOT/'decomp/vc1.0/cl/gui-check.c')],
                        [TC / 'bin/LINK.EXE', '/nologo', '/subsystem:console', '/out:'+windows(gui_checker), windows(gui_obj), windows(TC/'lib/USER32.LIB'), windows(TC/'lib/GDI32.LIB')]):
            result = invoke(command, env, cwd=SCRATCH)
            if result.returncode:
                raise RuntimeError(result.stdout.decode('latin1') + result.stderr.decode('latin1'))
    for name, case in manifest['phases'].items():
        if args.phase not in ('all', name):
            continue
        target = ROOT / case['target']
        assert hashlib.sha256(target.read_bytes()).hexdigest() == case['sha256'], 'target changed'
        for entry in case.get('toolchain_dependencies', []):
            assert hashlib.sha256((ROOT / entry['path']).read_bytes()).hexdigest() == entry['sha256'], 'toolchain dependency changed'
        work = case_scratch / ('verify-' + name)
        if work.exists():
            shutil.rmtree(work)
        work.mkdir()
        for entry in case['inputs']:
            source = fixture / entry['path']
            assert hashlib.sha256(source.read_bytes()).hexdigest() == entry['sha256'], 'fixture changed'
            shutil.copyfile(source, work / source.name)
        phase_env = env.copy()
        phase_env.update({key: value.format(bin=windows(TC/'bin'), include=windows(TC/'include'),
                                           lib=windows(TC/'lib'))
                          for key, value in case['environment'].items()})
        arguments = [launcher, windows(target)]
        arguments.append(' '.join(case['arguments']))
        arguments.append(case['command_line_template'].format(bin=windows(TC/'bin')))
        result = invoke(arguments, phase_env, cwd=work)
        assert result.returncode == case['expected_exit'], (name, result.returncode)
        assert result.stdout == case['expected_stdout'].encode(), (name, result.stdout)
        assert result.stderr == case['expected_stderr'].encode(), (name, result.stderr)
        for entry in case['outputs']:
            actual = bytearray((work / entry['output_name']).read_bytes())
            expected = bytearray((fixture / entry['path']).read_bytes())
            assert hashlib.sha256(expected).hexdigest() == entry['sha256'], 'expected fixture changed'
            for field in entry.get('ignored_fields', []):
                start, length = field['offset'], field['bytes']
                actual[start:start+length] = expected[start:start+length] = b'\0' * length
            assert actual == expected, (name, entry['output_name'])
        expected_files = {Path(e['path']).name for e in case['inputs']} | {e['output_name'] for e in case['outputs']}
        assert {p.name for p in work.iterdir()} == expected_files, 'unexpected phase products'
        if 'generated_program' in case:
            generated = case['generated_program']
            executable = work / generated.get('path', 'test.exe')
            if generated.get('kind') == 'win32-gui':
                report = case_scratch / 'gui-window.json'
                report.unlink(missing_ok=True)
                subprocess.run(['xvfb-run', '-a', 'wine', str(gui_checker), windows(executable),
                                windows(case_scratch/'gui-window.bmp'), windows(report)],
                               env=env, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                               stderr=subprocess.DEVNULL, timeout=45, check=True)
                result = json.loads(report.read_text())
                assert result['exit_status'] == 0 and result['dark_pixels'] >= 20, 'generated window failed'
            else:
                program = invoke([executable], env, cwd=work)
                assert program.returncode == 0 and not program.stdout and not program.stderr, 'generated executable failed'
        print(name + ': exit 0, output streams match, all generated files match' +
              (' (COFF timestamp excluded)' if name != 'c1' else ' byte for byte'))


if __name__ == '__main__':
    main()
