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
    'm68k-debug-swapper': (BINARY/'cases/m68k-debug-swapper','link'),
    'definition-directives': (BINARY/'cases/definition-directives','library'),
    'm68k-minimal': (BINARY/'cases/m68k-minimal','link'),
    'm68k-reloc-loader': (BINARY/'cases/m68k-reloc-loader','link'),
    'dump-disasm-x86-rich': (BINARY/'cases/disassembler-x86-rich', 'dump'),
    'dump-disasm-code': (BINARY/'cases/disassembler-code', 'dump'),
    'dump-disasm-data': (BINARY/'cases/disassembler-data', 'dump'),
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
    'hello-win32': ('hello-win32', 'link'),
    'hello-dll': ('hello-dll', 'link'),
    'hello-debug-dll': ('link-debug-dll', 'link'),
}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def captured(path, record):
    data = path.read_bytes()  # Missing references deliberately fail.
    if 'bytes' in record:
        assert len(data) == record['bytes'], str(path) + ': captured size'
    assert sha(data) == record['sha256'], str(path) + ': captured hash'
    return data


def parser_module(fixture=None,metadata=None):
    if metadata and metadata.get('artifact_parser'):
        record=metadata['artifact_parser']
        path=(BINARY/record['path']).resolve()
        assert path.parent==(BINARY/'scripts').resolve(), 'fixture parser outside binary scripts'
        captured(path,record)
    else:
        path = fixture/'artifacts.py' if fixture is not None else FIXTURES/'link-omf/artifacts.py'
    if not path.exists():
        path = FIXTURES/'link-omf/artifacts.py'
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
    if hasattr(parser,'normalize_artifact'):
        expected = parser.normalize_artifact(reference, kind)
        normalized = parser.normalize_artifact(actual, kind)
    else:
        expected = parser.normalized(reference, expected_fields)
        normalized = parser.normalized(actual, expected_fields)
    assert sha(expected) == record.get('normalized_sha256', record['sha256'])
    assert normalized == expected, record['output_name'] + ': artifact differs'
    return dict(name=record['output_name'], bytes=len(actual), sha256=sha(actual),
                normalized_sha256=sha(normalized), ignored_fields=expected_fields)


