"""Run only native ELF C2 against captured IR, original traces and object bytes."""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import resource
import shutil
import signal
import subprocess
import sys
import tempfile
import time

BINARY = Path(__file__).resolve().parents[1]
ROOT = BINARY.parents[2]
sys.path.insert(0, str(ROOT/'lib/recon'))
from spawn import SpawnBroker

spec = importlib.util.spec_from_file_location('c2_native_traces', BINARY/'scripts/native-traces.py')
traces = importlib.util.module_from_spec(spec)
spec.loader.exec_module(traces)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def captured(path, record):
    data = path.read_bytes()
    assert digest(data) == record['sha256'], 'captured fixture changed: ' + str(path)
    if 'bytes' in record:
        assert len(data) == record['bytes'], 'captured fixture size changed'
    return data


def outputs(work, phase, fixture):
    record = phase['outputs'][0]
    expected = bytearray(captured(fixture/record['path'], record))
    assert len(record['ignored_fields']) == 1
    assert (record['ignored_fields'][0]['offset'], record['ignored_fields'][0]['bytes']) == (4, 4)
    filename = record.get('output_name', Path(record['path']).name)
    raw = (work/filename).read_bytes()
    actual = bytearray(raw)
    expected[4:8] = actual[4:8] = b'\0'*4
    assert actual == expected, 'object differs beyond COFF timestamp'
    assert digest(actual) == record['normalized_sha256']
    assert {p.name for p in work.iterdir()} == {Path(r['path']).name for r in phase['inputs']} | {filename}, 'unexpected artifacts'
    return dict(name=filename, bytes=len(raw), normalized_sha256=digest(actual), ignored_timestamp=[4, 4])


def reject_process(request):
    raise AssertionError('C2 fixtures have no captured external dependencies: ' + str(request))


