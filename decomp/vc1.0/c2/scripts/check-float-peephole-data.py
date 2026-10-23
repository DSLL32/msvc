"""Independent bounded original/compiled-native x87 peephole data checks.

No Wine or reference execution. The tiny native probe only exercises the
register-encoding macro, not reconstructed control flow or fixtures.
"""
from case_paths import C2_SCRATCH
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile
import time
from binary import PE


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    started = time.perf_counter()
    original = PE('../../../toolchains/1.0-nt/bin/C23232.EXE')
    source = Path('src/float-peepholes.c')
    native = Path('build/native/C23232.EXE')
    fields = ['normal','popping','right_popping','integer','right_integer','reversed']
    expression = ('python import gdb,json; print("X87_DATA "+json.dumps(dict('
        'forms=[[int(gdb.parse_and_eval("x87_arithmetic_forms[%d].%s"%(i,f))) '
        'for f in '+repr(fields)+'] for i in range(6)],'
        'conditions=[int(gdb.parse_and_eval("exchanged_x87_condition[%d]"%i)) for i in range(16)],'
        'form_bytes=int(gdb.parse_and_eval("sizeof(X87ArithmeticForms)")),'
        'operand_bytes=int(gdb.parse_and_eval("sizeof(MachineOperand)")),'
        'flags_offset=int(gdb.parse_and_eval("(unsigned)&((MachineOperand*)0)->flags")),'
        'value_offset=int(gdb.parse_and_eval("(unsigned)&((MachineOperand*)0)->value")))))')
    gdb_started = time.perf_counter()
    log = subprocess.check_output(['gdb','-q','--batch','-ex',expression,str(native)],stderr=subprocess.STDOUT,text=True)
    compiled = json.loads(next(l[9:] for l in log.splitlines() if l.startswith('X87_DATA ')))
    gdb_seconds = time.perf_counter()-gdb_started
    assert (compiled['form_bytes'],compiled['operand_bytes'],compiled['flags_offset'],compiled['value_offset']) == (24,8,0,4)
    assert [row[0] for row in compiled['forms']] == list(range(0x45,0x4b))
    assert [row[1] for row in compiled['forms']] == list(range(0x4b,0x51))
    views = []
    for field,address in [('normal',0x82d90),('integer',0x82da8),('right_popping',0x82f60),('right_integer',0x82f78),('reversed',0x82f90)]:
        expected = list(struct.unpack('<6I',original.read(address,24)))
        actual = [r[fields.index(field)] for r in compiled['forms']]
        assert actual == expected, (field,actual,expected)
        views.append(dict(field=field,original_extent=[hex(address),hex(address+24)],opcodes=actual,matched=True))
    assert compiled['conditions'] == list(struct.unpack('<16I',original.read(0x82dc8,64)))
    macro = re.search(r'#define FP_SHIFT_REGISTER\(.*?\n\} while \(0\)',source.read_text(),re.S).group()
    with tempfile.TemporaryDirectory(prefix='float-encoding-probe-',dir=C2_SCRATCH) as scratch:
        scratch = Path(scratch)
        probe = scratch/'probe.c'
        probe.write_text('#include "'+str(Path('src/instruction-selection.h').resolve())+'"\n#undef write\nextern int write(int,const void *,unsigned);\n'+macro+'''
int main(void)
{
    MachineOperand operand;
    unsigned reg;
    for (reg=15;reg<22;++reg) {
        FP_SHIFT_REGISTER(operand,reg,-1);
        if (write(1,&operand,sizeof(operand)) != sizeof(operand)) return 1;
    }
    for (reg=14;reg<21;++reg) {
        FP_SHIFT_REGISTER(operand,reg,1);
        if (write(1,&operand,sizeof(operand)) != sizeof(operand)) return 2;
    }
    return 0;
}
''')
        command = ['clang','-m32','-std=gnu89','-fms-extensions','-fasm-blocks','-O0','-g','-DRECON_NATIVE_TEST',
            '-I../../../lib/recon/include','-I../../../lib/recon/native',str(probe),'-o',str(scratch/'probe')]
        compile_started = time.perf_counter()
        result = subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        assert result.returncode == 0, result.stdout.decode()
        compile_seconds = time.perf_counter()-compile_started
        run_started = time.perf_counter()
        actual = subprocess.check_output([str(scratch/'probe')])
        execution_seconds = time.perf_counter()-run_started
        expected = b''.join(original.read(0x82cd8+reg*8,8) for reg in range(15,22)) + b''.join(original.read(0x82ce8+reg*8,8) for reg in range(14,21))
        assert len(actual) == 112 and actual == expected
        probe_sha256 = sha(probe)
    receipt = dict(target_sha256=hashlib.sha256(original.data).hexdigest(),source_sha256=sha(source),native_sha256=sha(native),
        shape={k:compiled[k] for k in ('form_bytes','operand_bytes','flags_offset','value_offset')},
        arithmetic_views=views,conditions=dict(original_extent=['0x82dc8','0x82e08'],values=compiled['conditions'],matched=True),
        stack_registers=dict(lower_domain=[15,21],upper_domain=[14,20],rows=14,matched=True,
            lower_original_extents=['0x82d50','0x82d88'],upper_original_extents=['0x82d58','0x82d90'],
            macro_sha256=hashlib.sha256(macro.encode()).hexdigest(),probe_source_sha256=probe_sha256,
            rule='register operand flags1; low DWORD register number plus/minus1; original table addresses never escape'),
        mismatches=0,executes_wine=False,timing=dict(gdb_seconds=gdb_seconds,probe_compile_seconds=compile_seconds,
            probe_execution_seconds=execution_seconds,total_seconds=time.perf_counter()-started))
    Path('cases/floating-peephole-data-validation.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print('Six opcode forms, five bounded table views, sixteen condition DWORDs and fourteen register encodings match')


if __name__ == '__main__':
    main()