def run_case(name, executable, directory, parser, state, tracing=None):
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
        # Cached original help used wine-tool.py's LIB setting. This affects
        # search-path function entry order even though help output is identical.
        env['LIB'] = str(ROOT/'toolchains/1.0-nt/lib')
        fixture, phase = None, None
    else:
        fixture_name, phase_name = CASES[name]
        fixture = FIXTURES / fixture_name
        metadata_path = fixture / 'manifest.json'
        metadata = json.loads(metadata_path.read_text())
        phase = metadata['phases'][phase_name]
        parser = parser_module(fixture,metadata)
        args, expected_exit = phase['arguments'], phase['expected_exit']
        canonical=phase.get('canonical_working_directory')
        if canonical:
            assert canonical.startswith('N:\\') and len(canonical.split('\\'))==2
            root=work
            work=root/canonical.split('\\')[-1]
            work.mkdir()
            env['RECON_DRIVE_N']=str(root)
            env['RECON_CANONICAL_CWD']=canonical
            env['RECON_PROGRAM_PATH']=str(work/'LINK.EXE')
        for key in phase.get('unset_environment', []):
            env.pop(key, None)
        for key, value in phase.get('environment', {}).items():
            env[key] = value.replace('{lib}', str(ROOT / 'toolchains/1.0-nt/lib'))
        for item in phase['inputs']:
            data = captured(fixture / item['path'], item)
            target = work / Path(item['path']).name
            assert not target.exists(), 'duplicate input basename'
            target.write_bytes(data)
        for item in metadata.get('toolchain_dependencies', [])+phase.get('toolchain_dependencies', []):
            captured(ROOT / item['path'], item)
        # Verify all output references before running anything.
        for item in phase.get('outputs', []):
            captured(fixture / item['path'], item)
    if name == 'edit':
        catalog = json.loads((BINARY / 'cases/library-edit.json').read_text())['catalog']
        (work / 'link.err').write_bytes(captured(ROOT / catalog['path'], catalog))
    trace_options = {}
    trace_stream = state_stream = None
    process_fixture = None
    if name in ('hello-win32','hello-dll','hello-debug-dll'):
        canonical = r'Z:\root\msvc2\toolchains\1.0-nt\bin\LINK.EXE'
        env['RECON_PROGRAM_PATH'] = canonical
        env['LIB'] = r'Z:\root\msvc2\toolchains\1.0-nt\lib'
        trace_options['executable'] = str(executable)
        launch_program = canonical
    else:
        launch_program = str(executable)
    if tracing is not None:
        cache = tracing['cache']
        entry = cache['cases'][tracing['aliases'].get(name,name)]
        assert entry['terminal']==dict(kind='exit',code=expected_exit)
        seen = cache['accepted_application'][:entry['seen_count']]
        (work/'trace.seen').write_text(''.join(n+'\n' for n in seen))
        (work/'trace.expected').write_text(''.join(n+'\n' for n in entry['expected']))
        trace_stream = (work/'trace.entries').open('wb')
        trace_options['pass_fds'] = (trace_stream.fileno(),)
        if name in ('hello-dll','hello-debug-dll'):
            state_stream = (work/'trace.state').open('w+b')
            state_stream.truncate(32+len(cache['application_definitions']))
            trace_options['pass_fds'] += (state_stream.fileno(),)
            env['RECON_TRACE_STATE_FD'] = str(state_stream.fileno())
            state['shared_trace_state_bytes'] = 32+len(cache['application_definitions'])
        env.update(RECON_TRACE_FD=str(trace_stream.fileno()),
                   RECON_TRACE_FILTER=str(tracing['filter']),
                   RECON_TRACE_SEEN=str(work/'trace.seen'))
    if name in ('hello-dll','hello-debug-dll'):
        env['RECON_SELF_SPAWN'] = '1'
        spec = importlib.util.spec_from_file_location('native_process_fixture', Path(__file__).with_name('native-processes.py'))
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        process_fixture = module.ProcessFixture(name,work,env)
        trace_options['pass_fds'] = trace_options.get('pass_fds',())+(process_fixture.fileno(),)
    setup = time.perf_counter() - start
    state['setup_seconds'] = setup
    launch = time.perf_counter()
    try:
        result = subprocess.run([launch_program, *args], cwd=work, env=env,
                                stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE, timeout=30, **trace_options)
    finally:
        if trace_stream is not None:
            trace_stream.close()
        if state_stream is not None:
            state_stream.close()
        if process_fixture is not None:
            process_fixture.close()
    run_seconds = time.perf_counter() - launch
    (work / 'stdout.bin').write_bytes(result.stdout)
    (work / 'stderr.bin').write_bytes(result.stderr)
    state.update(exit=result.returncode, execution_seconds=run_seconds,
                 stdout_bytes=len(result.stdout), stdout_sha256=sha(result.stdout),
                 stderr_bytes=len(result.stderr), stderr_sha256=sha(result.stderr))
    if process_fixture is not None:
        state['process_fixture'] = process_fixture.receipt()
        (work/'spawn.json').write_text(json.dumps(state['process_fixture'],indent=2)+'\n')
        process_fixture.check()
    if tracing is not None:
        trace_start = time.perf_counter()
        raw = (work/'trace.entries').read_bytes()
        assert not raw or raw.endswith(b'\n'), 'incomplete native entry record'
        actual = raw.decode('ascii').splitlines()
        expected = entry['expected']
        state.update(trace_entries=len(actual),trace_sha256=sha(raw),
                     trace_prefix_entries=len(seen))
        for i in range(max(len(actual),len(expected))):
            left = expected[i] if i<len(expected) else '<end>'
            right = actual[i] if i<len(actual) else '<end>'
            assert left==right, f'native trace divergence at entry {i}: expected {left}, actual {right}'
        if process_fixture is not None:
            markers = ['library_main','build_import_library'] if name=='hello-dll' else ['read_definition_file']
            assert all(marker in actual for marker in markers), 'native child application entries missing'
            state['process_fixture']['child_application_entries'] = markers
            (work/'spawn.json').write_text(json.dumps(state['process_fixture'],indent=2)+'\n')
        state['trace_check_seconds'] = time.perf_counter()-trace_start
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
    ap.add_argument('--exe', type=Path)
    ap.add_argument('--trace', action='store_true', help='check cached application entry segments using separate instrumented ELF')
    ap.add_argument('--directory', type=Path)
    ap.add_argument('--trace-cache', type=Path, default=BINARY/'cases/native-traces.json',
                    help='cached reference segments; override for divergence diagnostics')
    options = ap.parse_args()
    executable = (options.exe or BINARY/('build/native-trace/LINK.EXE' if options.trace else 'build/native/LINK.EXE')).resolve()
    assert executable.read_bytes()[:4] == b'\x7fELF', 'native checker requires ELF'
    directory = options.directory
    if directory is None:
        scratch = Path.home() / '.ai-tmp/vc1.0-link'
        scratch.mkdir(parents=True, exist_ok=True)
        directory = Path(tempfile.mkdtemp(prefix='native-check-', dir=scratch))
    else:
        directory.mkdir(parents=True, exist_ok=False)
    trace_prepare_start = time.perf_counter()
    tracing = None
    if options.trace:
        spec = importlib.util.spec_from_file_location('native_trace_cache', Path(__file__).with_name('native-traces.py'))
        trace_module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(trace_module)
        cache = trace_module.load(BINARY/'build/native-trace', options.trace_cache)
        filter_path = directory/'trace.filter'
        filter_path.write_text(''.join(n+'\n' for n in cache['application_definitions']))
        tracing = dict(cache=cache,filter=filter_path,aliases=trace_module.ALIASES)
    trace_prepare_seconds = time.perf_counter()-trace_prepare_start
    parser = parser_module()
    receipt = dict(mode='captured-reference-only', executable=str(executable),
                   executable_sha256=sha(executable.read_bytes()), directory=str(directory),
                   build_seconds='unmeasured; build is a separate make target', cases=[])
    if options.trace:
        receipt.update(trace_mode='cached application first-entry segments',
                       trace_prepare_seconds=trace_prepare_seconds,
                       trace_cache=str(options.trace_cache),
                       trace_cache_sha256=sha(options.trace_cache.read_bytes()))
    start = time.perf_counter()
    for name in options.case or CASES:
        case_start = time.perf_counter()
        result = dict(case=name)
        try:
            result.update(run_case(name, executable, directory, parser, result, tracing))
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
