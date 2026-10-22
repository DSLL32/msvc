#!/usr/bin/env python3
"""Check native ECX-member stack cleanup against C fastcall reconstructions."""
import re
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_IMM, X86_OP_MEM
from binary import u32
from indexes import indexes

original, rebuilt, config, _, counterparts = indexes()
decoder = Cs(CS_ARCH_X86, CS_MODE_32)
decoder.detail = True

def return_pops(image, record):
    pending = [record['start']]
    visited, pops = set(), set()
    while pending:
        cursor = pending.pop()
        while record['start'] <= cursor < record['end'] and cursor not in visited:
            visited.add(cursor)
            decoded = list(decoder.disasm(image.read(cursor, 15), cursor, count=1))
            assert decoded, (record['name'], hex(cursor))
            instruction = decoded[0]
            if instruction.mnemonic.startswith('ret'):
                pops.add(instruction.operands[0].imm if instruction.operands else 0)
                break
            if instruction.mnemonic.startswith('j'):
                operand = instruction.operands[0]
                if operand.type == X86_OP_IMM:
                    pending.append(operand.imm)
                elif instruction.mnemonic == 'jmp' and operand.type == X86_OP_MEM:
                    # Native indexed switch tables contain internal code pointers.
                    table = operand.mem.disp
                    for index in range(4096):
                        try:
                            target = u32(image.read(table + index * 4, 4), 0)
                        except (ValueError, KeyError, StopIteration, IndexError):
                            break
                        if not record['start'] <= target < record['end']:
                            break
                        pending.append(target)
                if instruction.mnemonic == 'jmp':
                    break
            cursor += instruction.size
    return pops

count = 0
for record in config['driver']:
    if record['name'] not in config['accepted'] or not record.get('symbol', '').startswith('@'):
        continue
    counterpart = next(item for item in counterparts if item['name'] == record['name'])
    argument_bytes = int(re.search(r'@(\d+)$', record['symbol'])[1])
    expected = {argument_bytes - 8}  # ECX this and unused EDX occupy register slots.
    native, actual = return_pops(original, record), return_pops(rebuilt, counterpart)
    assert native == actual == expected, (record['name'], native, actual, expected)
    count += 1
print('%d ECX-member routines preserve native return stack cleanup.' % count)
