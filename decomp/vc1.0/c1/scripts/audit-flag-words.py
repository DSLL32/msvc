#!/usr/bin/env python3
"""Check C1's two flag words independently of source state annotations.

A byte prefilter selects all functions mentioning either original or rebuilt
address. Instruction traversal follows direct branches and compiler DWORD
switches, excluding their table data. This is a target-specific data operand
check; shared Rust remains responsible for function graph and ABI discovery.

Symbolic bit vectors normalize byte/WORD/DWORD tests and updates into a union
of affected bits for each function. Register masks are derived from decoded
instructions. The union does not prove branch order or execution coverage.
Epilogue POPs are ignored while gathering other branch bodies' masks. Every
unresolved register mask fails. PCH's whole live-state fread/fwrite is checked
separately against raw operand bytes and the compiled portable schema layout.
"""
from pathlib import Path
import sys, json, collections
B = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(B / 'scripts'))
import argparse, hashlib, time, struct
started = time.perf_counter()
parser = argparse.ArgumentParser(description='Independent original/canonical flag-word operand and mask audit (Python capstone required).')
parser.add_argument('--report', type=Path, default=Path('build/flag-words.json'))
parser.add_argument('--rebuilt', type=Path)
options = parser.parse_args()
from images import indexes, publics
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_MEM, X86_OP_IMM, X86_OP_REG, X86_REG_EFLAGS

def instructions(md, pe, start, end):
    """Only reachable application instructions; switch DWORDs are not code."""
    from capstone.x86 import X86_OP_IMM, X86_OP_MEM
    import struct
    seen = {}
    pending = [start]
    while pending:
        address = pending.pop()
        while start <= address < end and address not in seen:
            ins = next(md.disasm(pe.read(address, min(15, end - address)), address, count=1), None)
            assert ins is not None, hex(address)
            seen[address] = ins
            successor = address + ins.size
            if ins.mnemonic.startswith('ret') or ins.mnemonic in ['int3', 'ud2']:
                break
            if ins.mnemonic.startswith('j') or ins.mnemonic.startswith('loop'):
                op = ins.operands[0]
                if op.type == X86_OP_IMM and start <= op.imm < end:
                    pending.append(op.imm)
                elif ins.mnemonic == 'jmp' and op.type == X86_OP_MEM and (not op.mem.base and op.mem.index and (op.mem.scale == 4) or (op.mem.base and (not op.mem.index) and op.mem.disp)):
                    table = op.mem.disp
                    for n in range(2048):
                        target = struct.unpack('<I', pe.read(table + 4 * n, 4))[0]
                        if not start <= target < end:
                            break
                        pending.append(target)
                if ins.mnemonic == 'jmp':
                    break
            address = successor
    return sorted(seen.values(), key=lambda ins: ins.address)
p, q, cfg, records, rebuilt_records = indexes()
if options.rebuilt:
    from binary import PE
    q = PE(options.rebuilt)
pub = publics()
md = Cs(CS_ARCH_X86, CS_MODE_32)
md.detail = True
bases = [{'scanner_flags': 441156, 'parser_flags': 441160}, {'scanner_flags': pub['_scanner_flags'], 'parser_flags': pub['_parser_flags']}]
code = sorted({a for n, a in pub.items() if any((s['flags'] & 32 and q.base + s['rva'] <= a < q.base + s['rva'] + s['virtual_size'] for s in q.sections))})
ends = dict(zip(code, code[1:]))
unknown = ['?'] * 32
aliases = {}
for b in ['a', 'b', 'c', 'd']:
    for n, off, w in [('e' + b + 'x', 0, 32), (b + 'x', 0, 16), (b + 'l', 0, 8), (b + 'h', 8, 8)]:
        aliases[n] = (b + 'x', off, w)
for b in ['si', 'di', 'bp', 'sp']:
    aliases[b] = (b, 0, 16)
    aliases['e' + b] = (b, 0, 32)

