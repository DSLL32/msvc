#!/usr/bin/env python3
"""Additional SEH roots from the verified shipped CRT scope-table relocations."""
import json
from pathlib import Path
import struct
from binary import PE
from importlib.machinery import SourceFileLoader

project=Path(__file__).resolve().parents[1]
original={r['name']:r for r in json.loads((project/'runtime.json').read_text())}
rebuilt={r['name']:r for r in json.loads((project/'build/runtime.json').read_text())}
# crtexe.obj .rdata has one {-1,filter,handler} record. Its two pointer fields
# are independently proved by identify-runtime.py, including their text offsets.
functions={}
for name in ('_mainCRTStartup',):
    record={}
    for side,index in (('original',original),('rebuilt',rebuilt)):
        start=index[name]['start']
        bound=index[name]['evidence']['bindings']
        scope=[r for r in bound if r['section']=='.rdata']
        assert [r['actual'] for r in scope]==[start+270,start+291]
        record[side]={'roots':[start+270,start+291],'text_data':[],'switches':{}}
    functions[name]=record
# Main uses an ordinary one-entry compiler SEH scope table. Read the two
# roots through its actual verified prologue immediate in each PE, so the
# filter and handler are traversed even though ordinary control flow skips them.
identity=SourceFileLoader('runtime_identity',str(project/'scripts/identify-runtime.py')).load_module()
main_roots={}
for side,path,start in [('original',project.parents[2]/'toolchains/5.10/bin/CL.EXE',0x401743),
        ('rebuilt',project/'build/cl.exe',identity.public_map(project/'build/cl.map').get('_main'))]:
    pe=PE(path)
    assert pe.read(start,6)==bytes.fromhex('558bec6aff68'),'unexpected main SEH prologue'
    scope=struct.unpack('<I',pe.read(start+6,4))[0]
    enclosing,filter,handler=struct.unpack('<III',pe.read(scope,12))
    assert enclosing==0xffffffff and start<filter<handler,'unexpected one-entry main scope table'
    main_roots[side]={'roots':[filter,handler],'text_data':[],'switches':{}}
functions['_main']=main_roots
# /Od lowered read_option's six mode cases to a bounded two-level switch.
# Its pointer and byte-index tables are text data, never instructions. Retain
# exact reviewed offsets and validate the bound, indices and in-function roots.
rebuilt_pe=PE(project/'build/cl.exe')
public=identity.public_map(project/'build/cl.map')
start=public['_read_option'];jump=start+0x203;table=start+0x3aa;indices=table+24
assert rebuilt_pe.read(jump,3)==bytes.fromhex('ff248d')
assert struct.unpack('<I',rebuilt_pe.read(jump+3,4))[0]==table
assert rebuilt_pe.read(jump-6,2)==bytes.fromhex('8a8a')
assert struct.unpack('<I',rebuilt_pe.read(jump-4,4))[0]==indices
assert rebuilt_pe.read(start+0x1e8,7)==bytes.fromhex('83bdb4fbffff1d')
targets=struct.unpack('<6I',rebuilt_pe.read(table,24))
assert all(start<=target<table for target in targets)
assert list(rebuilt_pe.read(indices,30))==[{ord('#'):0,ord('$'):1,ord('*'):2,ord('+'):3,ord('@'):4}.get(c,5)for c in range(ord('#'),ord('@')+1)]
functions['_read_option']={'original':{'roots':[],'text_data':[],'switches':{}},
    'rebuilt':{'roots':[],'text_data':[[table,indices+30]],'switches':{str(jump):list(targets)}}}
# The complete formatter has the same /Od lowering for c,d,s,x.
start=public['_format_message'];jump=start+0x9b;table=start+0x28e;indices=table+20
assert rebuilt_pe.read(jump,3)==bytes.fromhex('ff248d')
assert struct.unpack('<I',rebuilt_pe.read(jump+3,4))[0]==table
assert rebuilt_pe.read(jump-6,2)==bytes.fromhex('8a8a')
assert struct.unpack('<I',rebuilt_pe.read(jump-4,4))[0]==indices
assert rebuilt_pe.read(start+0x80,3)==bytes.fromhex('83e863')
assert rebuilt_pe.read(start+0x86,4)==bytes.fromhex('837de015')
targets=struct.unpack('<5I',rebuilt_pe.read(table,20))
assert all(start<=target<table for target in targets)
assert list(rebuilt_pe.read(indices,22))==[{ord('c'):0,ord('d'):1,ord('s'):2,ord('x'):3}.get(c,4)for c in range(ord('c'),ord('x')+1)]
functions['_format_message']={'original':{'roots':[],'text_data':[],'switches':{}},
    'rebuilt':{'roots':[],'text_data':[[table,indices+22]],'switches':{str(jump):list(targets)}}}
(project/'graph-metadata.json').write_text(json.dumps({'schema':1,'functions':functions,'callback_tables':[]},indent=2)+'\n')
