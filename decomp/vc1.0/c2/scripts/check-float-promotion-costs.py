"""Compare compiled contextual promotion columns to their original bounded views."""
from case_paths import C2_SCRATCH
from pathlib import Path
import hashlib
import json
import re
import struct
import subprocess
import tempfile
import time
from binary import PE

started = time.perf_counter()
source = Path('src/float-machine.c').read_text()
startup = re.search(r'static const int float_context_startup_cost\[6\] = .*?;',source).group()
columns = source[source.index('typedef struct FloatPromotionCosts {'):source.index('/* Original [0003c9a0,')]
p = PE('../../../toolchains/1.0-nt/bin/C23232.EXE')
with tempfile.TemporaryDirectory(prefix='float-costs-',dir=C2_SCRATCH) as d:
    d = Path(d)
    probe = d/'probe.c'
    probe.write_text('extern int write(int,const void*,unsigned);\n'+startup+'\n'+columns+'''
int main(void) {
    if (write(1,float_context_startup_cost,sizeof(float_context_startup_cost)) != sizeof(float_context_startup_cost)) return 1;
    if (write(1,i386_float_promotion,sizeof(i386_float_promotion)) != sizeof(i386_float_promotion)) return 2;
    if (write(1,i486_float_promotion,sizeof(i486_float_promotion)) != sizeof(i486_float_promotion)) return 3;
    if (write(1,pentium_float_promotion,sizeof(pentium_float_promotion)) != sizeof(pentium_float_promotion)) return 4;
    return 0;
}
''')
    phase = time.perf_counter()
    subprocess.run(['clang','-m32','-std=gnu89','-O0','-g',str(probe),'-o',str(d/'probe')],check=True)
    compile_seconds = time.perf_counter()-phase
    phase = time.perf_counter()
    actual = subprocess.check_output([str(d/'probe')])
    execution_seconds = time.perf_counter()-phase
    probe_sha = hashlib.sha256(probe.read_bytes()).hexdigest()
expected = p.read(0x82f30,24)
views = []
for base, columns in [(0x82e80,4),(0x82ec0,4),(0x82f00,3)]:
    views += [[hex(base+16*c+4),hex(base+16*c+16)] for c in range(columns)]
    for size_class in (1,2,3):
        for column in range(columns): expected += p.read(base+16*column+4*size_class,4)
assert len(actual)==156 and actual==expected
receipt = dict(target_sha256=hashlib.sha256(p.data).hexdigest(),source_sha256=hashlib.sha256(source.encode()).hexdigest(),
    startup_view=['0x82f30','0x82f48'],promotion_column_views=views,
    startup_rows=6,i386_rows=3,i486_rows=3,pentium_rows=3,checked_dwords=39,mismatches=0,
    probe_sha256=probe_sha,executes_wine=False,
    limits=['Promotion function has three valid size classes established by its original diagnostic.',
            'Original zero-index padding columns are not accessed and are not copied.',
            'These value/shape checks do not claim all CPU/width branches executed by the floating case.'],
    timing=dict(compile_seconds=compile_seconds,execution_seconds=execution_seconds,total_seconds=time.perf_counter()-started))
Path('cases/floating-promotion-costs-proof.json').write_text(json.dumps(receipt,indent=2)+'\n')
print('39 compiled cost DWORDs match six startup rows and eleven bounded promotion columns')
