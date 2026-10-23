"""Check operand grammar/typed descriptors against actual original decoding."""
from pathlib import Path
import hashlib,itertools,json,struct,subprocess,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP
from binary import PE
started=time.perf_counter();pe=PE('../../../toolchains/1.0-nt/bin/C23232.EXE')
formats=[(0,''),(1,'mr'),(2,'mv'),(3,'msog'),(4,'mdbg'),(4,'msobg'),(5,'mdibxg'),(5,'msoibxg'),(6,'mso'),(7,'mso'),(8,'ms'),(9,'ms'),(10,'mv'),(0,'m#'),(0,'ms'),(0,'ms'),(12,'mdg'),(0,'mso'),(0,'msr'),(0,'mdbg'),(11,'mr'),(0,'')]
for index,(kind,pattern)in enumerate(formats):
    original_kind,pointer=struct.unpack('<II',pe.read(0x823e0+8*index,8))
    assert original_kind==kind and pe.read(pointer,20).split(b'\0')[0].decode()==pattern
for kind in range(13):assert bool(struct.unpack('<I',pe.read(0x82490+4*kind,4))[0]&1)==(3<=kind<=7 or kind==12)
expected_switch={'b':0x1af65,'d':0x1afb1,'g':0x1afc7,'i':0x1afe6,'o':0x1afb1,'r':0x1b017,'s':0x1b030,'v':0x1b086,'x':0x1b097}
for offset,destination in enumerate(struct.unpack('<23I',pe.read(0x1b17c,92))):assert destination==expected_switch.get(chr(ord('b')+offset),0x1af50)
def identifier(value):
    return struct.pack('<H',value)if value<0x8000 else struct.pack('<HH',(value&0x7fff)|0x8000,value>>15)
def operand(tag,reg=6,index=2,scale=4,displacement=17,symbol=7,size=4):
    payload=bytes([tag])
    for ch in formats[tag][1][1:]:
        if ch in 'do':payload+=struct.pack('<I',displacement&0xffffffff)
        elif ch=='s':payload+=identifier(10 if tag==11 else symbol)
        elif ch=='v':payload+=struct.pack('<I',displacement&0xffffffff)
        elif ch in 'rb':payload+=bytes([reg])
        elif ch=='i':payload+=bytes([index])
        elif ch=='x':payload+=bytes([scale])
        elif ch=='g':payload+=bytes([size])
    return payload
payloads=[operand(10,symbol=value)for value in [0,0x7fff,0x8000,0x7fffffff]]
for tag,(kind,pattern)in enumerate(formats):
    if kind:payloads.append(operand(tag))
    elif tag!=18:payloads.append(bytes([tag]))
for reg in [0,7,40,47,127,128,255]:payloads.append(operand(1,reg=reg));payloads.append(operand(20,reg=reg))
for tag,reg,displacement in itertools.product([4,5],[0,128,255],[0,-1,0x80000000]):payloads.append(operand(tag,reg=reg,displacement=displacement))
for tag,scale in itertools.product([6,7],[1,2,4,8]):payloads.append(operand(tag,scale=scale))
for tag in [3,5,7,8,9,10]:payloads.append(operand(tag,symbol=0x123456))
for reg in [9,128]:
    payloads.append(b'\x12'+identifier(7)+bytes([reg])+operand(4,reg=reg))
    payloads.append(b'\x12'+identifier(7)+bytes([reg])+operand(6,reg=2,index=reg))
# First original diagnostics, not execution of error reporting itself.
payloads += [b'\x12'+identifier(7)+b'\x09'+operand(6,reg=9,index=9),b'\x12'+identifier(7)+b'\x09'+operand(4,reg=2),b'\x12'+identifier(7)+b'\x09'+operand(5,reg=9),b'\x12'+identifier(7)+b'\x09'+operand(1,reg=9),b'\x12'+identifier(10)+b'\x09'+operand(4,reg=9)]
rows=[]
for payload in payloads:
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x90000);u.mem_map(0x200000,0x100000);u.mem_map(0x300000,0x10000);u.mem_map(0x400000,0x1000)
    for section in pe.sections:
        if section['size']:u.mem_write(pe.base+section['rva'],pe.data[section['off']:section['off']+section['size']])
    def word(address):return struct.unpack('<I',u.mem_read(address,4))[0]
    def put(address,*values):u.mem_write(address,struct.pack('<'+'I'*len(values),*values))
    u.mem_write(0x200000,payload);put(0x300800,0x400000,0x220000);put(0x7b960,0)
    u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,0x210000);u.reg_write(UC_X86_REG_EDX,0x200000);allocations=[];lookups=[];errors=[]
    def hook(machine,address,count,unused):
        if address==0x400000:machine.emu_stop();return
        if address==0x6fb70:errors.append(machine.reg_read(UC_X86_REG_EDX));machine.emu_stop();return
        if address not in [0x4d210,0x70ae0]:return
        stack=machine.reg_read(UC_X86_REG_ESP)
        if address==0x4d210:
            assert machine.reg_read(UC_X86_REG_ECX)==0x220000;key=machine.reg_read(UC_X86_REG_EDX);lookups.append(key);result=0x220100 if key==7 else 0
        else:
            assert machine.reg_read(UC_X86_REG_ECX)==1 and machine.reg_read(UC_X86_REG_EDX)==16
            result=0x230000+len(allocations)*0x100;put(result,0,0,0,0);allocations.append(16)
        machine.reg_write(UC_X86_REG_EAX,result);machine.reg_write(UC_X86_REG_ESP,stack+4);machine.reg_write(UC_X86_REG_EIP,word(stack))
    u.hook_add(UC_HOOK_CODE,hook);u.emu_start(0x1ae20,0,count=100000)
    native=subprocess.run(['build/native/check-assembly-operand',payload.hex()],stdout=subprocess.PIPE,text=True)
    if errors:
        assert native.returncode==90 and native.stdout.strip()==f'ERR {errors[0]:x}',(payload,errors,native.stdout)
        rows.append(dict(payload=payload.hex(),diagnostic=hex(errors[0])));continue
    assert native.returncode==0,(payload,native.stdout)
    flags=word(0x210000);value=word(0x210004);kind=flags&15;row=[flags,0,0,0,0,0,0]
    if 3<=kind<=7 or kind==12:row[2:6]=[word(value),42 if word(value+4)==0x220100 else 0,word(value+8),word(value+12)]
    elif kind==8:row[1]=42 if value==0x220100 else 0
    else:row[1]=value
    original=[u.reg_read(UC_X86_REG_EAX)-0x200000,len(allocations),len(lookups),*row]
    assert list(map(int,native.stdout.split()))==original,(payload,native.stdout,original)
    rows.append(dict(payload=payload.hex(),normalized=original,symbol_lookups=lookups))
Path('cases/assembly-operand-validation.json').write_text(json.dumps(dict(target_sha256=hashlib.sha256(pe.data).hexdigest(),extent='1ae20..1b17c',switch='1b17c..1b1d8,23 DWORDs',descriptors='823e0..82490,22 typed kind/pattern rows',cases=rows,seconds=time.perf_counter()-started,limits='Lookup and zeroed successful allocator are explicit seams. Actual operand C/compact symbol-id/address allocation/log2 and corresponding original bodies execute. Error cases compare only first diagnostic requests; reporting bodies remain named traps. Descriptor indices outside0..21 and malformed truncated payloads are outside the verified frontend contract, without invented guards.'),indent=2)+'\n')
print(len(rows),'original/native operand states and first diagnostics match;22 descriptors/23 grammar targets/13 kind properties verified')
