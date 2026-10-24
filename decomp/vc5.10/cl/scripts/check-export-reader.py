#!/usr/bin/env python3
"""Offline old/new export-reader controls over the actual captured providers."""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import time
from binary import PE

PROJECT=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--provider',choices=('all','msvcrt','c1','mspdb'),default='all');parser.add_argument('--output',type=Path);args=parser.parse_args()
SCRATCH=args.output or Path('/root/.ai-tmp/msvc2/vc5.10/cl/export-reader-review')
SCRATCH.mkdir(parents=True,exist_ok=True)
started=time.perf_counter()
subprocess.run(['gcc','-std=c89','-pedantic','-Wall','-Wextra','-Werror','-O2','-shared','-fPIC',
    str(PROJECT/'scripts/check-export-reader.c'),'-o',str(SCRATCH/'reader.so')],check=True)
library=ctypes.CDLL(str(SCRATCH/'reader.so'));probe=library.parser_probe
U=ctypes.c_uint32
probe.argtypes=[ctypes.c_void_p,U,U,ctypes.c_char_p,ctypes.POINTER(U),ctypes.POINTER(U),U,ctypes.c_int]
probe.restype=ctypes.c_int

def run(data,base,name,old=False,fail=0):
    storage=ctypes.create_string_buffer(data);address=U();calls=U()
    result=probe(storage,len(data),base,name.encode('ascii'),ctypes.byref(address),ctypes.byref(calls),fail,old)
    return result,address.value,calls.value


def replace(data,offset,value,kind='I'):
    result=bytearray(data);struct.pack_into('<'+kind,result,offset,value);return bytes(result)


