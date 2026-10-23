"""Probe the actual scheduled FXCH body and original operand/slot8 layout.

Synthetic state exercises all1..8 slot inputs; this is not evidence that the
compiler case itself executes slot8. No Wine/reference recapture occurs.
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

started = time.perf_counter()
source = Path('src/scheduler.c').read_text()
body = re.search(r'void __fastcall exchange_scheduled_float_stack\(int slot\)\n\{.*?\n\}', source, re.S).group()
operand = re.search(r'#define SCHEDULER_STACK_OPERAND.*?\n} while \(0\)', source, re.S).group()
with tempfile.TemporaryDirectory(prefix='scheduler-slot-alias-', dir=C2_SCRATCH) as directory:
    directory = Path(directory)
    probe = directory/'probe.c'
    probe.write_text('#include "'+str(Path('src/scheduler.h').resolve())+'"\n'+
        '#include "'+str(Path('src/instruction-selection.h').resolve())+'"\n'+
        '#include "'+str(Path('src/float-machine.h').resolve())+'"\n'+r'''
#undef write
extern int write(int,const void*,unsigned);
MachineInstruction *scheduler_cursor;
int scheduler_float_values[8], scheduler_float_serial, *scheduler_float_positions;
unsigned float_motion_reset_word;
FloatStreamUse *float_stream_uses;
static MachineInstruction created;
static InstructionSchedule schedule;
MachineInstruction *__fastcall new_machine_instruction(unsigned kind)
{ created.opcode=kind; return &created; }
InstructionSchedule *__cdecl new_instruction_schedule(void) { return &schedule; }
void __fastcall compiler_internal_error(char *file, unsigned line) { __builtin_trap(); }
'''+operand+'\n'+body+r'''
int main(void)
{
    MachineInstruction before, after;
    int slot, i, positions[3];
    unsigned row[10];
    MachineOperand copied;
    for(slot=0;slot<9;++slot) {
        float_motion_reset_word=0xabcddcbaU;
        float_stream_uses=(FloatStreamUse *)0x12345678U;
        SCHEDULER_STACK_OPERAND(copied,slot);
        if(write(1,&copied,sizeof(copied))!=sizeof(copied)) return 1;
    }
    for(slot=1;slot<=8;++slot) {
        for(i=0;i<8;++i) scheduler_float_values[i]=-1;
        scheduler_float_values[0]=1;
        if(slot<8) scheduler_float_values[slot]=2;
        scheduler_float_serial=2;
        positions[1]=0; positions[2]=slot;
        scheduler_float_positions=positions;
        before.next=&after; after.previous=&before;
        scheduler_cursor=&before;
        exchange_scheduled_float_stack(slot);
        row[0]=created.operands[0].flags; row[1]=created.operands[0].value.constant;
        row[2]=positions[1]; row[3]=positions[2]; row[4]=scheduler_float_values[0];
        row[5]=slot==8?scheduler_float_serial:scheduler_float_values[slot];
        row[6]=scheduler_float_serial; row[7]=created.opcode;
        row[8]=before.next==&created && created.previous==&before &&
            created.next==&after && after.previous==&created;
        row[9]=scheduler_cursor==&created && created.record.scheduling==&schedule;
        if(write(1,row,sizeof(row))!=sizeof(row)) return 2;
    }
    return 0;
}
''')
    phase = time.perf_counter()
    result = subprocess.run(['clang','-m32','-std=gnu89','-fms-extensions','-fasm-blocks',
        '-O0','-g','-DRECON_NATIVE_TEST','-I../../../lib/recon/include',
        '-I../../../lib/recon/native',str(probe),'-o',str(directory/'probe')],capture_output=True)
    assert result.returncode==0,result.stderr.decode()
    build_seconds=time.perf_counter()-phase
    phase=time.perf_counter();actual=subprocess.check_output([str(directory/'probe')]);run_seconds=time.perf_counter()-phase
    probe_sha=hashlib.sha256(probe.read_bytes()).hexdigest()
original=PE('../../../toolchains/1.0-nt/bin/C23232.EXE')
expected=original.read(0x82e28,64)+struct.pack('<II',0xabcddcba,0x12345678)
for slot in range(1,9):
    flags,value=(0xabcddcba,0x12345678) if slot==8 else (1,14+slot)
    expected+=struct.pack('<10I',flags,value,slot,0,2,1,1 if slot==8 else 2,0x5f80,1,1)
assert actual==expected, (actual.hex(),expected.hex())
receipt=dict(method='Actual source body and operand macro compiled native i386 O0; original eight operand rows plus explicit synthetic live slot8 alias sentinels',
    original_operand_rows=['0x82e28','0x82e68'],slot8_aliases=['0x82e68 float_motion_reset_word','0x82e6c float_stream_uses','0x7c338 scheduler_float_serial'],
    slots_checked=list(range(1,9)),operand_rows_checked=list(range(9)),
    checks=['operand byte layout','logical identities and inverse positions','serial alias mutation','FXCH opcode/width','linked insertion and scheduling record'],
    source_sha256=hashlib.sha256(source.encode()).hexdigest(),probe_sha256=probe_sha,
    target_sha256=hashlib.sha256(original.data).hexdigest(),bytes_compared=len(actual),
    limits='Synthetic slot8 test is distinct from fresh target-case execution; original diagnostic1..8 contract retained.',
    timing=dict(build_seconds=build_seconds,execution_seconds=run_seconds,total_seconds=time.perf_counter()-started))
Path('cases/scheduler-stack-alias-validation.json').write_text(json.dumps(receipt,indent=2)+'\n')
print('Scheduled FXCH slots1..8 and all operand rows/typed aliases match')
