#!/usr/bin/env python3
"""Exercise actual argv bytes, storage layout and pass argument aliases."""
import argparse
import copy
import json
from pathlib import Path
import struct
from frontier_state import decode,snapshot

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('original',type=Path);parser.add_argument('native',type=Path,nargs='?');args=parser.parse_args()
expected=decode(args.original,'original')['fields'];assert expected['frontier']=='execute_pass'
if args.native:assert decode(args.native,'native')['fields']==expected
raw=json.loads((args.original/'frontier-memory.json').read_text());backing={}
for item in raw['ranges']:
    for i,byte in enumerate(bytes.fromhex(item['data'])):
        address=item['address']+i
        assert address not in backing or backing[address]==byte
        backing[address]=byte
assert snapshot(lambda a,n:bytes(backing[a+i]for i in range(n)),raw['context'])==expected
context=raw['context'];mode,record,name,environment,argv=context['arguments']
word=lambda a:struct.unpack('<I',bytes(backing[a+i]for i in range(4)))[0]
argv0=word(argv);passed=[]
def control(label,mutate):
    memory=backing.copy();state=copy.deepcopy(context);mutate(memory,state)
    try:actual=snapshot(lambda a,n:bytes(memory[a+i]for i in range(n)),state)
    except (AssertionError,KeyError):passed.append(label);return
    assert actual!=expected,'changed execution state accepted: '+label
    passed.append(label)
def put(memory,address,value):
    for i,byte in enumerate(struct.pack('<I',value)):memory[address+i]=byte
control('name copied instead of true pass-name alias',lambda m,c:c['arguments'].__setitem__(2,argv0))
control('argv0 aliases compiler-name allocation',lambda m,c:put(m,argv,name))
control('argv loses null terminator',lambda m,c:put(m,argv+4*26,argv0))
control('mode changed to process invocation',lambda m,c:c['arguments'].__setitem__(0,3))
control('option input byte changed',lambda m,c:m.__setitem__(word(argv+4)+2,ord('x')))
control('source input byte changed',lambda m,c:m.__setitem__(word(argv+16),ord('b')))
print('Actual26-token pass argv/storage/aliases match; %d semantic mutation controls pass'%len(passed))
