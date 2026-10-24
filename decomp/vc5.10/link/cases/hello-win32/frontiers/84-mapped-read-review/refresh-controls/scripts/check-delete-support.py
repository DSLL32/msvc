#!/usr/bin/env python3
"""Native/read-only support-controller identity and scope rejection controls."""
import copy,importlib,json,time
from pathlib import Path
from accept import trial_for
m=importlib.import_module('delete-support');project=Path(__file__).resolve().parents[1]
current=json.loads((project/'cases/help/operator-delete/first-target/functions.json').read_text());current['accepted']=[name for name in current['accepted']if name not in ('_free_buffer4',m.NAME)];runtime=json.loads((project/'runtime.json').read_text());record=next(r for r in runtime if r['name']==m.NAME);proof=json.loads(m.PROOF.read_text())
start=time.perf_counter();positive=m.check(record);trial=trial_for(current,'_free_buffer4',[m.NAME]);assert trial['accepted']==current['accepted']+['_free_buffer4',m.NAME]
checks=[]
def deny(name,call):
 try:call()
 except (AssertionError,ValueError,KeyError):checks.append(name);return
 raise AssertionError('accepted invalid support: '+name)
for field,value in [('classification','crt'),('classification','application'),('library','OTHER.LIB'),('start',0x4651b5),('end',0x4651bd),('status','stub')]:
 changed=dict(record);changed[field]=value;deny('support '+field+' '+str(value),lambda r=changed:m.check(r))
for field,value in [('identity','msvcrt.dll!free'),('abi','stdcall void(void*)')]:
 changed=copy.deepcopy(proof);changed[field]=value;deny('wrong '+field,lambda p=changed:m.check(record,p))
changed=copy.deepcopy(proof);changed['target_thunk']['iat']=0x40121c;deny('swapped IAT slot',lambda:m.check(record,changed))
deny('unrelated support name',lambda:trial_for(current,'_free_buffer4',['import:invented']))
deny('support with unrelated app',lambda:trial_for(current,'_free_buffer8',[m.NAME]))
deny('duplicate support',lambda:trial_for(current,'_free_buffer4',[m.NAME,m.NAME]))
changed=copy.deepcopy(current);changed['driver'].append(dict(name='_overlap_delete_support',start=0x4651b7,end=0x4651b8,status='stub'));deny('overlapping app/support ownership',lambda:trial_for(changed,'_free_buffer4',[m.NAME]))
print(json.dumps(dict(status='static support gate passed; application still requires fresh paired evidence',positive=positive,rejected=checks,elapsed_seconds=time.perf_counter()-start),indent=2))
