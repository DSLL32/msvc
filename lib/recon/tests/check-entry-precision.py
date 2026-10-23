#!/usr/bin/env python3
"""Verify opt-in native entry precision and unchanged default precision."""
from pathlib import Path
import subprocess
import sys

for executable,expected in zip(sys.argv[1:],[b'precision=0\r\n',b'precision=65536\r\n']):
    result=subprocess.run([str(Path(executable).resolve())],stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,timeout=10)
    assert result.returncode==0 and not result.stderr and result.stdout==expected,result
print('default and opt-in entry precision passed')
