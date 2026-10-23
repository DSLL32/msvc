"""Whole CodeView symbol dispatcher, including real alias self-recursion."""
from pathlib import Path
import hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_indexes import indexes,publics
started=time.perf_counter();original,canonical,config,left,right=indexes();names=publics()
leaves=[('frame',0x69730,'@write_codeview_frame_variable@4'),('register',0x69880,'@write_codeview_register@4'),('local_data',0x697c0,'@write_codeview_local_data@4'),('global_data',0x69f30,'@write_codeview_global_data@4'),('label',0x69e70,'@write_codeview_label@4'),('constant',0x69d80,'@write_codeview_constant@4'),('typedef',0x69d10,'@write_codeview_typedef@4')]
def run(image,right,row):
    kind,forced,tag,flags,role,allocation,register,named,optimize,override,target_kind,mark_effect=row
    u=Uc(UC_ARCH_X86,UC_MODE_32)
    for a,n in [(0x10000,0x100000),(0x200000,0x10000),(0x300000,0x10000),(0x400000,0x1000)]:u.mem_map(a,n)
    for s in image.sections:
        if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
    def put(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
    def word(a):return struct.unpack('<I',u.mem_read(a,4))[0]
    def short(a):return struct.unpack('<H',u.mem_read(a,2))[0]
    def pshort(a,v):u.mem_write(a,struct.pack('<H',v&65535))
    storage=names['_codeview_allocation_class']if right else 0x84ff8;put(storage,override);put(names['_optimize_global']if right else 0x80624,optimize);put((names['_output_callbacks']if right else 0x7b8f0)+72,0x400100)
    for p,k,n,offset in [(0x200000,kind,0x202000 if named else 0,0xfffffff0),(0x201000,target_kind,0x202100,32)]:
        u.mem_write(p,bytes(64));put(p+4,offset);u.mem_write(p+12,struct.pack('<bB',k,tag if p==0x200000 else 0));pshort(p+14,flags if p==0x200000 else 0);put(p+16,n);pshort(p+32,4099 if p==0x200000 else 4100);pshort(p+34,forced if p==0x200000 else 0x20);put(p+36,role|(allocation<<7)|register if p==0x200000 else 0x10|0x8000);put(p+40,77)
    for p,s in [(0x202000,b'alias_name\0'),(0x202100,b'target_name\0'),(0x202200,b'marked_name\0')]:u.mem_write(p,s)
    put(0x300800,0x400000);u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,0x200000)
    targets={names[n]if right else a:label for label,a,n in leaves};lookup=names['@lookup_global_symbol@4']if right else 0x4dce0;events=[]
    def snapshot(p):return [p,word(p+16),short(p+32),word(p+4),short(p+34),word(storage)]
    def hook(m,a,size,unused):
        if a==0x400000:m.emu_stop();return
        if a not in targets and a not in [lookup,0x400100]:return
        esp=m.reg_read(UC_X86_REG_ESP);ecx=m.reg_read(UC_X86_REG_ECX);result=0
        if a==lookup:assert ecx==77;events.append(['lookup',ecx]);result=0x201000
        elif a==0x400100:
            assert ecx==0x201000 and short(ecx+34)&4;events.append(['mark',*snapshot(ecx)])
            if mark_effect:put(ecx+16,0x202200);put(ecx+4,word(ecx+4)+7);pshort(ecx+32,short(ecx+32)^16);pshort(ecx+34,short(ecx+34)|0x40)
        else:events.append([targets[a],*snapshot(ecx)])
        m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_EIP,word(esp));m.reg_write(UC_X86_REG_ESP,esp+4)
    u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names['@write_debug_symbol@4']if right else 0x69930,0,count=100000);assert u.reg_read(UC_X86_REG_ESP)==0x300804
    return dict(events=events,allocation=word(storage),source=bytes(u.mem_read(0x200000,48)).hex(),target=bytes(u.mem_read(0x201000,48)).hex())
inputs=[]
for forced,tag,flags,role,allocation,register,named,override in itertools.product([0,4],[0,31,36],[0,0x800],range(0,0x80,0x10),[0,1,4],[0,0x8000],[0,1],[0,4]):inputs.append((1,forced,tag,flags,role,allocation,register,named,0,override,1,0))
for forced,tag,flags,optimize,override in itertools.product([0,4],[0,31],[0,0x1000],[0,1],[0,4]):inputs.append((3,forced,tag,flags,0,0,0,0,optimize,override,1,0))
for forced,target_kind,role,allocation,override,effect in itertools.product([0,4],[1,3,11],[0x10,0x20,0x60],[0,4],[0,4],[0,1]):inputs.append((7,forced,0,0,role,allocation,0,1,0,override,target_kind,effect))
for kind,forced,override in itertools.product([2,8,11,19],[0,4],[0,4]):inputs.append((kind,forced,0,0,0,0,0,1,0,override,1,0))
transcript=hashlib.sha256();witnesses={}
for row in inputs:
    a=run(original,False,row);b=run(canonical,True,row);assert a==b,(row,a,b);transcript.update(json.dumps([row,a],sort_keys=True,separators=(',',':')).encode())
    for event in a['events']:witnesses.setdefault(event[0],dict(input=row,output=a))
assert set(witnesses)=={'frame','register','local_data','global_data','label','constant','typedef','lookup','mark'}
Path('cases/codeview-dispatch-validation.json').write_text(json.dumps(dict(cases=len(inputs),axis_order=['kind','forced_flag','tag','header_flags','role70','allocation780','register8000','named','optimize','override','alias_target_kind','mark_effect'],transcript_sha256=transcript.hexdigest(),witnesses=witnesses,seconds=time.perf_counter()-started,scope='Whole484-byte original versus actual original-compiler C, including true alias self-recursion. Matrices cover qualification, raw70/780/8000 domains, tags, allocation override, seven leaf routes, target kind/name/type/offset and post-mark restore timing/clear-only-forced-flag. Child leaf/lookup/mark mocked; bounded mark mutations are controls, not actual callback behavior acceptance. Actual first descriptor is unnamed kind3/tag31 and suppressed; cold aliases/other variants are not real-case branch coverage. No new child accepted by mocks.'),indent=2)+'\n');print(len(inputs),'whole original/canonical dispatch states and9 route witnesses match')
