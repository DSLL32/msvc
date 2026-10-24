#!/usr/bin/env python3
"""Focused integration checks for the shared runner, not binary-specific tests."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / 'target/i686-unknown-linux-gnu/debug/recon-tool'
CASES = ROOT / 'tests/fixture/cases'
subprocess.run(['make', '-C', str(CASES.parent), '-j4', 'recon-native'], check=True)
for case in sorted(CASES.iterdir()):
    subprocess.run(['make', '-C', str(case), '-j4', 'RECON_SKIP_BINARY_BUILD=1'], check=True)

scratch_root = Path.home() / '.ai-tmp'
scratch_root.mkdir(exist_ok=True)
scratch = Path(tempfile.mkdtemp(prefix='recon-tool-test-', dir=scratch_root))

try:
    def run(name, number, fixture, *, passed=True):
        source = scratch / (name + '.json')
        packed = scratch / (name + '.bin')
        source.write_text(json.dumps(fixture))
        subprocess.run([TOOL, 'io', 'pack', source, packed], check=True)
        unpacked = subprocess.check_output([TOOL, 'io', 'unpack', packed])
        source.write_bytes(unpacked)
        roundtrip = scratch / (name + '-roundtrip.bin')
        subprocess.run([TOOL, 'io', 'pack', source, roundtrip], check=True)
        assert packed.read_bytes() == roundtrip.read_bytes()
        case = next(CASES.glob(number + '-*'))
        report=scratch/(name+'-report.json')
        result = subprocess.run([TOOL, 'run', case, '--io', packed, '--report',report])
        assert not Path(json.loads(report.read_text())[0]['workspace']).exists(),name+' workspace leaked'
        assert (result.returncode == 0) == passed, name

    run('io', '0000', dict(args=['fixture'], stdin='text:hello\n',
        stdout='text:result=5 hello\n', stderr='text:', exit_code=0,
        output_files=[dict(path='out.bin', data='hex:00ff')], reject_unexpected_files=True))
    subprocess.run([TOOL, 'test', CASES/'0000-io'], check=True)
    run('frontier', '0001', dict(args=['fixture', 'trap'],
        partial=dict(stdout='text:', stderr='text:', reject_unexpected_files=True)))
    run('orphan', '0002', dict(args=['fixture', 'orphan'], stdout='text:', stderr='text:',
        output_files=[dict(path='grandchild.txt', data='text:done')], reject_unexpected_files=True))
    run('timeout', '0003', dict(args=['fixture', 'timeout'], timeout_ms=40), passed=False)
    run('event', '0004', dict(args=['fixture', 'event'], stdout='text:ready\ngot:hello\n',
        stderr='text:', events=[dict(stdout_contains='text:ready\n',
        action=dict(kind='stdin', data='text:hello\n')), dict(after_ms=0, action=dict(kind='close_stdin'))]))
    run('direct', '0005', dict(stdout='text:', stderr='text:', exit_code=0, reject_unexpected_files=True))
    run('wrong-output', '0000', dict(args=['fixture'], stdin='text:x\n', stdout='text:WRONG'), passed=False)
    run('missing-is-not-empty', '0005', dict(output_files=[dict(path='missing', data='hex:')]), passed=False)
    # A signal sent only to the CLI must reach its cleanup supervisor promptly.
    source=scratch/'cancel.json';source.write_text(json.dumps(dict(args=['fixture','timeout'],timeout_ms=30000)))
    packed=scratch/'cancel.bin';subprocess.run([TOOL,'io','pack',source,packed],check=True)
    report=scratch/'cancel-report.json'
    child=subprocess.Popen([TOOL,'run',CASES/'0003-timeout','--io',packed,'--report',report])
    import time
    time.sleep(.05)
    child.terminate()
    assert child.wait(timeout=2) != 0
    assert not Path(json.loads(report.read_text())[0]['workspace']).exists(), 'cancel workspace leaked'
    if shutil.which('gdb'):
        for test in [False,True]:
            script=scratch/('unit.gdb' if test else 'main.gdb')
            symbol='fixture_add' if test else 'fixture_main'
            script.write_text('break '+symbol+'\ncontinue\nprintf "RECON_GDB_HIT\\n"\ncontinue\n')
            args=[TOOL,'test' if test else 'run',CASES/'0000-io','--gdb-script',script]
            if test:args+=['--test','add']
            else:args+=['--io',scratch/'io.bin']
            result=subprocess.run(args,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,check=True)
            assert 'RECON_GDB_HIT' in result.stdout and 'PASS' in result.stdout
    print('shared native smoke checks passed')
finally:
    shutil.rmtree(scratch)
