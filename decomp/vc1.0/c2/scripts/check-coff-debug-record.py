"""Whole debug-record source-position callback with a named line child mock."""
from pathlib import Path
import itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_indexes import indexes,publics
started=time.perf_counter();original,canonical,config,left,right=indexes();names=publics()
def run(image,right,emitted,pending,first,line,offset):
    globals=[names[n]for n in ['_coff_emitted_function','_coff_has_function_lines','_coff_function_line','_coff_last_function_line']]if right else[0x850c8,0x7e8b4,0x7e8c4,0x7e8b8]
    start=names['@coff_output_debug_record@8']if right else 0x6d3f0;child=names['@coff_record_line@8']if right else 0x6ec30
    u=Uc(UC_ARCH_X86,UC_MODE_32)
    for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x1000)]:u.mem_map(a,n)
    for s in image.sections:
        if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
    def word(a):return struct.unpack('<I',u.mem_read(a,4))[0]
    for a,v in zip(globals,[emitted,0xa5a50000|pending,first,0xcccccccc]):u.mem_write(a,struct.pack('<I',v&0xffffffff))
    u.mem_write(0x300800,struct.pack('<I',0x400000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,line);u.reg_write(UC_X86_REG_EDX,offset);events=[]
    def hook(m,a,size,unused):
        if a==0x400000:m.emu_stop();return
        if a!=child:return
        esp=m.reg_read(UC_X86_REG_ESP);events.append([m.reg_read(UC_X86_REG_ECX)&65535,m.reg_read(UC_X86_REG_EDX)]);m.reg_write(UC_X86_REG_ECX,0x12345678);m.reg_write(UC_X86_REG_EDX,0x9abcdef0);m.reg_write(UC_X86_REG_EIP,word(esp));m.reg_write(UC_X86_REG_ESP,esp+4)
    u.hook_add(UC_HOOK_CODE,hook);u.emu_start(start,0,count=10000);assert u.reg_read(UC_X86_REG_ESP)==0x300804
    return dict(events=events,storage=[word(a)for a in globals])
rows=[]
for values in itertools.product([-1,0,1],[0,65535],[0,24,0xffffffff],[0,24,25,0xffffffff],[0,0xffffffff]):
    a=run(original,False,*values);b=run(canonical,True,*values);assert a==b,(values,a,b);rows.append(dict(inputs=values,observed=a))
Path('cases/coff-debug-record-validation.json').write_text(json.dumps(dict(cases=rows,seconds=time.perf_counter()-started,scope='Whole original65-byte body versus actual original-compiler C object with a named line child mock. Actual16-bit line contract observed at mock; WORD counter upper neighbour retained. This proves guard/relative-zero sentinel/wrap control, not child behavior/ABI acceptance. Real native entry formals and next frontier captured separately.'),indent=2)+'\n');print(len(rows),'whole original/canonical debug-record states match')
