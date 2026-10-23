#!/usr/bin/env python3
"""Synthetic PE32 import-identity CLI controls; no target execution or Wine."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import time

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--binary',type=Path,default=Path(__file__).parent/'target/release/graph-audit')
p.add_argument('--scratch',type=Path,required=True);p.add_argument('--report',type=Path,required=True)
a=p.parse_args();binary=a.binary.resolve();scratch=a.scratch.resolve()
assert scratch.is_relative_to(Path.home()/'.ai-tmp');scratch.mkdir(parents=True,exist_ok=True)
def image(code,dll='msvcrt.dll',symbol='Foo',mutate=None):
    b=bytearray(0x600)
    def word(at,n):struct.pack_into('<I',b,at,n)
    b[:2]=b'MZ';word(60,0x80);b[0x80:0x84]=b'PE\0\0';struct.pack_into('<HH',b,0x84,0x14c,1);struct.pack_into('<H',b,0x94,0xe0)
    struct.pack_into('<H',b,0x98,0x10b);word(0xb4,0x400000);word(0x98+92,16);word(0x98+104,0x1100);word(0x98+108,40)
    b[0x178:0x17d]=b'.text'
    for off,v in [(8,0x400),(12,0x1000),(16,0x400),(20,0x200),(36,0x60000020)]:word(0x178+off,v)
    b[0x200:0x220]=b'\xcc'*0x20;b[0x200:0x200+len(code)]=code;b[0x210:0x216]=b'\xff\x25\x50\x11\x40\x00'
    for at,v in [(0x300,0x1140),(0x30c,0x1180),(0x310,0x1150)]:word(at,v)
    entry=0x80000000|symbol if isinstance(symbol,int) else 0x11c0
    word(0x340,entry);word(0x350,entry);b[0x380:0x380+len(dll)+1]=dll.encode('ascii')+b'\0'
    if isinstance(symbol,str):b[0x3c2:0x3c2+len(symbol)+1]=symbol.encode('ascii')+b'\0'
    if mutate:mutate(b)
    return bytes(b)
# OriginalFirstThunk lookup uses one entry; index+4 is the valid null terminator,
# not an identified import slot.
iat=b'\xff\x15\x50\x11\x40\x00\xc3';thunk=b'\xe8\x0b\0\0\0\xc3'
results=[]
(scratch/'runtime.json').write_text('[]\n');(scratch/'metadata.json').write_text('{"schema":1,"functions":{},"callback_tables":[]}\n')
def run(label,left,right,expect=True,declared=False,check=True,legacy=False):
    original=scratch/'original.exe';rebuilt=scratch/'rebuilt.exe';original.write_bytes(left);rebuilt.write_bytes(right)
    # Caller extent follows the emitted RET; the imported thunk remains a
    # distinct declared source vertex only in explicit protection controls.
    end=0x401000+next(i+1 for i,b in enumerate(left[0x200:0x210])if b==0xc3)
    cfg=dict(target=dict(path=str(original),sha256=hashlib.sha256(left).hexdigest()),driver=[dict(name='_caller',start=0x401000,end=end)],accepted=['_caller'])
    if declared:cfg['driver'].append(dict(name='_wrapper',start=0x401010,end=0x401016))
    (scratch/'functions.json').write_text(json.dumps(cfg))
    rebuilt_end=0x401000+next(i+1 for i,b in enumerate(right[0x200:0x210])if b==0xc3)
    publics=[('_caller',0x401000),('_caller_end',rebuilt_end),('_wrapper',0x401010),('_wrapper_end',0x401016)]
    (scratch/'fixture.map').write_text(''.join(' 0001:%08x %s %08x\n'%(address-0x400000,name,address)for name,address in publics))
    cmd=[str(binary),'--profile','cl','--repo',str(scratch),'--project',str(scratch),'--original',str(original),'--rebuilt',str(rebuilt),'--map',str(scratch/'fixture.map'),'--runtime',str(scratch/'runtime.json'),'--rebuilt-runtime',str(scratch/'runtime.json'),'--config',str(scratch/'functions.json')]
    if not legacy:cmd+=['--metadata',str(scratch/'metadata.json'),'--check-ownership','--check-extents']
    else:cmd+=['--discovery','legacy']
    if check:cmd+=['--check-imports']
    start=time.perf_counter();p=subprocess.run(cmd,capture_output=True,text=True);elapsed=time.perf_counter()-start
    assert (p.returncode==0)==expect,(label,p.stderr)
    result=json.loads(p.stdout)if p.returncode==0 else None
    results.append(dict(case=label,accepted=p.returncode==0,seconds=elapsed,diagnostic=p.stderr.strip(),caller=result['functions']['_caller']if result else None))
    return result
run('direct_iat_equals_import_thunk',image(thunk),image(iat))
run('DLL_basename_case_normalization',image(iat,r'C:\WINNT\MSVCRT.DLL'),image(iat))
for label,dll,symbol in [('different_provider','other.dll','Foo'),('different_export_case','msvcrt.dll','foo'),('ordinal_is_not_name','msvcrt.dll',17)]:
    run(label,image(iat),image(iat,dll,symbol),False)
run('different_ordinal',image(iat,symbol=17),image(iat,symbol=18),False)
run('ordinal_like_name_stays_name',image(iat,symbol='ordinal_17'),image(iat,symbol=17),False)
run('wrong_IAT_slot',image(iat),image(b'\xff\x15\x54\x11\x40\0\xc3'),False)
run('unresolved_register_call_is_not_import',image(iat),image(b'\xff\xd0\xc3'),False)
run('segment_override_is_not_image_IAT',image(iat),image(b'\x64'+iat),False)
run('declared_vertex_cannot_be_collapsed',image(thunk),image(iat),False,declared=True)
protected=run('declared_vertex_preserved',image(thunk),image(thunk),declared=True)
assert protected['functions']['_caller']=={'edges':['_wrapper'],'imports':[]}
run('flag_off_preserves_old_topology',image(thunk),image(iat),False,check=False)
run('legacy_rejects_optional_imports',image(iat),image(iat),False,legacy=True)
run('zero_OriginalFirstThunk_uses_unbound_IAT',image(iat,mutate=lambda b:struct.pack_into('<I',b,0x300,0)),image(iat))
run('malformed_directory_bounds',image(iat),image(iat,mutate=lambda b:struct.pack_into('<I',b,0x98+104,0xfffffff0)),False)
run('unterminated_directory',image(iat),image(iat,mutate=lambda b:struct.pack_into('<I',b,0x98+108,20)),False)
run('reserved_ordinal_bits',image(iat),image(iat,mutate=lambda b:struct.pack_into('<I',b,0x340,0x80010001)),False)
a.report.write_text(json.dumps(dict(scope=__doc__,binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),results=results),indent=2)+'\n')
print('Import identity CLI controls: %d cases passed.'%len(results))
