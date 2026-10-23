"""Independently compare the contextual x87 stack-effect expression to all ISA rows.

Only a native -O0 probe runs; no Wine, section snapshot or fixture recapture.
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


def main():
    started = time.perf_counter()
    source = Path('src/float-machine.c')
    expression = re.search(r'#define FM_STACK_EFFECT\(op\).*?\? 1 : 0\)', source.read_text(), re.S).group()
    original = PE('../../../toolchains/1.0-nt/bin/C23232.EXE')
    names = re.findall(r'^OP\((\w+),', Path('src/machine-opcodes.def').read_text(), re.M)
    with tempfile.TemporaryDirectory(prefix='float-stack-effect-', dir=Path.home()/'.ai-tmp/vc1.0-c2') as directory:
        directory = Path(directory)
        probe = directory/'probe.c'
        probe.write_text('#include "'+str(Path('src/machine.h').resolve())+'"\n#undef write\nextern int write(int,const void*,unsigned);\n'+expression+'''
int main(void)
{
    int operation, effect;
    for (operation=0; operation<MC_OPCODE_COUNT; ++operation) {
        effect=FM_STACK_EFFECT(operation);
        if (write(1,&effect,sizeof(effect)) != sizeof(effect)) return 1;
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
        actual = list(struct.unpack('<'+str(len(names))+'i', subprocess.check_output([str(directory/'probe')])))
        execution_seconds = time.perf_counter()-phase
        probe_sha = hashlib.sha256(probe.read_bytes()).hexdigest()
    expected = [struct.unpack('<h',original.read(0x82210+2*i,2))[0] >> 14 for i in range(len(names))]
    assert actual == expected, [(names[i], a, e) for i,(a,e) in enumerate(zip(actual,expected)) if a!=e]
    receipt = dict(target_sha256=hashlib.sha256(original.data).hexdigest(),
        source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
        original_view=['0x82210',hex(0x82210+2*len(names))],
        rule='signed short opcode property >>14', rows=len(names), mismatches=0,
        nonzero_classes={str(effect):[name for name,value in zip(names,actual) if value==effect] for effect in [-2,-1,1]},
        expression_sha256=hashlib.sha256(expression.encode()).hexdigest(), probe_sha256=probe_sha,
        executes_wine=False, timing=dict(compile_seconds=compile_seconds,execution_seconds=execution_seconds,total_seconds=time.perf_counter()-started))
    Path('cases/floating-stack-effects-proof.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(str(len(names))+' original signed x87 stack effects match compiled semantic classes')


if __name__ == '__main__':
    main()
