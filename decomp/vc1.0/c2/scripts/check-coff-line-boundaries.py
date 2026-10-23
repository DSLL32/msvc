"""Whole .lf/.ef writer call shapes and precise auxiliary write extent."""
from pathlib import Path
import itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_indexes import indexes,publics
started=time.perf_counter();original,canonical,config,left,right=indexes();names=publics()
def run(image,right,kind,debug,section,line,offset,upper):
    u=Uc(UC_ARCH_X86,UC_MODE_32)
    for a,n in [(0x10000,0x100000),(0x200000,0x10000),(0x300000,0x10000),(0x400000,0x1000)]:u.mem_map(a,n)
    for s in image.sections:
        if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
    def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
    def word(a):return struct.unpack('<I',u.mem_read(a,4))[0]
    put(names['_coff_debug']if right else 0x7905c,debug);put(names['_coff_selected_section']if right else 0x790e8,section)
    u.mem_write(0x200000,bytes([0xa5])*18);put(0x300800,0x400000);u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,upper|line);u.reg_write(UC_X86_REG_EDX,offset)
    child=names['@coff_define_symbol@20'if kind=='marker'else'@coff_define_aux_symbol@24']if right else(0x6e890 if kind=='marker'else 0x6ea70);events=[]
    def hook(m,a,size,unused):
        if a==0x400000:m.emu_stop();return
        if a!=child:return
        esp=m.reg_read(UC_X86_REG_ESP);name=bytes(m.mem_read(m.reg_read(UC_X86_REG_ECX),4));assert name==(b'.lf\0'if kind=='marker'else b'.ef\0')
        values=[m.reg_read(UC_X86_REG_EDX)&65535,*[word(esp+i)for i in [4,8,12]]];assert values==[section,0x65,line if kind=='marker'else offset,0];events.append(values)
        if kind=='end':put(word(esp+16),0x200000)
        m.reg_write(UC_X86_REG_EAX,31);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_EIP,word(esp));m.reg_write(UC_X86_REG_ESP,esp+(16 if kind=='marker'else 20))
    u.hook_add(UC_HOOK_CODE,hook);entry=names['@coff_line_marker@4'if kind=='marker'else'@coff_end_lines@8']if right else(0x6eda0 if kind=='marker'else 0x6eae0);u.emu_start(entry,0,count=10000);assert u.reg_read(UC_X86_REG_ESP)==0x300804
    return dict(events=events,auxiliary=bytes(u.mem_read(0x200000,18)).hex())
rows=[]
for kind,debug,section,line,offset,upper in itertools.product(['marker','end'],[0,1,2,3],[7,65535],[0,65535],[0,0xffffffff],[0,0xffff0000]):
    a=run(original,False,kind,debug,section,line,offset,upper);b=run(canonical,True,kind,debug,section,line,offset,upper);assert a==b;rows.append(dict(function=kind,debug=debug,section=section,line=line,offset=offset,upper=upper,result=a))
Path('cases/coff-line-boundaries-validation.json').write_text(json.dumps(dict(cases=rows,seconds=time.perf_counter()-started,scope='Whole30-byte.lf and56-byte.ef bodies versus actual original-compiler C, named child mocks. Proves lowWORD mask, section/storage/type/offset args, debugbit0 and only2 auxiliary bytes changed; untouched16 bytes retained. Void return used by all recovered callers; incidental child EAX not claimed as independent return contract. Child bodies/failures not accepted by mocks; real native entry/frontier separately observed.'),indent=2)+'\n');print(len(rows),'whole original/canonical line boundary shapes match')
