"""Compare actual C subtraction overflow with the isolated original x86 body."""
from pathlib import Path
import hashlib
import itertools
import json
import re
import struct
import subprocess
import tempfile
import time
from binary import PE
from case_paths import C2_SCRATCH
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP

started=time.perf_counter()
source=Path('src/integer-fold.c').read_text()
body=re.search(r'int __cdecl subtraction_overflows\([^\n]+\)\n\{.*?\n\}',source,re.S).group()
values=[0,1,0x7f,0x80,0xff,0x100,0x7fff,0x8000,0xffff,0x10000,
        0x7fffffff,0x80000000,0xffffffff,0x7fff0000,0x8000ffff]
cases=list(itertools.product(values,values,[0,1],[0,1,2,3,4,0xffffffff]))
with tempfile.TemporaryDirectory(prefix='subtraction-overflow-',dir=C2_SCRATCH) as temporary:
    directory=Path(temporary);probe=directory/'probe.c'
    probe.write_text('#define __cdecl\nextern int read(int,void*,unsigned);\nextern int write(int,const void*,unsigned);\n'+body+'''
int main(void) {
    unsigned row[4]; int value;
    while (read(0,row,sizeof(row))==sizeof(row)) {
        value=subtraction_overflows(row[0],row[1],row[2],row[3]);
        if (write(1,&value,4)!=4) return 1;
    }
    return 0;
}
''')
    phase=time.perf_counter()
    result=subprocess.run(['clang','-m32','-std=gnu89','-O0','-g',str(probe),'-o',str(directory/'probe')],capture_output=True)
    assert result.returncode==0,result.stderr.decode()
    build_seconds=time.perf_counter()-phase
    phase=time.perf_counter()
    result=subprocess.run([str(directory/'probe')],input=b''.join(struct.pack('<4I',*r) for r in cases),capture_output=True,check=True)
    actual=struct.unpack('<%dI'%len(cases),result.stdout)
    native_seconds=time.perf_counter()-phase
original=PE('../../../toolchains/1.0-nt/bin/C23232.EXE')
emulator=Uc(UC_ARCH_X86,UC_MODE_32);emulator.mem_map(0x71000,4096)
emulator.mem_write(0x71a3a,original.read(0x71a3a,0x71a76-0x71a3a))
emulator.mem_map(0x200000,4096)
phase=time.perf_counter()
for row,observed in zip(cases,actual):
    emulator.mem_write(0x200100,struct.pack('<5I',0x200200,*row))
    emulator.reg_write(UC_X86_REG_ESP,0x200100)
    emulator.emu_start(0x71a3a,0x200200,count=64)
    expected=emulator.reg_read(UC_X86_REG_EAX)
    assert observed==expected,(row,observed,expected)
receipt=dict(cases=len(cases),original_extent=['0x71a3a','0x71a76'],
    source_body_sha256=hashlib.sha256(body.encode()).hexdigest(),
    widths=[0,1,2,3,4,'0xffffffff'],signedness=[0,1],
    limit='Boundary cross-products, not exhaustive32-bit values; explicitly verifies original unsigned path ignores borrow.',
    build_seconds=build_seconds,native_seconds=native_seconds,original_emulation_seconds=time.perf_counter()-phase,
    seconds=time.perf_counter()-started)
Path('cases/subtraction-overflow-validation.json').write_text(json.dumps(receipt,indent=2)+'\n')
print('%d native/original subtraction-overflow cases match'%len(cases))
