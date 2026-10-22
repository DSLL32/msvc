#!/usr/bin/env python3
"""Check encountered semantic lexical tables against original initialization."""
from binary import PE
from reference import wt, publics

original = PE(wt.TC/'bin/RCPP.EXE')
reconstructed = PE('build/rcpp.exe')
names = publics()
for name, original_address in (('_rc_character_class', 0x0101f9b0),
                               ('_rc_character_flags', 0x0101fab0)):
    assert original.read(original_address, 256) == reconstructed.read(names[name], 256), name
print('Both 256-entry semantic lexical tables match original initial values.')

from binary import u32
for token in range(1, 133):
    old = original.read(0x0101ea68 + 8*token, 8)
    new = reconstructed.read(names['_rc_token_info'] + 8*token, 8)
    assert u32(old, 4) == u32(new, 4), ('token category', token)
    old_name, new_name = u32(old, 0), u32(new, 0)
    assert bool(old_name) == bool(new_name), ('token spelling presence', token)
    if old_name:
        assert original.string(old_name) == reconstructed.string(new_name), ('token spelling', token)
print('All 132 valid token descriptions and categories match the original.')

# Compare classifications produced by signed native byte indices, including
# original prefix quirks rather than interpreting later LIBC layout as native.
from binary import u16
old_ctype = u32(original.read(0x0101f160,4),0)
new_ctype = u32(reconstructed.read(names['__pctype'],4),0)
negative = names['_rc_negative_native_ctype_flags']
for ch in range(-128,256):
    old = u16(original.read(old_ctype + 2*ch,2),0) & 0x15f
    new = u16(reconstructed.read(negative + 2*(ch+128),2),0) if ch < 0 else u16(reconstructed.read(new_ctype + 2*ch,2),0) & 0x15f
    assert old == new, ('native ctype flags',ch,old,new)
print('All native PRINT/SPACE/DIGIT flags match for signed and ordinary byte indices.')
