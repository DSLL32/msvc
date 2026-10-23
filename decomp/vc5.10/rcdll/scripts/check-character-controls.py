#!/usr/bin/env python3
"""Scoped data-proof controls, including the explicitly unknown class boundary."""
import importlib.util
import json
from pathlib import Path
import re
import shutil
import struct
import tempfile
import time
HERE=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('characters',HERE/'scripts/check-characters.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
started=time.perf_counter();module.verify();controls=[]
with tempfile.TemporaryDirectory(prefix='character-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/rcdll')as directory:
    root=Path(directory);(root/'build').mkdir();(root/'src').mkdir()
    for name in ['RCDLL.DLL','rcdll.map']:shutil.copyfile(HERE/'build'/name,root/'build'/name)
    shutil.copyfile(HERE/'src/characters.c',root/'src/characters.c');module.HERE=root
    good=module.verify();path=root/'build/RCDLL.DLL';original=path.read_bytes();data=bytearray(original)
    at=struct.unpack_from('<I',data,60)[0];optional=at+24;size=struct.unpack_from('<H',data,at+20)[0]
    def offset(rva):
        for index in range(struct.unpack_from('<H',data,at+6)[0]):
            _,start,length,raw=struct.unpack_from('<IIII',data,optional+size+40*index+8)
            if start<=rva<start+length:return raw+rva-start
        raise AssertionError('control RVA not backed')
    token=int(good['token_table']['rebuilt_rva'],16)
    mapping=(root/'build/rcdll.map').read_text();base=struct.unpack_from('<I',data,optional+28)[0]
    classes=int(re.findall(r'\s_rc_character_classes\s+([0-9a-f]+)\s',mapping,re.I)[0],16)-base
    for name,rva in [('changed-identified-token',token+ord('S')*2),('changed-class0-alias',classes),('changed-identifier-class',classes+ord('i')*2),('changed-high-class-default',classes+255*2)]:
        changed=bytearray(original);changed[offset(rva)]^=1;path.write_bytes(changed)
        try:module.verify()
        except AssertionError:controls.append(name)
        else:raise AssertionError('bad data passed: '+name)
    changed=bytearray(original);struct.pack_into('<H',changed,offset(classes+512),0x7777);path.write_bytes(changed)
    module.verify();controls.append('unknown-class256-remains-unclaimed')
result=dict(status='pass',controls=controls,seconds=time.perf_counter()-started,scope='Static proof controls only, not acceptance of a modified application or unknown class behavior.')
print(json.dumps(result,indent=2))
