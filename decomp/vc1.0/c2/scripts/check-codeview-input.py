"""Whole db-header wrapper control, AL return width and input FILE handover."""
from pathlib import Path
import itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_indexes import indexes,publics
started=time.perf_counter();original,canonical,config,left,right=indexes();names=publics()
def run(image,right,tag,version):
    u=Uc(UC_ARCH_X86,UC_MODE_32)
    for a,n in [(0x10000,0x100000),(0x200000,0x10000),(0x300000,0x10000),(0x400000,0x1000)]:u.mem_map(a,n)
    for s in image.sections:
        if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
    def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
    def word(a):return struct.unpack('<I',u.mem_read(a,4))[0]
    current=names['_input_file']if right else 0x85d9c;put(current,0x205100);put(0x300800,0x400000);u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,0x205000)
    byte=names['_input_byte']if right else 0x6fb90;read=names['@read_initializers@4']if right else 0x4e840;error=names['@compiler_internal_error@8']if right else 0x6fb70;events=[];count=[0];terminal=['return']
    def hook(m,a,size,unused):
        if a==0x400000:m.emu_stop();return
        if a not in [byte,read,error]:return
        esp=m.reg_read(UC_X86_REG_ESP);result=0;assert word(current)==0x205000
        if a==byte:
            assert count[0]<2;value=tag if count[0]==0 else version;count[0]+=1;events.append(['byte',value]);result=0xface0000|value
        elif a==read:assert m.reg_read(UC_X86_REG_ECX)==0x205000 and count[0]==2;events.append(['initializers',0x205000])
        else:
            source=bytes(m.mem_read(m.reg_read(UC_X86_REG_ECX),64)).split(b'\0')[0].decode();assert source=='@(#)emit.c:1.20'and m.reg_read(UC_X86_REG_EDX)==0x328;events.append(['diagnostic',source,0x328]);terminal[0]='named diagnostic';m.emu_stop();return
        m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_EIP,word(esp));m.reg_write(UC_X86_REG_ESP,esp+4)
    u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names['@read_codeview_initializers@4']if right else 0x69ba0,0,count=10000)
    if tag==12:assert u.reg_read(UC_X86_REG_ESP)==0x300804
    return dict(events=events,terminal=terminal[0],input_file=word(current))
rows=[]
for values in itertools.product([0,12,255],[0,1,255]):
    a=run(original,False,*values);b=run(canonical,True,*values);assert a==b,(values,a,b);rows.append(dict(input=values,output=a))
Path('cases/codeview-input-validation.json').write_text(json.dumps(dict(cases=rows,seconds=time.perf_counter()-started,scope='Whole48-byte original versus actual original-compiler C: input_file handover precedes input_byte, AL-only header12 comparison despite poisoned upper EAX, ignored second byte and unchanged FILE forward. Byte/initializer/diagnostic children mocked. Invalid headers stop at named diagnostic; no real parsing/file/CRT failure acceptance. Actual FILE owner/state and original/native frontier separately captured.'),indent=2)+'\n');print('9 whole original/canonical db wrapper/AL-width states match')
