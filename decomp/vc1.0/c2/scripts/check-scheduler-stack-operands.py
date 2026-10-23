"""Check scheduler ST-slot operand encoding against eight bounded original rows."""
from case_paths import C2_SCRATCH
from pathlib import Path
import hashlib
import json
import re
import subprocess
import tempfile
import time
from binary import PE
started = time.perf_counter()
source = Path('src/scheduler.c').read_text()
macro = re.search(r'#define SCHEDULER_STACK_OPERAND.*?\n} while \(0\)',source,re.S).group()
p = PE('../../../toolchains/1.0-nt/bin/C23232.EXE')
with tempfile.TemporaryDirectory(prefix='scheduler-st-',dir=C2_SCRATCH) as d:
    d = Path(d)
    probe = d/'probe.c'
    probe.write_text('#include "'+str(Path('src/instruction-selection.h').resolve())+'"\n#include \"'+str(Path('src/float-machine.h').resolve())+'\"\n#undef write\nextern int write(int,const void*,unsigned);\nunsigned float_motion_reset_word;\nFloatStreamUse *float_stream_uses;\n'+macro+'''
int main(void) {
    MachineOperand operand;
    int slot;
    for (slot=0; slot<8; ++slot) {
        SCHEDULER_STACK_OPERAND(operand,slot);
        if (write(1,&operand,sizeof(operand)) != sizeof(operand)) return 1;
    }
    return 0;
}
''')
    phase = time.perf_counter()
    subprocess.run(['clang','-m32','-std=gnu89','-fms-extensions','-fasm-blocks','-O0','-g','-DRECON_NATIVE_TEST','-I../../../lib/recon/include','-I../../../lib/recon/native',str(probe),'-o',str(d/'probe')],check=True)
    compile_seconds = time.perf_counter()-phase
    phase = time.perf_counter()
    actual = subprocess.check_output([str(d/'probe')])
    execution_seconds = time.perf_counter()-phase
    probe_sha = hashlib.sha256(probe.read_bytes()).hexdigest()
assert len(actual)==64 and actual==p.read(0x82e28,64)
receipt = dict(target_sha256=hashlib.sha256(p.data).hexdigest(),source_sha256=hashlib.sha256(source.encode()).hexdigest(),
    original_extent=['0x82e28','0x82e68'],rows=8,operand_bytes=8,mismatches=0,executes_wine=False,
    macro_sha256=hashlib.sha256(macro.encode()).hexdigest(),probe_sha256=probe_sha,
    timing=dict(compile_seconds=compile_seconds,execution_seconds=execution_seconds,total_seconds=time.perf_counter()-started))
Path('cases/floating-scheduler-stack-operands-proof.json').write_text(json.dumps(receipt,indent=2)+'\n')
print('Eight compiled scheduler ST operands match original rows')
