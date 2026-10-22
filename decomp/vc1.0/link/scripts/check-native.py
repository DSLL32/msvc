#!/usr/bin/env python3
"""Run the Linux reconstruction against captured references, without Wine.

Only this ELF executable is launched. Fixture modules loaded below are pure byte
parsers; no original executable, reference builder or generated PE is run.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time

BINARY = Path(__file__).resolve().parents[1]
ROOT = BINARY.parents[2]
FIXTURES = BINARY.parent / 'fixtures'
CASES = {
    'help': None,
    'library-create': ('link-library-tools', 'create'),
    'library-list': ('link-library-tools', 'list'),
    'library-extract': ('link-library-tools', 'extract'),
    'library-remove': ('link-library-tools', 'remove'),
    'library-list-trimmed': ('link-library-tools', 'list-trimmed'),
    'dump-library': ('link-library-tools', 'dump-library'),
    'dump-object': ('link-library-tools', 'dump-object'),
    'dump-dll': ('link-library-tools', 'dump-dll'),
    'dump-edited': ('link-library-tools', 'dump-edited'),
    'edit': ('link-library-tools', 'edit'),
    'omf-library': ('link-omf', 'library'),
    'omf-link': ('link-omf', 'link'),
    'cpp-undecorate': ('link-undecorate', 'dump'),
    'minimal-c': ('minimal-c', 'link'),
}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def captured(path, record):
    data = path.read_bytes()  # Missing references deliberately fail.
    if 'bytes' in record:
        assert len(data) == record['bytes'], str(path) + ': captured size'
    assert sha(data) == record['sha256'], str(path) + ': captured hash'
    return data


def parser_module():
    path = FIXTURES / 'link-omf/artifacts.py'
    spec = importlib.util.spec_from_file_location('native_artifacts', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def validate_output(work, fixture, record, parser):
    reference = captured(fixture / record['path'], record)
    actual = (work / record['output_name']).read_bytes()
    kind = record.get('artifact_kind', 'plain-pe')
    expected_fields = record.get('ignored_fields', [])
    assert parser.timestamp_fields(reference, kind) == expected_fields
    assert parser.timestamp_fields(actual, kind) == expected_fields
    expected = parser.normalize_artifact(reference, kind)
    normalized = parser.normalize_artifact(actual, kind)
    assert sha(expected) == record.get('normalized_sha256', record['sha256'])
    assert normalized == expected, record['output_name'] + ': artifact differs'
    return dict(name=record['output_name'], bytes=len(actual), sha256=sha(actual),
                normalized_sha256=sha(normalized), ignored_fields=expected_fields)


def run_case(name, executable, directory, parser, state):
    start = time.perf_counter()
    work = directory / name
    work.mkdir()
    env = {key:value for key,value in os.environ.items()
           if not key.startswith('RECON_')}
    env['RECON_PROGRAM_PATH'] = str(work / 'LINK.EXE')
    for key in ('CL', '_CL_', 'LINK', '_LINK_', 'MSC_CMD_FLAGS', 'LINK32_DB'):
        env.pop(key, None)
    # A private, empty temporary directory prevents accidental host inputs.
    temporary = work / 'tmp'
    temporary.mkdir()
    env['TMP'] = env['TEMP'] = env['TMPDIR'] = str(temporary)
    # Original captures use the NT CRT Pacific timezone (including DST).
    env['TZ'] = 'PST8PDT'
    env['LC_ALL'] = 'C'
    if name == 'help':
        metadata_path = BINARY / 'cases/help.json'
        metadata = json.loads(metadata_path.read_text())
        args, expected_exit = ['/?'], metadata['exit_status']
        fixture, phase = None, None
    else:
        fixture_name, phase_name = CASES[name]
        fixture = FIXTURES / fixture_name
        metadata_path = fixture / 'manifest.json'
        metadata = json.loads(metadata_path.read_text())
        phase = metadata['phases'][phase_name]
        args, expected_exit = phase['arguments'], phase['expected_exit']
        for key in phase.get('unset_environment', []):
            env.pop(key, None)
        for key, value in phase.get('environment', {}).items():
            env[key] = value.replace('{lib}', str(ROOT / 'toolchains/1.0-nt/lib'))
        for item in phase['inputs']:
            data = captured(fixture / item['path'], item)
            target = work / Path(item['path']).name
            assert not target.exists(), 'duplicate input basename'
            target.write_bytes(data)
        for item in phase.get('toolchain_dependencies', []):
            captured(ROOT / item['path'], item)
        # Verify all output references before running anything.
        for item in phase.get('outputs', []):
            captured(fixture / item['path'], item)
    if name == 'edit':
        catalog = json.loads((BINARY / 'cases/library-edit.json').read_text())['catalog']
        (work / 'link.err').write_bytes(captured(ROOT / catalog['path'], catalog))
    setup = time.perf_counter() - start
    state['setup_seconds'] = setup
    launch = time.perf_counter()
    result = subprocess.run([str(executable), *args], cwd=work, env=env,
                            stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, timeout=30)
    run_seconds = time.perf_counter() - launch
    (work / 'stdout.bin').write_bytes(result.stdout)
    (work / 'stderr.bin').write_bytes(result.stderr)
    state.update(exit=result.returncode, execution_seconds=run_seconds,
                 stdout_bytes=len(result.stdout), stdout_sha256=sha(result.stdout),
                 stderr_bytes=len(result.stderr), stderr_sha256=sha(result.stderr))
    check_start = time.perf_counter()
    assert result.returncode == expected_exit, f'exit {result.returncode}, expected {expected_exit}'
    if phase is None:
        for key, data in [('stdout', result.stdout), ('stderr', result.stderr)]:
            assert len(data) == metadata['streams'][key]['bytes'], key + ': length'
            assert sha(data) == metadata['streams'][key]['sha256'], key + ': hash'
        outputs = []
    else:
        assert result.stdout == phase['expected_stdout'].encode('latin1'), 'stdout differs'
        assert result.stderr == phase['expected_stderr'].encode('latin1'), 'stderr differs'
        outputs = [validate_output(work, fixture, item, parser)
                   for item in phase.get('outputs', [])]
    return dict(case=name, status='pass', exit=result.returncode,
                manifest=str(metadata_path.relative_to(ROOT)),
                manifest_sha256=sha(metadata_path.read_bytes()),
                stdout_bytes=len(result.stdout), stdout_sha256=sha(result.stdout),
                stderr_bytes=len(result.stderr), stderr_sha256=sha(result.stderr),
                outputs=outputs, setup_seconds=setup, execution_seconds=run_seconds,
                oracle_seconds=time.perf_counter()-check_start,
                total_seconds=time.perf_counter()-start)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--case', action='append', choices=CASES)
    ap.add_argument('--exe', type=Path, default=BINARY / 'build/native/LINK.EXE')
    ap.add_argument('--directory', type=Path)
    options = ap.parse_args()
    executable = options.exe.resolve()
    assert executable.read_bytes()[:4] == b'\x7fELF', 'native checker requires ELF'
    directory = options.directory
    if directory is None:
        scratch = Path.home() / '.ai-tmp/vc1.0-link'
        scratch.mkdir(parents=True, exist_ok=True)
        directory = Path(tempfile.mkdtemp(prefix='native-check-', dir=scratch))
    else:
        directory.mkdir(parents=True, exist_ok=False)
    parser = parser_module()
    receipt = dict(mode='captured-reference-only', executable=str(executable),
                   executable_sha256=sha(executable.read_bytes()), directory=str(directory),
                   build_seconds='unmeasured; build is a separate make target', cases=[])
    start = time.perf_counter()
    for name in options.case or CASES:
        case_start = time.perf_counter()
        result = dict(case=name)
        try:
            result.update(run_case(name, executable, directory, parser, result))
        except Exception as error:
            result.update(status='fail', error=str(error),
                          total_seconds=time.perf_counter()-case_start)
        receipt['cases'].append(result)
        print(f"{name}: {result['status']} ({result['total_seconds']:.3f}s)", flush=True)
    receipt['total_seconds'] = time.perf_counter()-start
    receipt['status'] = 'pass' if all(x['status']=='pass' for x in receipt['cases']) else 'fail'
    (directory / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print('receipt:', directory / 'receipt.json')
    return 0 if receipt['status']=='pass' else 1


if __name__ == '__main__':
    sys.exit(main())
