#!/usr/bin/env python3
"""Build native i386 adoption probes against the current shared sources.

No historical executable, Wine process, reference capture, or reference refresh
is part of this runner. Its receipt pins all shared code used by the checks.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time


ROOT = Path(__file__).resolve().parents[1]


def fingerprint(sources):
    paths = set(sources + [ROOT / 'src/trace.c', ROOT / 'spawn.py'])
    for directory in ('include', 'native'):
        paths.update(path for path in (ROOT / directory).rglob('*')
                     if path.suffix in ('.h', '.c', '.py'))
    return {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
            for path in sorted(paths)}


def test_fingerprint():
    paths = {ROOT / 'Makefile'}
    paths.update(path for path in (ROOT / 'tests').rglob('*')
                 if path.suffix in ('.h', '.c', '.py'))
    return {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
            for path in sorted(paths)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('sources', nargs='+', type=Path,
                        help='current shared final-link sources; use make adoption-check')
    arguments = parser.parse_args()
    setup_start = time.perf_counter()
    sources = [(ROOT / path).resolve() if not path.is_absolute() else path.resolve()
               for path in arguments.sources]
    scratch = Path.home() / '.ai-tmp/recon-adoption'
    scratch.mkdir(parents=True, exist_ok=True)
    directory = Path(tempfile.mkdtemp(prefix='checks-', dir=scratch))
    environment = {key: value for key, value in os.environ.items()
                   if not key.startswith('RECON_')}
    environment['LC_ALL'] = 'C'
    before = fingerprint(sources)
    test_before = test_fingerprint()
    test_changes = {}
    records = []
    totals = dict(build=0.0, setup=time.perf_counter() - setup_start,
                  execution=0.0, checks=0.0)
    started = time.perf_counter()
    compiler_versions = {}
    receipt_path = directory / 'receipt.json'

    def invoke(name, command, phase='execution', env=None, pass_fds=(),
               expected_stdout=None, expected_stderr=None, expected_code=0):
        started_call = time.perf_counter()
        process = subprocess.run(command, cwd=ROOT, env=environment if env is None else env,
                                 pass_fds=pass_fds, capture_output=True)
        elapsed = time.perf_counter() - started_call
        totals[phase] += elapsed
        stdout = directory / (name + '.stdout')
        stderr = directory / (name + '.stderr')
        stdout.write_bytes(process.stdout)
        stderr.write_bytes(process.stderr)
        record = dict(check=name, phase=phase, command=[str(part) for part in command],
                      seconds=elapsed, status='passed', exit_status=process.returncode,
                      stdout=str(stdout), stderr=str(stderr))
        records.append(record)
        checking = time.perf_counter()
        try:
            assert process.returncode == expected_code, (
                name, process.returncode, process.stderr.decode(errors='replace'), str(stderr))
            if expected_stdout is not None:
                assert process.stdout == expected_stdout, (name, process.stdout, expected_stdout)
            if expected_stderr is not None:
                assert process.stderr == expected_stderr, (name, process.stderr, expected_stderr)
        except Exception:
            record['status'] = 'failed'
            raise
        finally:
            totals['checks'] += time.perf_counter() - checking
        return process

    def build(compiler, optimization, source, name, extra=()):
        executable = directory / name
        command = [compiler, '-m32', '-std=gnu89', '-O' + str(optimization), '-g',
                   '-Wall', '-Wextra', '-DRECON_NATIVE_TEST',
                   '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'native'),
                   str(ROOT / source), *map(str, sources), *map(str, extra), '-o', str(executable)]
        invoke(name + '-build', command, phase='build')
        return executable

    def naked_run(executable, name, mode):
        setting_up = time.perf_counter()
        trace = directory / (name + '.trace')
        names = directory / (name + '.filter')
        seen = directory / (name + '.seen')
        names.write_text('naked_store_extended\n')
        seen.write_text('naked_store_extended\n' if mode == 'accepted-prefix' else '')
        env = dict(environment, RECON_TRACE_FILTER=str(names), RECON_TRACE_SEEN=str(seen))
        if mode == 'disabled':
            env.update(RECON_TRACE_FILTER=str(directory / 'missing-filter'),
                       RECON_TRACE_SEEN=str(directory / 'missing-prefix'))
        totals['setup'] += time.perf_counter() - setting_up
        with trace.open('wb') as stream:
            if mode != 'disabled':
                env['RECON_TRACE_FD'] = str(stream.fileno())
            invoke(name, [str(executable)], env=env, pass_fds=(stream.fileno(),),
                   expected_stdout=b'naked ABI preserved\r\n', expected_stderr=b'')
        checking = time.perf_counter()
        actual = trace.read_text().splitlines()
        expected = ['naked_store_extended'] if mode == 'first-entry' else []
        assert actual == expected, (name, actual, expected)
        records[-1]['entries'] = actual
        totals['checks'] += time.perf_counter() - checking

    status = 'passed'
    error = None
    try:
        for compiler in ('gcc', 'clang'):
            assert shutil.which(compiler), compiler + ' is required for adoption checks'
            compiler_versions[compiler] = subprocess.check_output(
                [compiler, '--version'], text=True, env=environment).splitlines()[0]
            for optimization in (0, 2):
                for source, label in (('tests/context-target.c', 'context'),
                                      ('tests/decimal-target.c', 'decimal'),
                                      ('tests/check-win32-adoption.c', 'win32')):
                    name = f'{compiler}-O{optimization}-{label}'
                    executable = build(compiler, optimization, source, name)
                    invoke(name, [str(executable)], expected_stdout=b'', expected_stderr=b'')
            # GCC O0 PIE emits a GOT thunk even before a naked function's first
            # asm statement, clobbering EAX and flags. GCC O2 omits that thunk;
            # Clang, used by reconstructed private-ABI entries, supports both.
            configurations = [(0, False), (2, False), (2, True)]
            if compiler == 'clang':
                configurations.insert(1, (0, True))
            for optimization, pie in configurations:
                name = f'{compiler}-O{optimization}-naked-' + ('pie' if pie else 'exec')
                flags = ['-DRECON_NATIVE_TRACE', str(ROOT / 'src/trace.c')]
                flags += ['-fPIE', '-pie'] if pie else ['-fno-pie', '-no-pie']
                executable = build(compiler, optimization, 'tests/naked-target.c', name, flags)
                for mode in ('first-entry', 'accepted-prefix', 'disabled'):
                    naked_run(executable, name + '-' + mode, mode)

        # Build the established suites once and execute each once. Building is
        # measured separately from running their existing validation scripts.
        invoke('existing-suites-build', ['make', '-j4', 'build/check', 'build/trace-pie',
                                        'build/trace-exec', 'build/process-target'], phase='build')
        invoke('existing-crt', [str(ROOT / 'build/check')])
        invoke('existing-trace', [sys.executable, str(ROOT / 'tests/check-trace.py'),
                                 str(ROOT / 'build/trace-pie'), str(ROOT / 'build/trace-exec')])
        invoke('existing-process', [sys.executable, str(ROOT / 'tests/check-process.py'),
                                   str(ROOT / 'build/process-target')])
        checking = time.perf_counter()
        after = fingerprint(sources)
        assert after == before, 'shared production source/header files changed during adoption checks'
        test_after = test_fingerprint()
        test_changes = {name: dict(before=test_before.get(name), after=test_after.get(name))
                        for name in sorted(test_before.keys() | test_after.keys())
                        if test_before.get(name) != test_after.get(name)}
        executed_tests = ('context-target.c', 'decimal-target.c', 'check-win32-adoption.c',
                          'naked-target.c', 'check.c', 'trace-target.c', 'check-trace.py',
                          'process-target.c', 'check-process.py')
        assert not any('tests/' + name in test_changes for name in executed_tests), (
            'executed test sources changed during adoption checks', test_changes)
        totals['checks'] += time.perf_counter() - checking
    except Exception as failure:
        status = 'failed'
        error = repr(failure)
        raise
    finally:
        receipt = dict(status=status, scope='native i386 shared adoption checks; no Wine/original execution',
                       compilers=compiler_versions, source_sha256=before, checks=records,
                       test_sha256=test_before, test_changes_during_run=test_changes,
                       measured_seconds=totals, elapsed_seconds=time.perf_counter() - started,
                       limitation='GCC O0 PIE inserts a register/flags-clobbering naked-function GOT thunk',
                       receipt=str(receipt_path))
        if error is not None:
            receipt['error'] = error
        receipt_path.write_text(json.dumps(receipt, indent=2) + '\n')
        print(json.dumps({key: receipt[key] for key in ('status', 'measured_seconds', 'elapsed_seconds', 'receipt')}
                         | dict(checks=len(records))))


if __name__ == '__main__':
    main()
