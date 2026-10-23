"""Check original formatter callers without adding a reconstructed range check."""
from pathlib import Path
import hashlib
import json
import struct
from binary import PE

p = PE('../../../toolchains/1.0-nt/bin/C23232.EXE')
code = p.read(0x17000, 399693)
# Scan bytewise because the code section also contains inline data.
calls = [0x17000+i for i in range(len(code)-4)
         if code[i] in (0xe8, 0xe9)
         and 0x17000+i+5+struct.unpack_from('<i', code, i+1)[0] == 0x707c0]
assert calls == [0x4e142, 0x4e1d9, 0x4e1ff, 0x4e268, 0x7070c, 0x70720], calls
for address in (0x4e137, 0x4e1ce, 0x4e1f5, 0x4e25e):
    assert p.read(address, 2) == b'\x6a\x0a'  # push 10
# The only remaining two sites share this %d/%x selector:
# mov al,[ebp+1]; sub al,'d'; cmp al,1; sbb eax,eax;
# and eax,-6; add eax,16; push eax. Entry dispatch permits only d or x.
assert p.read(0x706f0, 16).hex() == '8a45012c643c011bc083e0fa83c01050'
assert p.read(0x706a0, 5).hex() == '83f878744b'  # cmp eax,'x'; je706f0
assert p.read(0x706c1, 5).hex() == '83f864742a'  # cmp eax,'d'; je706f0
assert struct.pack('<I', 0x707c0) not in p.data  # no stored function pointer
assert p.read(0x85dc0,16) == b'0123456789abcdef'
source = Path('src/names.c').read_text()
body = source[source.index('unsigned __fastcall format_integer(unsigned value, char *text, unsigned radix)\n{'):]
assert '__asm' not in body and 'radix >' not in body
receipt = dict(target_sha256=hashlib.sha256(p.data).hexdigest(),
    source_sha256=hashlib.sha256(source.encode()).hexdigest(),
    original_extent=['0x707c0','0x70833'], direct_sites=[hex(a) for a in calls],
    symbol_call_radix=10, diagnostic_specifiers={'d':10,'x':16},
    absolute_function_pointer_occurrences=0, digit_extent=['0x85dc0','0x85dd0'],
    digit_bytes=16, guard_added=False, executes_wine=False,
    limits=['No radix validation exists in the original body; it is not introduced in C.',
            'Only decimal identifier173 is reached by the floating case at this frontier.',
            'Other radix values and reverse-buffer capacity remain original caller assumptions.',
            'Adjacent diagnostic source text is a distinct object and is not included.'])
Path('cases/integer-radix-contract.json').write_text(json.dumps(receipt,indent=2)+'\n')
print('Six direct original callers supply only decimal or hexadecimal; no stored pointer or added guard')
