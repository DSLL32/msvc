"""Compare the conditional opcode C predicate with its bounded original table."""
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile
import time
from binary import PE
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from case_paths import C2_SCRATCH
started = time.perf_counter()
source = Path('src/value-selection.c').read_text()
function = re.search(r'int __fastcall conditional_integer_opcode\(unsigned opcode\)\n\{.*?\n\}',source,re.S).group()
original = PE('../../../toolchains/1.0-nt/bin/C23232.EXE')
cs = Cs(CS_ARCH_X86,CS_MODE_32)
def result(address):
    body = list(cs.disasm(original.read(address,8),address))
    assert body[1].mnemonic == 'ret'
    if body[0].mnemonic == 'mov' and body[0].op_str == 'eax, 1':return 1
    assert body[0].mnemonic == 'sub' and body[0].op_str == 'eax, eax'
    return 0
# The original subtract2/unsigned limit23 guard rejects all other values.
guard = list(cs.disasm(original.read(0x1a4c0,8),0x1a4c0))
assert [(i.mnemonic,i.op_str) for i in guard[:2]] == [('sub','ecx, 2'),('cmp','ecx, 0x17')]
assert guard[2].mnemonic == 'ja' and guard[2].op_str == '0x1a4d0'
targets = struct.unpack('<24I',original.read(0x1a4dc,96))
assert set(targets) == {0x1a4d0,0x1a4d3}
expected = bytes(result(targets[n-2]) if 2 <= n <= 25 else 0 for n in range(256))+bytes([0])
with tempfile.TemporaryDirectory(prefix='conditional-opcodes-',dir=C2_SCRATCH) as name:
    directory = Path(name)
    probe = directory/'probe.c'
    probe.write_text('#include <recon.h>\n#undef write\nextern int write(int,const void*,unsigned);\n'+function+'''\nint main(void) {
    unsigned n; unsigned char values[257];
    for(n=0;n<256;++n) values[n]=(unsigned char)conditional_integer_opcode(n);
    values[256]=(unsigned char)conditional_integer_opcode(~0U);
    return write(1,values,sizeof(values)) == sizeof(values) ? 0 : 1;
}\n''')
    compile_start = time.perf_counter()
    subprocess.run(['clang','-m32','-std=gnu89','-fms-extensions','-fasm-blocks','-O0','-g',
        '-DRECON_NATIVE_TEST','-I../../../lib/recon/include','-I../../../lib/recon/native',str(probe),'-o',str(directory/'probe')],check=True)
    compiled = time.perf_counter()
    actual = subprocess.check_output([str(directory/'probe')])
    executed = time.perf_counter()
assert actual == expected
receipt = dict(method='Native-O0 predicate over all256 byte opcode IDs plus UINT_MAX compared to original guard/table/return blocks',
    source_sha256=hashlib.sha256(Path('src/value-selection.c').read_bytes()).hexdigest(),
    original_extent=['0x1a4c0','0x1a53c'],table_extent=['0x1a4dc','0x1a53c'],entries=24,
    true_ids=[n for n in range(256) if expected[n]],mismatches=0,executes_wine=False,
    timing=dict(compile_seconds=compiled-compile_start,execution_seconds=executed-compiled,total_seconds=time.perf_counter()-started))
Path('cases/conditional-opcode-validation.json').write_text(json.dumps(receipt,indent=2)+'\n')
print('257 conditional opcode outcomes match original bounded table/return blocks')
