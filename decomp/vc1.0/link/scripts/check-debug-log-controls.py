#!/usr/bin/env python3
"""Focused offline denials against the frozen actual diagnostic-log packet."""
import copy,json,time
from pathlib import Path
from debug_log_abi import compare,COUNTS
c=Path('cases/direct-debug-log/acceptance/complete')
left=[json.loads(l)for l in(c/'original.jsonl').read_text().splitlines()]
left=[e for e in left if e.get('event')in ['entry','return','frontier']and e.get('name')in COUNTS]
right=json.loads((c/'native.json').read_text());start=time.perf_counter()
assert compare(left,right,complete=True)['matched_observations']==36
checks=[]
def event(events,name,kind):return next(e for e in events if e['name']==name and e['event']==kind)
def flag_other(a,b):
    for e in[a[0],b['events'][0]]:
        raw=bytearray.fromhex(e['debug_flags']);raw[99]=1;e['debug_flags']=raw.hex()
def missing(a,b):
    a[:]=[e for e in a if e['name']!='perror']
    b['events']=[e for e in b['events']if e['name']!='perror']
for name,edit in[
 ('wrongactualarraybase',lambda a,b:a[0].update(debug_flags_address=0x5c06c)),
 ('bothsidesmutateotherflagbyte',flag_other),
 ('wrongoriginalpubliccaller',lambda a,b:event(a,'perror','entry').update(caller=0x3a944)),
 ('wrongnativepubliccaller',lambda a,b:event(b['events'],'perror','entry').update(actual_caller='other')),
 ('missingbothpublicpairs',missing),
 ('wrongsetterindex',lambda a,b:event(b['events'],'set_debug_flag','entry')['arguments'].__setitem__(0,3)),
 ('wrongsetterbyte',lambda a,b:event(b['events'],'set_debug_flag','entry')['arguments'].__setitem__(1,0)),
 ('inventedvariadicword',lambda a,b:event(b['events'],'debug_printf','entry').update(variadic_words_consumed=1)),
 ('precisioncontrol',lambda a,b:b['events'][0].update(fp_control=0x37f)),
 ('C0statusstrict',lambda a,b:b['events'][0].update(fp_status=0x100)),
 ('nonemptyFPtag',lambda a,b:b['events'][0].update(fp_tag=0xfffc)),
 ('changedLastError',lambda a,b:event(b['events'],'perror','return')['errors'].__setitem__(2,0)),
 ('bothsideschangederrortransition',lambda a,b:(event(a,'perror','return')['errors'].__setitem__(1,2),event(b['events'],'perror','return')['errors'].__setitem__(1,2))),
 ('returnstack',lambda a,b:event(b['events'],'open_debug_log','return').update(esp=0)),
 ('nonvolatile',lambda a,b:event(b['events'],'open_debug_log','return').update(esi=0)),
 ('wrongFILEdescriptor',lambda a,b:event(b['events'],'fopen','return')['streams'][2]['raw'].__setitem__(4,0)),
 ('wrongFILEpending',lambda a,b:event(b['events'],'debug_printf','return')['streams'][0].update(pending='00')),
 ('inputguard',lambda a,b:b['events'][0].update(names=b['events'][0]['names'][:-2]+'00')),
]:
    a,b=copy.deepcopy(left),copy.deepcopy(right);edit(a,b)
    try:compare(a,b,complete=True)
    except(AssertionError,IndexError,KeyError,ValueError):checks.append(dict(control=name,status='rejected'));continue
    raise AssertionError(('negativeaccepted',name))
print(json.dumps(dict(status='pass',checks=checks,seconds=time.perf_counter()-start,scope='Frozen actual packets only; no runtime/original/Wine/GDB. No error/FP qualification.'),indent=2))
