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
