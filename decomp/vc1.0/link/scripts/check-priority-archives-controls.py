#!/usr/bin/env python3
"""Focused offline denials for actual priority-list ownership/ABI packets."""
import copy,json,time
from pathlib import Path
from priority_archives_abi import compare
c=Path('cases/direct-priority-archives/acceptance/closure');left=[json.loads(l)for l in(c/'original.jsonl').read_text().splitlines()];right=json.loads((c/'native.json').read_text());start=time.perf_counter();compare(left,right,complete=True);checks=[]
def deny(name,edit):
    a=copy.deepcopy(left);b=copy.deepcopy(right);edit(a,b)
    try:compare(a,b,complete=True)
    except(AssertionError,KeyError,IndexError,ValueError):checks.append(dict(control=name,status='rejected'));return
    raise AssertionError(('negativeaccepted',name))
for name,edit in[
 ('emptyinventory',lambda a,b:b.update(events=[])),
 ('missingformal',lambda a,b:b['events'][0]['arguments'].pop()),
 ('wrongtextowner',lambda a,b:b['events'][0]['arguments'].__setitem__(0,0)),
 ('wrongoriginalowner',lambda a,b:b['events'][0]['arguments'].__setitem__(1,0)),
 ('wrongprioritybyte',lambda a,b:b['events'][0]['arguments'].__setitem__(2,1)),
 ('wrongnodeedge',lambda a,b:b['events'][-1]['nodes'][0]['raw'].__setitem__(3,0)),
 ('wrongcount',lambda a,b:b['events'][-1]['list'].__setitem__(2,3)),
 ('wrongcursor',lambda a,b:b['events'][-1]['cursors'].__setitem__(0,0)),
 ('nodeunowned',lambda a,b:b['events'][-1]['nodes'][0].update(address=0)),
 ('sourceguardwrite',lambda a,b:b['events'][0].update(names=b['events'][0]['names'][:-2]+'00')),
 ('wrongreturnowner',lambda a,b:b['events'][1].update(eax=0)),
 ('wrongmainresult',lambda a,b:b['events'][-1].update(eax=1)),
 ('wrongstackcleanup',lambda a,b:b['events'][1].update(esp=0)),
 ('wrongnonvolatile',lambda a,b:b['events'][1].update(esi=0)),
 ('wrongargc',lambda a,b:b['events'][-1]['arguments'].__setitem__(0,3)),
 ('wrongargv',lambda a,b:b['events'][-1]['argv_names'].__setitem__(0,'other')),
 ('wrongerrno',lambda a,b:b['events'][0]['errors'].__setitem__(0,2)),
 ('wrongDOS',lambda a,b:b['events'][0]['errors'].__setitem__(1,2)),
 ('wrongLast',lambda a,b:b['events'][0]['errors'].__setitem__(2,2)),
 ('wrongCW',lambda a,b:b['events'][0].update(fp_control=0x37f)),
 ('wrongSW',lambda a,b:b['events'][0].update(fp_status=0x100)),
 ('wrongtags',lambda a,b:b['events'][0].update(fp_tag=0xfffe)),
 ]:deny(name,edit)
# Actual by-value stack representations are retained but have no immutable caller contract.
b=copy.deepcopy(right);b['events'][-1]['stack_argument_words'][0]=17;compare(left,b,complete=True)
# The unwritten, unconsumed +8 raw word has no invented initialization/equality guarantee.
b=copy.deepcopy(right);b['events'][-1]['nodes'][0]['raw'][2]=0x12345678;compare(left,b,complete=True)
print(json.dumps(dict(status='pass',checks=checks,allowed_controls=['postcall-byvalue-parameterword','unwritten-unconsumed-node8-raw'],seconds=time.perf_counter()-start,scope='Frozenpacketsonly; no runtime/original/GDB/Wine. Actual liveinputs/definedgraph/errors/FP strict.'),indent=2))
