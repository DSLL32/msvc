#!/usr/bin/env python3
"""Check encountered semantic lexical tables against original initialization."""
from binary import PE
from compare import wt, publics

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
