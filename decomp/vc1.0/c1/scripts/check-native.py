#!/usr/bin/env python3
"""Execute only native C1 against immutable original inputs/outputs and traces."""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import time
from case_paths import windows, native_work

BINARY = Path(__file__).resolve().parents[1]
ROOT = BINARY.parents[2]
FIXTURES = BINARY.parent/'fixtures'
CASES = {'minimal':('minimal-c','c1'), 'hello-win32':('hello-win32','c1'),
         'hello-dll':('hello-dll','c1'), 'numeric-varargs':('numeric-varargs','c1'),
         'pch-create':('pch','c1-create'), 'pch-use':('pch','c1-use'),
         'do-while':('do-while','c1'), 'seh':('seh','c1'), 'inline-asm':('inline-asm','c1'), 'browser':('browser','c1'),
         'preprocessor-E':('preprocessor','c1-E'), 'preprocessor-EP':('preprocessor','c1-EP'),
         'preprocessor-P':('preprocessor','c1-P'), 'comments':('comments','c1'), 'prototypes':('prototypes','c1')}


CASE_DEPENDENCIES = {
    'pch-use':('pch-create','state.pch'),
    'automatic-pch-reuse':('automatic-pch-create','state.pch'),
    'automatic-pch-changed':('automatic-pch-reuse','state.pch'),
}


def ordered_cases(requested):
    ordered = []
    visiting = set()
    def append(name):
        assert name not in visiting, 'cyclic case dependency'
        if name in ordered:
            return
        visiting.add(name)
        if name in CASE_DEPENDENCIES:
            append(CASE_DEPENDENCIES[name][0])
        visiting.remove(name)
        ordered.append(name)
    for name in requested:
        append(name)
    return ordered


def sha(data):
    return hashlib.sha256(data).hexdigest()


def captured(path, record):
    data = path.read_bytes()
    assert sha(data) == record['sha256'], 'captured hash differs: '+str(path)
    if 'bytes' in record:
        assert len(data) == record['bytes'], 'captured size differs: '+str(path)
    return data


def normalize(data, record):
    fields = record.get('ignored_fields', [])
    if fields:
        permitted = {'irdb':[(14,4)], 'msvc.pdb':[(48,4)]}
        assert [(f['offset'],f['bytes']) for f in fields] == permitted[record['output_name']]
    result = bytearray(data)
    for field in fields:
        start, size = field['offset'], field['bytes']
        assert len(result) >= start+size
        result[start:start+size] = bytes(size)
    return bytes(result)


def cross_outputs(work, phase):
    for check in phase.get('cross_output_checks', []):
        assert check['kind'] == 'equal-u32'
        values = []
        for side in ('left', 'right'):
            item = check[side]
            data = (work/item['file']).read_bytes()
            values.append(struct.unpack_from('<I', data, item['offset'])[0])
        assert values[0] == values[1], 'raw cross-output timestamps differ'


def validate_artifacts(work, fixture, phase):
    expected_files = {Path(i['path']).name for i in phase['inputs']} | {i['output_name'] for i in phase['outputs']}
    assert {p.name for p in work.iterdir()} == expected_files, 'complete file set differs'
    cross_outputs(work, phase)  # Raw equality must precede timestamp normalization.
    outputs = []
    for item in phase['outputs']:
        reference = captured(fixture/item['path'],item)
        actual = (work/item['output_name']).read_bytes()
        assert len(actual) == item['bytes'], 'output size differs: '+item['output_name']
        expected = normalize(reference,item)
        assert normalize(actual,item) == expected, 'output bytes differ: '+item['output_name']
        assert sha(expected) == item.get('normalized_sha256',item['sha256'])
        outputs.append(dict(name=item['output_name'],bytes=len(actual),sha256=sha(actual)))
    return outputs


