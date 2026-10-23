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
         'preprocessor-P':('preprocessor','c1-P'), 'comments':('comments','c1'), 'prototypes':('prototypes','c1'), 'automatic-pch-create':('automatic-pch','c1-create'),
         'automatic-pch-reuse':('automatic-pch','c1-reuse'), 'automatic-pch-changed':('automatic-pch','c1-changed'),
         'embedded-codeview':('embedded-codeview','c1'),
         'inlining-ob1':('inlining','c1-ob1'), 'inlining-ob2':('inlining','c1-ob2'),
         'combined-types-literals':('combined-types-literals','c1'),
         'debug-pch-create':('debug-pch','c1-create'),
         'debug-pch-use':('debug-pch','c1-use'),
         'inline-asm-rich':('inline-asm-rich','c1'),
         'storage-pragmas':('storage-pragmas','c1'),
         'diagnostic-recovery':('diagnostic-recovery','c1'),
         'based-record-arena':('based-record-arena','c1'),
         'pdb-pch-binding-create':('pdb-pch-binding','c1-create'),
         'pdb-pch-binding-use':('pdb-pch-binding','c1-use'),
         'legacy-callback-adapter':('legacy-callback-adapter','c1'),
         'inline-asm-expressions':('inline-asm-expressions','c1'),
         'browser-pch-create':('browser-pch-timestamp','c1-create'),
         'browser-pch-use':('browser-pch-timestamp','c1-use'),
         'asm-diagnostic-recovery':('asm-diagnostic-recovery','c1'),
         'calibration-table':('calibration-table','c1'),
         'packet-view-adapter':('packet-view-adapter','c1'),
         'arena-api-compile':('arena-api-prototypes','c1-compile'),
         'arena-api-prototypes':('arena-api-prototypes','c1-prototypes'),
         'portability-diagnostics':('portability-diagnostics','c1'),
         'explicit-base-control':('explicit-base-control','c1'),
         'typed-x87-bitscan':('typed-x87-bitscan','c1'),
         'generated-header-missing':('generated-header-failures','c1-missing'),
         'generated-header-incomplete':('generated-header-failures','c1-incomplete')}


CASE_DEPENDENCIES = {
    'browser-pch-use':('browser-pch-create','api.pch'),
    'pdb-pch-binding-use':('pdb-pch-binding-create','binding.pch'),
    'pch-use':('pch-create','state.pch'),
    'debug-pch-use':('debug-pch-create','debug.pch'),
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
    for name in phase.get('expected_absent_outputs', []):
        assert Path(name).name == name, 'invalid absent output name'
        assert not (work/name).exists(), 'forbidden output exists: '+name
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


def validate_generated_signature(work, signature):
    assert struct.unpack_from('<I', (work/'msvc.pdb').read_bytes(), 48)[0] == signature, 'consumer database signature changed'
    assert struct.unpack_from('<I', (work/'irdb').read_bytes(), 14)[0] == signature, 'consumer reference signature changed'


def validate_result(name, returncode, stdout, stderr, phase):
    assert returncode == phase['expected_exit'], ('native status',name,returncode)
    assert stdout == phase['expected_stdout'].encode('latin1'), 'stdout differs: '+name
    assert stderr == phase['expected_stderr'].encode('latin1'), 'stderr differs: '+name


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
    generated_inputs = phase.get('generated_input_dependencies', [])
    generated_contract = None
    generated_signature = None
    if generated_inputs:
        assert dependency is not None, 'consumer requires actual producer state'
        producer_case = generated_inputs[0]['case']
        assert CASE_DEPENDENCIES[name][0] == producer_case
        producer_phase = metadata['phases'][phase['producer_phase']]
        # This also proves the complete captured C2 input contract before
        # exposing its frozen producer.obj response to the frontend consumer.
        generated_contract = validate_artifacts(dependency.parent, fixture, producer_phase)
        assert phase['captured_backend_dependency']['input_outputs'] == [
            row['output_name'] for row in producer_phase['outputs']]
        assert phase['captured_backend_dependency']['object_input'] == 'producer.obj'
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
        for generated_input in generated_inputs:
            if basename != generated_input['input_name']:
                continue
            assert generated_input['case'] == CASE_DEPENDENCIES[name][0]
            generated = (dependency.parent/generated_input['output_name']).read_bytes()
            assert len(generated) == len(data), 'generated dependency size differs'
            clock_offset = generated_input.get('clock_offset')
            if clock_offset is None:
                assert generated == data, 'generated PCH differs from captured dependency'
            else:
                assert basename == 'msvc.pdb' and clock_offset == 48
                assert generated[:48] + generated[52:] == data[:48] + data[52:], 'generated database differs outside signature'
                generated_signature = struct.unpack_from('<I', generated, 48)[0]
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
    validate_result(name,result.returncode,result.stdout,result.stderr,phase)
    outputs = validate_artifacts(work, fixture, phase)
    if generated_signature is not None:
        validate_generated_signature(work, generated_signature)
    state.update(status='pass', manifest=str(manifest.relative_to(ROOT)),
                 manifest_sha256=sha(manifest.read_bytes()), outputs=outputs,
                 generated_dependency=(str(dependency) if dependency else None),
                 generated_input_contract=generated_contract,
                 generated_input_signature=generated_signature,
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
