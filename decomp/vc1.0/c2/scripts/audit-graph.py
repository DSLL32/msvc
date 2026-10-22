"""Audit function edges without interpreting jump tables as instructions."""
from pe_indexes import indexes
from binary import u32
import re
from capstone import Cs, CS_ARCH_X86, CS_MODE_32, CS_GRP_JUMP, CS_GRP_RET
from capstone.x86_const import X86_OP_IMM, X86_OP_MEM

cs = Cs(CS_ARCH_X86, CS_MODE_32)
cs.detail = True
fast_cs = Cs(CS_ARCH_X86, CS_MODE_32)
memory_operand = re.compile(r'\[[^\]]*\]')
numeric_operand = re.compile(r'(?<!\w)[-+]?(?:0x[0-9a-f]+|\d+)(?!\w)')

def address_names(records):
    names = {}
    for record in records:
        # Preserve the first alias, as the original linear lookup did.
        names.setdefault(record['start'], record['name'])
    return names

class FunctionDecoder:
    """Cache lightweight batches; materialize operands only when inspected.

    Branch destinations still start their own decode stream, including a
    destination inside a previously decoded instruction. Cached instructions
    are not visited code until the control-flow walker actually reaches them.
    """
    def __init__(self, pe, record):
        self.start, self.end = record['start'], record['end']
        # Include up to one instruction beyond the endpoint for the existing
        # truncated-operand check. The prefix scan remains bounded separately.
        self.code = pe.read(self.start, self.end-self.start+14)
        self.instructions, self.details = {}, {}

    def prefix(self):
        for instruction in fast_cs.disasm_lite(self.code[:min(self.end-self.start, 256)], self.start):
            self.instructions[instruction[0]] = instruction
            yield instruction

    def instruction(self, address):
        if address not in self.instructions:
            offset = address-self.start
            for instruction in fast_cs.disasm_lite(self.code[offset:offset+256], address):
                self.instructions[instruction[0]] = instruction
        return self.instructions.get(address)

    def detail(self, address, size):
        if address not in self.details:
            offset = address-self.start
            self.details[address] = next(cs.disasm(self.code[offset:offset+size], address, count=1))
        return self.details[address]

def exception_entries(pe, record, decoder=None):
    """MSVC's _except_handler2 scope table references filter/handler blocks."""
    entries = []
    start, end = record['start'], record['end']
    decoder = decoder or FunctionDecoder(pe, record)
    for address, size, mnemonic, operands in decoder.prefix():
        # Numeric text only prefilters possible table addresses. Most immediate
        # operands are small constants, not table pointers: reject those before
        # allocating Capstone's detailed operand structures. Every plausible
        # table is still checked against actual IMM operand types below.
        plausible = False
        for literal in numeric_operand.findall(memory_operand.sub('', operands)):
            candidate = int(literal, 16 if '0x' in literal else 10) & 0xffffffff
            try:
                data = pe.read(candidate, 12)
                previous, filter_address, handler = (u32(data, i) for i in (0, 4, 8))
            except Exception:
                continue
            if (previous == 0xffffffff and start <= handler < end and
                    (not filter_address or start <= filter_address < end)):
                plausible = True
                break
        if not plausible:
            continue
        instruction = decoder.detail(address, size)
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

def edges(pe, records, record, names=None):
    start, end = record['start'], record['end']
    decoder = FunctionDecoder(pe, record)
    names = address_names(records) if names is None else names
    pending = [start] + exception_entries(pe, record, decoder)
    visited, result = set(), set()
    while pending:
        address = pending.pop()
        while start <= address < end and address not in visited:
            visited.add(address)
            # Decode past the declared end so a truncated RET immediate (or
            # another instruction operand) cannot silently end this walk.
            decoded = decoder.instruction(address)
            if decoded is None:
                break
            _, size, mnemonic, operands = decoded
            following = address + size
            if following > end:
                raise ValueError('%s extent ends at %08x inside %s at %08x' %
                                 (record['name'], end, mnemonic, address))
            # Prefixes such as "repz ret" and "bnd jmp" remain classified by
            # Capstone groups below, rather than changing the walk's semantics.
            operation = mnemonic.split()[-1]
            if not (operation.startswith(('j', 'ret', 'loop')) or
                    operation in ('call', 'lcall', 'ljmp', 'xbegin')):
                if mnemonic == 'int3':
                    break
                address = following
                continue
            instruction = decoder.detail(address, size)
            operands = instruction.operands
            if instruction.mnemonic == 'call':
                if operands[0].type == X86_OP_IMM:
                    target = operands[0].imm & 0xffffffff
                    result.add(names.get(target, hex(target)))
                else:
                    result.add('indirect')
            elif instruction.group(CS_GRP_JUMP):
                if operands[0].type == X86_OP_IMM:
                    target = operands[0].imm & 0xffffffff
                    if start <= target < end:
                        pending.append(target)
                    else:
                        result.add(names.get(target, hex(target)))
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
    original_names, reconstructed_names = address_names(original_index), address_names(reconstructed_index)
    reconstructed_by_name = {}
    for record in reconstructed_index:
        reconstructed_by_name.setdefault(record['name'], record)
    for pe, records, names in ((original, original_index, original_names),
                               (reconstructed, reconstructed_index, reconstructed_names)):
        for record in records:
            if record['name'] in accepted and record['name'] not in driver_names:
                edges(pe, records, record, names)
    checked, failures = 0, 0
    for record in config['driver']:
        if record['name'] not in accepted:
            continue
        counterpart = reconstructed_by_name[record['name']]
        actual = edges(original, original_index, record, original_names)
        expected = edges(reconstructed, reconstructed_index, counterpart, reconstructed_names)
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
