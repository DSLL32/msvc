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

# STRNCMP's full leaf implementation has no relocations.
member, payload = next((n,d) for n,d in archive(wt.TC/'lib/LIBC.LIB') if n.endswith('STRNCMP.OBJ'))
obj = COFF(payload)
text = next(s for s in obj.sections if s['flags'] & 0x20)
assert not text['relocs']
assert pe.read(0x01010a68,len(text['data'])) == text['data']
result.append(dict(member=member,member_sha256=hashlib.sha256(payload).hexdigest(),
                   code_start=0x01010a68,code_bytes=len(text['data']),all_bytes_equal=True))
print('STRNCMP: complete leaf matches the shipped object exactly.')

# CRT0DAT's exit wrappers and _initterm leaf, identified independently of
# the compatible common exit body's minor state/control-flow differences.
member, payload = next((n,d) for n,d in archive(wt.TC/'lib/LIBC.LIB') if n.endswith('CRT0DAT.OBJ'))
obj = COFF(payload)
text = next(s for s in obj.sections if s['flags'] & 0x20)
for symbol, offset, start, length in [('_exit',0x30,0x01007af0,18),
                                     ('__exit',0x50,0x01007b10,18),
                                     ('__initterm',0x110,0x01007bd0,34)]:
    expected = bytearray(text['data'][offset:offset+length])
    actual = bytearray(pe.read(start,length))
    relocations = []
    for off,sym,kind in text['relocs']:
        if offset <= off < offset + length:
            assert kind == 20 and sym['name'] == '_doexit'
            displacement = struct.unpack_from('<i',expected,off-offset)[0]
            destination = start + off-offset + 4 + struct.unpack_from('<i',actual,off-offset)[0]
            assert destination == 0x01007b50 + displacement
            relocations.append(dict(symbol='_doexit',target=destination))
            expected[off-offset:off-offset+4] = b'\0'*4
            actual[off-offset:off-offset+4] = b'\0'*4
    assert actual == expected
    result.append(dict(member=member,symbol=symbol,code_start=start,code_bytes=length,
                       all_nonrelocation_bytes_equal=True,relocations=relocations))
print('CRT0DAT: exit wrappers and complete _initterm leaf verified.')

# The initializer's complete code and all seven data/call references.
part = dict(text, data=text['data'][:48], relocs=[r for r in text['relocs'] if r[0] < 48])
assert match_section(pe.read(0x01007ac0,48), part) == [0]
targets = {'__FPinit':0x01012194, '___xi_z':0x01021030,
           '___xi_a':0x01021020, '___xc_z':0x01021010,
           '___xc_a':0x01021000, '__initterm':0x01007bd0}
proof = []
for off,sym,kind in part['relocs']:
    if kind == 20:
        value = 0x01007ac0+off+4+struct.unpack('<i',pe.read(0x01007ac0+off,4))[0]
    else:
        assert kind == 6
        value = u32(pe.read(0x01007ac0+off,4),0)
    assert value == targets[sym['name']] + u32(part['data'],off)
    proof.append(dict(symbol=sym['name'],target=value))
fp_section = next(s for s in pe.sections if pe.base+s['rva'] <= 0x01012194 < pe.base+s['rva']+s['size'])
assert fp_section['flags'] & 0x80 and fp_section['off'] == 0  # Loader-zeroed BSS.
assert pe.read(0x01021000,16) == b'\0'*16
assert pe.read(0x01021020,16) == b'\0'*16
result.append(dict(member=member,symbol='__cinit',code_start=0x01007ac0,
                   code_bytes=48,all_nonrelocation_bytes_equal=True,
                   all_relocations_verified=True,initial_callback_state_exact=True,
                   relocations=proof))
print('CRT0DAT: C initializer code, seven relocations and initial callbacks verified.')

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
                        ('LSEEK.OBJ', 0x010088c0), ('FGETC.OBJ', 0x0100e8c0),
                        ('FCLOSE.OBJ', 0x01007ed0)):
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
print('Seven additional stdio objects match all code and relocation destinations.')
