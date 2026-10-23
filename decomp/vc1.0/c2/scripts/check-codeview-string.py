"""Compare actual native counted-string C with isolated original machine code."""
from pathlib import Path
import hashlib,json,subprocess,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
from binary import PE
started=time.perf_counter();pe=PE('../../../toolchains/1.0-nt/bin/C23232.EXE');executable=Path('build/native/check-codeview-string');rows=[]
for count in [0,1,2,31,254,255,256,300,1000]:
    source=bytes(65+i%26 for i in range(count))+b'\0';u=Uc(UC_ARCH_X86,UC_MODE_32)
    for address in [0x69000,0x72000,0x70000,0x200000,0x300000,0x400000]:u.mem_map(address,0x1000)
    u.mem_write(0x69520,pe.read(0x69520,0x60));u.mem_write(0x72c00,pe.read(0x72c00,0x25));u.mem_write(0x200000,b'\xcc'*512);u.mem_write(0x300000,source)
    u.mem_write(0x400800,(0x400000).to_bytes(4,'little'));u.reg_write(UC_X86_REG_ESP,0x400800);u.reg_write(UC_X86_REG_ECX,0x200000);u.reg_write(UC_X86_REG_EDX,0 if count==1000 else 0x300000);warnings=[]
    def hook(machine,address,size,unused):
        if address==0x400000:machine.emu_stop()
        elif address==0x70490:
            stack=machine.reg_read(UC_X86_REG_ESP);data=machine.mem_read(stack,16);values=[int.from_bytes(data[i:i+4],'little')for i in range(0,16,4)];assert values[1:3]==[1,0x312];warnings.append(values[3]);machine.reg_write(UC_X86_REG_ESP,stack+4);machine.reg_write(UC_X86_REG_EIP,values[0])
    u.hook_add(UC_HOOK_CODE,hook);u.emu_start(0x69520,0,count=100000)
    original=(u.reg_read(UC_X86_REG_EAX),len(warnings),warnings[0]if warnings else 0,bytes(u.mem_read(0x200000,512)))
    output=subprocess.check_output([str(executable),str(count)],text=True).splitlines();native=(*map(int,output[0].split()),bytes.fromhex(output[1]));assert native==original,(count,native[:3],original[:3])
    length=0 if count==1000 else min(count,255)
    assert original[3][length+1]==0xcc and original[3][length+2]==0
    rows.append(dict(source_bytes=None if count==1000 else count,result=original[0],warnings=warnings,buffer_sha256=hashlib.sha256(original[3]).hexdigest()))
Path('cases/codeview-string-validation.json').write_text(json.dumps(dict(cases=rows,seconds=time.perf_counter()-started,method='Original69520..69580 plus exact original strncpy leaf in Unicorn; actual compiled native C helper; diagnostic edge captures actual cdecl arguments without inventing diagnostic execution',limit='Helper scalar/buffer/ABI shapes only; does not claim long-name diagnostics executed in the listing case'),indent=2)+'\n');print(len(rows),'original/native counted-string shapes match')
