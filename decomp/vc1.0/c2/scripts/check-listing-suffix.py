"""Whole size-suffix results and diagnostic call shape, with named mock."""
from pathlib import Path
import json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_indexes import indexes,publics
started=time.perf_counter();original,canonical,config,left,right=indexes();names=publics()
def run(image,right,value):
    u=Uc(UC_ARCH_X86,UC_MODE_32)
    for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x1000)]:u.mem_map(a,n)
    for s in image.sections:
        if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
    def word(a):return struct.unpack('<I',u.mem_read(a,4))[0]
    u.mem_write(0x300800,struct.pack('<I',0x400000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,value);u.reg_write(UC_X86_REG_EDX,0xfaceabcd);events=[];error=names['@compiler_internal_error@8']if right else 0x6fb70
    def hook(m,a,size,unused):
        if a==0x400000:m.emu_stop();return
        if a!=error:return
        esp=m.reg_read(UC_X86_REG_ESP);source=bytes(m.mem_read(m.reg_read(UC_X86_REG_ECX),64)).split(b'\0')[0].decode();events.append([source,m.reg_read(UC_X86_REG_EDX)]);m.reg_write(UC_X86_REG_EAX,0xabcdef01);m.reg_write(UC_X86_REG_EIP,word(esp));m.reg_write(UC_X86_REG_ESP,esp+4)
    u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names['@listing_size_suffix@4']if right else 0x2a840,0,count=10000);assert u.reg_read(UC_X86_REG_ESP)==0x300804
    pointer=u.reg_read(UC_X86_REG_EAX);return dict(value=bytes(u.mem_read(pointer,2)).hex()if pointer else None,diagnostics=events)
rows=[]
for value in [0,1,2,3,4,8,0xffffffff]:
    a=run(original,False,value);b=run(canonical,True,value);assert a==b,(value,a,b);rows.append(dict(input=value,result=a))
assert [r['result']['value']for r in rows if r['input']in [1,2,4]]==['6200','7700','6400']
assert all(r['result']['diagnostics']==[['@(#)listing.c:1.76',0x522]]for r in rows if r['input']not in [1,2,4])
Path('cases/listing-suffix-validation.json').write_text(json.dumps(dict(cases=rows,seconds=time.perf_counter()-started,scope='Whole54-byte original versus actual original-compiler C: three contextual b/w/d string values plus invalid-size diagnostic source/code and null return after a mocked returning diagnostic. Real fatal diagnostic body/failure outcome not accepted. Literal address identity not observed by recovered callers; ordinary point-of-use literals retain bytes.'),indent=2)+'\n');print('3 suffix values and4 diagnostic shapes match original/canonical')
