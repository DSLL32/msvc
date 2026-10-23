#!/usr/bin/env python3
"""Explicit original-only CVPACK probe with all MSPDB import references watched."""
import importlib.util
import argparse
import json
from pathlib import Path
import re
import shutil
import subprocess
import time
from state import BINARY,ROOT,load,sha
from binary import PE,u32

spec=importlib.util.spec_from_file_location('original_tool',BINARY/'scripts/wine-tool.py')
tool=importlib.util.module_from_spec(spec);spec.loader.exec_module(tool)
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--label',default='original-probe-0001')
parser.add_argument('--contract',action='store_true')
options=parser.parse_args();assert re.fullmatch(r'original-probe-[0-9]{4}',options.label)
output=BINARY/'cases/hello-debug-dll'/options.label;output.mkdir(exist_ok=False)
work=Path.home()/'.ai-tmp/msvc2/vc5.10/cvppack/hello-debug-dll'/options.label;work.mkdir(parents=True,exist_ok=False)
config,pe=load()
for source,name in [(ROOT/config['target']['path'],'CVPACK.EXE'),(BINARY/'build/debug-run.exe','debug-run.exe'),
    (BINARY/'cases/hello-debug-dll/preparation-0001/hello.dll','hello.dll')]+[(tool.TC/'bin'/name,name) for name in ('MSVCRT.DLL','MSPDB50.DLL')]:shutil.copyfile(source,work/name)
shutil.copyfile(work/'hello.dll',output/'input.dll')
(output/'runtime-plan').write_bytes(b'')
q=pe.offset(pe.base+u32(pe.data,pe.optional+104));imports={}
while u32(pe.data,q):
    ilt=u32(pe.data,q);dll=pe.string(pe.base+u32(pe.data,q+12));iat=u32(pe.data,q+16)
    if dll.lower()=='mspdb50.dll':
        i=0
        while u32(pe.data,pe.offset(pe.base+ilt+i*4)):
            imports[pe.base+iat+i*4]=pe.string(pe.base+u32(pe.data,pe.offset(pe.base+ilt+i*4))+2);i+=1
    q+=20
disasm=subprocess.check_output(['objdump','-d',str(ROOT/config['target']['path'])],text=True)
probes=[]
for line in disasm.splitlines():
    m=re.match(r'\s*([0-9a-f]+):\s+((?:[0-9a-f]{2} )+)\s*(.+)',line)
    if not m:continue
    operands=re.findall(r'0x([0-9a-f]+)',m[3])
    for address in (int(v,16) for v in operands):
        if address in imports:probes.append(dict(site=int(m[1],16),iat=address,symbol=imports[address],instruction=m[3]))
assert len(probes)==21
command=['wine',tool.windows(work/'debug-run.exe'),tool.windows(work/'CVPACK.EXE'),
    tool.windows(ROOT/config['target']['path'])+' /nologo hello.dll',tool.windows(output/'stdout'),tool.windows(output/'stderr'),
    tool.windows(output/'events'),tool.windows(work),tool.windows(output/'runtime-plan'),tool.windows(ROOT/config['target']['path']),
    tool.windows(work/'loaded-msvcrt.dll'),'0']
env=tool.environment();env['VC510_CVPACK_PROBES']=','.join('%x:pdb-reference-%d'%(r['site'],i) for i,r in enumerate(probes))
if options.contract:
    sites=[0x4102a2,0x4102d5,0x4102d8,0x4102f7,0x4102fa,0x41031a,0x41031d,0x4102b6,0x4010b7,0x4010c7]
    env['VC510_CVPACK_PROBES']+=','+','.join('%x:contract-%x'%(site,site) for site in sites)
    env['VC510_CVPACK_PDB_PROVIDER']=tool.windows(work/'loaded-mspdb50.dll')
begin=time.perf_counter();result=subprocess.run(command,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=60);elapsed=time.perf_counter()-begin
(output/'host-stdout').write_bytes(result.stdout);(output/'host-stderr').write_bytes(result.stderr)
assert result.returncode==0,(result.returncode,result.stdout,result.stderr)
shutil.copyfile(work/'hello.dll',output/'output.dll')
assert sha(work/'loaded-msvcrt.dll')=='e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210'
receipt=dict(schema=1,original_only=True,command=command,target_sha256=sha(work/'CVPACK.EXE'),debugger_sha256=sha(work/'debug-run.exe'),
    debugger_source_sha256=sha(BINARY/'scripts/debug-run.c'),seconds=elapsed,pdb_reference_probes=probes,actual_loaded_crt=dict(path=str(work/'loaded-msvcrt.dll'),sha256=sha(work/'loaded-msvcrt.dll')),
    files=[dict(name=p.name,bytes=p.stat().st_size,sha256=sha(p)) for p in output.iterdir()])
if options.contract:
    receipt['actual_loaded_mspdb']=dict(path=str(work/'loaded-mspdb50.dll'),sha256=sha(work/'loaded-mspdb50.dll'))
    receipt['contract_probe_sites']=sites
(output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
events=(output/'events').read_text()
print(json.dumps(dict(label=options.label,seconds=elapsed,input_sha256=sha(output/'input.dll'),output_sha256=sha(output/'output.dll'),
    exit=re.findall(r'^exit (\d+)$',events,re.M),pdb_hits={r['symbol']:events.count('pdb-reference-'+str(i)+' ') for i,r in enumerate(probes) if events.count('pdb-reference-'+str(i)+' ')},
    contract_records=len(re.findall(r'^widen ',events,re.M))),indent=2))
