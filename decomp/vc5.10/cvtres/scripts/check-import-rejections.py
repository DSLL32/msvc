#!/usr/bin/env python3
"""Bounded negative checks for CVTRES main's cached-import certificate."""
import argparse
import importlib.util
import json
import struct
from pathlib import Path
from binary import PE
from layout import import_addresses
from mask import BINARY,ROOT,sha
spec=importlib.util.spec_from_file_location('residual',BINARY/'scripts/import-residual.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
p=argparse.ArgumentParser();p.add_argument('--output',required=True,type=Path);a=p.parse_args()
original=PE(ROOT/'toolchains/5.10/bin/CVTRES.EXE');rebuilt=PE(BINARY/'build/CVTRES.EXE')
obj=(BINARY/'build/main.obj').read_bytes();maptext=(BINARY/'build/cvtres.map').read_text()
m.check(original,rebuilt,obj,maptext)
printf=next(slot for slot,(_,name) in import_addresses(original).items() if name=='printf')
tests=[('swapped-IAT-slot',0x10014e7,struct.pack('<I',printf),'cached IAT slot swapped'),
 ('changed-register-call',0x1001658,b'\xff\xd6','cached import callsite set changed'),
 ('overwritten-cached-register',0x10014eb,b'\x31\xff\x90','callee-saved cached register overwritten'),
 ('branch-bypassing-load',0x10014da,struct.pack('<i',0x1001530-(0x10014d8+6)),'external branch bypasses cached import load')]
results=[]
for name,pc,data,reason in tests:
 mutated=PE(ROOT/'toolchains/5.10/bin/CVTRES.EXE');raw=bytearray(mutated.data);offset=mutated.offset(pc);raw[offset:offset+len(data)]=data;mutated.data=bytes(raw)
 try:m.check(mutated,rebuilt,obj,maptext)
 except AssertionError as e:
  assert reason in str(e),(name,str(e));results.append(dict(name=name,rejected=True,reason=str(e),address=pc,mutated_sha256=sha(mutated.data)))
 else:raise AssertionError(name+' was accepted')
report=dict(schema=1,scope='Pure Linux validation of copied bytes; no original execution or modified toolchain files.',checker_sha256=sha((BINARY/'scripts/import-residual.py').read_bytes()),original_sha256=sha(original.data),rebuilt_sha256=sha(rebuilt.data),main_object_sha256=sha(obj),cases=results)
a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(results,indent=2))
