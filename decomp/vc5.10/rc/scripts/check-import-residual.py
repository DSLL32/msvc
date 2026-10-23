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
                start=publics[logical]
                row={'start':start,'end':min(a for a in publics.values() if a>start)}
            assert iat.cached_import(pe,row['start'],row['end'],logical)==proof
            for kind in ['slot','call-register']:
                changed=PE(path);data=bytearray(changed.data)
                if kind=='slot':
                    other=next(slot for slot,identity in iat.slots(pe).items() if tuple(identity)!=tuple(proof['identity']))
                    offset=pe.offset(proof['load']['va'])+2
                    data[offset:offset+4]=struct.pack('<I',other)
                else:
                    offset=pe.offset(proof['calls'][0]['va'])+1
                    data[offset]^=1
                changed.data=bytes(data)
                try:iat.cached_import(changed,row['start'],row['end'],logical)
                except AssertionError:rejected.append(side+':'+logical+':'+kind)
                else:raise AssertionError('Accepted wrong cached import '+kind)
    return {'status':'pass','rejected':rejected,'qualification':'specific callee-saved-register residual, separate from Rust CFG and --check-imports'}

if __name__=='__main__':print(json.dumps(check(),indent=2))