def origin(v):
    return {b[:2] for b in v if isinstance(b, tuple)}

def bitsnum(v):
    return sum((b << i for i, b in enumerate(v))) if all((isinstance(b, int) for b in v)) else None
results = {}
raw = []
issues = []
for r in cfg['driver']:
    if r['name'] not in cfg['accepted'] or r['name'] not in pub:
        continue
    if r['name'] not in ['_relocate_pch_state', '_restore_pch_state']:
        original_bytes = p.read(r['start'], r['end'] - r['start'])
        rebuilt_bytes = q.read(pub[r['name']], ends[pub[r['name']]] - pub[r['name']])
        if not any((struct.pack('<I', address + offset) in data for data, base in [(original_bytes, bases[0]), (rebuilt_bytes, bases[1])] for address in base.values() for offset in range(4))):
            continue
    sides = []
    for side, pe, base, start, end in [('original', p, bases[0], r['start'], r['end']), ('rebuilt', q, bases[1], pub[r['name']], ends[pub[r['name']]])]:
        ins = instructions(md, pe, start, end)
        regs = {}
        locals_ = {}
        actions = collections.defaultdict(int)
        flags = set()

        def target(op):
            if op.type != X86_OP_MEM or op.mem.base or op.mem.index:
                return None
            for word, address in base.items():
                if address <= op.mem.disp < address + 4:
                    return (word, op.mem.disp - address, op.size)

        def getreg(op):
            name = md.reg_name(op.reg)
            if name not in aliases:
                return ['?'] * (op.size * 8)
            key, off, width = aliases[name]
            return regs.get(key, unknown)[off:off + width]

        def putreg(op, v):
            name = md.reg_name(op.reg)
            if name not in aliases:
                return
            key, off, width = aliases[name]
            a = list(regs.get(key, unknown))
            a[off:off + width] = (v + unknown)[:width]
            regs[key] = a

        def get(op):
            if op.type == X86_OP_REG:
                return getreg(op)
            if op.type == X86_OP_IMM:
                return [op.imm >> i & 1 for i in range(op.size * 8)]
            t = target(op)
            if t:
                return [(t[0], 8 * t[1] + i, 0) for i in range(op.size * 8)]
            if op.type == X86_OP_MEM and md.reg_name(op.mem.base) in ['ebp', 'esp'] and (not op.mem.index):
                return locals_.get((md.reg_name(op.mem.base), op.mem.disp), ['?'] * (op.size * 8))
            return ['?'] * (op.size * 8)

        def use(v, kind='read'):
            for word, bit in origin(v):
                actions[word, kind] |= 1 << bit

        def put(op, v):
            if op.type == X86_OP_REG:
                putreg(op, v)
                return
            t = target(op)
            if t:
                for n, b in enumerate(v[:op.size * 8]):
                    bit = 8 * t[1] + n
                    if b == (t[0], bit, 0):
                        continue
                    kind = 'clear' if b == 0 else 'set' if b == 1 else 'toggle' if b == (t[0], bit, 1) else 'copy-write'
                    actions[t[0], kind] |= 1 << bit
            elif op.type == X86_OP_MEM:
                if md.reg_name(op.mem.base) in ['ebp', 'esp'] and (not op.mem.index):
                    locals_[md.reg_name(op.mem.base), op.mem.disp] = v
                else:
                    use(v, 'copy-read')

        def bitop(a, b, m):
            out = []
            for x, y in zip(a, b):
                if m in ['and', 'test']:
                    z = 0 if x == 0 or y == 0 else y if x == 1 else x if y == 1 else x if x == y else '?'
                elif m == 'or':
                    z = 1 if x == 1 or y == 1 else y if x == 0 else x if y == 0 else x if x == y else '?'
                elif x == y:
                    z = 0
                elif x == 0:
                    z = y
                elif y == 0:
                    z = x
                elif x == 1 and isinstance(y, tuple):
                    z = (y[0], y[1], 1 - y[2])
                elif y == 1 and isinstance(x, tuple):
                    z = (x[0], x[1], 1 - x[2])
                elif isinstance(x, int) and isinstance(y, int):
                    z = x ^ y
                else:
                    z = '?'
                out.append(z)
            return out
        for i in ins:
            ops = i.operands
            m = i.mnemonic
            for op in ops:
                if op.type == X86_OP_IMM and any(address <= op.imm < address+4 for address in base.values()):
                    issues.append((r['name'],side,hex(i.address),'flag address escape requires explicit audit',i.op_str))
                t = target(op)
                if t:
                    assert m in ['mov','movzx','movsx','and','or','xor','test','cmp','push'], ('unmodeled flag operation',side,hex(i.address),m)
                    raw.append(dict(function=r['name'], side=side, address=i.address, bytes=i.bytes.hex(), instruction=m + ' ' + i.op_str, word=t[0], offset=t[1], width=t[2]))
            if m in ['mov', 'movzx', 'movsx'] and len(ops) == 2:
                v = get(ops[1])
                width = ops[0].size * 8
                if len(v) < width:
                    v = v + ([v[-1]] if m == 'movsx' else [0]) * (width - len(v))
                put(ops[0], v[:width])
                continue
            if m in ['and', 'or', 'xor', 'test'] and len(ops) == 2:
                a = get(ops[0])
                b = get(ops[1])
                v = bitop(a, b, m)
                flags = origin(v)
                if m == 'test':
                    use(v)
                else:
                    put(ops[0], v)
                if any((target(op) for op in ops)) and ops[1].type == X86_OP_REG and (bitsnum(b) is None):
                    issues.append((r['name'], side, hex(i.address), 'unresolved register mask', i.op_str))
                continue
            if m in ['shr', 'shl', 'sar'] and len(ops) == 2:
                a = get(ops[0])
                n = bitsnum(get(ops[1]))
                n = n & 31 if n is not None else None
                if n is None:
                    v = ['?'] * len(a)
                elif not n:
                    v = a
                elif m == 'shl':
                    v = ([0] * n + a)[:len(a)]
                else:
                    v = (a[n:] + ([a[-1]] if m == 'sar' else [0]) * n)[:len(a)]
                put(ops[0], v)
                flags = origin(v)
                continue
            if m == 'cmp':
                flags = origin(get(ops[0])) | origin(get(ops[1]))
                use(get(ops[0]))
                use(get(ops[1]))
                continue
            if m.startswith('j') and m != 'jmp' or m.startswith('set') or m in ['sbb', 'adc']:
                for word, bit in flags:
                    actions[word, 'read'] |= 1 << bit
            if m == 'push' and ops:
                use(get(ops[0]), 'copy-read')
            if m == 'call':
                for key in ['ax', 'cx', 'dx']:
                    regs[key] = unknown
                flags = set()
                continue
            if m == 'pop':
                continue
            reads, writes = i.regs_access()
            for reg in writes:
                name = md.reg_name(reg)
                if name in aliases:
                    key, off, width = aliases[name]
                    v = list(regs.get(key, unknown))
                    v[off:off + width] = ['?'] * width
                    regs[key] = v
            if X86_REG_EFLAGS in writes:
                flags = set()
        sides.append({word + ':' + action: mask for (word, action), mask in sorted(actions.items()) if mask})
    if any(sides):
        results[r['name']] = {'original': sides[0], 'rebuilt': sides[1]}
