"""Check remapped inline-scan opcode predicates against original table bits."""
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

started = time.perf_counter()
source = Path('src/tree-input.c').read_text()
table = re.search(r'const unsigned short node_shapes\[NODE_OPCODE_COUNT\] = \{.*?\n\};', source, re.S).group()
with tempfile.TemporaryDirectory(prefix='inline-shapes-', dir=C2_SCRATCH) as directory:
    directory = Path(directory)
    probe = directory/'probe.c'
    probe.write_text('#include "'+str(Path('src/tree-input.h').resolve())+'"\n'+table+'''
#undef write
extern int write(int, const void *, unsigned);
int main(void) {
    unsigned i, flags[6];
    for (i=0; i<NODE_OPCODE_COUNT; ++i) {
        flags[0]=node_shapes[i]&NODE_ARITY;
        flags[1]=!!(node_shapes[i]&NODE_HAS_SECOND);
        flags[2]=!!(node_shapes[i]&NODE_SIDE_EFFECT);
        flags[3]=!!(node_shapes[i]&NODE_WRITES_LEFT);
        flags[4]=!!(node_shapes[i]&NODE_MEMORY_ACCESS);
        flags[5]=!!(node_shapes[i]&0x200); /* rejected original-bit reuse */
        if (write(1,flags,sizeof(flags))!=sizeof(flags)) return 1;
    }
    return 0;
}
''')
    result = subprocess.run(['clang','-m32','-std=gnu89','-fms-extensions','-fasm-blocks','-O0',
        '-DRECON_NATIVE_TEST','-I../../../lib/recon/include','-I../../../lib/recon/native',
        str(probe),'-o',str(directory/'probe')],capture_output=True)
    assert result.returncode == 0, result.stderr.decode()
    actual = struct.unpack('<1092I',subprocess.check_output([str(directory/'probe')]))
original = PE('../../../toolchains/1.0-nt/bin/C23232.EXE')
original_flags = struct.unpack('<182H',original.read(0x857b8,364))
for i, flags in enumerate(original_flags):
    expected = (flags&3, bool(flags&0x2000), bool(flags&0x200), bool(flags&0x800), bool(flags&0x400))
    assert actual[i*6:i*6+5] == expected, (hex(i),actual[i*6:i*6+5],expected)
wrong_effects = [hex(i) for i,f in enumerate(original_flags) if actual[i*6+5] != bool(f&0x200)]
wrong_writes = [hex(i) for i,f in enumerate(original_flags) if actual[i*6+4] != bool(f&0x800)]
assert wrong_effects and wrong_writes
scan = Path('src/inline-calls.c').read_text().split('void __fastcall scan_inline_argument_uses(',1)[1]
assert 'node_shapes[node->opcode] & NODE_SIDE_EFFECT' in scan
assert 'node_shapes[node->opcode] & NODE_WRITES_LEFT' in scan
assert not re.search(r'node_shapes[^\n]*&\s*0x(?:200|800)\b',scan)
receipt = dict(rejected_old_mappings={'raw_0x200_effects':wrong_effects,'memory_access_for_writes':wrong_writes},opcodes=182,predicates=5,checks=910,
    original_table=['0x857b8','0x85924'],
    source_sha256=hashlib.sha256(Path('src/tree-input.c').read_bytes()).hexdigest(),
    mapping={'arity':'3 -> NODE_ARITY','second':'0x2000 -> NODE_HAS_SECOND',
             'effects':'0x200 -> NODE_SIDE_EFFECT','writes':'0x800 -> NODE_WRITES_LEFT','memory':'0x400 -> NODE_MEMORY_ACCESS'},
    limit='Predicate value proof; does not claim all alias paths executed by inlrec.',
    seconds=time.perf_counter()-started)
Path('cases/inline-shape-validation.json').write_text(json.dumps(receipt,indent=2)+'\n')
print('182 original/native opcode rows: 910 inline-scan predicates match')
