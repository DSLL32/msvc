#!/usr/bin/env python3
"""Check source function calls, callbacks, and RET stack cleanup against RC."""
import json
import hashlib
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
            # Decode beyond the declared end: a clipped decoder can hide a
            # truncated RET immediate or another final instruction operand.
            instructions = list(decoder.disasm(pe.read(address, 15), address, 1))
            if not instructions:
                raise AssertionError('invalid instruction at %08x' % address)
            insn = instructions[0]
            after = address + insn.size
            assert after <= end, (function['name'], 'extent truncates instruction',
                                  hex(address), insn.mnemonic, hex(end), hex(after))
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


def audit_crt_tables(original, reconstruction, oi, ri, accepted):
    """Compare callable initializer/termination entries, including their order.

    Linker section alignment can add different numbers of null slots. The
    callable sequence must match and every non-null entry must be accepted.
    Original boundaries come from the recovered cinit/doexit call arguments.
    """
    oa, ra = aliases(oi), aliases(ri)
    names = compare.publics()
    report = {}
    for label, begin, end in [('xc',0x101f000,0x101f010), ('xi',0x101f020,0x101f030),
                              ('xp',0x101f040,0x101f060), ('xt',0x101f070,0x101f080)]:
        left = [u32(original.read(a,4),0) for a in range(begin,end,4)]
        first, last = names['___'+label+'_a'], names['___'+label+'_z']
        right = [u32(reconstruction.read(a,4),0) for a in range(first,last,4)]
        left, right = [a for a in left if a], [a for a in right if a]
        assert len(left) == len(right), (label, 'CRT callback count differs')
        entries = []
        for a, b in zip(left, right):
            shared = oa[a] & ra[b] & set(accepted)
            assert shared, (label, 'unaccepted or different CRT callback', hex(a), hex(b))
            entries.append(dict(names=sorted(shared), original=hex(a), reconstruction=hex(b)))
        report[label] = dict(original_range=[hex(begin),hex(end)],
                             reconstructed_range=[hex(first),hex(last)], callbacks=entries)
    Path('build/runtime-tables.json').write_text(json.dumps(report,indent=2)+'\n')


def audit_runtime_aliases(original, reconstruction, oi, ri, cfg):
    """Verify encountered CRT aliases, callback slots and native object layout."""
    names = compare.publics()
    oa, ra = aliases(oi), aliases(ri)
    report = {}
    original_iob, reconstructed_iob = 0x101d3b0, names['__iob']
    left_last = u32(original.read(0x101d630,4),0) - original_iob
    right_last = u32(reconstruction.read(names['__lastiob'],4),0) - reconstructed_iob
    assert left_last == right_last == 19 * 32
    for index in range(20):
        for offset in (12,16):
            assert original.read(original_iob+index*32+offset,4) == reconstruction.read(reconstructed_iob+index*32+offset,4)
    report['standard_streams'] = dict(count=20, sizeof_FILE=32, flag_offset=12,
                                      descriptor_offset=16, last_stream_offset=left_last,
                                      diagnostics_stream_index=1)
    left_table = u32(original.read(0x101d710,4),0)
    right_table = u32(reconstruction.read(names['__pctype'],4),0)
    left, right = original.read(left_table-2,514), reconstruction.read(right_table-2,514)
    assert left == right
    assert original.read(0x101d3a0,4) == reconstruction.read(names['___mb_cur_max'],4)
    left_decimal = original.string(u32(original.read(0x101d3a8,4),0))
    right_decimal = reconstruction.string(u32(reconstruction.read(names['__decimal_point'],4),0))
    assert left_decimal == right_decimal == '.'
    report['C_locale'] = dict(character_classes_including_EOF=257,
                              ctype_sha256=hashlib.sha256(left).hexdigest(),
                              multibyte_max=u32(original.read(0x101d3a0,4),0), decimal_point=left_decimal)
    slots = []
    for index in range(6):
        left = u32(original.read(0x101dbf0+4*index,4),0)
        right = u32(reconstruction.read(names['__cfltcvt_tab']+4*index,4),0)
        shared = oa[left] & ra[right]
        assert shared, ('different floating callback',index,hex(left),hex(right))
        slots.append(dict(slot=index, names=sorted(shared),
                          accepted=bool(shared & set(cfg['accepted']))))
    report['floating_conversion_slots'] = slots
    if '_is_kanji_lead@4' in cfg['accepted']:
        functions = [next(r for r in records if r['name']=='_is_kanji_lead@4') for records in (oi,ri)]
        for pe, record, base in [(original,functions[0],0x1018558),
                                  (reconstruction,functions[1],names['_kanji_ranges'])]:
            fields = set()
            for insn in decoder.disasm(pe.read(record['start'],record['end']-record['start']),record['start']):
                for operand in insn.operands:
                    if operand.type == X86_OP_MEM and not operand.mem.base and not operand.mem.index:
                        offset = operand.mem.disp - base
                        assert offset in (0,4,8,12) and operand.size == 2 and insn.mnemonic == 'movsx'
                        fields.add(offset)
            assert fields == {0,4,8,12}
            assert pe.read(base,16) == bytes(16)
        report['KANJI_bounds'] = dict(bytes=16, element_bytes=2, signed=True,
                                      field_offsets=[0,4,8,12], range_stride=8)
    Path('build/runtime-aliases.json').write_text(json.dumps(report,indent=2)+'\n')


def main():
    original, reconstruction, cfg, oi, ri = compare.indexes()
    oa, ra = aliases(oi), aliases(ri)
    # CRT/import ranges also enter the mask and need complete instructions.
    boundaries = {}
    for label, pe, records in (('original',original,oi), ('reconstruction',reconstruction,ri)):
        checked = set()
        for record in records:
            if record['name'] in cfg['accepted']:
                describe(pe, record)
                checked.add((record['name'],record['start'],record['end']))
        boundaries[label] = dict(checked_extents=len(checked),
            checked_names=len({r[0] for r in checked}), complete_instruction_ends=True)
    Path('build/instruction-extents.json').write_text(json.dumps(boundaries,indent=2)+'\n')
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
    if '_doexit' in cfg['accepted'] and '__endstdio' in cfg['accepted']:
        audit_crt_tables(original, reconstruction, oi, ri, cfg['accepted'])
    audit_runtime_aliases(original, reconstruction, oi, ri, cfg)
    print('%d accepted source functions: direct calls, function references and return cleanup agree.' % len(report))


if __name__ == '__main__':
    main()