# Every extra accepted text range and CRT/import extent is also screened.
# A future such reference must receive explicit operand analysis here.
nonapplication_ranges = 0
shared_ranges = 0
driver_names = {r['name'] for r in cfg['driver']}
for side, pe, base, entries in [('original', p, bases[0], records),
                               ('rebuilt', q, bases[1], rebuilt_records)]:
    for record in entries:
        if record['name'] not in cfg['accepted']:
            continue
        spans = list(record.get('shared', [])) if side == 'original' else []
        shared_ranges += len(spans)
        if record['name'] not in driver_names:
            spans.append((record['start'],record['end']))
            nonapplication_ranges += 1
        for left,right in spans:
            data = pe.read(left,right-left)
            candidates = [address+offset for address in base.values() for offset in range(4)
                          if struct.pack('<I',address+offset) in data]
            assert not candidates, ('extra flag address needs operand audit',side,record['name'],hex(left),candidates)
anchors = []
for address, byte_string, word, offset, width, mask in [(0x35db0, 'f60546bb060002', 'scanner_flags', 2, 1, 2), (0x35b61, 'f60546bb060008', 'scanner_flags', 2, 1, 8), (0x2fbcb, 'f60544bb060040', 'scanner_flags', 0, 1, 64), (0x1eee0, '66830d46bb060002', 'scanner_flags', 2, 2, 2), (0x19173, '66830d4abb060001', 'parser_flags', 2, 2, 1)]:
    expected = bytes.fromhex(byte_string)
    assert p.read(address, len(expected)) == expected, ('original identity bytes changed', hex(address))
    instruction = next(md.disasm(expected, address, count=1))
    memory = instruction.operands[0]
    assert memory.type == X86_OP_MEM and memory.size == width
    assert instruction.operands[1].imm == mask
    assert memory.mem.disp - offset == bases[0][word], ('raw word identity differs', word)
    anchors.append(dict(address=address, bytes=byte_string, word=word, offset=offset, width=width, mask=mask))
