"""Compare encountered typed intrinsic records in original and canonical images.

Only explicitly named contextual records are read; no reference process runs.
"""
from pathlib import Path
import hashlib
import json
import struct
from binary import PE
from pe_indexes import publics

records = json.loads(Path('cases/intrinsic-descriptors.json').read_text())
original = PE('../../../toolchains/1.0-nt/bin/C23232.EXE')
rebuilt = PE('build/c2.exe')
names = publics()

def string(image, address):
    data = bytearray()
    while True:
        value = image.read(address, 1)[0]
        if not value:
            return bytes(data)
        data.append(value)
        address += 1
        assert len(data) < 256, 'unterminated intrinsic command/runtime name'

checked = []
for record in records:
    start = 0x839d8 + record['id'] * 16
    assert start == int(record['original'], 16)
    expected = (record['result_type'], 0, record['commands'].encode(),
                record['runtime_name'].encode('latin1'), record['flags'])
    sides = []
    for image, address in [(original, start), (rebuilt, names['_'+record['name']])]:
        result, reserved, commands, runtime, flags = struct.unpack('<HHIII', image.read(address, 16))
        actual = result, reserved, string(image, commands), string(image, runtime), flags
        assert actual == expected, (record['name'], hex(address), actual, expected)
        sides.append(hex(address))
    checked.append(dict(name=record['name'], id=record['id'], addresses=sides, extent_bytes=16))
# The generated-runtime helper has a bounded24-way inline-floating switch.
# Verify both its extent and every target independently of source switch order.
inline_ids = {0x6e,0x6f,0x72,0x73,0x82,0x83,0x84,0x85}
targets = struct.unpack('<24I', original.read(0x63ec0, 96))
assert all(target == (0x63eb8 if 0x6e+index in inline_ids else 0x63d85)
           for index,target in enumerate(targets))
receipt = dict(method='Original/canonical typed record, recipe and runtime-name value checks',
               checked=checked, generated_inline_switch=dict(start='0x63ec0',end='0x63f20',entries=24,inline_ids=sorted(inline_ids)), target_sha256=hashlib.sha256(original.data).hexdigest(),
               rebuilt_sha256=hashlib.sha256(rebuilt.data).hexdigest(),executes_wine=False,
               address_identity='Interpreter does not expose descriptor addresses or perform arithmetic between descriptors')
Path('cases/intrinsic-descriptor-validation.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(receipt)
