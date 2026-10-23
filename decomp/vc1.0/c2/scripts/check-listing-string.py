"""Check actual listing C against original bytes for quoting/wrapping/ctype use."""
from pathlib import Path
import itertools,json,struct,subprocess,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP
from binary import PE
started=time.perf_counter();pe=PE('../../../toolchains/1.0-nt/bin/C23232.EXE');rows=[]
word=lambda b:struct.unpack('<I',b)[0]
ctype=word(pe.read(0x86420,4))
for byte in range(256):assert bool(struct.unpack('<H',pe.read(ctype+2*byte,2))[0]&0x157)==(32<=byte<=126)
payloads=[b'',b"'",b"a'b",b'A\0\1Z',b'A'*65,b'A'*80,bytes(range(256)),b'\0'*20]
for payload,maximum,column in itertools.product(payloads,[1,2],[0,9,73]):
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x90000);u.mem_map(0x200000,0x10000);u.mem_map(0x300000,0x10000);u.mem_map(0x400000,0x1000)
    for section in pe.sections:
        if section['size']:u.mem_write(pe.base+section['rva'],pe.data[section['off']:section['off']+section['size']])
    u.mem_write(0x200000,payload or b'\0');u.mem_write(0x300800,struct.pack('<I',0x400000));u.mem_write(0x80d40,struct.pack('<I',column));u.mem_write(0x8662c,struct.pack('<I',maximum))
    u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,0x200000);u.reg_write(UC_X86_REG_EDX,len(payload));output=bytearray();classified=[]
    def hook(machine,address,size,unused):
        if address==0x400000:machine.emu_stop();return
        if address not in [0x71b40,0x71bf0]:return
        stack=machine.reg_read(UC_X86_REG_ESP)
        if address==0x71bf0:
            byte,mask=struct.unpack('<II',machine.mem_read(stack+4,8));assert mask==0x157 and byte<=255;classified.append(byte);result=int(32<=byte<=126)
        else:
            pointer=word(machine.mem_read(stack+4,4));fmt=bytes(machine.mem_read(pointer,50)).split(b'\0')[0]
            if fmt==b'%c':piece=bytes([word(machine.mem_read(stack+8,4))&255])
            elif fmt==b'0%xH':piece=('0%xH'%word(machine.mem_read(stack+8,4))).encode()
            else:assert b'%'not in fmt;piece=fmt
            output.extend(piece);result=len(piece)
        machine.reg_write(UC_X86_REG_EAX,result);machine.reg_write(UC_X86_REG_ESP,stack+4);machine.reg_write(UC_X86_REG_EIP,word(machine.mem_read(stack,4)))
    u.hook_add(UC_HOOK_CODE,hook);u.emu_start(0x28ea0,0,count=100000)
    native=subprocess.check_output(['build/native/check-listing-string',payload.hex(),str(maximum),str(column)],text=True).splitlines()
    final_column=word(u.mem_read(0x80d40,4));assert native==[f'{final_column} {len(classified)}',output.hex()],(payload,maximum,column,native,output)
    rows.append(dict(payload_hex=payload.hex(),mb_cur_max=maximum,start_column=column,end_column=final_column,ctype_requests=len(classified),output_hex=output.hex()))
Path('cases/listing-string-validation.json').write_text(json.dumps(dict(cases=rows,seconds=time.perf_counter()-started,ctype='All256 original C-locale table entries independently prove printability mask0x157; MB_CUR_MAX>1 helper calls use observed byte/mask and equivalent C-locale predicate.',limits='printf formatting and _isctype are isolated contract probes; no CRT helper ABI/body acceptance is claimed, and non-C locales/DBCS behavior is not covered.'),indent=2)+'\n');print(len(rows),'actual original/native quoting, byte classes, wrapping and column states match')