for address, byte_string in [(0x46c6f, '681c050000'), (0x46c76, '6830b60600'), (0x45c0a, '681c050000'), (0x45c11, '6830b60600')]:
    expected = bytes.fromhex(byte_string)
    assert p.read(address, len(expected)) == expected, ('PCH block operand changed', hex(address))
    assert (bases[0]['scanner_flags'] - 0x6b630, bases[0]['parser_flags'] - 0x6b630) == (1300, 1304)
exceptions = {'_relocate_pch_state': dict(original={}, rebuilt={'parser_flags:copy-read': 0xffffffff, 'scanner_flags:copy-read': 0xffffffff}), '_restore_pch_state': dict(original={}, rebuilt={'parser_flags:copy-write': 0xffffffff, 'scanner_flags:copy-write': 0xffffffff})}
mismatches = {n: row for n, row in results.items() if row['original'] != row['rebuilt'] and row != exceptions.get(n)}
report = dict(status='fail' if mismatches or issues else 'pass', addresses=bases, original_sha256=hashlib.sha256(p.data).hexdigest(), rebuilt_sha256=hashlib.sha256(q.data).hexdigest(), accepted_sha256=hashlib.sha256(json.dumps(cfg['accepted']).encode()).hexdigest(), source_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(), original_identity_anchors=anchors,extra_ranges_screened=dict(shared=shared_ranges,CRT_and_import=nonapplication_ranges,flag_addresses=0), functions=results, raw=raw, issues=issues, mismatches=mismatches, pch_indirect_serialization=dict(original_base=0x6b630, bytes=1308, scanner_offset=1300, parser_offset=1304, evidence_addresses=[0x46c6f, 0x46c76, 0x45c0a, 0x45c11], layout_assertions='src/pch-format.h', explicit_named_copies=exceptions), scope='All accepted application instructions accessing either word; per-function union of tested/cleared/set/toggled/copied bits. This does not establish branch coverage or execution order.', elapsed_seconds=time.perf_counter() - started)
options.report.parent.mkdir(parents=True, exist_ok=True)
options.report.write_text(json.dumps(report, indent=2) + '\n')
assert not issues, ('unresolved register masks', issues)
assert not mismatches, ('flag-word or mask identity differs', mismatches)
print('%d functions, %d original and %d canonical operand accesses: flag words/masks match.' % (len(results), sum((r['side'] == 'original' for r in raw)), sum((r['side'] == 'rebuilt' for r in raw))))
