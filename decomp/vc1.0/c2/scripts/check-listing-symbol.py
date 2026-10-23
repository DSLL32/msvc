"""Actual C/original symbol formats and local handled-bit control, no Wine."""
from pathlib import Path
import itertools,json,struct,subprocess,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP
from binary import PE
started=time.perf_counter();pe=PE('../../../toolchains/1.0-nt/bin/C23232.EXE');rows=[]
def word(u,a):return struct.unpack('<I',u.mem_read(a,4))[0]
def string(u,a):return bytes(u.mem_read(a,100)).split(b'\0')[0]
cases=list(itertools.product([1],[0,1,4,19],range(8),[0,0x10,0x30,0x60],[0xffffffff],[0]))
cases+=list(itertools.product([0],[0,1,4,19],[0],[0],[0,17,0xffffffff],[0,1]))
for local,kind,flags,attributes,value,null in cases:
    u=Uc(UC_ARCH_X86,UC_MODE_32)
    for address,size in [(0x10000,0x90000),(0x200000,0x10000),(0x300000,0x10000),(0x400000,0x1000)]:u.mem_map(address,size)
    for s in pe.sections:
        if s['size']:u.mem_write(pe.base+s['rva'],pe.data[s['off']:s['off']+s['size']])
    symbol=0x200000;u.mem_write(symbol+4,struct.pack('<I',value));u.mem_write(symbol+12,bytes([kind]));u.mem_write(symbol+34,struct.pack('<H',flags));u.mem_write(symbol+36,struct.pack('<I',attributes));u.mem_write(0x80d40,struct.pack('<I',5));u.mem_write(0x201000,b'display\0')
    u.mem_write(0x300800,struct.pack('<I',0x400000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,0 if null else symbol);output=bytearray();formats=[]
    def hook(m,a,size,unused):
        if a==0x400000:m.emu_stop();return
        if a not in [0x4e000,0x71b40]:return
        stack=m.reg_read(UC_X86_REG_ESP)
        if a==0x4e000:
            assert m.reg_read(UC_X86_REG_ECX)==symbol;m.reg_write(UC_X86_REG_EAX,0x201000)
        else:
            fmt=string(m,word(m,stack+4));formats.append(fmt.decode());args=stack+8
            if fmt==b'%s = %ld':piece=string(m,word(m,args))+b' = '+str(struct.unpack('<i',m.mem_read(args+4,4))[0]).encode()
            elif fmt==b'%ld+':piece=str(struct.unpack('<i',m.mem_read(args,4))[0]).encode()+b'+'
            elif fmt==b'%s':piece=string(m,word(m,args))
            else:assert fmt==b'\n',fmt;piece=fmt
            output.extend(piece);m.reg_write(UC_X86_REG_EAX,len(piece))
        m.reg_write(UC_X86_REG_EIP,word(m,stack));m.reg_write(UC_X86_REG_ESP,stack+4)
    u.hook_add(UC_HOOK_CODE,hook);u.emu_start(0x282d0 if local else 0x28230,0,count=10000)
    result=subprocess.check_output(['build/native/check-listing-symbol',*[str(x)for x in [local,kind,flags,attributes,value,null]]],text=True).splitlines()
    expected=[str(word(u,0x80d40))+' '+str(struct.unpack('<H',u.mem_read(symbol+34,2))[0]),output.hex()]
    assert result==expected,(local,kind,flags,attributes,value,null,result,expected)
    rows.append(dict(local=local,kind=kind,flags=flags,attributes=attributes,value=value,null=null,formats=formats,output_hex=output.hex(),final_state=expected[0]))
Path('cases/listing-symbol-validation.json').write_text(json.dumps(dict(cases=rows,seconds=time.perf_counter()-started,scope='Actual compiled symbol/local listing bodies versus full original code. Explicit fixed symbol-name and bounded printf-contract mocks; includes NULL labels, exact%ld+ literal, signed values and all low output-flag combinations. Not a new public CRT or symbol-name ABI acceptance.'),indent=2)+'\n')
print(len(rows),'whole original/native symbol formats and local handled states pass')
