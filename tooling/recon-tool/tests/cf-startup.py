#!/usr/bin/env python3
"""Finite startup planner controls; no process or original execution."""
from pathlib import Path
import struct
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'watcom'))
from gate import Function, Gate, Hold, Span

memory = bytearray(0x4000)
stack = Span(0x2000, 0x1000, 'stack', write=True)
data = Span(0x1000, 0x100, 'data', write=True)
segments = {'cs': 0x1b, 'ss': 0x23, 'ds': 0x23, 'es': 0x23}
base_context = dict(eip=0x100, esp=0x2800, eax=0, ecx=0, edx=0, ebx=0,
                    ebp=0, esi=0x1000, edi=0x1040, eflags=2, fs=0x3b, gs=0, **segments)


def gate(raw, context=None, **options):
    memory[0x100:0x100 + len(raw)] = raw
    memory[0x100 + len(raw)] = 0x90
    functions = [Function('startup', 0x100, {0x100: raw, 0x100 + len(raw): b'\x90'})]
    return Gate(functions, lambda address, size: memory[address:address + size],
                [stack, data, Span(0x100, len(raw) + 1, 'code')], stack, 1,
                Span(0, 0x4000, 'arena'), flat_segments=segments, startup=True, **options)


def denied(callback):
    try:
        callback()
    except Hold:
        return
    raise AssertionError('unsafe startup operation accepted')


# Genuine selector stores and 16-bit general-register stores; no CS/SS loads.
for raw in (b'\x66\x89\x1d\x00\x10\x00\x00', b'\x66\x8c\x1d\x00\x10\x00\x00',
            b'\x66\x8c\x15\x00\x10\x00\x00'):
    assert gate(raw).plan(dict(base_context), 1).writes == ((0x1000, 2),)
for raw in (b'\x17', b'\x8e\xd0', b'\x0f\xa9', b'\x66\x50', b'\x67\xac'):
    denied(lambda: gate(raw).plan(dict(base_context), 1))

# Selector push occupies a DWORD stack slot but verifies only the defined low word.
for raw in (b'\x06', b'\x1e'):
    g = gate(raw)
    decision = g.plan(dict(base_context), 1)
    assert decision.next_sp == 0x27fc and decision.writes == ((0x27fc, 2),)
    memory[0x27fc:0x2800] = b'\x23\x00\xfe\xca'
    after = dict(base_context, esp=0x27fc, eip=0x101)
    g.observe(after, 1)
struct.pack_into('<I', memory, 0x2800, 0xdead0023)
g = gate(b'\x07')
g.plan(dict(base_context), 1)
g.observe(dict(base_context, esp=0x2804, eip=0x101), 1)
struct.pack_into('<I', memory, 0x2800, 0x1b)
denied(lambda: gate(b'\x07').plan(dict(base_context), 1))

# Exact per-iteration REP zeroing; reject nonzero, reverse, overflow and unowned spans.
for raw, width in ((b'\xf3\xab', 4), (b'\xf3\xaa', 1)):
    g = gate(raw, zero_span=data)
    before = dict(base_context, ecx=3)
    for remaining in (2, 1, 0):
        decision = g.plan(before, 1)
        after = dict(before, ecx=remaining, edi=before['edi'] + width,
                     eip=0x100 if remaining else 0x102)
        memory[before['edi']:before['edi'] + width] = bytes(width)
        g.observe(after, 1)
        before = after
    for update in ({'eax': 1}, {'eflags': 0x402}, {'edi': 0x10ff, 'ecx': 2}, {'ecx': 65537}):
        denied(lambda: gate(raw, zero_span=data).plan((base_context | {"ecx": 3} | update), 1))
    g = gate(raw, zero_span=data)
    decision = g.plan(dict(base_context, ecx=0), 1)
    assert decision.next_pc == 0x102 and not decision.writes
    denied(lambda: gate(raw).plan(dict(base_context, ecx=3), 1))

memory[0x1000] = 0x41
g = gate(b'\xac')
g.plan(dict(base_context, eax=0x12340000), 1)
g.observe(dict(base_context, eax=0x12340041, esi=0x1001, eip=0x101), 1)
denied(lambda: gate(b'\xac').plan(dict(base_context, esi=0x1100), 1))
g = gate(b'\xac')
g.plan(dict(base_context), 1)
denied(lambda: g.observe(dict(base_context, eax=0x42, esi=0x1001, eip=0x101), 1))
print('CF startup bounds/selectors/word operations/LODSB/REP planner PASS')
