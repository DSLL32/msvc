#!/usr/bin/env python3
"""Semantic grammar audit controls; original and rebuilt files stay untouched."""
import copy
import json
import struct
from rules import PROJECT,ROOT,ELF,decode,decode_settings,verify
from binary import PE

proof=json.loads((PROJECT/'cases/help/rule-grammar.json').read_text())
original=PE(ROOT/'toolchains/5.10/bin/CL.EXE')
expected=decode(original,proof['start'],{r['address']:r['name']for r in proof['callbacks']})
native=ELF(PROJECT/'build/native-trace/CL.EXE');start=native.symbols['option_rules']
callbacks={native.symbols[r['name']]:r['name']for r in proof['callbacks']}
assert verify()['records']==174

def patch(address,payload):
    image=copy.copy(native);data=bytearray(image.data)
    section=next(s for s in image.sections if s[3]<=address< s[3]+s[5])
    at=section[4]+address-section[3];data[at:at+len(payload)]=payload
    image.data=bytes(data);return image

first_pattern=struct.unpack('<I',native.read(start,4))[0]
assert decode(patch(first_pattern,b'C'),start,callbacks)!=expected,'changed spelling was ignored'
for name,address,payload in [('wrong handler',start+78*20+16,struct.pack('<I',0x1234)),
                             ('wrong terminator',start+174*20+4,struct.pack('<I',1))]:
    try:decode(patch(address,payload),start,callbacks)
    except AssertionError:pass
    else:raise AssertionError('accepted '+name)
settings=proof['driver_settings'];setting_start=native.symbols['driver_options']
targets={native.symbols[r.get('target_symbol',r['target'])]+r.get('target_offset',0):r['target']for r in settings['records']}
expected_settings=decode_settings(native,setting_start,targets)
assert decode_settings(patch(setting_start+4,struct.pack('<I',4)),setting_start,targets)!=expected_settings,'changed action tag was ignored'
for name,address in [('unknown setting target',setting_start+8),
                     ('nonzero setting terminator',setting_start+31*12+4)]:
    try:decode_settings(patch(address,struct.pack('<I',0x1234)),setting_start,targets)
    except AssertionError:pass
    else:raise AssertionError('accepted '+name)
print('174 option rules and 31 driver settings match three images; six semantic negative controls pass')
