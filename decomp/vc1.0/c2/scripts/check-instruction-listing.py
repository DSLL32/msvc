"""All legal opcode/prefix and code-byte modes of the actual C listing driver."""
from pathlib import Path
import json,re,struct,subprocess,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP
from binary import PE
started=time.perf_counter();pe=PE('../../../toolchains/1.0-nt/bin/C23232.EXE');names=re.findall(r'^OP\((\w+),',Path('src/machine-opcodes.def').read_text(),re.M)
native=subprocess.check_output(['build/native/check-instruction-listing'],text=True).splitlines();assert len(native)==2*len(names)
repeat=[name for i,name in enumerate(names)if struct.unpack('<H',pe.read(0x82210+2*i,2))[0]&0x100];assert repeat==['REP','REPNZ','REPZ']
rows=[]
for operation,name in enumerate(names):
    for code in [0,1]:
        u=Uc(UC_ARCH_X86,UC_MODE_32)
        for address,size in [(0x10000,0x90000),(0x200000,0x10000),(0x300000,0x10000),(0x400000,0x1000)]:u.mem_map(address,size)
        for s in pe.sections:
            if s['size']:u.mem_write(pe.base+s['rva'],pe.data[s['off']:s['off']+s['size']])
        def word(a):return struct.unpack('<I',u.mem_read(a,4))[0]
        def put(a,*v):u.mem_write(a,struct.pack('<'+'I'*len(v),*v))
        put(0x200000,0x200200,0,operation<<8);put(0x200100,0x200300);put(0x201000,17);put(0x80d40,0);put(0x80610,code);put(0x300800,0x400000)
        u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,0x200000);u.reg_write(UC_X86_REG_EDX,0x201000);events=[];buffers=[]
        def hook(m,a,size,unused):
            if a==0x400000:m.emu_stop();return
            if a not in [0x68c10,0x25540,0x29a60,0x29b30,0x29bb0,0x71b40]:return
            stack=m.reg_read(UC_X86_REG_ESP);ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);pop=4
            if a==0x68c10:assert ecx==0x200000;events.append('R;');m.reg_write(UC_X86_REG_EAX,0x200100)
            elif a==0x25540:
                assert ecx in [0x200000,0x200100]and word(stack+8)==0;which=1 if ecx==0x200000 else 2;count=2 if which==1 else 3
                if which==1:buffers.append(edx)
                else:assert edx==buffers[0]+2
                m.mem_write(edx,bytes(range(0x11 if which==1 else 0x21,(0x11 if which==1 else 0x21)+count)));events.append('E%d@%u;'%(which,word(stack+4)));m.reg_write(UC_X86_REG_EAX,count);pop=12
            elif a==0x29a60:
                assert word(stack+4)==buffers[0]and edx<=5;events.append('B%u:%u:%s;'%(ecx,edx,bytes(m.mem_read(word(stack+4),edx)).hex()));pop=8
            elif a==0x29b30:assert ecx in [0x200000,0x200100];events.append('T%d;'%(1 if ecx==0x200000 else 2))
            elif a==0x29bb0:assert ecx==0x200000;events.append('C1;')
            else:
                fmt=bytes(m.mem_read(word(stack+4),2));assert fmt in[b'\n\0',b'\t\0',b' \0'];events.append({b'\n\0':'N;',b'\t\0':'TAB;',b' \0':'SP;'}[fmt]);m.reg_write(UC_X86_REG_EAX,1)
            m.reg_write(UC_X86_REG_EIP,word(stack));m.reg_write(UC_X86_REG_ESP,stack+pop)
        u.hook_add(UC_HOOK_CODE,hook);u.emu_start(0x28370,0,count=10000);result=u.reg_read(UC_X86_REG_EAX);assert result in [0x200200,0x200300]
        expected='%u %u %u %u %u %s'%(operation,code,word(0x201000),word(0x80d40),1 if result==0x200200 else 2,''.join(events));assert native[2*operation+code]==expected,(name,code,native[2*operation+code],expected)
        rows.append(dict(operation=name,code_bytes=bool(code),result=expected))
Path('cases/instruction-listing-validation.json').write_text(json.dumps(dict(opcode_rows=len(names),cases=rows,repeat_property_original='82210 word bit0x100',repeat_operations=repeat,seconds=time.perf_counter()-started,scope='Actual C driver, actual C/original column padding and compiled normalized machine_selection_properties versus full original28370..28434 with original231-row property view. Explicit encoder/next-operation/printing/comment mocks verify argument shapes, buffer chaining, order, location update, selected successor and column reset. No unrecovered child implementation or branch-execution claim for the real compile case.'),indent=2)+'\n')
print(len(rows),'whole original/native listing-driver opcode/mode states match')
