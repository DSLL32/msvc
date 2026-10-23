#!/usr/bin/env python3
"""Read-only real C1XX PGO-domain CLI fixture plus metadata mutation controls.

Audits the original PE against itself, with explicit symbol/index metadata. This
checks auditor discovery/ownership/ABI policies, not reconstruction acceptance.
No PE is modified; no target, compiler, debugger, or Wine process is launched.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import subprocess
import time

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[2])
parser.add_argument('--binary',type=Path,default=Path(__file__).parent/'target/release/graph-audit')
parser.add_argument('--scratch',type=Path,required=True)
parser.add_argument('--report',type=Path,required=True)
a=parser.parse_args();repo=a.repo.resolve();binary=a.binary.resolve();scratch=a.scratch.resolve()
assert scratch.is_relative_to(Path.home()/'.ai-tmp'),'scratch must be under ~/.ai-tmp'
scratch.mkdir(parents=True,exist_ok=True)
fixture=json.loads((Path(__file__).parent/'fixtures/code-chunks.json').read_text())['targets'][0]
original=repo/fixture['path'];assert hashlib.sha256(original.read_bytes()).hexdigest()==fixture['sha256']
drivers=[dict(name=f['name'],start=f['start'],end=f['end'],code_chunks=f['code_chunks'],rebuilt_code_chunks=f['code_chunks'])for f in fixture['functions']]
cfg=dict(target=dict(path=fixture['path'],sha256=fixture['sha256']),driver=drivers,accepted=[f['name']for f in drivers])
runtime=[dict(name='_DllMain@12',start=0x1042e52a,end=0x1042e54d)]
(scratch/'runtime.json').write_text(json.dumps(runtime));(scratch/'metadata.json').write_text(json.dumps(dict(schema=1,functions={},callback_tables=[])))
(scratch/'fixture.map').write_text(''.join(' 0001:%08x %s %08x\n'%(f['start']-0x10400000,f['name'],f['start'])for f in drivers+runtime))
base=[str(binary),'--repo',str(repo),'--project',str(scratch),'--profile','cl','--original',str(original),'--rebuilt',str(original),'--map',str(scratch/'fixture.map'),'--runtime',str(scratch/'runtime.json'),'--rebuilt-runtime',str(scratch/'runtime.json'),'--metadata',str(scratch/'metadata.json'),'--config',str(scratch/'functions.json')]
flags=['--check-ret','--check-references','--check-ownership','--check-extents','--check-accepted-extents']
results=[]
def run(label,config,expect=True,extra=None):
    (scratch/'functions.json').write_text(json.dumps(config))
    cmd=base+(flags if extra is None else extra)
    start=time.perf_counter();p=subprocess.run(cmd,capture_output=True,text=True);elapsed=time.perf_counter()-start
    assert (p.returncode==0)==expect,(label,p.stderr)
    report=json.loads(p.stdout)if p.returncode==0 else None
    results.append(dict(case=label,accepted=p.returncode==0,seconds=elapsed,diagnostic=p.stderr.strip()))
    return report
positive=run('real_C1XX_two_split_functions',cfg)
assert positive['checked_functions']==2
for side in ['original','rebuilt']:
    domain=positive['owned_code_chunks'][side]
    assert domain['logical_function_count']==2 and domain['interval_count']==4
    for f in drivers:assert domain['functions'][f['name']]==[[f['start'],f['end']],*f['code_chunks']]
assert positive['functions']['__DllMainCRTStartup@12']['edges']==sorted(['__CRT_INIT@12','_DllMain@12','indirect'])
for label in ['missing_chunk','overlap_primary','overlap_foreign','split_cold_ret','outside_file','empty_interval','malformed_chunks','malformed_rebuilt_chunks']:
    config=copy.deepcopy(cfg);f=config['driver'][0]
    if label=='missing_chunk':f.pop('code_chunks')
    elif label=='overlap_primary':f['code_chunks']=[[f['start'],f['end']]]
    elif label=='overlap_foreign':f['code_chunks']=[[drivers[1]['start'],drivers[1]['end']]]
    elif label=='split_cold_ret':f['code_chunks'][0][1]=0x1046acbb
    elif label=='outside_file':f['code_chunks']=[[0x20400000,0x20400010]]
    elif label=='empty_interval':f['code_chunks']=[[0x1046acb3,0x1046acb3]]
    elif label=='malformed_chunks':f['code_chunks']='not an interval list'
    else:f['rebuilt_code_chunks']='not an interval list'
    run(label,config,False)
run('legacy_rejects_fragment_metadata',cfg,False,['--discovery','legacy'])
a.report.write_text(json.dumps(dict(scope=__doc__,binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),target_sha256=fixture['sha256'],positive_report=positive,results=results),indent=2)+'\n')
print('Real C1XX split-function CLI fixture: 2 vertices, 4 intervals per side; 9 rejection controls passed.')
