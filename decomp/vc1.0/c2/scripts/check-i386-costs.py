"""Compare actual C i386 costs to isolated original x86 execution for all forms.

Analysis-only bounded original instructions/table/property view are emulated;
no Wine, fixture recapture, generated reconstruction or data snapshot is used.
"""
from pathlib import Path
import hashlib
import json
import re
import struct
import subprocess
import tempfile
import time
from binary import PE
from case_paths import C2_SCRATCH
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP

started=time.perf_counter()
source=Path('src/physical-allocation.c').read_text()
body=re.search(r'int __fastcall i386_instruction_cost\(MachineInstruction \*instruction\)\n\{.*?\n\}',source,re.S).group()
macros='\n'.join(re.search(r'^#define '+name+r'.*$',source,re.M).group() for name in ['COST_SIMPLE_SOURCE','COST_IMMEDIATE_SOURCE'])
with tempfile.TemporaryDirectory(prefix='i386-cost-proof-',dir=C2_SCRATCH) as directory:
    directory=Path(directory);probe=directory/'probe.c'
    probe.write_text('#include "'+str(Path('src/physical-allocation.h').resolve())+'"\n#undef write\nextern int write(int,const void*,unsigned);\n'+macros+'\n'+body+'''
int main(void) {
    MachineInstruction instruction;
    unsigned operation,a,b;
    int value;
    for(operation=0;operation<MC_OPCODE_COUNT;++operation)
        for(a=0;a<16;++a) for(b=0;b<16;++b) {
            instruction.opcode=operation<<8;
            instruction.operands[0].flags=a;
            instruction.operands[1].flags=b;
            value=i386_instruction_cost(&instruction);
            if(write(1,&value,4)!=4) return 1;
        }
    return 0;
}
''')
    phase=time.perf_counter();result=subprocess.run(['clang','-m32','-std=gnu89','-fms-extensions','-fasm-blocks','-O0','-g',
        '-DRECON_NATIVE_TEST','-I../../../lib/recon/include','-I../../../lib/recon/native',str(probe),'-o',str(directory/'probe')],capture_output=True)
    assert result.returncode==0,result.stderr.decode();build_seconds=time.perf_counter()-phase
    phase=time.perf_counter();actual=subprocess.check_output([str(directory/'probe')]);native_seconds=time.perf_counter()-phase
    probe_sha=hashlib.sha256(probe.read_bytes()).hexdigest()
original=PE('../../../toolchains/1.0-nt/bin/C23232.EXE')
names=re.findall(r'^OP\((\w+),',Path('src/machine-opcodes.def').read_text(),re.M)
table=struct.unpack('<46I',original.read(0x2d55c,184))
assert len(set(table))==8 and all(0x2d350<=target<0x2d55c for target in table),table
emulator=Uc(UC_ARCH_X86,UC_MODE_32)
emulator.mem_map(0x10000,0x80000)
emulator.mem_write(0x2d350,original.read(0x2d350,0x2d614-0x2d350))
emulator.mem_write(0x82210,original.read(0x82210,2*len(names)))
emulator.mem_map(0x100000,4096);emulator.mem_map(0x200000,4096)
emulator.mem_write(0x200000,struct.pack('<I',0x200100))
expected=bytearray();phase=time.perf_counter()
for operation in range(len(names)):
    emulator.mem_write(0x100008,struct.pack('<I',operation<<8))
    for a in range(16):
        emulator.mem_write(0x100010,struct.pack('<I',a))
        for b in range(16):
            emulator.mem_write(0x100018,struct.pack('<I',b))
            emulator.reg_write(UC_X86_REG_ECX,0x100000);emulator.reg_write(UC_X86_REG_ESP,0x200000)
            emulator.emu_start(0x2d350,0x200100,count=100)
            expected+=struct.pack('<I',emulator.reg_read(UC_X86_REG_EAX))
original_seconds=time.perf_counter()-phase
assert actual==expected,[(i//256,names[i//256],i//16%16,i%16,a,b) for i,(a,b) in enumerate(zip(struct.unpack('<%dI'%(len(actual)//4),actual),struct.unpack('<%dI'%(len(expected)//4),expected))) if a!=b][:20]
receipt=dict(method='Actual C function native i386 O0 versus isolated original x86 function emulation for every legal opcode and both low-four-bit operand classes',
    target_sha256=hashlib.sha256(original.data).hexdigest(),source_sha256=hashlib.sha256(source.encode()).hexdigest(),probe_sha256=probe_sha,
    original_extent=['0x2d350','0x2d614'],dispatch_view=['0x2d55c','0x2d614'],dispatch_rows=46,dispatch_targets=sorted(hex(t) for t in set(table)),
    opcode_rows=len(names),operand_classes=16,comparisons=len(names)*256,mismatches=0,output_sha256=hashlib.sha256(actual).hexdigest(),
    limits=['Original complete instruction-cost routine has no callees; isolated emulator uses only its708-byte body/dispatch and bounded legal-opcode property rows.','Twelve alignment bytes2d614..2d620 are excluded from accepted function extent; previous720-byte figure was the next-entry bound.','This exhaustive cost-form proof is distinct from which forms execute in the frozen G3 compile case.'],
    timing=dict(native_build_seconds=build_seconds,native_execution_seconds=native_seconds,original_emulation_seconds=original_seconds,total_seconds=time.perf_counter()-started))
Path('cases/i386-cost-validation.json').write_text(json.dumps(receipt,indent=2)+'\n')
print('%d original/native i386 cost results match'%(len(names)*256))
