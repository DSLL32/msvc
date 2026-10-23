"""Special instruction parent: whole bodies, explicit named child contracts."""
from pathlib import Path
import itertools,json,re,struct,subprocess,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESP,UC_X86_REG_EIP
from binary import PE
started=time.perf_counter();pe=PE('../../../toolchains/1.0-nt/bin/C23232.EXE');names=re.findall(r'^OP\((\w+),',Path('src/machine-opcodes.def').read_text(),re.M)
ids={n:i for i,n in enumerate(names)}
for operation,name in enumerate(names):
    expected=bool(struct.unpack('<H',pe.read(0x82210+2*operation,2))[0]&1)
    actual=ids['FADD']<=operation<=ids['FLD1']or ids['FUCOM']<=operation<=ids['FCOS']
    assert actual==expected,(name,actual,expected)
native=subprocess.check_output(['build/native/check-listing-special'],text=True).splitlines();assert len(native)==len(names)*12;rows=[]
for operation,mode,width in itertools.product(range(len(names)),range(4),[1,4,10]):
    u=Uc(UC_ARCH_X86,UC_MODE_32)
    for a,n in [(0x10000,0x90000),(0x200000,0x10000),(0x300000,0x10000),(0x400000,0x1000)]:u.mem_map(a,n)
    for s in pe.sections:
        if s['size']:u.mem_write(pe.base+s['rva'],pe.data[s['off']:s['off']+s['size']])
    def word(a):return struct.unpack('<I',u.mem_read(a,4))[0]
    def put(a,*v):u.mem_write(a,struct.pack('<'+'I'*len(v),*[x&0xffffffff for x in v]))
    put(0x200000,0,0,(operation<<8)|(width<<4),0xe000 if mode&1 else 0,0x102 if mode==3 else 0x101,5,0x204 if mode else 0x201,0x201000 if mode or names[operation]=='IMUL3'else 2)
    put(0x201000,-9,1 if mode==2 else 0,3,0);put(0x80d40,24);put(0x300800,0x400000)
    u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,0x200000);events=[]
    def hook(m,a,size,unused):
        if a==0x400000:m.emu_stop();return
        if a not in [0x2a140,0x29d00,0x35870,0x71b40]:return
        esp=m.reg_read(UC_X86_REG_ESP);ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);pop=4
        if a==0x2a140:assert ecx==0x200000;events.append('M;')
        elif a==0x29d00:
            assert ecx==0x200000 and edx in [0x200010,0x200018];events.append('O%d:%x:%u;'%(int(edx==0x200018),word(edx),word(esp+4)));pop=8
        elif a==0x35870:
            events.append('R%u:%u;'%(ecx,edx));m.mem_write(0x202000,('r%u_%u\0'%(ecx,edx)).encode());m.reg_write(UC_X86_REG_EAX,0x202000)
        else:
            fmt=bytes(m.mem_read(word(esp+4),32)).split(b'\0')[0]
            if fmt in [b'1, %s',b'%s, ',b', %s, cl',b', %s, %d']:
                reg=bytes(m.mem_read(word(esp+8),32)).split(b'\0')[0].decode();text=fmt.decode().replace('%s',reg)
                if '%d'in text:text=text.replace('%d',str(struct.unpack('<i',m.mem_read(esp+12,4))[0]))
            elif fmt in [b', %ld',b'%d']:
                value=struct.unpack('<i',m.mem_read(esp+8,4))[0];text=(', 'if fmt==b', %ld'else '')+str(value)
            else:assert fmt==b', ';text=', '
            events.append('P['+text+'];');m.reg_write(UC_X86_REG_EAX,len(text))
        m.reg_write(UC_X86_REG_EIP,word(esp));m.reg_write(UC_X86_REG_ESP,esp+pop)
    u.hook_add(UC_HOOK_CODE,hook);u.emu_start(0x2a3e0,0,count=10000);assert u.reg_read(UC_X86_REG_ESP)==0x300804
    expected='%u %u %u %d %u %x %x %s'%(operation,mode,width,u.reg_read(UC_X86_REG_EAX),word(0x80d40),word(0x200010),word(0x200018),''.join(events));index=(operation*4+mode)*3+[1,4,10].index(width)
    assert native[index]==expected,(names[operation],mode,width,native[index],expected)
    rows.append(dict(operation=names[operation],mode=mode,bytes=width,result=expected))
Path('cases/listing-special-validation.json').write_text(json.dumps(dict(cases=rows,seconds=time.perf_counter()-started,scope='Actual unchanged C special-form parent versus whole original2a3e0..2a6f6, all legal opcodes, widths1/4/10 and four register/address/record-flag shapes. Named mnemonic/operand/register-name and ASCII printf mocks compare argument values, event order, count, return and postoperand flags. Unusual IMUL3 input tag deliberately still owns an address. This is parent control/data proof, not actual compile branch coverage or child implementation/ABI acceptance.'),indent=2)+'\n');print(len(rows),'whole special instruction parent states match')
