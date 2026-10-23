"""Whole ordinary COFF line-record insertion with a bounded allocator mock."""
from pathlib import Path
import itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_indexes import indexes,publics
started=time.perf_counter();original,canonical,config,left,right=indexes();names=publics()
def run(image,right,count,reuse,line,line_count,offset,base):
    section=names['_coff_current_section']if right else 0x851ac;allocator=names['_coff_allocator']if right else 0x790fc;free=names['_coff_free_lines']if right else 0x851cc
    u=Uc(UC_ARCH_X86,UC_MODE_32)
    for a,n in [(0x10000,0x100000),(0x200000,0x10000),(0x300000,0x10000),(0x400000,0x1000)]:u.mem_map(a,n)
    for s in image.sections:
        if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
    def word(a):return struct.unpack('<I',u.mem_read(a,4))[0]
    def put(a,*v):u.mem_write(a,struct.pack('<'+'I'*len(v),*[x&0xffffffff for x in v]))
    put(section,0x200000);put(allocator,0x400100);put(free,0x202000 if reuse else 0);put(0x200000,0x200100);put(0x200000+12,base);put(0x200100+20,0x200200);u.mem_write(0x200100+54,struct.pack('<H',line_count))
    u.mem_write(0x200200,bytes([0xa5])*308);put(0x200200,count,0x200200)
    u.mem_write(0x202000,bytes([0x5a])*308);put(0x202000+4,0x202400)
    put(0x300800,0x400000);u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,0xface0000|line);u.reg_write(UC_X86_REG_EDX,offset);events=[]
    def hook(m,a,size,unused):
        if a==0x400000:m.emu_stop();return
        if a!=0x400100:return
        assert m.reg_read(UC_X86_REG_ECX)==308;events.append('allocate308');esp=m.reg_read(UC_X86_REG_ESP);m.reg_write(UC_X86_REG_EAX,0x202000);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_EIP,word(esp));m.reg_write(UC_X86_REG_ESP,esp+4)
    u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names['@coff_record_line@8']if right else 0x6ec30,0,count=10000);assert u.reg_read(UC_X86_REG_ESP)==0x300804
    return dict(events=events,contribution=bytes(u.mem_read(0x200100,56)).hex(),block=bytes(u.mem_read(0x200200,308)).hex(),new=bytes(u.mem_read(0x202000,308)).hex(),free=word(free))
rows=[]
for values in itertools.product([0,48,49],[0,1],[0,65535],[0,65535],[0,0xffffffff],[0,4096]):
    a=run(original,False,*values);b=run(canonical,True,*values);assert a==b,(values,a,b);rows.append(dict(inputs=values,events=a['events']))
Path('cases/coff-record-line-validation.json').write_text(json.dumps(dict(cases=rows,seconds=time.perf_counter()-started,scope='Whole original100-byte body versus canonical C object: section-relative unsigned subtraction, WORD line/count wrap,49->50 growth, circular links, free-list reuse and untouched entry bytes. Allocator308 mocked, no allocation failure acceptance; actual native case/formals checked separately.'),indent=2)+'\n');print(len(rows),'whole original/canonical ordinary line states match')
