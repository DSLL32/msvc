"""Audit function edges without interpreting jump tables as instructions."""
from pe_indexes import indexes
from binary import u32
from capstone import Cs, CS_ARCH_X86, CS_MODE_32, CS_GRP_JUMP, CS_GRP_RET
from capstone.x86_const import X86_OP_IMM, X86_OP_MEM

cs = Cs(CS_ARCH_X86, CS_MODE_32)
cs.detail = True

def at(address, records):
    exact = [r for r in records if r['start'] == address]
    return exact[0]['name'] if exact else hex(address)

def exception_entries(pe, record):
    """MSVC's _except_handler2 scope table references filter/handler blocks."""
    entries = []
    start, end = record['start'], record['end']
    for instruction in cs.disasm(pe.read(start, min(end-start, 256)), start):
        for operand in instruction.operands:
            if operand.type != X86_OP_IMM:
                continue
            address = operand.imm & 0xffffffff
            try:
                data = pe.read(address, 12)
                previous, filter_address, handler = (u32(data, i) for i in (0, 4, 8))
            except Exception:
                continue
            if previous != 0xffffffff or not start <= handler < end:
                continue
            if filter_address and not start <= filter_address < end:
                continue
            entries.append(handler)
            if filter_address:
                entries.append(filter_address)
    return entries

def edges(pe, records, record):
    start, end = record['start'], record['end']
    pending = [start] + exception_entries(pe, record)
    visited, result = set(), set()
    while pending:
        address = pending.pop()
        while start <= address < end and address not in visited:
            visited.add(address)
            # Decode past the declared end so a truncated RET immediate (or
            # another instruction operand) cannot silently end this walk.
            instruction = next(cs.disasm(pe.read(address, 15), address), None)
            if instruction is None:
                break
            following = address + instruction.size
            if following > end:
                raise ValueError('%s extent ends at %08x inside %s at %08x' %
                                 (record['name'], end, instruction.mnemonic, address))
            operands = instruction.operands
            if instruction.mnemonic == 'call':
                if operands[0].type == X86_OP_IMM:
                    result.add(at(operands[0].imm & 0xffffffff, records))
                else:
                    result.add('indirect')
            elif instruction.group(CS_GRP_JUMP):
                if operands[0].type == X86_OP_IMM:
                    target = operands[0].imm & 0xffffffff
                    if start <= target < end:
                        pending.append(target)
                    else:
                        result.add(at(target, records))
                else:
                    # Both target and reconstruction use dword switch tables.
                    operand = operands[0]
                    table_targets = []
                    if operand.type == X86_OP_MEM and (operand.mem.index or operand.mem.base) and start <= (operand.mem.disp & 0xffffffff) < end:
                        table = operand.mem.disp & 0xffffffff
                        for index in range(256):
                            try:
                                target = u32(pe.read(table+index*4, 4), 0)
                            except Exception:
                                break
                            if not start <= target < end:
                                break
                            table_targets.append(target)
                    if table_targets:
                        pending.extend(table_targets)
                    else:
                        result.add('indirect')
                if instruction.mnemonic == 'jmp':
                    break
            elif instruction.group(CS_GRP_RET) or instruction.mnemonic == 'int3':
                break
            address = following
    return result

def main():
    original, reconstructed, config, original_index, reconstructed_index = indexes()
    # Runtime and import bodies also enter the trap mask. Their source graphs
    # come from the original libraries, but their accepted extents must still
    # end on complete instructions on both sides.
    driver_names = {record['name'] for record in config['driver']}
    accepted = set(config['accepted'])
    for pe, records in ((original, original_index), (reconstructed, reconstructed_index)):
        for record in records:
            if record['name'] in accepted and record['name'] not in driver_names:
                edges(pe, records, record)
    checked, failures = 0, 0
    for record in config['driver']:
        if record['name'] not in config['accepted']:
            continue
        counterpart = next(r for r in reconstructed_index if r['name'] == record['name'])
        actual = edges(original, original_index, record)
        expected = edges(reconstructed, reconstructed_index, counterpart)
        checked += 1
        if actual != expected:
            print(record['name'], 'original only', actual-expected,
                  'reconstruction only', expected-actual)
            failures += 1
    if failures:
        raise SystemExit(1)
    print('%d reconstructed functions have matching graph edges (including indirect calls and tail calls).' % checked)

if __name__ == '__main__':
    main()
