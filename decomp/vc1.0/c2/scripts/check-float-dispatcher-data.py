"""Independently compare contextual stack operands and popping arithmetic reversal.

Only a native -O0 probe runs; no Wine, section snapshot or fixture recapture.
"""
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


def main():
    started = time.perf_counter()
    source = Path('src/float-machine.c')
    expression = re.search(r'#define FM_SET_STACK_OPERAND.*?\n} while \(0\)',source.read_text(),re.S).group() + '\n' + re.search(r'#define FM_REVERSE_POP.*?\(op\)\)',source.read_text(),re.S).group()
    original = PE('../../../toolchains/1.0-nt/bin/C23232.EXE')
    names = re.findall(r'^OP\((\w+),', Path('src/machine-opcodes.def').read_text(), re.M)
    with tempfile.TemporaryDirectory(prefix='float-stack-effect-', dir=C2_SCRATCH) as directory:
        directory = Path(directory)
        probe = directory/'probe.c'
        probe.write_text('#include "'+str(Path('src/instruction-selection.h').resolve())+'"\n#undef write\nextern int write(int,const void*,unsigned);\n'+expression+'''
int main(void)
{
    MachineOperand operand;
    unsigned slot, operation, reverse;
    for (slot=0; slot<8; ++slot) {
        FM_SET_STACK_OPERAND(operand,slot);
        if (write(1,&operand,sizeof(operand)) != sizeof(operand)) return 1;
    }
    for (operation=MC_FADDP; operation<=MC_FDIVRP; ++operation) {
        reverse=FM_REVERSE_POP(operation);
        if (write(1,&reverse,sizeof(reverse)) != sizeof(reverse)) return 2;
    }
    return 0;
}
''')
        command = ['clang','-m32','-std=gnu89','-fms-extensions','-fasm-blocks','-O0','-g','-DRECON_NATIVE_TEST',
                   '-I../../../lib/recon/include','-I../../../lib/recon/native',str(probe),'-o',str(directory/'probe')]
        phase = time.perf_counter()
        result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        assert result.returncode == 0, result.stdout.decode()
        compile_seconds = time.perf_counter()-phase
        phase = time.perf_counter()
        actual = subprocess.check_output([str(directory/'probe')])
        execution_seconds = time.perf_counter()-phase
        probe_sha = hashlib.sha256(probe.read_bytes()).hexdigest()
    expected = original.read(0x82e28,64)+original.read(0x82f60,24)
    assert len(actual)==88 and actual==expected
    receipt = dict(target_sha256=hashlib.sha256(original.data).hexdigest(),
        source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
        operand_view=['0x82e28','0x82e68'], popping_reversal_view=['0x82f60','0x82f78'],
        operand_rows=8, reversal_rows=6, mismatches=0,
        expression_sha256=hashlib.sha256(expression.encode()).hexdigest(), probe_sha256=probe_sha,
        executes_wine=False, timing=dict(compile_seconds=compile_seconds,execution_seconds=execution_seconds,total_seconds=time.perf_counter()-started))
    Path('cases/floating-dispatcher-data-proof.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print('Eight stack operand rows and six popping reversal rows match compiled macros')


if __name__ == '__main__':
    main()
