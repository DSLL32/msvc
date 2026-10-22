#!/usr/bin/env python3
"""Check source function calls, callbacks, and RET stack cleanup against RC."""
import json
from pathlib import Path
from collections import defaultdict
from capstone import Cs, CS_ARCH_X86, CS_MODE_32, CS_GRP_JUMP
from capstone.x86 import X86_OP_IMM, X86_OP_MEM
import compare
from binary import u32

decoder = Cs(CS_ARCH_X86, CS_MODE_32)
decoder.detail = True


def describe(pe, function):
    start, end = function['start'], function['end']
    queue = [start]
    seen, calls, references, returns, tail_calls = set(), set(), set(), set(), set()
    indirect = False
    while queue:
        address = queue.pop()
        while start <= address < end and address not in seen:
            seen.add(address)
            instructions = list(decoder.disasm(pe.read(address, min(16, end - address)), address, 1))
            if not instructions:
                raise AssertionError('invalid instruction at %08x' % address)
            insn = instructions[0]
            after = address + insn.size
            for operand in insn.operands:
                if operand.type != X86_OP_IMM:
                    continue
                immediate = operand.imm & 0xffffffff
                if insn.mnemonic not in ('call', 'jmp') and not insn.group(CS_GRP_JUMP):
                    references.add(immediate)
                    # __except_handler2 scope records provide additional CFG
                    # entries for filters and handlers reached by the OS.
                    try:
                        scope = pe.read(immediate, 12)
                        if len(scope) == 12 and u32(scope, 0) == 0xffffffff:
                            for offset in (4, 8):
                                target = u32(scope, offset)
                                if start <= target < end:
                                    queue.append(target)
                    except ValueError:
                        pass
            if insn.mnemonic == 'call':
                if insn.operands[0].type == X86_OP_IMM:
                    calls.add(insn.operands[0].imm & 0xffffffff)
                else:
                    indirect = True
            if insn.mnemonic.startswith('ret'):
                returns.add(insn.operands[0].imm if insn.operands else 0)
                break
            if insn.group(CS_GRP_JUMP):
                operand = insn.operands[0]
                if operand.type == X86_OP_IMM:
                    target = operand.imm & 0xffffffff
                    if start <= target < end:
                        queue.append(target)
                    elif insn.mnemonic == 'jmp':
                        calls.add(target)
                        tail_calls.add(target)
                elif operand.type == X86_OP_MEM and operand.mem.disp and (operand.mem.base or operand.mem.index):
                    table = operand.mem.disp & 0xffffffff
                    for index in range(512):
                        try:
                            target = u32(pe.read(table + index * 4, 4), 0)
                        except (ValueError, IndexError):
                            break
                        if not start <= target < end:
                            break
                        queue.append(target)
                if insn.mnemonic == 'jmp':
                    break
            if insn.mnemonic == 'int3':
                break
            address = after
    return dict(calls=calls, references=references - calls, returns=returns,
                tail_calls=tail_calls, indirect=indirect, instructions=len(seen))


def return_cleanup(pe, function, index, visiting=None):
    """An optimized tail jump inherits its callee's RET convention.

    Follow only identified whole functions. This keeps a C call/return wrapper
    comparable with the original tail-call wrapper without accepting an
    unknown stack convention or introducing reconstruction assembly.
    """
    visiting = set() if visiting is None else visiting
    assert function['start'] not in visiting, ('tail-call cycle', function['name'])
    visiting = visiting | {function['start']}
    description = describe(pe, function)
    result = set(description['returns'])
    for address in description['tail_calls']:
        candidates = [r for r in index if r['start'] == address and not r.get('pending_extent')]
        assert candidates, ('unidentified tail-call ABI', function['name'], hex(address))
        inherited = return_cleanup(pe, candidates[-1], index, visiting)
        assert inherited, ('tail callee has no determined RET convention', candidates[-1]['name'])
        result.update(inherited)
    return result


def aliases(index):
    result = defaultdict(set)
    for record in index:
        result[record['start']].add(record['name'])
    return result


def equivalent(left, right, oi, ri):
    """Require a bijection, so a duplicate CRT alias cannot hide a new vertex."""
    if len(left) != len(right):
        return False
    candidates = {r: {l for l in left if oi[l] & ri[r]} for r in right}
    paired = {}

    def match(right_address, visited):
        for left_address in candidates[right_address]:
            if left_address in visited:
                continue
            visited.add(left_address)
            if left_address not in paired or match(paired[left_address], visited):
                paired[left_address] = right_address
                return True
        return False

    return all(match(address, set()) for address in sorted(right, key=lambda a: len(candidates[a])))


def main():
    original, reconstruction, cfg, oi, ri = compare.indexes()
    oa, ra = aliases(oi), aliases(ri)
    report = {}
    for function in cfg['driver']:
        name = function['name']
        if name not in cfg['accepted']:
            continue
        other = next(r for r in ri if r['name'] == name)
        left, right = describe(original, function), describe(reconstruction, other)
        lc, rc = left['calls'], right['calls']
        assert equivalent(lc, rc, oa, ra), (name, 'call graph differs',
            {hex(a): sorted(oa[a]) for a in lc}, {hex(a): sorted(ra[a]) for a in rc})
        lrefs = {a for a in left['references'] if a in oa and a not in lc}
        rrefs = {a for a in right['references'] if a in ra and a not in rc}
        assert equivalent(lrefs, rrefs, oa, ra), (name, 'function references differ', lrefs, rrefs)
        left_returns = return_cleanup(original, function, oi)
        right_returns = return_cleanup(reconstruction, other, ri)
        assert left_returns == right_returns, (name, 'return stack cleanup differs', left_returns, right_returns)
        assert left['indirect'] == right['indirect'], (name, 'indirect call topology differs')
        report[name] = dict(original_calls={hex(a): sorted(oa[a]) for a in lc},
                            reconstructed_calls={hex(a): sorted(ra[a]) for a in rc},
                            stack_cleanup=sorted(left_returns),
                            indirect_calls=left['indirect'])
    Path('build/graph-audit.json').write_text(json.dumps(report, indent=2) + '\n')
    print('%d accepted source functions: direct calls, function references and return cleanup agree.' % len(report))


if __name__ == '__main__':
    main()
