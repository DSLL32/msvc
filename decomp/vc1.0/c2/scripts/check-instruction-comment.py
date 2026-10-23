"""Whole comment writer with named comment/demangle child contracts."""
from pathlib import Path
import itertools,json,re,struct,subprocess,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESP,UC_X86_REG_EIP
from binary import PE
started=time.perf_counter();pe=PE('../../../toolchains/1.0-nt/bin/C23232.EXE');names=re.findall(r'^OP\((\w+),',Path('src/machine-opcodes.def').read_text(),re.M);ids={n:i for i,n in enumerate(names)};cases=[]
for k in range(16):cases.append((k,k,0,0,0,0,ids['MOV']))
for a,b in itertools.product([-2,-1,0,9,10,-2147483648,2147483647],repeat=2):cases.append((2,2,0,0,a,b,ids['MOV']))
for kind,hint in itertools.product([4,5],[0,1,7]):cases.append((kind,kind,hint,0,0,0,ids['MOV']))
for kind,shape in itertools.product([3,6,8],range(6)):cases.append((kind,kind,0,shape,0,0,ids['MOV']))
for operation in ['PUSH','FLD1']:cases.append((2,2,7,1,10,-2,ids[operation]))
for a,b in [(2,8),(8,2),(4,6),(6,5)]:cases.append((a,b,3,1,17,-2,ids['MOV']))
rows=[]
for c in cases:
    a,b,hint,shape,va,vb,operation=c;u=Uc(UC_ARCH_X86,UC_MODE_32)
    for address,n in [(0x10000,0x90000),(0x200000,0x10000),(0x300000,0x10000),(0x400000,0x1000)]:u.mem_map(address,n)
    for s in pe.sections:
        if s['size']:u.mem_write(pe.base+s['rva'],pe.data[s['off']:s['off']+s['size']])
    def word(address):return struct.unpack('<I',u.mem_read(address,4))[0]
    def put(address,*v):u.mem_write(address,struct.pack('<'+'I'*len(v),*[x&0xffffffff for x in v]))
    symbol=0x202000 if shape else 0;kind=1 if shape==1 else 4 if shape==2 else 2 if shape==3 else 4
    put(0x202000,0,0,0,kind|((shape==4)<<8),0x203000);u.mem_write(0x203000,b'plain\0'if shape==5 else b'?decorated\0');put(0x201000,0,symbol,0,0)
    def value(kind,n):return 0x201000 if kind in [3,6]else symbol if kind==8 else n
    put(0x200000,0,0,operation<<8,hint<<13,a,value(a,va),b,value(b,vb));put(0x80d40,35);put(0x300800,0x400000)
    u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,0x200000);events=[];output=bytearray()
    def hook(m,address,size,unused):
        if address==0x400000:m.emu_stop();return
        if address not in [0x28280,0x28120,0x71b40]:return
        esp=m.reg_read(UC_X86_REG_ESP);ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX)
        if address==0x28280:
            assert ecx in [0,1];events.append('C%d;'%ecx);put(0x80d40,word(0x80d40)+(2 if ecx else 1));m.reg_write(UC_X86_REG_EAX,0)
        elif address==0x28120:
            assert ecx==0x202000 and edx in [0,1];events.append('D%d;'%edx);put(0x80d40,word(0x80d40)+1);m.reg_write(UC_X86_REG_EAX,0)
        else:
            fmt=bytes(m.mem_read(word(esp+4),16)).split(b'\0')[0]
            if fmt==b'%.8lxH':text=('%08xH'%word(esp+8)).encode()
            else:assert fmt==b'pad=%d';text=('pad=%d'%word(esp+8)).encode()
            output.extend(text);m.reg_write(UC_X86_REG_EAX,len(text))
        m.reg_write(UC_X86_REG_EIP,word(esp));m.reg_write(UC_X86_REG_ESP,esp+4)
    u.hook_add(UC_HOOK_CODE,hook);u.emu_start(0x29bb0,0,count=100000);assert u.reg_read(UC_X86_REG_ESP)==0x300804
    expected=['%u %s'%(word(0x80d40),''.join(events)),output.hex()];actual=subprocess.check_output(['build/native/check-instruction-comment',*[str(v)for v in c]],text=True).splitlines()
    assert actual==expected,(c,actual,expected);rows.append(dict(input=list(c),result=expected))
Path('cases/instruction-comment-validation.json').write_text(json.dumps(dict(cases=rows,seconds=time.perf_counter()-started,scope='Actual unchanged C parent versus whole original29bb0..29cdb with7-way table29cdc..29cf8. Named comment-prefix/demangle child mocks assert formal values and first→subsequent state; ASCII printf intercepted. Signed−1/9 cuts/extremes, all kind defaults, record hints, symbol class/attribute/name guards and zero/one/two operands tested. No child ABI/implementation or actual compile branch coverage claim.'),indent=2)+'\n');print(len(rows),'whole instruction-comment shapes match')
