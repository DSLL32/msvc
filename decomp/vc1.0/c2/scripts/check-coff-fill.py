"""Compare actual C fill writer with original code, including odd direct requests."""
from pathlib import Path
import hashlib,itertools,json,struct,subprocess,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
from binary import PE
started=time.perf_counter();pe=PE('../../../toolchains/1.0-nt/bin/C23232.EXE');executable=Path('build/native/check-coff-fill');rows=[]
cases=[(1,0,b,n,v,1000)for b,n,v in itertools.product([0,499,500],[0,1,2,500,501,1001],[0,1,128,255])]
cases += [(1,1,b,n,v,1000)for b,n,v in itertools.product([0,499],[0,2,1000000],[0,255])]
cases += [(0,0,0,n,v,1000)for n,v in itertools.product([0,1,79,80,81,160],[0,128,255])]
for kind,count_only,buffered,count,value,completed in cases:
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x90000)
    for section in pe.sections:
        if section['size']:u.mem_write(pe.base+section['rva'],pe.data[section['off']:section['off']+section['size']])
    for address in [0x200000,0x210000,0x300000,0x400000,0x500000]:u.mem_map(address,0x10000)
    put=lambda a,*v:u.mem_write(a,struct.pack('<'+'I'*len(v),*v))
    word=lambda a:struct.unpack('<I',u.mem_read(a,4))[0]
    put(0x200000,kind,count_only,completed,buffered,7,0x210000);put(0x210000,0x210000);u.mem_write(0x210004,b'\xcc'*500)
    put(0x851bc,0x200000);put(0x851c4,0);put(0x790fc,0x500000);put(0x79064,0x200f00)
    put(0x300800,0x400000);u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,value);u.reg_write(UC_X86_REG_EDX,count);writes=[];allocated=[]
    def hook(machine,address,size,unused):
        if address==0x400000:machine.emu_stop();return
        if address not in [0x500000,0x72c30]:return
        stack=machine.reg_read(UC_X86_REG_ESP)
        if address==0x500000:
            assert machine.reg_read(UC_X86_REG_ECX)==508
            pointer=0x211000+len(allocated)*0x1000;machine.mem_write(pointer,b'\0'*508);allocated.append(pointer);result=pointer
        else:
            pointer,n,items,file=struct.unpack('<IIII',machine.mem_read(stack+4,16));assert items==1 and file==0x200f00 and pointer==0x7e8d0
            assert bytes(machine.mem_read(pointer,80))==bytes([value])*80
            writes.append(n);result=1
        machine.reg_write(UC_X86_REG_EAX,result);machine.reg_write(UC_X86_REG_ESP,stack+4);machine.reg_write(UC_X86_REG_EIP,word(stack))
    u.hook_add(UC_HOOK_CODE,hook);u.emu_start(0x6f1c0,0,count=1000000)
    block=0x210000;data=[]
    for unused in range(20):
        data.append(bytes(u.mem_read(block+4,500)));block=word(block)
        if block==0x210000:break
    else:raise AssertionError('Original block ring failed to close')
    original=(word(0x20000c),word(0x200008),word(0x200010),len(allocated),writes,data)
    lines=subprocess.check_output([str(executable),*map(str,[kind,count_only,buffered,count,value,completed])],text=True,timeout=3).splitlines();header=list(map(int,lines[0].split()));write_line=list(map(int,lines[1].split()));assert write_line[0]==len(write_line)-1
    native=(*header,write_line[1:],[bytes.fromhex(line)for line in lines[2:]])
    assert native==original,((kind,count_only,buffered,count,value),native[:5],original[:5])
    rows.append(dict(kind=kind,count_only=count_only,buffered=buffered,count=count,value=value,result=header,writes=writes,blocks_sha256=hashlib.sha256(b''.join(data)).hexdigest()))
Path('cases/coff-fill-validation.json').write_text(json.dumps(dict(cases=rows,seconds=time.perf_counter()-started,
 method='Actual compiled native C and original6f1c0..6f36e/new-block helper in Unicorn; actual500-byte ring contents/counters, allocator calls and direct fwrite requests compared',
 direct_file_limit='Mock fwrite observes count/80 defined scratch bytes only. It never reads the original branch\'s out-of-contract oversized span; no adjacent data or real direct-file conformance claimed.',
 error_limit='No failing allocator/fwrite/error callback branch is exercised; those remain the original named call edges.'),indent=2)+'\n');print(len(rows),'original/native fill states and direct request shapes match')