records=[];total_old=total_new=0;controls=[];inputs={}
for provider,name,path,sha in (
    ('msvcrt','MSVCRT.DLL',Path('/root/.ai-tmp/msvc2/vc5.10/cl/cleanup-files-1-original/loaded-msvcrt.bin'),'e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210'),
    ('c1','C1.DLL',Path('/root/.ai-tmp/msvc2/vc5.10/cl/cleanup-files-1-original/observer/loaded-c1.bin'),'7f12a4a889c5a0277f12c391ca462657dc81ef7be769faf629331bd117983d5d'),
    ('mspdb','MSPDB50.DLL',Path('/root/.ai-tmp/msvc2/vc5.10/cl/mre-prefix-analysis-2/loaded-mspdb50.bin'),'cd4536b0f88ef633dbba509062edae7423407b206199a89ea971b444ff249a33')):
    if args.provider not in ('all',provider):continue
    image=PE(path);assert hashlib.sha256(image.data).hexdigest()==sha
    inputs[name]={'path':str(path),'sha256':sha}
    nt=struct.unpack_from('<I',image.data,60)[0];image_size=struct.unpack_from('<I',image.data,image.optional+56)[0]
    mapped=bytearray(image_size);headers=struct.unpack_from('<I',image.data,image.optional+60)[0]
    mapped[:headers]=image.data[:headers]
    for section in image.sections:
        mapped[section['rva']:section['rva']+section['size']]=image.data[section['off']:section['off']+section['size']]
    data=bytes(mapped);directory,size=struct.unpack_from('<II',data,nt+24+96)
    _,_,_,_,module,ordinal_base,functions,count,addresses,names,ordinals=struct.unpack_from('<IIHH7I',data,directory)
    expected={}
    for index in range(count):
        pointer=struct.unpack_from('<I',data,names+4*index)[0]
        text=data[pointer:data.index(b'\0',pointer)].decode('ascii')
        ordinal=struct.unpack_from('<H',data,ordinals+2*index)[0]
        expected[text]=image.base+struct.unpack_from('<I',data,addresses+4*ordinal)[0]
    before_old=time.perf_counter();old_results={text:run(data,image.base,text,True)for text in expected};old_seconds=time.perf_counter()-before_old
    before_new=time.perf_counter();new_results={text:run(data,image.base,text)for text in expected};new_seconds=time.perf_counter()-before_new
    for text,address in expected.items():
        assert old_results[text][:2]==new_results[text][:2]==(1,address)
    total_old+=sum(value[2]for value in old_results.values());total_new+=sum(value[2]for value in new_results.values())
    wanted=next(iter(expected));first=struct.unpack_from('<I',data,names)[0]
    def reject(label,value,base=image.base,fail=0):
        result,address,_=run(value,base,wanted,fail=fail)
        assert result<0 and address==0,'reader accepted malformed '+name+': '+label
        controls.append(name+': '+label)
    reject('DOS signature',replace(data,0,0,'H'))
    reject('PE signature',replace(data,nt,0))
    reject('machine',replace(data,nt+4,0,'H'))
    reject('optional-header length',replace(data,nt+20,0,'H'))
    reject('PE32 magic',replace(data,nt+24,0,'H'))
    reject('missing export directory entry',replace(data,nt+24+92,0))
    reject('NT offset overflow',replace(data,60,0xfffffff0))
    reject('truncated NT headers',data[:nt+100])
    reject('zero image size',replace(data,nt+24+56,0))
    reject('oversized image',replace(data,nt+24+56,0xffffffff))
    reject('load base overflow',data,base=0xfffff000)
    reject('export allocation cap',replace(data,nt+24+100,0x100001))
    reject('export extent outside image',replace(data,nt+24+96,image_size-4))
    reject('unknown provider timestamp',replace(data,nt+8,0))
    reject('unknown preferred image',replace(data,nt+24+28,0))
    reject('missing actual export bytes',data[:directory+size-1])
    reject('module identity',data[:module]+b'X'+data[module+1:])
    reject('zero function count',replace(data,directory+20,0))
    reject('function count cap',replace(data,directory+20,65537))
    reject('name count exceeds functions',replace(data,directory+24,functions+1))
    for label,offset in [('functions',28),('names',32),('ordinals',36)]:
        reject(label+' array outside proven extent',replace(data,directory+offset,directory-1))
    reject('name RVA outside proven extent',replace(data,names,directory-1))
    reject('ordinal index outside array',replace(data,ordinals,functions,'H'))
    ordinal=struct.unpack_from('<H',data,ordinals)[0]
    reject('named null function RVA',replace(data,addresses+4*ordinal,0))
    reject('function RVA outside image',replace(data,addresses+4*ordinal,image_size))
    tail=directory+size-1
    malformed=replace(data,names,tail);malformed=malformed[:tail]+b'X'+malformed[tail+1:]
    reject('unterminated export name',malformed)
    reject('unterminated module name',replace(malformed,directory+12,tail))
    reject('malformed entry after requested valid prefix',replace(data,names+4,0xffffffff))
    for failure in (1,2,3):reject('remote read failure '+str(failure),data,fail=failure)
    assert run(data,image.base,'not_a_real_export')[:2]==(0,0)
    controls.append(name+': missing export remains missing')
    relocated=0x20000000
    assert run(data,relocated,wanted)[:2]==(1,relocated+expected[wanted]-image.base)
    controls.append(name+': relocation base retained')
    # An existing bounded string makes a harmless synthetic forwarder: both
    # algorithms return its address rather than resolving/executing it.
    forward=replace(data,addresses+4*ordinal,module)
    assert run(forward,image.base,wanted)[:2]==run(forward,image.base,wanted,True)[:2]==(1,image.base+module)
    controls.append(name+': forwarder address behavior retained')
    malformed=replace(data,addresses+4*ordinal,tail);malformed=malformed[:tail]+b'X'+malformed[tail+1:]
    reject('unterminated forwarder',malformed)
    records.append({'module':name,'export_names':len(expected),'old_algorithm_seconds':old_seconds,
        'new_algorithm_seconds':new_seconds,'new_reads_per_lookup':3})
receipt={'result':'PASS','kind':'native offline analysis parser controls; no PE execution or Wine',
    'inputs':inputs,'equivalent_named_exports':sum(r['export_names']for r in records),'records':records,
    'old_remote_reads':total_old,'new_remote_reads':total_new,'controls':len(controls),
    'control_names':controls,'seconds':time.perf_counter()-started,
    'sources':{name:hashlib.sha256((PROJECT/name).read_bytes()).hexdigest()for name in
        ('scripts/export-reader.h','scripts/reference-debugger.c','scripts/check-export-reader.c','scripts/check-export-reader.py','scripts/helper-build.py','scripts/original.py')}}
(SCRATCH/'validation.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps({key:receipt[key]for key in ('result','equivalent_named_exports','old_remote_reads','new_remote_reads','controls','seconds')}))
