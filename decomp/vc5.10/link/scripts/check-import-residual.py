#!/usr/bin/env python3
"""Mutation checks for LINK main's bounded cached-IAT certificate."""
import importlib
import json
from pathlib import Path
import struct
from binary import PE
from mask import BINARY,ROOT

module=importlib.import_module('import-residual')
checker=module.check
original=PE(ROOT/'toolchains/5.10/bin/LINK.EXE');rebuilt=PE(BINARY/'build/link.exe')
obj=(BINARY/'build/driver.obj').read_bytes();maptext=(BINARY/'build/link.map').read_text()
checker(original,rebuilt,obj,maptext)
mutations=[
 ('swapped setvbuf slot',0x40e05a,struct.pack('<I',0x401238)),
 ('swapped second setvbuf slot',0x40e089,struct.pack('<I',0x401238)),
 ('swapped printf slot',0x40e157,struct.pack('<I',0x401234)),
 ('wrong register call',0x40e05e,b'\xff\xd6'),
 ('EBX overwrite',0x40e063,b'\x31\xdb\x90\x90\x90'),
 ('BH alias overwrite',0x40e063,b'\xb7\x01\x90\x90\x90'),
 ('branch skips cache load',0x40e044,bytes([0x40e08d-(0x40e043+2)])),
 ('changed restore',0x40e0dc,b'\x5e'),
 ('wrong dispatch kind',0x467028,struct.pack('<I',4)),
 ('wrong dispatch function',0x46702c,struct.pack('<I',0x43ce20)),
 ('wrong dispatch order',0x467020,struct.pack('<I',0x40152c)),
 ('nonzero dispatch sentinel',0x467070,struct.pack('<I',1)),
]
passed=[]
for name,site,patch in mutations:
 mutated=PE(ROOT/'toolchains/5.10/bin/LINK.EXE');data=bytearray(mutated.data);offset=mutated.offset(site);data[offset:offset+len(patch)]=patch;mutated.data=bytes(data)
 try:checker(mutated,rebuilt,obj,maptext)
 except (AssertionError,KeyError,IndexError):passed.append(name)
 else:raise AssertionError('mutation accepted: '+name)
debug_obj=(BINARY/'build/driver-stubs.obj').read_bytes()
module.check_debug(original,rebuilt,debug_obj,maptext)
for name,site,patch in [
 ('debug swapped IAT',0x415258,struct.pack('<I',0x401244)),
 ('debug wrong call register',0x415264,b'\xff\xd3'),
 ('debug EBP overwrite',0x415288,b'\x31\xed\x90\x90\x90'),
 ('debug BP alias overwrite',0x415288,b'\x66\x31\xed\x90\x90'),
 ('debug load-bypassing branch',0x41525f,bytes([0x415286-(0x41525e+2)])),
 ('debug restore differs',0x4152ea,b'\x5b'),
]:
 mutated=PE(ROOT/'toolchains/5.10/bin/LINK.EXE');data=bytearray(mutated.data);offset=mutated.offset(site);data[offset:offset+len(patch)]=patch;mutated.data=bytes(data)
 try:module.check_debug(mutated,rebuilt,debug_obj,maptext)
 except (AssertionError,KeyError,IndexError):passed.append(name)
 else:raise AssertionError('mutation accepted: '+name)
report=dict(positive=True,negative_count=len(passed),rejected=passed)
(BINARY/'cases/help/main-import-negatives.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
