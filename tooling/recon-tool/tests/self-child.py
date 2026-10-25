#!/usr/bin/env python3
"""Actual shim self-child execution, inherited IO, status and supervisor failures."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
TOOL = Path(os.environ.get('RECON_TOOL_TEST_EXE', ROOT / 'target/i686-unknown-linux-gnu/debug/recon-tool'))
FIXTURE = ROOT / 'tests/native-self'
subprocess.run(['make', '-C', str(FIXTURE), '-j4', 'cases'], check=True)
scratch = Path(tempfile.mkdtemp(prefix='recon-self-child-', dir=Path.home() / '.ai-tmp'))
try:
    for mode in ['success', 'seven', 'dword', 'captured', 'trap', 'raised-trap', 'missing-receipt', 'hang']:
        source = scratch / (mode + '.json')
        capture = scratch / (mode + '.bin')
        report = scratch / (mode + '-report.json')
        expected_code = 7 if mode in ['seven', 'dword'] else 0
        stdout = 'parent\nchild\n' + ('' if mode in ['trap', 'raised-trap', 'hang'] else f'parent-status:{expected_code}\n')
        if mode == 'captured':
            stdout += 'captured\n'
        value = dict(args=['T:\\self.exe'], stdin='text:payload\ntail\n',
                     stdout='text:' + stdout, stderr='text:child diagnostic\n',
                     exit_code=0, reject_unexpected_files=True, timeout_ms=500 if mode == 'hang' else 2000,
                     env={'RECON_SELF_SPAWN': '1', 'RECON_PROGRAM_PATH': 'T:\\self.exe',
                          'SELF_MARKER': 'inherited', 'SELF_CASE': mode},
                     logical_cwd='S:\\native-self',
                     assets=[dict(path='marker', data='text:A')])
        source.write_text(json.dumps(value))
        subprocess.run([TOOL, 'capture', 'pack', source, capture], check=True)
        case = FIXTURE / 'cases' / ('0001-captured' if mode == 'captured' else '0000-self')
        result = subprocess.run([TOOL, 'run', case, '--capture', capture, '--report', report], capture_output=True, text=True, timeout=5)
        data = json.loads(report.read_text())[0]
        passed = mode not in ['trap', 'raised-trap', 'missing-receipt', 'hang']
        assert data['pass'] == passed and (result.returncode == 0) == passed, (mode, result.stdout, result.stderr, data)
        assert not Path(data['workspace']).exists(), (mode, 'workspace leaked')
        assert data['first_entry_order'] == [0x1000, 0x1100, 0x1200], (mode, data)
        timing = (f'{data["elapsed_ms"]:.1f}ms; setup {data["setup_ms"]:.1f}ms, '
                  f'execution {data["execution_ms"]:.1f}ms, checks {data["checks_ms"]:.1f}ms')
        if mode == 'hang':
            assert 'timeout' in data['errors'], data
            print(f'hang: expected failure, {timing}')
            continue
        process = data['process_requests']
        assert process['requests'] == (2 if mode == 'captured' else 1), (mode, data)
        children = process['self_children']
        assert len(children) == 1, (mode, data)
        child = children[0]
        assert child['parent_pid'] == process['pids'][0], (mode, data)
        if mode in ['trap', 'raised-trap']:
            assert child['terminal'] == 'signal:5' and any('unexpected native child signal 5' in e for e in data['errors']), data
            if mode == 'trap':
                assert child['trap_rva'] == 0x1200 and child['trap_site'] is not None, data
            else:
                assert child['trap_rva'] is None and any('was not an INT3' in e for e in child['errors']), data
        elif mode == 'missing-receipt':
            assert any('missing terminal status publication' in e for e in data['errors']), data
        else:
            assert child['terminal'] == f'exit:{expected_code}', (mode, data)
            assert child['logical_exit_code'] == (0x12340007 if mode == 'dword' else expected_code), (mode, data)
            assert not child['errors'], (mode, data)
        print(f'{mode}: expected {"pass" if passed else "failure"}, {data["terminal"]}, {timing}')
    print('native self-child checks passed')
finally:
    shutil.rmtree(scratch)
