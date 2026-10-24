#!/usr/bin/env python3
"""Focused offline denials for actual parser/mapped-owner ABI packets."""
import copy,json,time
from pathlib import Path
from bind_parser_abi import compare
c=Path('cases/direct-bind-parser/acceptance/closure');left=[json.loads(l)for l in(c/'original.jsonl').read_text().splitlines()];right=json.loads((c/'native.json').read_text());start=time.perf_counter();compare(left,right,complete=True);checks=[]
def deny(name,edit):
    a=copy.deepcopy(left);b=copy.deepcopy(right);edit(a,b)
    try:compare(a,b,complete=True)
    except(AssertionError,KeyError,IndexError,ValueError):checks.append(dict(control=name,status='rejected'));return
    raise AssertionError(('negativeaccepted',name))
for name,edit in[
 ('emptyinventory',lambda a,b:b.update(events=[])),
 ('missingformal',lambda a,b:b['events'][0]['arguments'].pop()),
 ('wrongargc',lambda a,b:b['events'][0]['arguments'].__setitem__(0,2)),
 ('wrongvectorowner',lambda a,b:b['events'][0]['arguments'].__setitem__(1,0)),
 ('wrongvectorstring',lambda a,b:b['events'][0]['vectors'].__setitem__(0,0)),
 ('wrongglobalimageowner',lambda a,b:b['events'][3]['globals'].__setitem__(0,0)),
 ('wrongpersistedpath',lambda a,b:b['events'][-1]['globals'].__setitem__(3,0)),
 ('wrongusage',lambda a,b:b['events'][-1]['globals'].__setitem__(1,0)),
 ('wrongverbose',lambda a,b:b['events'][-1]['globals'].__setitem__(4,0)),
 ('mapwrongmode',lambda a,b:b['events'][4]['arguments'].__setitem__(3,1)),
 ('mapwrongfilename',lambda a,b:b['events'][4]['arguments'].__setitem__(0,0)),
 ('mapwrongworkspace',lambda a,b:b['events'][4]['arguments'].__setitem__(1,0)),
 ('wrongmappedbase',lambda a,b:b['events'][5]['workspace'].__setitem__(4,0)),
 ('wrongsectioncount',lambda a,b:b['events'][5]['workspace'].__setitem__(7,3)),
 ('wrongheaderbyte',lambda a,b:b['events'][5].update(headers='00'*248)),
 ('wronglivemapping',lambda a,b:b['events'][5].update(mapping_live=0)),
 ('wrongunmapowner',lambda a,b:b['events'][6]['arguments'].__setitem__(0,0)),
 ('wrongcloseowner',lambda a,b:b['events'][8]['arguments'].__setitem__(0,0)),
 ('guardwrite',lambda a,b:b['events'][0].update(workspace_guards='00'+b['events'][0]['workspace_guards'][2:])),
 ('sourceguardwrite',lambda a,b:b['events'][0].update(names=b['events'][0]['names'][:-2]+'00')),
 ('wrongreturn',lambda a,b:b['events'][1].update(eax=1)),
 ('wrongstack',lambda a,b:b['events'][1].update(esp=0)),
 ('wrongnonvolatile',lambda a,b:b['events'][1].update(esi=0)),
 ('wrongerrno',lambda a,b:b['events'][0]['errors'].__setitem__(0,2)),
 ('wrongDOS',lambda a,b:b['events'][0]['errors'].__setitem__(1,2)),
 ('wrongLast',lambda a,b:b['events'][0]['errors'].__setitem__(2,2)),
 ('wrongCW',lambda a,b:b['events'][0].update(fp_control=0x37f)),
 ('wrongSW',lambda a,b:b['events'][0].update(fp_status=0x100)),
 ('wrongtags',lambda a,b:b['events'][0].update(fp_tag=0xfffe)),
 ]:deny(name,edit)
b=copy.deepcopy(right);b['events'][-1]['stack_argument_words'][0]=17;compare(left,b,complete=True)
b=copy.deepcopy(right);b['events'][5]['workspace'][0]=0x12345678;compare(left,b,complete=True)
print(json.dumps(dict(status='pass',checks=checks,allowed_controls=['rawpostcallbyvalueparameter','unread-unwritten-BoundImage-firstword'],seconds=time.perf_counter()-start,scope='Frozenactualpackets only; no runtime/original/Wine/GDB. No error orFP qualification.'),indent=2))
