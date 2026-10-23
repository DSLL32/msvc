#!/usr/bin/env python3
"""Build the explicit original-only debugger outside native fixture targets."""
from pathlib import Path
import subprocess
import sys
from case_paths import SCRATCH, windows

BINARY = Path(__file__).resolve().parents[1]
output = SCRATCH/'vc1.0/c1/tools'
output.mkdir(parents=True,exist_ok=True)
subprocess.run([sys.executable,str(BINARY/'scripts/wine-tool.py'),'CL.EXE',
                '/nologo','/W3','/I../../../lib/recon/include',
                '/Fe'+windows(output/'capture-original.exe'),
                '/Fo'+windows(output/'capture-original.obj'),
                'src/capture-original.c'],cwd=BINARY,check=True)
