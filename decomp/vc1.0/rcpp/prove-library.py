#!/usr/bin/env python3
"""Prove the reused IOINIT object against RCPP code, calls and initial data."""
import hashlib
import json
import struct
from pathlib import Path
from binary import PE, COFF, archive, match_section, u32
from compare import imports, wt

pe = PE(wt.TC / 'bin/RCPP.EXE')
member, payload = next((n, d) for n, d in archive(wt.TC/'lib/LIBC.LIB') if n.endswith('IOINIT.OBJ'))
obj = COFF(payload)
text = next(s for s in obj.sections if s['flags'] & 0x20)
data = next(s for s in obj.sections if s['name'] == '.data')
start, data_start = 0x01008550, 0x0101f4c0
assert match_section(pe.read(start, len(text['data'])), text) == [0]
assert pe.read(data_start, len(data['data'])) == data['data']
thunks = {f['name'][7:]: f['start'] for f in imports(pe)}
proof = []
for off, sym, kind in text['relocs']:
    value = u32(pe.data, pe.offset(start+off))
    addend = u32(text['data'], off)
    if kind == 20:
        destination = start + off + 4 + struct.unpack('<i', pe.read(start+off, 4))[0]
        name = sym['name'][1:].rsplit('@', 1)[0]
        assert destination == thunks[name] + addend
        proof.append(dict(offset=off, kind='REL32', symbol=sym['name'], target=destination))
    elif kind == 6:
        assert sym['name'] == '.data'
        assert value == data_start + addend
        proof.append(dict(offset=off, kind='DIR32', symbol='.data', target=value))
    else:
        raise AssertionError(kind)
result = dict(target_sha256=hashlib.sha256(pe.data).hexdigest(), member=member,
              member_sha256=hashlib.sha256(payload).hexdigest(),
              code_start=start, code_bytes=len(text['data']),
              data_start=data_start, data_bytes=len(data['data']),
              all_nonrelocation_bytes_equal=True, initial_data_exact=True, relocations=proof)
Path('library-proof.json').write_text(json.dumps(result, indent=2)+'\n')
print('IOINIT: exact code/data and all 12 relocation destinations verified.')

member, payload = next((n, d) for n, d in archive(wt.TC/'lib/LIBC.LIB') if n.endswith('STDENVP.OBJ'))
obj = COFF(payload)
text = next(s for s in obj.sections if s['flags'] & 0x20)
start = 0x01009640
assert match_section(pe.read(start, len(text['data'])), text) == [0]
targets = {'_malloc': 0x01008960, '__amsg_exit': 0x01008ee0,
           '__aenvptr': 0x010121a4, '__environ': 0x0101f3b4}
proof = []
for off, sym, kind in text['relocs']:
    if kind == 20:
        value = start + off + 4 + struct.unpack('<i', pe.read(start+off, 4))[0]
        addend = struct.unpack_from('<i', text['data'], off)[0]
    elif kind == 6:
        value = u32(pe.data, pe.offset(start+off))
        addend = u32(text['data'], off)
    else:
        raise AssertionError(kind)
    assert value == targets[sym['name']] + addend
    proof.append(dict(offset=off, kind='REL32' if kind == 20 else 'DIR32',
                      symbol=sym['name'], target=value))
result = [result, dict(target_sha256=hashlib.sha256(pe.data).hexdigest(), member=member,
                      member_sha256=hashlib.sha256(payload).hexdigest(), code_start=start,
                      code_bytes=len(text['data']), all_nonrelocation_bytes_equal=True,
                      relocations=proof)]
Path('library-proof.json').write_text(json.dumps(result, indent=2)+'\n')
print('STDENVP: exact code and all 8 relocation destinations verified.')

member, payload = next((n, d) for n, d in archive(wt.TC/'lib/LIBC.LIB') if n.endswith('STRNCPY.OBJ'))
obj = COFF(payload)
text = next(s for s in obj.sections if s['flags'] & 0x20)
start = 0x01010aa8
assert not text['relocs']
assert pe.read(start, len(text['data'])) == text['data']
result.append(dict(target_sha256=hashlib.sha256(pe.data).hexdigest(), member=member,
                   member_sha256=hashlib.sha256(payload).hexdigest(), code_start=start,
                   code_bytes=len(text['data']), all_bytes_equal=True))
Path('library-proof.json').write_text(json.dumps(result, indent=2)+'\n')
print('STRNCPY: all 37 bytes match the shipped object exactly.')

# Further reused stdio objects. Targets are independently named by their
# public CRT interface/caller relocation, not accepted by mere byte signature.
config = json.loads(Path('functions.json').read_text())
functions = {f['name']: f['start'] for f in config['functions']}
variables = {'__iob': 0x0101eea0, '__lastiob': 0x0101f120,
             '__commode': 0x0101fc80, '__cflush': 0x0101f660,
             '__nhandle': 0x0101f4c0, '_errno': 0x0101f370,
             '__doserrno': 0x0101f374, '__osfile': 0x0101f4c8}
for basename, start in (('FOPEN.OBJ', 0x0100ec30), ('STREAM.OBJ', 0x0100ec90),
                        ('_OPEN.OBJ', 0x0100ecf0), ('_FLSBUF.OBJ', 0x01008bb0),
                        ('LSEEK.OBJ', 0x010088c0), ('FGETC.OBJ', 0x0100e8c0)):
    member, payload = next((n, d) for n, d in archive(wt.TC/'lib/LIBC.LIB')
                           if n.rsplit('\\', 1)[-1] == basename)
    obj = COFF(payload)
    text = next(s for s in obj.sections if s['flags'] & 0x20)
    assert match_section(pe.read(start, len(text['data'])), text) == [0], member
    proof = []
    for off, sym, kind in text['relocs']:
        symbol = sym['name']
        if sym['section'] == text['index']:
            expected = start + sym['value']
        elif symbol in functions:
            expected = functions[symbol]
        elif symbol in variables:
            expected = variables[symbol]
        elif symbol.startswith('_') and '@' in symbol:
            expected = thunks[symbol[1:].rsplit('@', 1)[0]]
        else:
            raise AssertionError(symbol)
        if kind == 20:
            value = start + off + 4 + struct.unpack('<i', pe.read(start+off, 4))[0]
            addend = struct.unpack_from('<i', text['data'], off)[0]
        elif kind == 6:
            value = u32(pe.data, pe.offset(start+off))
            addend = u32(text['data'], off)
        else:
            raise AssertionError(kind)
        assert value == expected + addend, (member, symbol, hex(value), hex(expected))
        proof.append(dict(offset=off, kind='REL32' if kind == 20 else 'DIR32',
                          symbol=symbol, target=value))
    result.append(dict(target_sha256=hashlib.sha256(pe.data).hexdigest(), member=member,
                       member_sha256=hashlib.sha256(payload).hexdigest(), code_start=start,
                       code_bytes=len(text['data']), all_nonrelocation_bytes_equal=True,
                       relocations=proof))
Path('library-proof.json').write_text(json.dumps(result, indent=2)+'\n')
print('Six additional stdio objects match all code and relocation destinations.')
