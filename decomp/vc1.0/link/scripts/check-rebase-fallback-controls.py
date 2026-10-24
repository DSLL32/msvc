#!/usr/bin/env python3
"""Offline actual-packet denials; no target/debugger/reference execution."""
import copy,importlib.util,json,time
from pathlib import Path
start=time.perf_counter();root=Path(__file__).resolve().parents[1];case=root/'cases/direct-rebase-fallback'
spec=importlib.util.spec_from_file_location('compare',root/'scripts/compare-rebase-fallback.py');cmp=importlib.util.module_from_spec(spec);spec.loader.exec_module(cmp)
original=[json.loads(l)for l in(case/'acceptance/complete/original-events.jsonl').read_text().splitlines()];native=json.loads((case/'acceptance/complete/native-state.json').read_text())
cmp.check(original,native);checks=[]
def deny(name,change):
    left,right=copy.deepcopy(original),copy.deepcopy(native);change(left,right)
    try:cmp.check(left,right)
    except (AssertionError,KeyError,IndexError):checks.append(dict(control=name,status='rejected'))
    else:raise AssertionError(name)
deny('missingbothstates',lambda l,r:(l.clear(),r['events'].clear()))
deny('wrongnativecaller',lambda l,r:r['events'][0].update(actual_caller='unproved_caller'))
deny('missingformal',lambda l,r:r['events'][0]['arguments'].pop())
deny('wrongcount',lambda l,r:r['events'][2]['arguments'].__setitem__(1,6))
deny('wrongdeltasign',lambda l,r:r['events'][4]['arguments'].__setitem__(3,0x18004))
deny('wrongpointerowner',lambda l,r:r['events'][0]['arguments'].__setitem__(2,r['events'][0]['page_address']))
deny('bothchangedpage',lambda l,r:(l[0].update(page='00'+l[0]['page'][2:]),r['events'][0].update(page='00'+r['events'][0]['page'][2:])))
deny('guardwrite',lambda l,r:r['events'][3].update(page_guard_after='00'*16))
deny('recordwrite',lambda l,r:r['events'][3].update(records='00'*16))
deny('wrongendpointer',lambda l,r:r['events'][3].update(eax=r['events'][3]['word_address']+12))
deny('wrongNULLreturn',lambda l,r:r['events'][7].update(eax=r['events'][7]['word_address']))
deny('returnstack',lambda l,r:r['events'][1].update(esp=r['events'][1]['esp']-16))
deny('nonvolatile',lambda l,r:r['events'][1].update(ebx=0))
deny('precisioncontrol',lambda l,r:r['events'][0].update(fp_control=0x37f))
deny('statusC0',lambda l,r:r['events'][0].update(fp_status=0x100))
deny('nonemptytag',lambda l,r:r['events'][0].update(fp_tag=0xfffc))
deny('errorstate',lambda l,r:r['events'][0].update(errors=[0,0,1224]))
row=dict(status='pass',controls=checks,seconds=time.perf_counter()-start,scope='Frozen actual8states; strictFP/errors/4formals/page/records/guards/endpointer/stack/nonvolatiles, no qualifications or runtime.')
(case/'checks.json').write_text(json.dumps(row,indent=2)+'\n');print(json.dumps(row))
