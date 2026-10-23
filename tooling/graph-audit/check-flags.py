#!/usr/bin/env python3
"""Focused real-PE flag tests on a scratch snapshot, without Wine."""
import argparse
import json
from pathlib import Path
import struct
import subprocess
import sys
from contextlib import contextmanager
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--snapshot',type=Path,required=True)
parser.add_argument('--binary',type=Path,default=Path(__file__).parent/'target/release/graph-audit')
parser.add_argument('--report',type=Path,required=True)
a=parser.parse_args();root=a.snapshot.resolve();project=root/'decomp/vc1.0/rc';binary=a.binary.resolve()
__import__('os').chdir(project)
sys.path.insert(0,str(project/'scripts'))
import reference_indexes
original,rebuilt,cfg,oi,ri=reference_indexes.indexes()
base=[str(binary),'--profile','rc','--repo',str(root),'--project',str(project)]
report=[]
def run(label,flags=(),accepted=True):
    p=subprocess.run(base+list(flags),cwd=project,capture_output=True,text=True)
    assert (p.returncode==0)==accepted,(label,p.stdout[-1000:],p.stderr)
    report.append(dict(test=label,flags=list(flags),accepted=accepted,diagnostic=p.stderr.strip()))
@contextmanager
def modified(*paths):
    saved={p:p.read_bytes()for p in paths}
    try:yield
    finally:
        for p,b in saved.items():p.write_bytes(b)
run('core-positive')
for flag in ('--check-ret','--check-references','--check-callbacks','--check-ownership','--check-extents','--check-instruction-overlap'):
    run('positive-'+flag,[flag])
exe=project/'build/rc.exe';config=project/'functions.json'
# A leaf stdcall function isolates exactly the optional RET check.
record=next(r for r in ri if r['name']=='_is_kanji_lead@4')
cs=Cs(CS_ARCH_X86,CS_MODE_32)
returns=[i for i in cs.disasm(rebuilt.read(record['start'],record['end']-record['start']),record['start'])if i.mnemonic=='ret'and len(i.bytes)==3]
assert returns
with modified(exe,config):
    subset=dict(cfg);subset['accepted']=['_is_kanji_lead@4'];config.write_text(json.dumps(subset))
    run('leaf-ret-positive',['--check-ret'])
    b=bytearray(exe.read_bytes())
    for i in returns:struct.pack_into('<H',b,rebuilt.offset(i.address)+1,8)
    exe.write_bytes(b)
    run('changed-ret-core-still-same-graph')
    run('changed-ret-optional-check-rejects',['--check-ret'],False)
# A table pointer change affects callbacks, not direct instruction call edges.
publics=reference_indexes.publics()
with modified(exe):
    b=bytearray(exe.read_bytes());struct.pack_into('<I',b,rebuilt.offset(publics['__cfltcvt_tab']),0xdeadbeef);exe.write_bytes(b)
    run('changed-callback-core-still-same-graph')
    run('changed-callback-optional-check-rejects',['--check-callbacks'],False)
# Optional non-overlap has a separate unit test with valid two-stream x86.
run('legacy-refuses-silently-ignored-flags',['--discovery','legacy','--check-ret'],False)
a.report.write_text(json.dumps(report,indent=2)+'\n')
print(len(report),'flag checks passed')