def run_case(name, executable, directory, traces=None, dependency=None):
    start = time.perf_counter()
    fixture_name, phase_name = CASES[name]
    fixture = BINARY/'cases'/fixture_name
    if not (fixture/'manifest.json').is_file():
        fixture = FIXTURES/fixture_name
    manifest = fixture/'manifest.json'
    metadata = json.loads(manifest.read_text())
    phase = metadata['phases'][phase_name]
    native_inputs = json.loads((BINARY/'cases/native-inputs.json').read_text())
    catalog = native_inputs['message_catalog']
    captured(ROOT/catalog['path'], catalog)
    case_root = directory/name
    case_root.mkdir()
    canonical = phase.get('canonical_working_directory')
    work, path_environment = native_work(case_root,canonical)
    env = dict(LC_ALL='C', TZ='PST8PDT',
               MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(
                   bin=windows(ROOT/'toolchains/1.0-nt/bin'),
                   include=windows(ROOT/'toolchains/1.0-nt/include')),
               RECON_PROGRAM_PATH=windows(ROOT/'toolchains/1.0-nt/bin/C13232.EXE'))
    env.update(path_environment)
    for item in phase['inputs']:
        data = captured(fixture/item['path'], item)
        basename = Path(item['path']).name
        generated_input = phase.get('generated_input_dependency')
        if generated_input:
            assert CASE_DEPENDENCIES[name] == (generated_input['case'],generated_input['output_name'])
        if (name == 'pch-use' and basename == 'state.pch') or (generated_input and basename == generated_input['input_name']):
            assert dependency is not None, 'PCH consumer requires actual producer output'
            generated = dependency.read_bytes()
            assert generated == data, 'generated PCH differs from captured dependency'
            data = generated
        target = work/basename
        assert not target.exists(), 'duplicate input basename'
        target.write_bytes(data)
        if 'mtime_ns' in item:
            os.utime(target,ns=(item['mtime_ns'],item['mtime_ns']))
    for item in metadata.get('toolchain_dependencies', []) + phase.get('toolchain_dependencies', []):
        captured(ROOT/item['path'], item)
    for item in phase['outputs']:
        captured(fixture/item['path'], item)
    trace_stream = state_stream = None
    options = {}
    if traces is not None:
        cache, filter_path = traces
        entry = cache['cases'][name]
        assert entry['terminal'] == dict(kind='exit', code=phase['expected_exit'])
        seen = cache['accepted_application'][:entry['seen_count']]
        (case_root/'seen').write_text(''.join(n+'\n' for n in seen))
        trace_stream = (case_root/'entries').open('wb')
        state_stream = (case_root/'trace.state').open('w+b')
        state_stream.truncate(32+len(cache['application_definitions']))
        options['pass_fds'] = (trace_stream.fileno(), state_stream.fileno())
        env.update(RECON_TRACE_FD=str(trace_stream.fileno()),
                   RECON_TRACE_STATE_FD=str(state_stream.fileno()),
                   RECON_TRACE_FILTER=str(filter_path), RECON_TRACE_SEEN=str(case_root/'seen'))
    setup = time.perf_counter()-start
    execution_start = time.perf_counter()
    try:
        result = subprocess.run([str(executable)], cwd=work, env=env,
                                stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                                stderr=(subprocess.STDOUT if phase.get('stdio', {}).get('stderr_to_stdout') else subprocess.PIPE),
                                timeout=30, **options)
    finally:
        if trace_stream is not None:
            trace_stream.close()
        if state_stream is not None:
            state_stream.close()
    if result.stderr is None:
        assert phase.get('stdio', {}).get('stderr_to_stdout')
        result.stderr = b''
    execution = time.perf_counter()-execution_start
    (case_root/'stdout').write_bytes(result.stdout)
    (case_root/'stderr').write_bytes(result.stderr)
    state = dict(case=name, work=str(work), exit=result.returncode,
                 stdio=phase.get('stdio', {'stderr_to_stdout':False}),
                 stdout_bytes=len(result.stdout), stderr_bytes=len(result.stderr),
                 setup_seconds=setup, execution_seconds=execution)
    (case_root/'execution.json').write_text(json.dumps(state, indent=2)+'\n')
    checks = time.perf_counter()
    if traces is not None:
        raw = (case_root/'entries').read_bytes()
        actual = raw.decode('ascii').splitlines()
        assert not raw or raw.endswith(b'\n'), 'partial native trace entry'
        expected = entry['expected']
        for index in range(max(len(actual),len(expected))):
            a = expected[index] if index < len(expected) else '<end>'
            b = actual[index] if index < len(actual) else '<end>'
            assert a == b, 'native trace %s index %d: expected %s, actual %s' % (name,index,a,b)
        state.update(application_entries=len(actual), accepted_prefix=len(seen),
                     trace_sha256=sha(raw), shared_state_bytes=32+len(cache['application_definitions']))
    assert result.returncode == phase['expected_exit'], ('native status',name,result.returncode)
    assert result.stdout == phase['expected_stdout'].encode('latin1'), 'stdout differs: '+name
    assert result.stderr == phase['expected_stderr'].encode('latin1'), 'stderr differs: '+name
    outputs = validate_artifacts(work, fixture, phase)
    state.update(status='pass', manifest=str(manifest.relative_to(ROOT)),
                 manifest_sha256=sha(manifest.read_bytes()), outputs=outputs,
                 generated_dependency=(str(dependency) if dependency else None),
                 oracle_seconds=time.perf_counter()-checks,
                 total_seconds=time.perf_counter()-start)
    (case_root/'receipt.json').write_text(json.dumps(state,indent=2)+'\n')
    print('%s: native pass %.6fs' % (name,state['total_seconds']), flush=True)
    return state, work


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case', action='append', choices=CASES)
    parser.add_argument('--exe', type=Path)
    parser.add_argument('--trace', action='store_true')
    parser.add_argument('--trace-cache', type=Path, default=BINARY/'cases/native-traces.json')
    parser.add_argument('--directory', type=Path)
    options = parser.parse_args()
    executable = (options.exe or BINARY/('build/native-trace/C13232.EXE' if options.trace else 'build/native/C13232.EXE')).resolve()
    header = executable.read_bytes()[:20]
    assert header[:5] == b'\x7fELF\x01' and header[18:20] == b'\x03\x00', 'native i386 ELF required'
    scratch = Path.home()/'.ai-tmp/msvc2/vc1.0/c1/validation'
    scratch.mkdir(parents=True,exist_ok=True)
    directory = options.directory or Path(tempfile.mkdtemp(prefix='native-check-', dir=scratch))
    if options.directory:
        directory.mkdir(parents=True, exist_ok=False)
    tracing = None
    trace_prepare_start = time.perf_counter()
    if options.trace:
        spec = importlib.util.spec_from_file_location('c1_native_traces',Path(__file__).with_name('native-traces.py'))
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        cache = module.load(BINARY/'build/native-trace',options.trace_cache)
        filter_path = directory/'filter'
        filter_path.write_text(''.join(n+'\n' for n in cache['application_definitions']))
        tracing = (cache,filter_path)
    trace_prepare = time.perf_counter()-trace_prepare_start
    names = ordered_cases(options.case or list(CASES))
    reports, completed = [], {}
    for name in names:
        dependency = None
        if name in CASE_DEPENDENCIES:
            producer, output = CASE_DEPENDENCIES[name]
            dependency = completed[producer]/output
        report,work = run_case(name,executable,directory,tracing,dependency)
        reports.append(report)
        completed[name] = work
    report = dict(status='passed', executable=str(executable), executable_sha256=sha(executable.read_bytes()),
                  trace=options.trace, trace_prepare_seconds=trace_prepare, cases=reports,
                  fixture_total_seconds=sum(r['total_seconds'] for r in reports),
                  execution_policy='Only native i386 ELF; immutable reference metadata/bytes; no Wine or original execution.',
                  directory=str(directory))
    (directory/'receipt.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Evidence:',directory/'receipt.json')


if __name__ == '__main__':
    main()
