"""Whole S_COMPILE packed record and shared compiler-string identity."""
from pathlib import Path
import itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_indexes import indexes,publics
started=time.perf_counter();original,canonical,config,left,right=indexes();names=publics();assert original.string(0x852b8)==canonical.string(names['_compiler_identification'])
def run(image,right,cpu,frontend):
    u=Uc(UC_ARCH_X86,UC_MODE_32)
    for a,n in [(0x10000,0x100000),(0x200000,0x10000),(0x300000,0x10000),(0x400000,0x1000)]:u.mem_map(a,n)
    for s in image.sections:
        if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
    def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
    def word(a):return struct.unpack('<I',u.mem_read(a,4))[0]
    text_address=names['_compiler_identification']if right else 0x852b8;text=image.string(text_address).encode('latin1');length=len(text)
    put(names['_cpu_model']if right else 0x806a4,cpu);put(names['_floating_frontend']if right else 0x83138,frontend);put(names['_debug_symbols_segment']if right else 0x7914c,0x201000);put(0x201004,100)
    callbacks=names['_output_callbacks']if right else 0x7b8f0
    for off,target in [(40,0x400100),(36,0x400200)]:put(callbacks+off,target)
    put(0x300800,0x400000);u.reg_write(UC_X86_REG_ESP,0x300800);copy=names['@copy_codeview_string@8']if right else 0x69520;events=[];record=[]
    def hook(m,a,size,unused):
        if a==0x400000:m.emu_stop();return
        if a not in [copy,0x400100,0x400200]:return
        esp=m.reg_read(UC_X86_REG_ESP);ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);result=0
        if a==copy:assert edx==text_address;events.append('copy_shared_compiler_string');m.mem_write(ecx,bytes([length])+text);m.mem_write(ecx+length+2,b'\0');result=length+1
        elif a==0x400100:assert ecx==0x201000;events.append('select')
        else:assert edx==length+9;events.append('data');record.append(bytes(m.mem_read(ecx,edx)).hex())
        m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_EIP,word(esp));m.reg_write(UC_X86_REG_ESP,esp+4)
    u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names['_write_codeview_compile_record']if right else 0x69bd0,0,count=100000);assert u.reg_read(UC_X86_REG_ESP)==0x300804
    return dict(events=events,record=record,segment_offset=word(0x201004))
rows=[]
for values in itertools.product([-1,0,3,5,256,257],[-1,0,1,2,256]):
    a=run(original,False,*values);b=run(canonical,True,*values);assert a==b,(values,a,b);rows.append(dict(input=values,output=a))
Path('cases/codeview-compile-validation.json').write_text(json.dumps(dict(cases=rows,compiler_string_original='0x852b8',compiler_string_canonical=hex(names['_compiler_identification']),seconds=time.perf_counter()-started,scope='Whole134-byte original versus actual original-compiler C.30 packed S_COMPILE records cover CPU lowBYTE/frontend nonzero-language2, shared compiler-string exact bytes+actual copy argument address, length/count/name/flags and call order/offset. Copy/output callbacks mocked, not their body acceptance. Actual entry has genuinely zero formals; real case CPU3/frontend0. Arbitrary frontend values are bounded controls, not new valid cases/coverage.'),indent=2)+'\n');print('30 whole original/canonical S_COMPILE records and shared string identity pass')
