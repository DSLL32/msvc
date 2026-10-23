"""Compare all size classes and diagnostics with original listing machine code."""
from pathlib import Path
import argparse,hashlib,json,struct,subprocess,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP
from binary import PE
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--type',action='store_true');args=parser.parse_args()
started=time.perf_counter();pe=PE('../../../toolchains/1.0-nt/bin/C23232.EXE');rows=[]
def string(machine,address):
    if not address:return 'NULL'
    result=bytearray()
    while True:
        byte=machine.mem_read(address+len(result),1)[0]
        if not byte:return result.decode('ascii')
        result.append(byte)
        assert len(result)<100
expected={1:'BYTE',2:'WORD',4:'DWORD',6:'FWORD',8:'QWORD',10:'TBYTE'}if args.type else{1:'DB',2:'DW',4:'DD',6:'DF',8:'DQ',10:'DT'}
# Independently check every original table DWORD and destination literal.
entries=struct.unpack('<10I',pe.read(0x2995c if args.type else 0x294dc,40))
for size,address in enumerate(entries,1):
    if size not in expected:assert address==(0x2992d if args.type else 0x294a0)
    else:
        assert pe.read(address,1)==b'\xb8' and pe.read(address+5,1)==b'\xc3'
        literal=struct.unpack('<I',pe.read(address+1,4))[0]
        assert pe.read(literal,6).split(b'\0')[0].decode()==expected[size]
for size in [-2147483648,-1,0,*range(1,12),2147483647]:
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x90000);u.mem_map(0x300000,0x10000);u.mem_map(0x400000,0x1000)
    for section in pe.sections:
        if section['size']:u.mem_write(pe.base+section['rva'],pe.data[section['off']:section['off']+section['size']])
    u.mem_write(0x300800,struct.pack('<I',0x400000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,size&0xffffffff);diagnostics=[]
    def hook(machine,address,count,unused):
        if address==0x400000:machine.emu_stop()
        elif address==0x6fb70:
            assert machine.reg_read(UC_X86_REG_EDX)==0x514
            assert string(machine,machine.reg_read(UC_X86_REG_ECX))=='@(#)listing.c:1.76'
            diagnostics.append(0x514);stack=machine.reg_read(UC_X86_REG_ESP)
            machine.reg_write(UC_X86_REG_ESP,stack+4);machine.reg_write(UC_X86_REG_EIP,struct.unpack('<I',machine.mem_read(stack,4))[0])
    u.hook_add(UC_HOOK_CODE,hook);u.emu_start(0x29920 if args.type else 0x29490,0,count=1000)
    original=f'{len(diagnostics)} {string(u,u.reg_read(UC_X86_REG_EAX))}'
    native=subprocess.check_output(['build/native/check-listing-'+('type'if args.type else'directive'),str(size)],text=True).strip()
    assert native==original,(size,native,original)
    assert native==f'{int(not args.type and size not in expected)} {expected.get(size,"NULL")}'
    rows.append(dict(bytes=size,result=original))
Path('cases/listing-'+('type'if args.type else'directive')+'-validation.json').write_text(json.dumps(dict(target_sha256=hashlib.sha256(pe.data).hexdigest(),extent='29920..2995c'if args.type else'29490..294dc',table='2995c..29984,10 DWORDs'if args.type else'294dc..29504,10 DWORDs',cases=rows,seconds=time.perf_counter()-started,limits='Unsupported widths returnNULL without diagnostics.'if args.type else'The error callback is observed and returned for this isolated probe; no claim that actual compiler error handling returns successfully.'),indent=2)+'\n')
print(len(rows),'original/native directive classes and diagnostic requests match;10 table destinations verified')
