#!/usr/bin/env python3
"""Read-only cached-import residual with targeted mutated-PE rejection controls."""
import importlib.util
import json
import struct
from common import HERE,ROOT,read,sha
from binary import PE

spec=importlib.util.spec_from_file_location('rc_iat',HERE/'scripts/iat-references.py')
iat=importlib.util.module_from_spec(spec);spec.loader.exec_module(iat)

def check():
    config=read(HERE/'functions.json')
    audit=read(HERE/'build/final-audit.json')
    rejected=[]
    for side,path in [('original',ROOT/config['target']['path']),('rebuilt',HERE/'build/rc.exe')]:
        pe=PE(path)
        assert sha(path)==audit['input_bindings'][side]
        for logical,proofs in audit['cached_import_residual'].items():
            proof=proofs[side]
            if side=='original':row=next(r for r in config['driver'] if r['name']==logical)
            else:
                import re
                publics={m[1]:int(m[2],16) for m in re.finditer(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',(HERE/'build/rc.map').read_text(),re.M)}
                target=next(r for r in config['driver'] if r['name']==logical)
                start=publics[target.get('symbol',logical)]
                row={'start':start,'end':min(a for a in publics.values() if a>start)}
            assert iat.cached_import(pe,row['start'],row['end'],logical)==proof
            for kind in ['slot','call-register','partial-register-write','branch-before-load','branch-after-restore']:
                changed=PE(path);data=bytearray(changed.data)
                if kind=='slot':
                    other=next(slot for slot,identity in iat.slots(pe).items() if tuple(identity)!=tuple(proof['identity']))
                    offset=pe.offset(proof['load']['va'])+2
                    data[offset:offset+4]=struct.pack('<I',other)
                elif kind=='call-register':
                    offset=pe.offset(proof['calls'][0]['va'])+1
                    data[offset]^=1
                elif kind=='partial-register-write':
                    instruction=next(ins for ins in proof['instruction_evidence'][1:] if len(bytes.fromhex(ins['bytes']))>=4)
                    offset=pe.offset(instruction['va'])
                    replacement=b'\x66'+(b'\xbe' if proof['register']=='esi' else b'\xbb')+b'\x01\x00'
                    size=len(bytes.fromhex(instruction['bytes']))
                    data[offset:offset+size]=replacement+b'\x90'*(size-4)
                else:
                    address=row['start'] if kind=='branch-before-load' else proof['restore']['va']+1
                    target=proof['calls'][0]['va'] if kind=='branch-before-load' else proof['calls'][-1]['va']
                    distance=target-address-2
                    assert -128<=distance<=127 and address+2<=row['end']
                    offset=pe.offset(address)
                    data[offset:offset+2]=b'\xeb'+bytes([distance&255])
                changed.data=bytes(data)
                try:iat.cached_import(changed,row['start'],row['end'],logical)
                except AssertionError as error:
                    expected={'slot':'Cached import load identity','call-register':'Unexpected register call','partial-register-write':'Cached register/alias overwritten','branch-before-load':'Branch bypasses cache load/restore','branch-after-restore':'Branch bypasses cache load/restore'}[kind]
                    assert expected in str(error),(kind,str(error))
                    rejected.append(side+':'+logical+':'+kind)
                else:raise AssertionError('Accepted wrong cached import '+kind)
    return {'status':'pass','rejected':rejected,'qualification':'specific callee-saved-register residual, separate from Rust CFG and --check-imports'}

if __name__=='__main__':print(json.dumps(check(),indent=2))
