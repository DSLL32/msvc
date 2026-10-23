"""Whole S_BPREL32 frame-variable packing and post-select input ordering."""
from pathlib import Path
import hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_indexes import indexes,publics
started=time.perf_counter();original,canonical,config,left,right=indexes();names=publics()
def run(image,right,flags,role,type,length,offset,mutate):
    u=Uc(UC_ARCH_X86,UC_MODE_32)
    for a,n in [(0x10000,0x100000),(0x200000,0x10000),(0x300000,0x10000),(0x400000,0x1000)]:u.mem_map(a,n)
    for s in image.sections:
        if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
    def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
    def word(a):return struct.unpack('<I',u.mem_read(a,4))[0]
    put(names['_debug_symbols_segment']if right else 0x7914c,0x201000);put(0x201004,100);put(names['_current_function']if right else 0x831b4,0x203000);put(0x203028,flags)
    callbacks=names['_output_callbacks']if right else 0x7b8f0
    for off,target in [(40,0x400100),(36,0x400200)]:put(callbacks+off,target)
    u.mem_write(0x200000,bytes(44));u.mem_write(0x20000c,b'\1');put(0x200004,offset);put(0x200020,0xa5a50000|type);put(0x200024,role|0x80|0x4000);text=b'x'*length;u.mem_write(0x202000,text+b'\0')
    put(0x300800,0x400000);u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,0x200000)
    diagnostic=names['@format_function_name@4']if right else 0x4df70;copy=names['@copy_codeview_string@8']if right else 0x69520;events=[];record=[]
    def hook(m,a,size,unused):
        if a==0x400000:m.emu_stop();return
        if a not in [diagnostic,copy,0x400100,0x400200]:return
        esp=m.reg_read(UC_X86_REG_ESP);ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);result=0
        if a==diagnostic:assert ecx==0x200000;events.append('name');result=0x202000
        elif a==copy:assert edx==0x202000;events.append('copy');m.mem_write(ecx,bytes([length])+text);m.mem_write(ecx+length+2,b'\0');result=length+1
        elif a==0x400100:
            assert ecx==0x201000;events.append('select')
            if mutate:put(0x200004,word(0x200004)+13)
        else:assert edx==length+11;events.append('data');record.append(bytes(m.mem_read(ecx,edx)).hex())
        m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_EIP,word(esp));m.reg_write(UC_X86_REG_ESP,esp+4)
    u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names['@write_codeview_frame_variable@4']if right else 0x69730,0,count=100000);assert u.reg_read(UC_X86_REG_ESP)==0x300804
    return dict(events=events,record=record,segment_offset=word(0x201004),input_offset=word(0x200004))
transcript=hashlib.sha256();count=0;witnesses=[]
for values in itertools.product([0,0x10,9,0x19],[0x10,0x30,0x70],[0,4097,65535],[0,6,247,255],[0,8,0xfffffff8,0x80000000],[0,1]):
    a=run(original,False,*values);b=run(canonical,True,*values);assert a==b,(values,a,b);transcript.update(json.dumps([values,a],sort_keys=True,separators=(',',':')).encode());count+=1
    if values[1]==0x30 and values[2]==4097 and values[3]==6 and values[4]==8:witnesses.append(dict(input=values,output=a))
Path('cases/codeview-frame-validation.json').write_text(json.dumps(dict(cases=count,axes=['function_flags','role70','type_word','name_length','offset','select_mutation'],transcript_sha256=transcript.hexdigest(),witnesses=witnesses,seconds=time.perf_counter()-started,scope='Whole143-byte original versus actual original-compiler C.1152 states cover packed S_BPREL32 fields, typeWORD, unsigned offset bits/wrap, function10/role30 correction, counted names and call order/segment offset. Select child controlled mutation proves symbol offset read follows select (not actual child behavior). All helper callbacks mocked; actual descriptor/member/flags and native next trap separate. Names>255 omitted original length behavior unexercised; no guard or normalization.'),indent=2)+'\n');print(count,'whole original/canonical frame-variable records match')
