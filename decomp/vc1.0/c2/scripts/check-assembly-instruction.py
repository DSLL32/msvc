"""Probe whole assembly-instruction normalization with an explicit operand seam."""
from pathlib import Path
import hashlib,itertools,json,re,struct,subprocess,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP
from binary import PE
started=time.perf_counter();pe=PE('../../../toolchains/1.0-nt/bin/C23232.EXE');names=re.findall(r'^OP\((\w+),',Path('src/machine-opcodes.def').read_text(),re.M);ids={n:i for i,n in enumerate(names)}
# Every original property word independently checks the normalized float test.
for operation in range(len(names)):
    expected=bool(struct.unpack('<H',pe.read(0x82210+operation*2,2))[0]&1)
    actual=ids['FADD']<=operation<=ids['FLD1']or ids['FUCOM']<=operation<=ids['FCOS']
    assert actual==expected,(names[operation],actual,expected)
cases=[]
def case(name,size,*operands):cases.append((name,size,list(operands)))
for size in [1,2,4]:case('MOV',size,(1,4),(1,7))
for name in ['INC','DEC']:case(name,4,(1,2),(2,1))
for name in ['FADD','FMUL','FSUB','FSUBR','FDIV','FDIVR']:case(name,8,(1,40),(1,47))
for name in ['FILD','FLD','FIST','FISTP','FST','FSTP']:case(name,8,(1,41))
for name in ['FSTSW','FNSTSW']:case(name,2,(1,0))
for size in [1,2]:case('CBW',size)
for size in [2,4]:case('CWD',size)
for condition in [0,3,15]:case('SCC',condition,(1,7))
for name in ['JCC','JMP','CALL','JCXZ','LOOP','LOOPNZ','LOOPZ']:
    for mode in range(4):case(name,4,(6,mode))
case('EMIT',1,(2,77));case('IMUL3',4,(1,2),(1,3),(2,17));case('IMUL3',4,(1,2),(1,3),(6,2))
rows=[]
for name,size,operands in cases:
    operation=ids[name];payload=bytes([3+8*len(operands),operation,size])+b''.join(struct.pack('<II',*op)for op in operands)
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x90000);u.mem_map(0x200000,0x100000);u.mem_map(0x300000,0x10000);u.mem_map(0x400000,0x1000)
    for section in pe.sections:
        if section['size']:u.mem_write(pe.base+section['rva'],pe.data[section['off']:section['off']+section['size']])
    def word(address):return struct.unpack('<I',u.mem_read(address,4))[0]
    def put(address,*values):u.mem_write(address,struct.pack('<'+'I'*len(values),*values))
    u.mem_write(0x200000,payload);put(0x300800,0x400000);put(0x7b950,0);put(0x7b960,0)
    u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,0x200000);u.reg_write(UC_X86_REG_EDX,0x220000);decoded=[];allocations=[]
    def hook(machine,address,count,unused):
        if address==0x400000:machine.emu_stop();return
        if address==0x6fb70:raise AssertionError((name,size,operands,'original diagnostic',hex(machine.reg_read(UC_X86_REG_EDX))))
        if address not in [0x1ae20,0x70ae0]:return
        stack=machine.reg_read(UC_X86_REG_ESP);cleanup=4
        if address==0x1ae20:
            destination=machine.reg_read(UC_X86_REG_ECX);cursor=machine.reg_read(UC_X86_REG_EDX);assert word(stack+4)==0x220000
            flags,value=struct.unpack('<II',machine.mem_read(cursor,8))
            if flags&15==6:
                pointer=0x240000+len(decoded)*0x100;put(pointer,4 if value&1 else 0,0x220000 if value&2 else 0,0,0);value=pointer
            put(destination,flags,value);decoded.append((flags,value));result=cursor+8;cleanup=8
        else:
            assert machine.reg_read(UC_X86_REG_ECX)==1
            length=machine.reg_read(UC_X86_REG_EDX);result=0x230000+len(allocations)*0x100;machine.mem_write(result,b'\0'*length);allocations.append(length)
        machine.reg_write(UC_X86_REG_EAX,result);machine.reg_write(UC_X86_REG_ESP,stack+cleanup);machine.reg_write(UC_X86_REG_EIP,word(stack))
    u.hook_add(UC_HOOK_CODE,hook);u.emu_start(0x1a980,0,count=100000)
    instruction=u.reg_read(UC_X86_REG_EAX);opcode=word(instruction+8);actual_operation=opcode>>8&255;original=[opcode,len(decoded),int(bool(word(0x7b960)))]
    for index in range(2):
        operand=instruction+16+index*8;flags=word(operand);value=word(operand+4);kind=flags&15;row=[flags,0,0,0,0,0,0,0]
        if index==1 and actual_operation==ids['CALL']:row[6:8]=[word(value),word(value+4)]
        elif index==1 and actual_operation in [ids['JCC'],ids['SCC']]:pass
        elif 3<=kind<=7 or kind==10:row[2:6]=[word(value),42 if word(value+4)==0x220000 else 0,word(value+8),word(value+12)]
        elif kind==8:row[1]=42 if value==0x220000 else 0
        else:row[1]=value
        original+=row
    original += [len(allocations),*allocations]
    arguments=[str(operation),str(size),*[str(value)for operand in operands for value in operand]]
    native=list(map(int,subprocess.check_output(['build/native/check-assembly-instruction',*arguments],text=True).split()))
    assert native==original,(name,size,operands,native,original)
    rows.append(dict(operation=name,size=size,operands=operands,normalized=original))
Path('cases/assembly-instruction-validation.json').write_text(json.dumps(dict(target_sha256=hashlib.sha256(pe.data).hexdigest(),function='1a980..1ae11',cases=rows,opcode_property_words=len(names),seconds=time.perf_counter()-started,method='Actual compiled C parent/new_machine_record/initialize_operand/address allocator and recycler versus original whole corresponding machine bodies; decoded operand and zeroed arena allocator contracts are explicit seams.',limits='Serialized operand parsing is not implemented by this probe: named decode_assembly_operand remains a trapping reconstruction stub. Only supplied valid operand shapes and C allocator success are covered; error paths and other serialized opcodes are not claimed exercised.'),indent=2)+'\n')
print(len(rows),'whole original/native instruction shapes and',len(names),'float-property classifications match')
