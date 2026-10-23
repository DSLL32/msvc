"""Whole recursive CodeView scope cursor/control with named child mocks."""
from pathlib import Path
import itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_indexes import indexes,publics
started=time.perf_counter();original,canonical,config,left,right=indexes();names=publics()
def run(image,right,depths,records,optimize):
    u=Uc(UC_ARCH_X86,UC_MODE_32)
    for a,n in [(0x10000,0x100000),(0x200000,0x10000),(0x300000,0x10000),(0x400000,0x1000)]:u.mem_map(a,n)
    for s in image.sections:
        if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
    def put(a,*v):u.mem_write(a,struct.pack('<'+'I'*len(v),*[x&0xffffffff for x in v]))
    def word(a):return struct.unpack('<I',u.mem_read(a,4))[0]
    cursor=names['_codeview_scope_cursor']if right else 0x7e8ac;put(cursor,0x201000 if depths else 0);put(names['_optimize_global']if right else 0x80624,optimize)
    for i,depth in enumerate(depths):
        a=0x201000+24*i;u.mem_write(a,struct.pack('<hHIIIII',depth,0,10*i,10*i+8,0x203000+0x100*i if records else 0,0,0x201000+24*(i+1)if i+1<len(depths)else 0))
        for j in range(records):put(0x203000+0x100*i+8*j,0x203000+0x100*i+8*(j+1)if j+1<records else 0,0x208000+32*(i*records+j))
    put(0x300800,0x400000);u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,0x200000)
    block=names['@write_codeview_block@12']if right else 0x69b20;symbol=names['@write_debug_symbol@4']if right else 0x69930;end=names['_begin_debug_function']if right else 0x69210;events=[]
    def hook(m,a,size,unused):
        if a==0x400000:m.emu_stop();return
        if a not in [block,symbol,end]:return
        esp=m.reg_read(UC_X86_REG_ESP);ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);pop=4
        if a==block:assert ecx==0x200000;events.append(['block',edx,word(esp+4),word(cursor)]);pop=8
        elif a==symbol:assert 0x208000<=ecx<0x209000;events.append(['symbol',ecx,word(cursor)])
        else:events.append(['end',word(cursor)])
        m.reg_write(UC_X86_REG_EAX,0xabcdef03);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_EIP,word(esp));m.reg_write(UC_X86_REG_ESP,esp+pop)
    u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names['@write_debug_scopes@4']if right else 0x69580,0,count=100000);assert u.reg_read(UC_X86_REG_ESP)==0x300804
    return dict(events=events,cursor=word(cursor))
patterns=[[],[0],[1,1],[1,2,1],[2,3,4,2,1],[3,4,5,4,3,2],[3,3,2],[-1,0,-1]];rows=[]
for depths,records,optimize in itertools.product(patterns,[0,1,2],[0,1]):
    a=run(original,False,depths,records,optimize);b=run(canonical,True,depths,records,optimize);assert a==b,(depths,records,optimize,a,b);rows.append(dict(depths=depths,records=records,optimize=optimize,output=a))
Path('cases/codeview-scopes-validation.json').write_text(json.dumps(dict(cases=rows,seconds=time.perf_counter()-started,scope='Whole207-byte original versus actual original-compiler C, including real self-recursion.48 scope chains exercise signed depth, same/deeper/outside levels,0/1/2 record lists, optimize flag, block begin/end order and shared final cursor. Block/symbol/S_END children explicitly mocked; their bodies/ABIs/failure behavior are not accepted by these controls. Actual native formal/global head and fresh next trap separate.'),indent=2)+'\n');print(len(rows),'whole original/canonical recursive scope states match')
