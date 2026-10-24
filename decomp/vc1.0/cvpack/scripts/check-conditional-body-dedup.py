#!/usr/bin/env python3
"""Small offline controls for call-local identical-body validation reuse."""
import copy,json,time
from pathlib import Path
import seams
from state import BINARY,load,sha
config,pe=load();runtime=json.loads((BINARY/'runtime.json').read_text());records=json.loads((BINARY/'runtime-support.json').read_text())
original_graph=seams.graph;calls=[];cache=[]
def graph(*args,**kwargs):
    calls.append(args[1:]);return original_graph(*args,**kwargs)
seams.graph=graph
validate=seams.validate

def wrapped(c,p,r,record,bodies=None):
    if bodies is not None:cache[:]=[bodies]
    return validate(c,p,r,record,bodies)
seams.validate=wrapped
start=time.perf_counter();cached=seams.approved(config,pe,runtime);cached_seconds=time.perf_counter()-start;cached_calls=len(calls);assert cached_calls==1 and len(cache[0][1])==1
calls.clear();start=time.perf_counter();plain=[validate(config,pe,runtime,r)for r in records];plain_seconds=time.perf_counter()-start;assert cached==plain and len(calls)==len(records)
rejected=[]
def reject(name,operation):
    try:operation()
    except(AssertionError,KeyError):rejected.append(name)
    else:raise AssertionError('accepted '+name)
for name,key,value in [('name','name','wrong.dll'),('mode','mode',0x8000),('share','share',0x10)]:
    r=copy.deepcopy(records[0]);r['guard'][key]=value
    reject('warm-cache-policy-'+name,lambda r=r:validate(config,pe,runtime,r,cache[0]))
r=copy.deepcopy(records[0]);r['phase']=2
reject('warm-cache-wrong-phase',lambda:validate(config,pe,runtime,r,cache[0]))
r=copy.deepcopy(records[0]);r['input']['sha256']='0'*64
reject('warm-cache-input-hash',lambda:validate(config,pe,runtime,r,cache[0]))
r=copy.deepcopy(records[0]);r['evidence_sha256']='0'*64
reject('warm-cache-body-evidence-key',lambda:validate(config,pe,runtime,r,cache[0]))
class ChangedPE:
    def read(self,address,size):
        raw=pe.read(address,size)
        return bytes([raw[0]^1])+raw[1:]if address==records[0]['start']else raw
reject('warm-cache-changed-actual-body',lambda:validate(config,ChangedPE(),runtime,records[0],cache[0]))
changed=copy.deepcopy(runtime);analysis=json.loads((BINARY/records[0]['evidence']).read_text());callee=analysis['original_callees'][0]
entry=next(x for x in changed['functions']+changed['imports']if x['start']==callee['address']and x['name']==callee['identity']);entry['name']+='-changed'
reject('fresh-context-changed-callee',lambda:seams.approved(config,pe,changed))
changed=copy.deepcopy(config);changed.setdefault('support_unmasked',[]).append(dict(name='bad-overlap',start=0x17000,end=0x17001))
reject('fresh-context-changed-ownership',lambda:seams.approved(changed,pe,runtime))
receipt=dict(policy_records=len(records),cached_body_checks=cached_calls,uncached_body_checks=len(records),cached_seconds=cached_seconds,uncached_seconds=plain_seconds,identical_results=True,rejected=rejected,seams_sha256=sha(BINARY/'scripts/seams.py'),scope='Only identical fullbody/CFG/callee/ownership facts deduplicated within one approved() call; complete config/runtime/data/decode/source/PE fingerprint. Every case/name/input/mode/share/phase checked each record; direct validate remains uncached.')
(BINARY/'cases/runtime-variants/conditional-body-dedup.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt,indent=2))