def run(profile, executable, directory, cache, tracing=True, expected_override=None):
    setup_started = time.perf_counter()
    config = json.loads((BINARY/'functions.json').read_text())
    record = config['cases'][profile]
    manifest = (BINARY/record['fixture_manifest']).resolve()
    raw = manifest.read_bytes()
    assert digest(raw) == record['fixture_manifest_sha256'], 'fixture manifest changed'
    phase = json.loads(raw)['phases']['c2']
    assert phase['sha256'] == config['target']['sha256']
    assert phase['arguments'] == [] and set(phase['environment']) == {'MSC_CMD_FLAGS'}
    case = cache['cases'][profile]
    directory.mkdir()
    work = directory/'work'
    work.mkdir()
    for source in phase['inputs']:
        (work/Path(source['path']).name).write_bytes(captured(manifest.parent/source['path'], source))
    assert executable.read_bytes()[:5] == b'\x7fELF\x01', 'native fixture requires i386 ELF'
    environment = dict(PATH=os.defpath, LC_ALL='C', TZ='UTC')
    windows_bin = 'Z:' + str(ROOT/'toolchains/1.0-nt/bin').replace('/', '\\')
    environment['MSC_CMD_FLAGS'] = phase['environment']['MSC_CMD_FLAGS'].format(bin=windows_bin)
    environment['RECON_PROGRAM_PATH'] = 'Z:' + str(ROOT/'toolchains/1.0-nt/bin/C23232.EXE').replace('/', '\\')
    filter_path, seen_path = directory/'filter', directory/'seen'
    names = list(cache['application_definitions'])
    filter_path.write_text('\n'.join(names)+'\n')
    seen_path.write_text('\n'.join(case['seen']) + ('\n' if case['seen'] else ''))
    trace_path, state_path = directory/'trace', directory/'state'
    with trace_path.open('wb') as trace, state_path.open('w+b') as state, SpawnBroker(reject_process) as broker:
        state.write(b'\0'*(32+len(names)))
        state.flush()
        environment.update(broker.environment())
        descriptors = [broker.fileno()]
        if tracing:
            environment.update(RECON_TRACE_FD=str(trace.fileno()), RECON_TRACE_FILTER=str(filter_path),
                               RECON_TRACE_SEEN=str(seen_path), RECON_TRACE_STATE_FD=str(state.fileno()))
            descriptors += [trace.fileno(), state.fileno()]
        (directory/'invocation.json').write_text(json.dumps(dict(
            executable=str(executable), executable_sha256=traces.sha(executable),
            cwd=str(work), argv=[str(executable)], environment=environment,
            manifest=str(manifest), manifest_sha256=digest(raw),
            expected_terminal=case['terminal']), indent=2)+'\n')
        setup_seconds = time.perf_counter()-setup_started
        run_started = time.perf_counter()
        result = subprocess.run([str(executable)], cwd=work, env=environment, pass_fds=descriptors,
                                stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                timeout=20)
        execution_seconds = time.perf_counter()-run_started
        check_started = time.perf_counter()
        broker.check()
        assert not broker.requests, 'unexpected C2 process dependency'
    (directory/'stdout').write_bytes(result.stdout)
    (directory/'stderr').write_bytes(result.stderr)
    actual = trace_path.read_text().splitlines()
    (directory/'observed.json').write_text(json.dumps(dict(
        returncode=result.returncode, application_entries=actual,
        setup_seconds=setup_seconds, execution_seconds=execution_seconds), indent=2)+'\n')
    expected = case['expected'] if expected_override is None else expected_override
    if tracing:
        mismatch = next(((i, actual[i] if i < len(actual) else '<end>',
                          expected[i] if i < len(expected) else '<end>')
                         for i in range(max(len(actual),len(expected)))
                         if i >= len(actual) or i >= len(expected) or actual[i] != expected[i]), None)
        assert actual == expected, ('application trace differs', profile, mismatch,
                                   len(actual), len(expected), result.returncode,
                                   result.stderr, str(directory))
    else:
        assert not actual, 'disabled trace emitted entries'
    assert result.returncode == case['returncode'], ('terminal differs', profile, result.returncode, result.stderr, directory)
    assert not result.stdout and not result.stderr, ('streams differ', result.stdout, result.stderr)
    output = outputs(work, phase, manifest.parent) if case['terminal'] == 'exit:0' else None
    return dict(profile=profile, terminal=case['terminal'], returncode=result.returncode,
                application_entries=len(actual), seeded_entries=len(case['seen']), output=output,
                setup_seconds=setup_seconds, execution_seconds=execution_seconds,
                check_seconds=time.perf_counter()-check_started, directory=str(directory))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profile', choices=tuple(json.loads((BINARY/'functions.json').read_text())['cases']))
    parser.add_argument('--no-trace', action='store_true')
    parser.add_argument('--reject-checks', action='store_true')
    args = parser.parse_args()
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    executable = BINARY/('build/native/C23232.EXE' if args.no_trace else 'build/native-trace/C23232.EXE')
    objects = executable.parent
    started = time.perf_counter()
    shared_files = list((ROOT/'lib/recon/src').glob('*.c')) + list((ROOT/'lib/recon/include').glob('*.h')) + list((ROOT/'lib/recon/native').glob('*.h')) + [ROOT/'lib/recon/spawn.py']
    shared_hashes = {str(path.relative_to(ROOT)):traces.sha(path) for path in sorted(shared_files)}
    cache = traces.load(objects)
    scratch = Path.home()/'.ai-tmp/vc1.0-c2'
    directory = Path(tempfile.mkdtemp(prefix='native-fixtures-', dir=scratch))
    suite_setup_seconds = time.perf_counter()-started
    profiles = [args.profile] if args.profile else ([p for p,c in cache['cases'].items() if c['terminal'].startswith('exit:')] if args.no_trace else list(cache['cases']))
    if args.no_trace:
        assert all(cache['cases'][profile]['terminal'].startswith('exit:') for profile in profiles), 'incomplete cases require tracing for named-stub evidence'
    results = [run(profile, executable, directory/profile, cache, not args.no_trace) for profile in profiles]
    rejected = []
    negative_started = time.perf_counter()
    if args.reject_checks:
        wrong = list(cache['cases']['minimal']['expected'])
        wrong[0], wrong[1] = wrong[1], wrong[0]
        try:
            run('minimal', executable, directory/'reject-wrong-entry', cache, True, wrong)
        except AssertionError as error:
            assert error.args and error.args[0][0] == 'application trace differs', error
            rejected.append('wrong expected application entry')
        else:
            raise AssertionError('wrong entry accepted')
        copy = directory/'reject-object'
        shutil.copytree(directory/'minimal'/'work', copy)
        cfg = json.loads((BINARY/'functions.json').read_text())['cases']['minimal']
        fixture = (BINARY/cfg['fixture_manifest']).resolve()
        phase = json.loads(fixture.read_text())['phases']['c2']
        name = phase['outputs'][0].get('output_name', Path(phase['outputs'][0]['path']).name)
        bad = bytearray((copy/name).read_bytes());bad[-1] ^= 1;(copy/name).write_bytes(bad)
        try:
            outputs(copy, phase, fixture.parent)
        except AssertionError:
            rejected.append('non-clock object corruption')
        else:
            raise AssertionError('corrupt object accepted')
        source = phase['inputs'][0]
        bad_input = directory/'reject-input'
        data = bytearray((fixture.parent/source['path']).read_bytes())
        data[0] ^= 1
        bad_input.write_bytes(data)
        try:
            captured(bad_input, source)
        except AssertionError:
            rejected.append('changed captured IR input')
        else:
            raise AssertionError('changed captured input accepted')
    negative_seconds = time.perf_counter()-negative_started if args.reject_checks else 0
    for path, value in shared_hashes.items():
        assert traces.sha(ROOT/path) == value, 'shared source changed during fixture: ' + path
    report = dict(method='native application entry and captured-output check', executable=str(executable),
                  executable_sha256=traces.sha(executable), trace_enabled=not args.no_trace,
                  shared_hashes=shared_hashes,
                  results=results, rejected=rejected, fixture_seconds=time.perf_counter()-started,
                  suite_setup_seconds=suite_setup_seconds, negative_check_seconds=negative_seconds,
                  timing_notes='No build, Wine, reference capture or prefix setup in fixtures; unmeasured phases not imputed.')
    path = BINARY/'cases/native-all-validation.json'
    path.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
