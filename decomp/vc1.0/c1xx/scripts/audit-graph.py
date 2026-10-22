"""Compare direct callees and indirect-call presence, excluding switch data."""
from win32_indexes import indexes
from binary import u32
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
import re

orig, rec, cfg, oi, ri = indexes()
cs = Cs(CS_ARCH_X86, CS_MODE_32)

def at(address, records):
    exact = [r for r in records if r['start'] == address]
    return exact[0]['name'] if exact else hex(address)

def instructions(pe, function):
    start, end = function['start'], function['end']
    tables = {}
    while True:
        result = []
        cursor = start
        for a, b in sorted(tables.items()) + [(end, end)]:
            result.extend(cs.disasm(pe.read(cursor, a-cursor), cursor))
            cursor = b
        discovered = dict(tables)
        for ins in result:
            if ins.mnemonic != 'jmp' or '[' not in ins.op_str:
                continue
            literals = re.findall(r'0x[0-9a-f]+', ins.op_str)
            if not literals:
                continue
            table = int(literals[-1], 16)
            if not start <= table < end:
                continue
            count = 0
            while table + count*4 < end:
                target = u32(pe.read(table+count*4, 4), 0)
                if not start <= target < end:
                    break
                count += 1
            if count:
                discovered[table] = table + count*4
        if discovered == tables:
            # Decode past the accepted endpoint for analysis only. A split
            # RET n operand otherwise becomes INT3 bytes when masking, while
            # a bounded decoder can silently omit that final instruction.
            tail_start = max([start] + list(tables.values()))
            for instruction in cs.disasm(pe.read(tail_start, end-tail_start+15), tail_start):
                assert not instruction.address < end < instruction.address + instruction.size, \
                    'extent truncates instruction: %s at %x (end %x)' % (function['name'], instruction.address, end)
                if instruction.address >= end:
                    break
            return result
        tables = discovered

def edges(pe, records, function):
    calls = set()
    for ins in instructions(pe, function):
        if ins.mnemonic == 'call' or (ins.mnemonic == 'jmp' and
            re.fullmatch(r'0x[0-9a-f]+', ins.op_str) and
            not function['start'] <= int(ins.op_str, 16) < function['end']):
            if re.fullmatch(r'0x[0-9a-f]+', ins.op_str):
                calls.add(at(int(ins.op_str, 16), records))
            else:
                calls.add('indirect')
    return calls

failures = checked = 0
for function in cfg['driver']:
    if function['name'] not in cfg['accepted']:
        continue
    rebuilt = next(r for r in ri if r['name'] == function['name'])
    a, b = edges(orig, oi, function), edges(rec, ri, rebuilt)
    checked += 1
    if a != b:
        print(function['name'], 'original only', a-b, 'reconstruction only', b-a)
        failures += 1
if failures:
    raise SystemExit(1)
print('%d reconstructed functions have matching call targets (including indirect calls).' % checked)
