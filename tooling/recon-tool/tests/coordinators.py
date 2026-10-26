#!/usr/bin/env python3
"""Exercise toolchain dispatch/output contracts with authored child Makefiles."""
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import time

TOOL = Path(__file__).resolve().parents[1]
REPO = TOOL.parents[1]
base = Path.home() / '.ai-tmp/msvc2/native-coordinator-tests'
base.mkdir(parents=True, exist_ok=True)
root = Path(tempfile.mkdtemp(prefix='run-', dir=base))
expected = {
    'vc1.0': dict(cl='CL.EXE', c1='C13232.EXE', c1xx='C1XX3232.EXE',
                  c2='C23232.EXE', link='LINK.EXE', cvpack='CVPACK.EXE',
                  rc='RC.EXE', rcpp='RCPP.EXE'),
    'vc5.10': dict(rc='RC.EXE'),
}
probe = root / 'probe.py'
probe.write_text('''import json, os, sys
from pathlib import Path
flags=os.environ['MAKEFLAGS']
assert '--jobserver-auth=' in flags, flags
auth=flags.split('--jobserver-auth=',1)[1].split()[0]
if not auth.startswith('fifo:'):
    for fd in auth.split(','): os.fstat(int(fd))
project=Path.cwd()
(project.parent/'results'/project.name).write_text(json.dumps(dict(output=sys.argv[1],flags=flags)))
''')
results = []
for toolchain, programs in expected.items():
    directory = root / toolchain
    directory.mkdir()
    (directory / 'results').mkdir()
    shutil.copyfile(REPO / 'decomp' / toolchain / 'Makefile', directory / 'Makefile')
    for project in set(programs) | {'cvtres', 'cvppack', 'rcdll'}:
        child = directory / project
        child.mkdir()
        (child / 'Makefile').write_text(
            '.PHONY: native-bin\nnative-bin:\n\t+python3 ' + str(probe) + " '$(NATIVE_BINARY)'\n")
    command = ['make', '-j4', 'native-bins', 'RECON_TOOL_DIR=' + str(TOOL)]
    begin = time.monotonic()
    run = subprocess.run(command, cwd=directory, capture_output=True, text=True)
    seconds = time.monotonic() - begin
    (directory / 'make.log').write_text(run.stdout + run.stderr)
    assert run.returncode == 0, run.stdout + run.stderr
    observed = {p.name: json.loads(p.read_text()) for p in (directory / 'results').iterdir()}
    assert set(observed) == set(programs), observed
    for project, basename in programs.items():
        assert observed[project]['output'] == str(directory / 'build/native/bin' / basename)
    results.append(dict(toolchain=toolchain, command=command, seconds=seconds,
                        programs=programs, observed=observed))
(root / 'receipt.json').write_text(json.dumps(results, indent=2) + '\n')
print('PASS VC1/VC5 coordinator selection, exact outputs and live -j4 jobserver')
print('Results:', root)
