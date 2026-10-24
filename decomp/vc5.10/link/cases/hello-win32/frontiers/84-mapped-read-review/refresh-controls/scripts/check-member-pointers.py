#!/usr/bin/env python3
"""Reject mutations of the three reviewed standalone member-pointer globals."""
import argparse
import copy
import json
from pathlib import Path
import struct
from binary import PE
from member_pointers import CONTRACTS,project,check_static
from mask import BINARY,ROOT

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--native',type=Path,required=True)
parser.add_argument('--report',type=Path,required=True)
args=parser.parse_args()
configuration=json.loads((args.native/'functions.json').read_text())
state=json.loads((args.native/'state.json').read_text())
original=PE(ROOT/'toolchains/5.10/bin/LINK.EXE');rebuilt=PE(BINARY/'build/link.exe')
map_text=(BINARY/'build/link.map').read_text()
proof=check_static(original,rebuilt,map_text,configuration)
passed=[]
def rejected(label,operation):
    try:operation()
    except (AssertionError,KeyError):passed.append(label);return
    raise AssertionError('mutation accepted: '+label)
for name,contract in CONTRACTS.items():
    methods=state['member_methods']
    for mode,method in contract['methods'].items():
        original_value=struct.pack('<I',contract['addresses'][mode])
        native=bytes.fromhex(state['typed_member_values'][method])
        assert project(original_value,native,mode,contract,methods)==method
        code,adjustment=struct.unpack('<II',native);assert adjustment==0
        rejected(name+' nonzero receiver adjustment',lambda:project(original_value,struct.pack('<II',code,4),mode,contract,methods))
        rejected(name+' unknown native code',lambda:project(original_value,struct.pack('<II',1,0),mode,contract,methods))
        rejected(name+' unknown original code',lambda:project(struct.pack('<I',1),native,mode,contract,methods))
        rejected(name+' swapped source mode',lambda:project(original_value,native,'vxd' if mode=='pe' else 'pe',contract,methods))
        rejected(name+' ambiguous null',lambda:project(bytes(4),native,None,contract,methods))
    assert project(bytes(4),bytes(8),None,contract,methods) is None
    rejected(name+' adjusted null',lambda:project(bytes(4),struct.pack('<II',0,4),None,contract,methods))
    rejected(name+' wrong native width',lambda:project(bytes(4),bytes(4),None,contract,methods))
    for site,patch,label in [(contract['assignments'][0]+6,struct.pack('<I',0x459d00),'wrong assigned method'),(contract['calls'][0][0],b'\x90','changed receiver/argument sequence')]:
        mutated=PE(ROOT/'toolchains/5.10/bin/LINK.EXE');data=bytearray(mutated.data);offset=mutated.offset(site);data[offset:offset+len(patch)]=patch;mutated.data=bytes(data)
        rejected(name+' '+label,lambda:check_static(mutated,rebuilt,map_text,configuration))
args.report.write_text(json.dumps(dict(positive=True,negative_count=len(passed),rejected=passed,static_contract=proof),indent=2)+'\n')
print('%d scoped member-pointer mutations rejected.'%len(passed))
