"""Whole CodeView S_UDT record packing and counted type name."""
from pathlib import Path
import itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_indexes import indexes,publics
started=time.perf_counter();original,canonical,config,left,right=indexes();names=publics()
def run(image,right,length,type):
    u=Uc(UC_ARCH_X86,UC_MODE_32)
    for a,n in [(0x10000,0x100000),(0x200000,0x10000),(0x300000,0x10000),(0x400000,0x1000)]:u.mem_map(a,n)
    for s in image.sections:
        if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
    def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
    def word(a):return struct.unpack('<I',u.mem_read(a,4))[0]
    put(names['_debug_symbols_segment']if right else 0x7914c,0x201000);put(0x201004,100)
    callbacks=names['_output_callbacks']if right else 0x7b8f0
    for off,target in [(40,0x400100),(36,0x400200)]:put(callbacks+off,target)
    u.mem_write(0x200000,bytes(44));u.mem_write(0x20000c,b'\13');put(0x200020,0xa5a50000|type);text=b'x'*length;u.mem_write(0x202000,text+b'\0')
    put(0x300800,0x400000);u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,0x200000)
    display=names['@symbol_display_name@4']if right else 0x4e000;diagnostic=names['@format_function_name@4']if right else 0x4df70;copy=names['@copy_codeview_string@8']if right else 0x69520;events=[];record=[]
    def hook(m,a,size,unused):
        if a==0x400000:m.emu_stop();return
        if a not in [display,diagnostic,copy,0x400100,0x400200,0x400300]:return
        esp=m.reg_read(UC_X86_REG_ESP);ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);result=0
        if a in [display,diagnostic]:assert ecx==0x200000 and a==diagnostic;events.append('name');result=0x202000
        elif a==copy:assert edx==0x202000;events.append('copy');m.mem_write(ecx,bytes([length])+text);m.mem_write(ecx+length+2,b'\0');result=length+1
        elif a==0x400100:assert ecx==0x201000;events.append('select')
        elif a==0x400200:
            assert edx==length+7;events.append('data');record.append(bytes(m.mem_read(ecx,edx)).hex())
        else:raise AssertionError('Unexpected relocation callback')
        m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_EIP,word(esp));m.reg_write(UC_X86_REG_ESP,esp+4)
    u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names['@write_codeview_typedef@4']if right else 0x69d10,0,count=100000);assert u.reg_read(UC_X86_REG_ESP)==0x300804
    return dict(events=events,record=record,segment_offset=word(0x201004))
rows=[]
for values in itertools.product([0,10,247,255],[0,4117,65535]):
    a=run(original,False,*values);b=run(canonical,True,*values);assert a==b,(values,a,b);rows.append(dict(input=values,output=a))
Path('cases/codeview-typedef-validation.json').write_text(json.dumps(dict(cases=rows,seconds=time.perf_counter()-started,scope='Whole110-byte original versus actual original-compiler C.12 typeWORD/name-length states compare full packed S_UDT bytes, counted names, call order and segment offset. Name/copy/output callbacks explicitly mocked; child behavior/failures not accepted. Actual global descriptor/formal and fresh next trap separately observed. Names>255 original precomputed-length behavior unexercised; no extra guard.'),indent=2)+'\n');print(len(rows),'whole original/canonical CodeView UDT records match')
