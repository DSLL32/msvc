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
p=argparse.ArgumentParser();p.add_argument('--function',choices=['main','insert_resource','convert_ce_image','warning','error'],default='main');p.add_argument('--output',required=True,type=Path);a=p.parse_args()
original=PE(ROOT/'toolchains/5.10/bin/CVTRES.EXE');rebuilt=PE(BINARY/'build/CVTRES.EXE')
obj=(BINARY/('build/'+{'main':'main','insert_resource':'tree','convert_ce_image':'bitmap','warning':'diagnostics','error':'diagnostics'}[a.function]+'.obj')).read_bytes();maptext=(BINARY/'build/cvtres.map').read_text()
check={'main':m.check,'insert_resource':m.check_insert,'convert_ce_image':m.check_bitmap}.get(a.function)
if a.function in ('warning','error'):check=lambda original,rebuilt,obj,maptext:m.check_diagnostic(original,rebuilt,obj,maptext,a.function)
check(original,rebuilt,obj,maptext)
printf=next(slot for slot,(_,name) in import_addresses(original).items() if name=='printf')
tests=[('swapped-IAT-slot',0x10014e7,struct.pack('<I',printf),'cached IAT slot swapped'),
 ('changed-register-call',0x1001658,b'\xff\xd6','cached import callsite set changed'),
 ('overwritten-cached-register',0x10014eb,b'\x31\xff\x90','callee-saved cached register overwritten'),
 ('branch-bypassing-load',0x10014da,struct.pack('<i',0x1001530-(0x10014d8+6)),'external branch bypasses cached import load')]
if a.function=='insert_resource':
 puts=next(slot for slot,(_,name) in import_addresses(original).items() if name=='puts')
 tests=[('swapped-IAT-slot',0x1001ece,struct.pack('<I',puts),'slot swapped'),
 ('changed-register-call',0x1001ed3,b'\xff\xd7','callsite set changed'),
 ('overwritten-cached-register',0x1001ed5,b'\x31\xf6\x90','register overwritten'),
 ('branch-bypassing-load',0x1001ebb,b'\xe9'+struct.pack('<i',0x1001ed3-0x1001ec0),'bypasses cached load')]
if a.function=='convert_ce_image':
 puts=next(slot for slot,(_,name) in import_addresses(original).items() if name=='puts')
 tests=[('swapped-IAT-slot',0x10023bd,struct.pack('<I',puts),'slot swapped'),
 ('changed-register-call',0x10023c6,b'\xff\xd7','callsite set changed'),
 ('overwritten-cached-register',0x10023c8,b'\x31\xf6\x90','register overwritten'),
 ('branch-bypassing-load',0x1002392,bytes([0x10023e2-0x1002393]),'bypasses cached load'),
 ('error-branch-enters-call',0x1002593,struct.pack('<i',0x10023c6-0x1002597),'bypasses cached load'),
 ('error-stub-enters-call',0x10023d6,struct.pack('<i',0x10023e2-0x10023da),'error-only jump changed')]
if a.function in ('warning','error'):
 puts=next(slot for slot,(_,name) in import_addresses(original).items() if name=='puts')
 load,call,overwrite=(0x1001c0a,0x1001c11,0x1001c13) if a.function=='warning' else (0x1001bc8,0x1001bcf,0x1001bd1)
 tests=[('swapped-IAT-slot',load+2,struct.pack('<I',puts),'slot swapped'),
 ('changed-register-call',call,b'\xff\xd7','callsite set changed'),
 ('overwritten-cached-register',overwrite,b'\x31\xf6\x90','register overwritten')]
results=[]
for name,pc,data,reason in tests:
 mutated=PE(ROOT/'toolchains/5.10/bin/CVTRES.EXE');raw=bytearray(mutated.data);offset=mutated.offset(pc);raw[offset:offset+len(data)]=data;mutated.data=bytes(raw)
 try:check(mutated,rebuilt,obj,maptext)
 except AssertionError as e:
  assert reason in str(e),(name,str(e));results.append(dict(name=name,rejected=True,reason=str(e),address=pc,mutated_sha256=sha(mutated.data)))
 else:raise AssertionError(name+' was accepted')
report=dict(schema=1,scope='Pure Linux validation of copied bytes; no original execution or modified toolchain files.',checker_sha256=sha((BINARY/'scripts/import-residual.py').read_bytes()),original_sha256=sha(original.data),rebuilt_sha256=sha(rebuilt.data),function=a.function,object_sha256=sha(obj),cases=results)
a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(results,indent=2))
