"""Check encountered runtime intrinsic ISA values and original switch extent."""
import hashlib
import json
import struct
from pathlib import Path
from binary import PE
from pe_indexes import publics

original = PE('../../../toolchains/1.0-nt/bin/C23232.EXE')
rebuilt = PE('build/c2.exe')
# Independent numeric ISA values, not parsed from the source enum or map.
rows = [(107,214),(106,214),(111,215),(110,215),
        (92,140),(93,140),(71,76),(0xffffffff,0)]
expected = b''.join(struct.pack('<II',*row) for row in rows)
assert original.read(0x80238,64) == expected
address = publics()['_runtime_intrinsic_opcodes']
assert rebuilt.read(address,64) == expected
routes = {
    0x1d0a0:{0x5c,0x5d,0x6a,0x6b,0x6e,0x6f},
    0x1d0d3:{0x60,0x61},0x1d1b0:{0x64,0x65,0x68,0x69},
    0x1d200:{0x72,0x73},0x1d236:{0x82,0x83},0x1d265:{0x84,0x85}}
targets = struct.unpack('<42I',original.read(0x1d304,168))
for intrinsic,target in enumerate(targets,0x5c):
    expected_target = next((address for address,ids in routes.items() if intrinsic in ids),0x1d2a0)
    assert target == expected_target,(intrinsic,target,expected_target)
receipt = dict(target_sha256=hashlib.sha256(original.data).hexdigest(),
    rebuilt_sha256=hashlib.sha256(rebuilt.data).hexdigest(),
    map=dict(original=['0x80238','0x80278'],canonical=hex(address),rows=rows),
    switch=dict(start='0x1d304',end='0x1d3ac',entries=42,
                routes={hex(address):sorted(ids) for address,ids in routes.items()},default='0x1d2a0'),
    instruction_constants='Independent numeric ISA IDs: FSIN214 FCOS215 FSQRT140 FMULP76',
    executes_reference=False)
Path('cases/runtime-intrinsic-data-validation.json').write_text(json.dumps(receipt,indent=2)+'\n')
print('Runtime intrinsic64-byte typed map and all42 original dispatch entries match')
