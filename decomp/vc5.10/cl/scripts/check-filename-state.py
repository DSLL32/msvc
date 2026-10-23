#!/usr/bin/env python3
"""Check actual filename-helper parameters and the map-naming globals it reads."""
import argparse
import copy
import json
from pathlib import Path
from frontier_state import decode,snapshot
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('original',type=Path);parser.add_argument('native',type=Path);args=parser.parse_args()
expected=decode(args.original,'original')['fields'];assert expected==decode(args.native,'native')['fields']
assert expected['frontier']in ('file_directory','make_map_name') and expected['parameters']['name']=='out/path.dll'
passed=[]
for platform,directory in (('original',args.original),('native',args.native)):
    record=json.loads((directory/'frontier-memory.json').read_text());memory={}
    for row in record['ranges']:
        for offset,byte in enumerate(bytes.fromhex(row['data'])):
            address=row['address']+offset;assert address not in memory or memory[address]==byte;memory[address]=byte
    def negative(label,change):
        context=copy.deepcopy(record['context']);altered=memory.copy();change(context,altered)
        try:fields=snapshot(lambda a,n:bytes(altered[a+i]for i in range(n)),context)
        except (AssertionError,KeyError,TypeError):passed.append(platform+': '+label);return
        assert fields!=expected,'ignored actual filename input: '+label
        passed.append(platform+': '+label)
    negative('NULL output pointer',lambda c,m:c['arguments'].__setitem__(0,0))
    negative('NULL source name',lambda c,m:c['arguments'].__setitem__(1,0))
    negative('source name byte',lambda c,m:m.__setitem__(c['arguments'][1],ord('X')))
    if expected['frontier']=='make_map_name':
        negative('map requested flag',lambda c,m:m.__setitem__(c['globals']['map_requested'],m[c['globals']['map_requested']]^1))
        negative('NULL map name',lambda c,m:[m.__setitem__(c['globals']['map_name']+i,0)for i in range(4)])
        negative('map name byte',lambda c,m:m.__setitem__(int.from_bytes(bytes(m[c['globals']['map_name']+i]for i in range(4)),'little'),ord('X')))
print(json.dumps({'passed':True,'frontier':expected['frontier'],'negative_controls':len(passed),'controls':passed}))
