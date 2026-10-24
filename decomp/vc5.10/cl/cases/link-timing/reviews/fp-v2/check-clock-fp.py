#!/usr/bin/env python3
"""Focused cross-image FP negatives, retaining the actual execution records."""
import argparse,copy,json,struct,time
from pathlib import Path
from clock_provider import verify
from clock_state import compare_fp

parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('directory',type=Path);args=parser.parse_args()
directory=args.directory;path=directory/'native-clock-gdb.json';original=path.read_bytes();passed=[];started=time.perf_counter()
verify(directory)
try:
    for register in['fctrl','fstat','ftag','fioff','fooff','fop','fiseg','foseg']+['st%d'%i for i in range(8)]:
        value=json.loads(original)
        # Corrupt both phases equally: mere within-process preservation must
        # not conceal different architectural inputs or selector premises.
        for when in('before','after'):
            raw=bytearray.fromhex(value['calls'][0][when]['x87_registers'][register]);raw[0]^=1
            value['calls'][0][when]['x87_registers'][register]=raw.hex()
            if register=='fctrl':value['calls'][0][when]['fctrl']^=1
        path.write_text(json.dumps(value,indent=2)+'\n')
        try:verify(directory)
        except(AssertionError,KeyError,ValueError):passed.append('both phases architectural '+register)
        else:raise AssertionError('accepted different FP '+register)
finally:path.write_bytes(original)
response=json.loads((directory/'clock-response.json').read_bytes())
saved=bytes.fromhex(response['clock']['calls'][0]['before']['FloatSave_hex'])
registers=json.loads(original)['calls'][0]['before']['x87_registers'];compare_fp(saved.hex(),registers)
for label,offset,name,new in[('nonempty tag',8,'ftag',0),('live instruction pointer',12,'fioff',1),('live data pointer',20,'fooff',1)]:
    changed=bytearray(saved);struct.pack_into('<I',changed,offset,new);native=copy.deepcopy(registers)
    native[name]=new.to_bytes(4,'little').hex()
    try:compare_fp(changed.hex(),native)
    except AssertionError:passed.append('coherently invalid qualification '+label)
    else:raise AssertionError('accepted widened qualification '+label)
for label,offset in[('original CS',16),('original DS',24)]:
    changed=bytearray(saved);changed[offset]^=1
    try:compare_fp(changed.hex(),registers)
    except AssertionError:passed.append('unknown '+label)
    else:raise AssertionError('accepted unknown original selector')
verify(directory)
print(json.dumps({'passed':True,'negative_controls':len(passed),'seconds':time.perf_counter()-started,
    'controls':passed,'scope':'Validator-only strengthening over unchanged original/native raw; no execution rerun'}))
