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
repro_obj=(BINARY/'build/reproduction.obj').read_bytes()
module.check_repro(original,rebuilt,repro_obj,maptext)
for name,site,patch in [
 ('repro swapped getenv slot',0x444b1b,struct.pack('<I',0x401138)),
 ('repro swapped fullpath slot',0x444b38,struct.pack('<I',0x401134)),
 ('repro wrong getenv register',0x444b24,b'\xff\xd6'),
 ('repro wrong fullpath register',0x444b47,b'\xff\xd7'),
 ('repro cache write',0x444b29,b'\x31\xff\x90\x90\x90'),
 ('repro DI alias write',0x444b29,b'\x66\x31\xff\x90\x90'),
 ('repro entry skips fullpath load',0x444b32,struct.pack('<i',0x444b47-(0x444b30+6))),
 ('repro early restore changed',0x444b9c,b'\x5e'),
 ('repro final restore changed',0x444c2a,b'\x5f'),
]:
 mutated=PE(ROOT/'toolchains/5.10/bin/LINK.EXE');data=bytearray(mutated.data);offset=mutated.offset(site);data[offset:offset+len(patch)]=patch;mutated.data=bytes(data)
 try:module.check_repro(mutated,rebuilt,repro_obj,maptext)
 except (AssertionError,KeyError,IndexError):passed.append(name)
 else:raise AssertionError('mutation accepted: '+name)
argument_obj=(BINARY/'build/arguments.obj').read_bytes()
module.check_commandline(original,rebuilt,argument_obj,maptext)
for name,site,patch in [
 ('parser swapped getenv slot',0x45cb27,struct.pack('<I',0x401138)),
 ('parser wrong call register',0x45cb32,b'\xff\xd6'),
 ('parser overwritten cache',0x45cb53,b'\x31\xdb\x90\x90\x90\x90\x90\x90\x90\x90'),
 ('parser BH alias overwrite',0x45cb53,b'\xb7\x01\x90\x90\x90\x90\x90\x90\x90\x90'),
 ('parser branch bypasses load',0x45cb20,b'\xeb\x10\x90\x90'),
 ('parser restore differs',0x45cbb2,b'\x5e'),
]:
 mutated=PE(ROOT/'toolchains/5.10/bin/LINK.EXE');data=bytearray(mutated.data);offset=mutated.offset(site);data[offset:offset+len(patch)]=patch;mutated.data=bytes(data)
 try:module.check_commandline(mutated,rebuilt,argument_obj,maptext)
 except (AssertionError,KeyError,IndexError):passed.append(name)
 else:raise AssertionError('mutation accepted: '+name)
module.check_switch(original,rebuilt,argument_obj,maptext)
for name,site,patch in [
 ('switch swapped comparison slot',0x444f3b,struct.pack('<I',0x401134)),
 ('switch wrong call register',0x444f43,b'\xff\xd3'),
 ('switch cached EBP overwritten',0x444f4c,b'\x31\xed\x90\x90\x90'),
 ('switch BP alias overwritten',0x444f4c,b'\x66\x31\xed\x90\x90'),
 ('switch branch skips cache load',0x444f20,b'\xeb\x21\x90\x90\x90'),
 ('switch early restore differs',0x444f6e,b'\x5e'),
 ('switch count-zero restore differs',0x444f75,b'\x5b'),
]:
 mutated=PE(ROOT/'toolchains/5.10/bin/LINK.EXE');data=bytearray(mutated.data);offset=mutated.offset(site);data[offset:offset+len(patch)]=patch;mutated.data=bytes(data)
 try:module.check_switch(mutated,rebuilt,argument_obj,maptext)
 except (AssertionError,KeyError,IndexError):passed.append(name)
 else:raise AssertionError('mutation accepted: '+name)
report=dict(positive=True,negative_count=len(passed),rejected=passed)
(BINARY/'cases/help/main-import-negatives.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
