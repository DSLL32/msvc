#!/usr/bin/env python3
"""Check actual two-node job input before the real grouped-stage reversal."""
import argparse,copy,json,struct
from pathlib import Path
from frontier_state import decode,snapshot
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('original',type=Path);parser.add_argument('native',type=Path);args=parser.parse_args()
expected=decode(args.original,'original')['fields'];assert expected==decode(args.native,'native')['fields']
assert expected['frontier']=='advance_jobs' and len(expected['parameters']['jobs'])==2
assert expected['globals']['driver']['current_jobs']==[{'ref':1},{'ref':2}]
passed=[]
for platform,directory in (('original',args.original),('native',args.native)):
    raw=json.loads((directory/'frontier-memory.json').read_text());memory={}
    for row in raw['ranges']:
        for offset,byte in enumerate(bytes.fromhex(row['data'])):
            address=row['address']+offset;assert address not in memory or memory[address]==byte;memory[address]=byte
    def word(address):return struct.unpack('<I',bytes(memory[address+i]for i in range(4)))[0]
    first=raw['context']['arguments'][0];second=word(first);assert first!=second and second and word(second)==0
    def patch(memory,address,value):
        for i,byte in enumerate(struct.pack('<I',value)):memory[address+i]=byte
    def negative(name,change):
        context=copy.deepcopy(raw['context']);changed=memory.copy();change(context,changed)
        try:actual=snapshot(lambda a,n:bytes(changed[a+i]for i in range(n)),context)
        except (AssertionError,KeyError,TypeError):passed.append(platform+': '+name);return
        assert actual!=expected,'changed grouped input ignored: '+name
        passed.append(platform+': '+name)
    negative('NULL head',lambda c,m:c['arguments'].__setitem__(0,0))
    negative('truncate list',lambda c,m:patch(m,first,0))
    negative('cycle list',lambda c,m:patch(m,second,first))
    negative('second node used as head',lambda c,m:c['arguments'].__setitem__(0,second))
    negative('source sequence',lambda c,m:patch(m,word(first+4)+12,0x10000))
print(json.dumps({'passed':True,'negative_controls':len(passed),'controls':passed}))
