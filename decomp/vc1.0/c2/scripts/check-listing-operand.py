"""Whole operand renderer; child request shapes and bounded ASCII formats."""
from pathlib import Path
import argparse,itertools,json,re,struct,subprocess,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESP,UC_X86_REG_EIP
from binary import PE
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--prefix',action='store_true');args=parser.parse_args()
started=time.perf_counter();pe=PE('../../../toolchains/1.0-nt/bin/C23232.EXE');names=re.findall(r'^OP\((\w+),',Path('src/machine-opcodes.def').read_text(),re.M);ids={n:i for i,n in enumerate(names)};cases=[]
# kind,operation,width,extra flags,hint,displacement/value,symbol,base,index,DOS
for kind in range(16):cases.append((kind,ids['PUSH'],4,0,0,0,0,5,2,1))
for op,width,reg in itertools.product(['PUSH','FADD','FSTSW','FNSTSW'],[1,4,10],[0,14,21]):cases.append((1,ids[op],width,0,0,0,0,reg,2,1))
for kind,symbol,disp,hint in itertools.product([3,4,6,7,12],[0,1],[0,-9,17],range(8)):cases.append((kind,ids['MOV'],4,0,hint,disp,symbol,3,2,1))
for scale,base,symbol,disp in itertools.product(range(4),[3,27],[0,1],[0,-9,17]):cases.append((5,ids['MOV'],4,scale<<8,3,disp,symbol,base,2,1))
for kind,value in itertools.product([2,10],[0,-9,17,-2147483648,2147483647]):cases.append((kind,ids['PUSH'],4,0,0,value,0,5,2,1))
for dos in [0,1]:cases.append((6,ids['MOV'],4,0,1,-9,1,3,2,dos))
if args.prefix:
    cases=[]
    for kind,operation,width,segment in itertools.product(range(16),['JCC','JMP','MOV'],[1,2,4],[0,3]):cases.append((kind,ids[operation],width,segment<<4,0,0,0,5,2,1))
rows=[]
for c in cases:
    kind,operation,width,extra,hint,disp,symbol,base,index,dos=c;u=Uc(UC_ARCH_X86,UC_MODE_32)
    for a,n in [(0x10000,0x90000),(0x200000,0x10000),(0x300000,0x10000),(0x400000,0x1000)]:u.mem_map(a,n)
    for s in pe.sections:
        if s['size']:u.mem_write(pe.base+s['rva'],pe.data[s['off']:s['off']+s['size']])
    def word(a):return struct.unpack('<I',u.mem_read(a,4))[0]
    def put(a,*v):u.mem_write(a,struct.pack('<'+'I'*len(v),*[x&0xffffffff for x in v]))
    value=0x201000 if 3<=kind<=7 or kind==12 else(0x202000 if symbol else 0)if kind==8 else base if kind in[1,11]else disp
    put(0x200000,0,0,(operation<<8)|(width<<4),hint<<13,kind|extra,value,0,0);put(0x201000,disp,0x202000 if symbol else 0,base,index);put(0x80d40,32);put(0x80684,dos);put(0x300800,0x400000,width)
    u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,0x200000);u.reg_write(UC_X86_REG_EDX,0x200010);events=[];output=bytearray()
    def hook(m,a,size,unused):
        if a==0x400000:m.emu_stop();return
        observed=[0x28230,0x2a7c0,0x35870,0x71b40]+([0x29920]if args.prefix else[0x2a700])
        if a not in observed:return
        esp=m.reg_read(UC_X86_REG_ESP);ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);pop=4
        if a==0x2a700:assert ecx==0x200000 and edx==0x200010 and word(esp+4)==width;events.append('Q%u;'%width);pop=8
        elif a==0x28230:
            assert ecx in [0,0x202000];events.append('S%d;'%int(bool(ecx)))
            if ecx:output.extend(b'symbol');put(0x80d40,word(0x80d40)+6)
        elif a==0x2a7c0:events.append('D%d:%d;'%(struct.unpack('<i',struct.pack('<I',ecx))[0],edx))
        elif a==0x29920:
            events.append('T%u;'%ecx);m.mem_write(0x203000,('t%u\0'%ecx).encode());m.reg_write(UC_X86_REG_EAX,0x203000)
        elif a==0x35870:
            events.append('R%u:%u;'%(ecx,edx));m.mem_write(0x203000,('r%u_%u\0'%(ecx,edx)).encode());m.reg_write(UC_X86_REG_EAX,0x203000)
        else:
            fmt=bytes(m.mem_read(word(esp+4),32)).split(b'\0')[0]
            if b'%s'in fmt:text=fmt.replace(b'%s',bytes(m.mem_read(word(esp+8),32)).split(b'\0')[0])
            elif b'%'in fmt:
                signed=struct.unpack('<i',m.mem_read(esp+8,4))[0];text=fmt.replace(b'%ld',str(signed).encode()).replace(b'%d',str(signed).encode())
                assert b'%'not in text,fmt
            else:text=fmt
            output.extend(text);m.reg_write(UC_X86_REG_EAX,len(text))
        m.reg_write(UC_X86_REG_EIP,word(esp));m.reg_write(UC_X86_REG_ESP,esp+pop)
    u.hook_add(UC_HOOK_CODE,hook);u.emu_start(0x2a700 if args.prefix else 0x29d00,0,count=100000);assert u.reg_read(UC_X86_REG_ESP)==0x300808
    expected=['%u %s'%(word(0x80d40),''.join(events)),output.hex()];actual=subprocess.check_output(['build/native/check-listing-'+('prefix'if args.prefix else'operand'),*[str(v)for v in c]],text=True).splitlines()
    assert actual==expected,(c,actual,expected)
    rows.append(dict(input=list(c),result=expected))
scope=('Actual unchanged C prefix versus whole original2a700..2a7bc. All16 kinds, JCC/JMP/MOV, widths1/2/4 and absent/present segment qualifier. Data-type/register-name child mocks plus intercepted ASCII printf; no child implementation or actual compile branch coverage claim.'if args.prefix else'Actual unchanged C operand parent versus whole original29d00..2a10f and12-kind switch table2a110..2a140. Prefix/symbol/displacement/register-name mocks validate request order/values; ASCII printf intercepted. Kind0..15 default, x87/register exceptions, symbol/zero/negative displacements, all8 hints, four scales/no-base sentinel, DOS prefix and signed32 limits. No child implementation/ABI or unvisited actual compile branch coverage claim.')
Path('cases/listing-'+('prefix'if args.prefix else'operand')+'-validation.json').write_text(json.dumps(dict(cases=rows,seconds=time.perf_counter()-started,scope=scope),indent=2)+'\n');print(len(rows),'whole',('prefix'if args.prefix else'operand'),'states match')
