#!/usr/bin/env python3
"""Compare bounded ENTER planning with actual native i386 execution; no Wine."""
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'watcom'))
from gate import Function, Gate, Hold, Span

source = r"""
.section .data
.balign 4
record: .zero 36
.section .text
.global _start
_start:
    movl $0x12345678, %ebp
    movl %esp, record
    movl %ebp, record+4
    pushfl
    popl record+8
enter_site:
    enter $88, $0
enter_end:
    movl %esp, record+12
    movl %ebp, record+16
    movl (%ebp), %eax
    movl %eax, record+20
    pushfl
    popl record+24
    movl $enter_site, record+28
    movl $enter_end, record+32
    leave
    movl $4, %eax
    movl $1, %ebx
    movl $record, %ecx
    movl $36, %edx
    int $0x80
    movl $1, %eax
    xorl %ebx, %ebx
    int $0x80
"""
with tempfile.TemporaryDirectory(prefix='recon-enter-', dir=Path.home() / '.ai-tmp') as work:
    work = Path(work)
    (work / 'enter.S').write_text(source)
    subprocess.run(['gcc', '-m32', '-nostdlib', '-no-pie', str(work / 'enter.S'),
                    '-o', str(work / 'enter')], check=True, capture_output=True)
    actual = subprocess.check_output([str(work / 'enter')])
sp, bp, flags, after_sp, after_bp, saved, after_flags, pc, end = struct.unpack('<9I', actual)
assert after_sp == sp - 92 and after_bp == sp - 4 and saved == bp and flags == after_flags
raw = bytes.fromhex('c8580000')
segments = dict(cs=0x23, ss=0x2b, ds=0x2b, es=0x2b)
before = dict(eip=pc, esp=sp, ebp=bp, eflags=flags, **segments)
after = dict(before, eip=end, esp=after_sp, ebp=after_bp, eflags=after_flags)


def make(code=raw, stack_start=sp - 4096, saved_word=saved):
    memory = {pc: code, end: b'\x90', sp - 4: struct.pack('<I', saved_word)}
    stack = Span(stack_start, sp - stack_start + 4, 'actual native stack', write=True)
    g = Gate([Function('MemInit frame control', pc, {pc: code, pc + len(code): b'\x90'})],
             lambda address, size: memory[address][:size], [stack, Span(pc, len(code) + 1, 'code')],
             stack, 1, Span(pc, 128, 'authored code'), flat_segments=segments)
    return g


def denied(callback):
    try:
        callback()
    except Hold:
        return
    raise AssertionError('invalid ENTER accepted')


g = make()
del g.code[end]
denied(lambda: g.plan(before, 1))
g = make()
p = g.plan(before, 1)
assert p.next_sp == after_sp and p.writes == ((sp - 4, 4),)
g.observe(after, 1)
for code in (bytes.fromhex('c8580001'), bytes.fromhex('66c8580000')):
    denied(lambda: make(code).plan(before, 1))
denied(lambda: make(stack_start=sp - 91).plan(before, 1))
for wrong in (dict(after, ebp=sp), dict(after, eflags=after_flags ^ 1)):
    g = make()
    g.plan(before, 1)
    denied(lambda: g.observe(wrong, 1))
g = make(saved_word=saved ^ 1)
g.plan(before, 1)
denied(lambda: g.observe(after, 1))
print('CF ENTER88,0 actual native frame/flags and negative controls PASS')
