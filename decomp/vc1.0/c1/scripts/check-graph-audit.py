#!/usr/bin/env python3
"""Reject graph, ABI, mapping, extent and mask mutations in private scratch."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile
import time
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from images import indexes

BINARY = Path(__file__).resolve().parents[1]
ROOT = BINARY.parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--directory',type=Path)
args = parser.parse_args()
work = args.directory or Path(tempfile.mkdtemp(prefix='c1-graph-',dir=Path.home()/'.ai-tmp'))
work.mkdir(parents=True,exist_ok=True)
original,rebuilt,config,oi,ri = indexes()
tool = ROOT/'tooling/graph-audit/target/release/graph-audit'
paths = {'exe':work/'c1.exe','map':work/'c1.map','config':work/'functions.json'}
saved = {'exe':(BINARY/'build/c1.exe').read_bytes(),
         'map':(BINARY/'build/c1.map').read_bytes(),
         'config':(BINARY/'functions.json').read_bytes()}
def restore():
    for name,path in paths.items(): path.write_bytes(saved[name])
flags = ['--discovery','cfg','--check-ret','--check-references','--check-ownership','--check-extents']
command = [str(tool),'--repo',str(ROOT),'--project',str(BINARY),'--profile','c1',
           '--rebuilt',str(paths['exe']),'--map',str(paths['map']),
           '--config',str(paths['config']),*flags]
results = []
def run(name,expected=False,mask=False):
    start=time.perf_counter()
    cmd = ['python3',str(BINARY/'scripts/audit-text-masks.py'),'--config',str(paths['config'])] if mask else command
    result = subprocess.run(cmd,cwd=BINARY,capture_output=True,text=True)
    assert (result.returncode == 0) == expected,(name,result.stdout[-2000:],result.stderr)
    results.append(dict(test=name,accepted=expected,seconds=time.perf_counter()-start,
                        diagnostic=result.stderr.strip()[-2000:]))
restore()
run('canonical-positive',True)
cs=Cs(CS_ARCH_X86,CS_MODE_32)
functions=[r for r in ri if r['name'] in config['accepted'] and r['name'] in {f['name']for f in config['driver']}]
call=next(i for f in functions for i in cs.disasm(rebuilt.read(f['start'],f['end']-f['start']),f['start']) if i.mnemonic=='call'and len(i.bytes)==5 and i.bytes[0]==0xe8)
b=bytearray(saved['exe']);struct.pack_into('<I',b,rebuilt.offset(call.address)+1,(0xdeadbeef-call.address-5)&0xffffffff);paths['exe'].write_bytes(b)
run('wrong-direct-callee')
restore()
name='_mark_pch_pointer_array@16';record=next(r for r in ri if r['name']==name)
b=bytearray(saved['exe']);ret=next(i for i in cs.disasm(rebuilt.read(record['start'],record['end']-record['start']),record['start'])if i.mnemonic=='ret'and len(i.bytes)==3);struct.pack_into('<H',b,rebuilt.offset(ret.address)+1,8);paths['exe'].write_bytes(b)
run('regressed-two-slot-PCH-ABI')
restore()
s=paths['map'].read_text();s,n=re.subn(r'(?m)^(\s+[0-9a-f]{4}:[0-9a-f]{8}\s+)'+re.escape(name)+r'(\s+[0-9a-f]{8}.*)$',r'\1__missing_PCH_public\2',s);assert n;paths['map'].write_text(s)
run('missing-public-mapping')
restore()
c=json.loads(saved['config']);next(f.update(end=0x444b2)for f in c['driver']if f['name']==name);paths['config'].write_text(json.dumps(c))
run('split-original-RET-operand')
restore()
c=json.loads(saved['config']);next(f.update(shared=[])for f in c['driver']if f['name']=='_fold_floating_binary@12');paths['config'].write_text(json.dumps(c))
run('masked-external-floating-switch-table',mask=True)
restore()
receipt=dict(status='pass',tool_sha256=hashlib.sha256(tool.read_bytes()).hexdigest(),canonical_inputs={name:hashlib.sha256(data).hexdigest()for name,data in saved.items()},flags=flags,checks=results,policy='Scratch mutations only; no Wine, original execution or working-tree artifact changes.')
(work/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(len(results),'C1 graph/ABI/mask checks passed:',work/'receipt.json')
