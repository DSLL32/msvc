#!/usr/bin/env python3
"""Check real native self-exec, cross-process entry state and explicit replay."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import time

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from spawn import SpawnBroker


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('executable', type=Path)
    executable = ap.parse_args().executable.resolve()
    scratch = Path.home()/'.ai-tmp/vc1.0-native-process'
    scratch.mkdir(parents=True, exist_ok=True)
    directory = Path(tempfile.mkdtemp(prefix='checks-', dir=scratch))
    canonical = r'Z:\virtual\LINK.EXE'
    results = []

    def run(name, mode, callback=None, shared=False, expected_code=0, trace=True, bad_state=False, seen=''):
        start = time.perf_counter()
        work = directory/name
        work.mkdir()
        (work/'filter').write_text('main\ncommon\nchild_only\n')
        (work/'seen').write_text(seen)
        (work/'input.bin').write_bytes(b'unpacked')
        (work/'original.exe').write_bytes(b'MZ'+b'\0'*30)
        env = {k:v for k,v in os.environ.items() if not k.startswith('RECON_')}
        env.update(RECON_PROGRAM_PATH=canonical, RECON_SELF_SPAWN='1', SHIM_CHILD_MARKER='preserved')
        with (work/'trace').open('wb') as entries, (work/'state').open('w+b') as state:
            state.truncate(1 if bad_state else 35)
            pass_fds = []
            if trace:
                pass_fds.append(entries.fileno())
                env.update(RECON_TRACE_FD=str(entries.fileno()), RECON_TRACE_FILTER=str(work/'filter'),
                           RECON_TRACE_SEEN=str(work/'seen'))
            if shared:
                pass_fds.append(state.fileno())
                env['RECON_TRACE_STATE_FD'] = str(state.fileno())
            broker = SpawnBroker(callback) if callback else None
            if broker:
                pass_fds.append(broker.fileno())
                env.update(broker.environment())
            try:
                process = subprocess.run([str(executable), mode], cwd=work, env=env,
                                         pass_fds=pass_fds, capture_output=True, timeout=10)
            finally:
                if broker:
                    broker.close()
        assert process.returncode == expected_code, (name,process.returncode,process.stderr)
        actual = (work/'trace').read_text().splitlines()
        if expected_code == 0:
            assert process.stdout == b'process target output\n', (name, process.stdout)
            assert not process.stderr, (name,process.stderr)
            if broker:
                broker.check()
        else:
            assert not process.stdout
        if mode == 'parent' and expected_code == 0:
            child_pid, = struct.unpack('<I',(work/'child.pid').read_bytes())
            assert child_pid > 0
            if broker:
                assert len(broker.requests) == 1 and child_pid != broker.requests[0]['pid']
            expected = ['main','common','child_only'] if shared else ['main','common','main','child_only','common']
            if seen:
                expected = ['child_only']
            assert actual == (expected if trace else []), (name,actual)
        elif mode in ('mock','pe') and expected_code == 0:
            assert actual == (['main'] if trace else []), (name,actual)
        if bad_state and trace:
            assert b'invalid RECON_TRACE_STATE_FD' in process.stderr
        results.append(dict(check=name,status='passed',seconds=time.perf_counter()-start,
                            entries=actual,requests=broker.requests if broker else [],
                            broker_errors=broker.errors if broker else []))

    def native(request):
        assert request['mode'] == 0 and request['file'] == canonical
        assert request['argv'] == [canonical,'child']
        assert 'SHIM_CHILD_MARKER=preserved' in request['environment']
        assert request['cwd'].startswith(str(directory))
        return None

    def replay(request):
        assert request['file'] == 'cvpack.exe'
        assert request['argv'] == ['cvpack.exe','/nologo','/cvnew','input.bin']
        target = Path(request['cwd'])/'input.bin'
        assert hashlib.sha256(target.read_bytes()).digest() == hashlib.sha256(b'unpacked').digest()
        target.write_bytes(b'packed')
        return 9

    def reject(request):
        raise AssertionError('deliberate wrong invocation/input')

    run('real-native-shared', 'parent', native, shared=True)
    run('real-native-local', 'parent', native)
    run('accepted-prefix-shared', 'parent', native, shared=True, seen='main\ncommon\n')
    run('explicit-replay', 'mock', replay)
    run('rejected-replay', 'mock', reject, expected_code=125)
    run('pe-no-fallback', 'pe')
    run('disabled-trace', 'parent', native, trace=False, shared=True, bad_state=True)
    run('invalid-shared-state', 'parent', shared=True, bad_state=True, expected_code=126)
    receipt = dict(status='passed',checks=results,seconds=sum(r['seconds'] for r in results),
                   scope='Native process tests only; mocked CVPACK is a protocol probe, no PE/Wine/original runs')
    path = directory/'receipt.json'
    path.write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(status='passed',checks=len(results),seconds=receipt['seconds'],receipt=str(path))))


if __name__ == '__main__':
    main()
