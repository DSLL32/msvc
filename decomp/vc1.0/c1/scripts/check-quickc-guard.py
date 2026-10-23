#!/usr/bin/env python3
"""Run the precise undefined-default guard and a genuine QuickC control."""
import json
from pathlib import Path
import subprocess
import tempfile
import time

BINARY = Path(__file__).resolve().parents[1]

def main():
    start = time.perf_counter()
    directory = Path(tempfile.mkdtemp(prefix='quickc-guard-', dir=Path.home()/'.ai-tmp/msvc2/vc1.0/c1/validation'))
    executable = BINARY/'build/quickc-tests/C13232.EXE'
    result = subprocess.run([str(executable), '--unsupported'], cwd=directory,
                            env={'LC_ALL':'C','TZ':'PST8PDT'}, capture_output=True)
    assert result.returncode == 125 and not result.stdout
    assert result.stderr == b'recon: unsupported native operation: emit_initializer_builtin_call: uninitialized original default\n'
    control = subprocess.run(['python3',str(BINARY/'scripts/check-native.py'),
        '--exe',str(executable),'--case','quickc-regions','--directory',str(directory/'control')], cwd=BINARY,
        capture_output=True)
    (directory/'control.log').write_bytes(control.stdout+control.stderr)
    assert control.returncode == 0, control.stdout+control.stderr
    report = dict(status='pass',unsupported_operation=0,exit=result.returncode,
        valid_control='ordinary QuickC case, complete cached outputs/status/streams',
        total_seconds=time.perf_counter()-start)
    (directory/'receipt.json').write_text(json.dumps(report,indent=2)+'\n')
    print('QuickC undefined default:125; genuine valid control:pass; %.6fs' % report['total_seconds'])
    print('Evidence:',directory/'receipt.json')

if __name__ == '__main__':
    main()
