"""Run the C++ fixture natively against immutable references.

No reference refresh, Win32 process, Wine lookup or downstream compiler is run.
The frozen oracle specifies an application frontier or a completed exit status.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import resource
import signal
import sys
import subprocess
import tempfile
import time
from case_paths import paths, native_work, windows, CAPTURE_ROOT

BINARY = Path(__file__).resolve().parents[1]
ROOT = BINARY.parents[2]
REFERENCE = BINARY / 'cases/native-cpp-basic'
sys.path.insert(0, str(ROOT / 'lib/recon'))
from spawn import SpawnBroker


def reject_dependency(request):
    raise AssertionError('unexpected C1XX external dependency: ' + request['file'])


def sha(data):
    return hashlib.sha256(data).hexdigest()


def captured(path, record):
    data = path.read_bytes()
    assert len(data) == record['bytes'] and sha(data) == record['sha256'], str(path) + ': cached input/reference changed'
    return data


def trace_module():
    spec = importlib.util.spec_from_file_location('c1xx_native_traces', Path(__file__).with_name('native-traces.py'))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def compare_trace(actual, expected):
    for index in range(max(len(actual), len(expected))):
        left = expected[index] if index < len(expected) else '<end>'
        right = actual[index] if index < len(actual) else '<end>'
        assert left == right, 'application entry %d: expected %s, actual %s' % (index, left, right)


def run(executable, directory, tracing, accounting, case_name='cpp_basic'):
    started = time.perf_counter()
    config=json.loads((BINARY/'functions.json').read_text())
    _,fixture,reference,_=paths(config,case_name)
    phase = json.loads((fixture / 'manifest.json').read_text())['phases']['c1']
    metadata_path = reference / 'manifest.json'
    metadata = json.loads(metadata_path.read_text())
    work, drive_environment = native_work(directory, phase['canonical_working_directory'])
    for item in phase['inputs']:
        (work / Path(item['path']).name).write_bytes(captured(fixture / item['path'], item))
    for item in metadata['outputs']:
        captured(reference / item['path'], item)
    for item in metadata.get('dependencies', []):
        captured(ROOT / item['path'], item)
    env = {k: v for k, v in os.environ.items() if not k.startswith('RECON_')}
    for name in ('CL', '_CL_', 'LINK', '_LINK_', 'MSC_CMD_FLAGS'):
        env.pop(name, None)
    windows_bin = windows(ROOT/'toolchains/1.0-nt/bin')
    env['MSC_CMD_FLAGS'] = phase['environment']['MSC_CMD_FLAGS'].format(
        bin=windows_bin, include=windows(ROOT/'toolchains/1.0-nt/include'))
    program = phase['command_line_template'].format(bin=windows_bin)
    env.update(drive_environment)
    env.update(RECON_PROGRAM_PATH=program, TZ='PST8PDT', LC_ALL='C')
    temporary = work / 'tmp'
    temporary.mkdir()
    env['TMP'] = env['TEMP'] = env['TMPDIR'] = str(temporary)
    pass_fds, streams = (), []
    entry = None
    if tracing:
        cache, filter_path = tracing
        entry = cache['cases'][case_name]
        seen = work / 'trace.seen'
        seen.write_text(''.join(n + '\n' for n in entry['seen']))
        trace = (work / 'trace.entries').open('wb')
        state = (work / 'trace.state').open('w+b')
        state.truncate(32 + len(cache['application_definitions']))
        streams = [trace, state]
        pass_fds = tuple(stream.fileno() for stream in streams)
        env.update(RECON_TRACE_FD=str(trace.fileno()), RECON_TRACE_FILTER=str(filter_path),
                   RECON_TRACE_SEEN=str(seen), RECON_TRACE_STATE_FD=str(state.fileno()))
    setup_seconds = time.perf_counter() - started
    accounting['setup_seconds'] = setup_seconds
    launched = time.perf_counter()
    try:
        with SpawnBroker(reject_dependency) as broker:
            env.update(broker.environment())
            result = subprocess.run([program], executable=str(executable), cwd=work, env=env,
                stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                pass_fds=pass_fds + (broker.fileno(),), timeout=30)
        assert not broker.requests, 'unexpected external process requests'
    finally:
        for stream in streams:
            stream.close()
    execution_seconds = time.perf_counter() - launched
    accounting['execution_seconds'] = execution_seconds
    (work / 'stdout.bin').write_bytes(result.stdout)
    (work / 'stderr.bin').write_bytes(result.stderr)
    checked = time.perf_counter()
    complete=metadata['terminal']['kind']=='exit'
    if complete:
        assert result.returncode==metadata['terminal']['status'],'expected completed frontend exit status, got %d'%result.returncode
    else:
        assert result.returncode == -signal.SIGTRAP, 'expected named frontier SIGTRAP, got %d' % result.returncode
    assert result.stdout == metadata['expected_stdout'].encode('latin1'), 'stdout differs'
    assert result.stderr == metadata['expected_stderr'].encode('latin1'), 'stderr differs'
    artifacts = []
    if 'phase_output_presence' in metadata:
        presence = metadata['phase_output_presence']
        from checkpoint_files import declared_outputs
        declared = set(declared_outputs(phase,config['cases'][case_name]))
        assert len(presence) == len(declared) and {item['path'] for item in presence} == declared, 'incomplete declared output presence'
        assert {item['path'] for item in presence if item['present']} == {item['path'] for item in metadata['outputs']}, 'presence/cache output list differs'
        for item in presence:
            assert (work/item['path']).exists() == item['present'], item['path'] + ': output presence differs'
        visible = metadata['visible_files']
        assert {item['path'] for item in visible} == {Path(item['path']).name for item in phase['inputs']} | {item['path'] for item in metadata['outputs']}, 'incomplete checkpoint visible inventory'
        for item in visible:
            data = (work/item['path']).read_bytes()
            assert len(data) == item['bytes'] and sha(data) == item['sha256'], item['path'] + ': visible file differs'
    for item in metadata['outputs']:
        data = (work / item['path']).read_bytes()
        assert data == captured(reference / item['path'], item), item['path'] + ': visible bytes differ'
        artifacts.append(dict(name=item['path'], bytes=len(data), sha256=sha(data)))
    allowed = {Path(e['path']).name for e in phase['inputs']} | {e['path'] for e in metadata['outputs']} | {'tmp','stdout.bin','stderr.bin'}
    if tracing:
        allowed |= {'trace.entries', 'trace.seen', 'trace.state'}
    assert {p.name for p in work.iterdir()} == allowed, 'unexpected fixture files'
    oracle_seconds = time.perf_counter() - checked
    trace_start = time.perf_counter()
    trace_count = 0
    if tracing:
        raw = (work / 'trace.entries').read_bytes()
        assert raw.endswith(b'\n'), 'missing/incomplete native entry trace'
        actual = raw.decode('ascii').splitlines()
        trace_count = len(actual)
        compare_trace(actual, entry['expected'])
    return dict(case=case_name, status='pass', terminal=metadata['terminal'],
        native_status=result.returncode, complete_frontend_case=complete,
        setup_seconds=setup_seconds, execution_seconds=execution_seconds,
        oracle_seconds=oracle_seconds, trace_check_seconds=time.perf_counter()-trace_start,
        total_seconds=time.perf_counter()-started, application_trace_entries=trace_count,
        stdout_sha256=sha(result.stdout), stderr_sha256=sha(result.stderr), outputs=artifacts,
        reference_manifest=str(metadata_path.relative_to(ROOT)),
        reference_manifest_sha256=sha(metadata_path.read_bytes()))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case',action='append',help='registered case; defaults to all')
    parser.add_argument('--trace', action='store_true')
    parser.add_argument('--exe', type=Path)
    parser.add_argument('--directory', type=Path)
    parser.add_argument('--trace-cache', type=Path, default=BINARY / 'cases/native-traces.json')
    args = parser.parse_args()
    executable = (args.exe or BINARY / ('build/native-trace/C1XX3232.EXE' if args.trace else 'build/native/C1XX3232.EXE')).resolve()
    assert executable.read_bytes()[:5] == b'\x7fELF\x01', 'native i386 ELF required'
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    scratch = CAPTURE_ROOT/'native'
    scratch.mkdir(parents=True, exist_ok=True)
    directory = args.directory or Path(tempfile.mkdtemp(prefix='native-check-', dir=scratch))
    if args.directory:
        directory.mkdir(parents=True, exist_ok=False)
    prepared = time.perf_counter()
    tracing = None
    if args.trace:
        cache = trace_module().load(BINARY / 'build/native-trace', args.trace_cache)
        filter_path = directory / 'trace.filter'
        filter_path.write_text(''.join(n + '\n' for n in cache['application_definitions']))
        tracing = cache, filter_path
    receipt = dict(measured_partial_phases={}, mode='native cached references only', executable=str(executable),
        executable_sha256=sha(executable.read_bytes()), trace_prepare_seconds=time.perf_counter()-prepared,
        build_seconds='separate target; unmeasured here', capture_seconds='none; immutable prior evidence')
    try:
        names=args.case or list(json.loads((BINARY/'functions.json').read_text())['cases'])
        receipt['cases']=[]
        for name in names:
            accounting={}
            receipt['measured_partial_phases'][name]=accounting
            receipt['cases'].append(run(executable,directory,tracing,accounting,name))
        receipt['status'] = 'pass'
    except Exception as error:
        receipt.update(status='fail', error=str(error))
    (directory / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(','.join(args.case or list(json.loads((BINARY/'functions.json').read_text())['cases']))+':',receipt['status'])
    print('receipt:', directory / 'receipt.json')
    return int(receipt['status'] != 'pass')


if __name__ == '__main__':
    raise SystemExit(main())
