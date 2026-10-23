"""Whole begin-lines C/original control with explicit allocator/child mocks."""
from pathlib import Path
import itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_indexes import indexes,publics
started=time.perf_counter();original,canonical,config,left,right=indexes();names=publics()
fields={'coff_current_section':0x851ac,'coff_debug':0x7905c,'coff_allocator':0x790fc,'coff_selected_section':0x790e8,'coff_symbol_entries':0x851a8,'coff_previous_begin_auxiliary':0x851d0,'coff_previous_function_auxiliary':0x851d4}
def run(image,canonical_side,count,debug,previous,line,line_count):
    globals={n:names['_'+n]if canonical_side else a for n,a in fields.items()}
    u=Uc(UC_ARCH_X86,UC_MODE_32)
    for a,n in [(0x10000,0x100000),(0x200000,0x20000),(0x300000,0x10000),(0x400000,0x1000)]:u.mem_map(a,n)
    for s in image.sections:
        if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
    def word(a):return struct.unpack('<I',u.mem_read(a,4))[0]
    def put(a,*v):u.mem_write(a,struct.pack('<'+'I'*len(v),*[x&0xffffffff for x in v]))
    def field(n,value):put(globals[n],value)
    for n,v in [('coff_current_section',0x200000),('coff_debug',debug),('coff_allocator',0x400100),('coff_selected_section',7),('coff_symbol_entries',0x201000),('coff_previous_begin_auxiliary',0x205000 if previous&1 else 0),('coff_previous_function_auxiliary',0x205100 if previous&2 else 0)]:field(n,v)
    put(0x200000,0x200100);put(0x200100+20,0x200200);u.mem_write(0x200100+54,struct.pack('<H',line_count))
    u.mem_write(0x200200,bytes([0xa5])*308);put(0x200200,count,0x200200)
    for a in [0x204000,0x204100,0x205000,0x205100]:u.mem_write(a,bytes([0x5a])*18)
    u.mem_write(0x202000,bytes([0xa5])*308);put(0x203000,4,0x204100)
    put(0x300800,0x400000,0x87654321,0x13572468);u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,10);u.reg_write(UC_X86_REG_EDX,0xface0000|line)
    define=names['@coff_define_aux_symbol@24']if canonical_side else 0x6ea70;get=names['@coff_vector_get@8']if canonical_side else 0x6e780;events=[]
    def hook(m,a,size,unused):
        if a==0x400000:m.emu_stop();return
        if a not in [0x400100,define,get]:return
        esp=m.reg_read(UC_X86_REG_ESP);ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);pop=4
        if a==0x400100:
            assert ecx==308;events.append('allocate308');result=0x202000
        elif a==define:
            assert bytes(m.mem_read(ecx,4))==b'.bf\0'and edx&65535==7
            assert [word(esp+i)for i in [4,8,12]]==[0x65,0x13572468,0]
            put(word(esp+16),0x204000);events.append('define.bf');result=31;pop=20
        else:
            assert ecx==0x201000 and edx==12;events.append('get_function_aux');result=0x203000
        m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02)
        m.reg_write(UC_X86_REG_EIP,word(esp));m.reg_write(UC_X86_REG_ESP,esp+pop)
    u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names['@coff_begin_lines@16']if canonical_side else 0x6eca0,0,count=10000)
    assert u.reg_read(UC_X86_REG_ESP)==0x30080c
    return dict(events=events,contribution=bytes(u.mem_read(0x200100,56)).hex(),original_block=bytes(u.mem_read(0x200200,308)).hex(),new_block=bytes(u.mem_read(0x202000,308)).hex(),begin=bytes(u.mem_read(0x204000,18)).hex(),function=bytes(u.mem_read(0x204100,18)).hex(),previous_begin=bytes(u.mem_read(0x205000,18)).hex(),previous_function=bytes(u.mem_read(0x205100,18)).hex(),chain=[word(globals[n])for n in ['coff_previous_begin_auxiliary','coff_previous_function_auxiliary']])
rows=[]
for count,debug,previous,line,line_count in itertools.product([0,48,49],[0,1,2,3],range(4),[0,65535],[0,65535]):
    a=run(original,False,count,debug,previous,line,line_count);b=run(canonical,True,count,debug,previous,line,line_count);assert a==b,(count,debug,previous,line,line_count,a,b)
    rows.append(dict(count=count,debug=debug,previous=previous,line=line,line_count=line_count,events=a['events']))
Path('cases/coff-begin-lines-validation.json').write_text(json.dumps(dict(cases=rows,seconds=time.perf_counter()-started,scope='Whole original253-byte body versus actual original-compiler C object with explicit allocator308/aux-symbol/vector-get mocks.192 combinations cover49->50 block growth/circular links, debugbit0, prior auxiliary chains, WORD wrap and untouched18-byte tails. Mock argument checks do not accept child body/ABI or failure behavior. Real native matched case checked separately.'),indent=2)+'\n');print(len(rows),'whole original/canonical begin-lines shapes match')
