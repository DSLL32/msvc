"""Whole32-bit CodeView procedure record and backend relocation call shape."""
from pathlib import Path
import itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_indexes import indexes,publics
started=time.perf_counter();original,canonical,config,left,right=indexes();names=publics()
def run(image,right,storage,name_length,frame,code,type):
    u=Uc(UC_ARCH_X86,UC_MODE_32)
    for a,n in [(0x10000,0x100000),(0x200000,0x10000),(0x300000,0x10000),(0x400000,0x1000)]:u.mem_map(a,n)
    for s in image.sections:
        if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
    def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
    def word(a):return struct.unpack('<I',u.mem_read(a,4))[0]
    put(names['_debug_symbols_segment']if right else 0x7914c,0x201000);put(0x201004,100)
    callbacks=names['_output_callbacks']if right else 0x7b8f0
    for offset,target in [(40,0x400100),(36,0x400200),(16,0x400300)]:put(callbacks+offset,target)
    u.mem_write(0x200000,bytes(84));u.mem_write(0x20000c,b'\4');put(0x200020,0xface0000|type);put(0x200024,storage);put(0x200038,code);u.mem_write(0x20003c,struct.pack('<HH',*frame));name=b'x'*name_length;u.mem_write(0x202000,name+b'\0')
    put(0x300800,0x400000);u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,0x200000)
    display=names['@format_function_name@4']if right else 0x4df70;copy=names['@copy_codeview_string@8']if right else 0x69520;events=[];record=[]
    def hook(m,a,size,unused):
        if a==0x400000:m.emu_stop();return
        if a not in [display,copy,0x400100,0x400200,0x400300]:return
        esp=m.reg_read(UC_X86_REG_ESP);ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);result=0
        if a==display:assert ecx==0x200000;events.append('name');result=0x202000
        elif a==0x400100:assert ecx==0x201000;events.append('select')
        elif a==copy:
            assert edx==0x202000;events.append('copy');m.mem_write(ecx,bytes([name_length])+name);m.mem_write(ecx+name_length+2,b'\0');result=name_length+1
        elif a==0x400200:
            events.append('data');assert edx==name_length+38;record.append(bytes(m.mem_read(ecx,edx)).hex())
        else:events.append('size');assert ecx==(26-(name_length+36))&0xffffffff and edx==0x200000
        m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_EIP,word(esp));m.reg_write(UC_X86_REG_ESP,esp+4)
    u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names['@write_codeview_function@4']if right else 0x69650,0,count=100000);assert u.reg_read(UC_X86_REG_ESP)==0x300804
    return dict(events=events,record=record,segment_offset=word(0x201004))
rows=[]
for values in itertools.product([0x30,0x50],[0,1,29,247,255],[(0,0),(9,180),(32767,32767),(32768,32768),(65535,65535)],[0,185,0xffffffff],[0,4099,65535]):
    a=run(original,False,*values);b=run(canonical,True,*values);assert a==b,(values,a,b);rows.append(dict(inputs=values,output=a))
Path('cases/codeview-procedure-validation.json').write_text(json.dumps(dict(cases=rows,seconds=time.perf_counter()-started,scope='Whole216-byte original versus actual original-compiler C.450 cases check packed length/kind/fields, signed16-bit prologue/epilogue promotion, lowWORD type, names0..255, call order, relocation displacement/symbol and segment offset. Display-name/counted-copy/output callbacks explicitly mocked, not child behavior acceptance. Names>255 and original oversize emitted-length policy are not exercised; no invented bound/normalization. Real native formal/frontier separately observed.'),indent=2)+'\n');print(len(rows),'whole original/canonical procedure records match')
