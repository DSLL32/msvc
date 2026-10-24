"""Own complete numeric leaf, consumed public predicates and subsequent suffix ABI."""
import copy
import hashlib
import json
import re
import struct
from pathlib import Path

from allocation_imports import pe_read, export_rva
from scaler_probe import symbols
from scaler_state import fp, semantic
from option_action_state import check_signature

KINDS = ('isspace', 'isdigit', 'atol')
PROVIDERS = (0x1c0be, 0x1c065, 0x5d30)
RETURNS = (0x39793, 0x397a9, 0x397bb)


def check(s, d, original, native, trial, text, state, prefix):
    def need(ok, message):
        s.require(ok, 'option integer: ' + message)

    def load(name):
        return json.loads((native / (name + '.json')).read_text())

    def reg(stem, count=16):
        return struct.unpack('<%uI' % count, (original / (stem + '-registers.bin')).read_bytes())

    def raw(phase, suffix):
        return (original / ('locale-' + phase + '-' + suffix + '.bin')).read_bytes()

    def argv(path):
        data = path.read_bytes()
        root, count = struct.unpack_from('<II', data)
        at, values = 8, []
        for _ in range(count):
            pointer, length = struct.unpack_from('<II', data, at)
            at += 8
            values.append((pointer, data[at:at + length]))
            at += length
        need(count == 26 and at == len(data), 'full independent Invoke26 ownership')
        return root, values

    append_mode=trial['trial']=='_append_option_vector' and bool(trial.get('append_observers'))
    continuation = trial['trial'] in ('_match_option_suffix','_append_option_vector') and bool(trial.get('suffix_observers'))
    need(trial['trial'] in ('_parse_option_integer','_match_option_suffix','_append_option_vector') and trial['integer_observers'] == ({
        'entry_rva': 0x39779, 'return_rva': 0x39694, 'calls': 1,
        'argument': 'borrowed Invoke[6] 1 NUL',
        'provider_names': list(KINDS), 'provider_rvas': list(PROVIDERS),
        'IAT_rvas': [0xc9154, 0xc9090, 0xc9000], 'API_return_rvas': list(RETURNS),
        'frontier_rva': 0x39757,
        'ctype_consumption': 'zero versus nonzero only; raw provider masks retained separately;SBCS1 input49'
    } if not append_mode else dict(entry_rva=0x39779,return_rva=0x39694,calls=5,argument='five actual borrowed numeric option spans; first Invoke[6] 1 NUL',inputs=['1','8','64','5','0'],provider_names=list(KINDS),provider_rvas=list(PROVIDERS),IAT_rvas=[0xc9154,0xc9090,0xc9000],API_return_rvas=list(RETURNS),frontier_rva=0x397db,ctype_consumption='zero versus nonzero only; raw provider masks retained separately;SBCS1 inputs49/56/54/53/48')), 'exact own observer/input/consumption contract')
    need([r for r in trial['ranges'] if r['owner'] == '_parse_option_integer'] == [
        {'owner': '_parse_option_integer', 'start_rva': 0x39779, 'end_rva': 0x397db, 'entry': 1},
        {'owner': '_parse_option_integer', 'start_rva': 0x72671, 'end_rva': 0x72696, 'entry': 0}
    ], 'whole135B hot+cold logical function')
    seed = bool(trial.get('test_seed'))
    names = symbols(native / 'native-executable.elf')
    entry_state = load('integer-entry-state')
    original_root, oldargs = argv(original / 'invoke-entry-argv.bin')
    root, args = argv(native / 'integer-entry-argv.bin')
    need([v for _, v in oldargs] == [v for _, v in args] and
         [v for _, v in args[5:8]] == [b'-W\0', b'1\0', b'-Ze\0'],
         'complete own ordered actual argument values')
    entries, ends = load('integer-entries'), load('integer-returns')
    api_entries, api_ends = load('integer-api-entries'), load('integer-api-returns')
    suffixes = load('integer-suffix-entries')
    need([len(r) for r in (entries, ends, api_entries, api_ends, suffixes)] == ([5,5,15,15,3] if append_mode else [1,1,3,3,1]),
         'complete one numeric/three public/one pending suffix inventory')
    entry, end, suffix = entries[0], ends[0], suffixes[0]
    a, b = reg('integer-0-entry'), reg('integer-0-return')
    need(a[1] == 0x10439779 and a[10:12] == (0x10439694, oldargs[6][0]) and
         a[14:] == (oldargs[6][0], 0) and b[14:] == a[14:] and
         entry['pc'] == names['parse_option_integer'][0] and entry['args'] == [args[6][0]] and
         entry['formal_inventory'] == [{'name': 'value', 'type': 'const char *'}] and
         entry['caller'] == 'apply_option_action' and entry['value'] == end['value'] == b'1\0'.hex(),
         'actual numeric true-entry formal/caller/borrowed string')
    for phase in ('entry', 'return'):
        need((original / ('integer-0-' + phase + '-input.bin')).read_bytes() == b'1\0',
             'whole numeric string lifetime')
    need(entry_state['pc'] == entry['pc'] and entry_state['esp'] == entry['esp'] and
         entry_state['args'] == {'value': args[6][0]}, 'true pre-body typed entry observation')

    def call(pc, target):
        code = bytes(d.elf_byte(native, pc - 5 + i) for i in range(5))
        need(code[0] == 0xe8 and pc + struct.unpack_from('<i', code, 1)[0] == target,
             'actual native direct callsite/target')

    def returned(before, after, old_before, old_after):
        need(after['pc'] == before['return_pc'] and after['entry_esp'] == before['esp'] and
             after['esp'] == before['esp'] + 4 and
             all(after[k] == before[k] for k in ('ebx', 'esi', 'edi', 'ebp')) and
             old_after[1] == old_before[10] and old_after[2] == old_before[2] + 4 and
             old_after[6:10] == old_before[6:10], 'actual cdecl RET0/ESP/nonvolatile preservation')

    call(entry['return_pc'], entry['pc'])
    returned(entry, end, a, b)
    need(b[3] == end['eax'] == 1, 'exact signed numeric return1')
    expected = copy.deepcopy(entry['guard'])
    old_bss = bytearray(raw('integer-0-entry', 'target-BSS'))
    old_data = bytearray((original / 'frontend-data-locale-integer-0-entry.bin').read_bytes())
    old_saved = (original / 'frontend-snapshot-locale-integer-0-entry.bin').read_bytes()
    front = entry_state['frontend']
    fields = {r['name']: r for r in load('state-model')['fields']}
    locale = {k: v for k, v in entry['locale'].items() if k != 'maps'}
    floating = semantic(entry['fp'])
    chronology = []

    def phase(name, row, record):
        chronology.append((record[0], name))
        need(row['guard'] == expected and raw(name, 'target-BSS') == bytes(old_bss) and
             (original / ('frontend-data-locale-' + name + '.bin')).read_bytes() == bytes(old_data) and
             (original / ('frontend-snapshot-locale-' + name + '.bin')).read_bytes() == old_saved,
             'full known/padding/saved/native and original unknown-neighbor guard ' + name)
        need(row['last_error'] == 203 and semantic(row['fp']) == floating and
             {k: v for k, v in row['locale'].items() if k != 'maps'} == locale,
             'native full locale/owner/selection/vector/errors/architectural FP ' + name)
        for part in ('words', 'mbctype', 'ctype', 'old-ctype', 'result', 'errors', 'fp',
                     'CRT-argv', 'CRT-environment', 'Invoke-argv'):
            need(raw(name, part) == raw('integer-0-entry', part),
                 'original full namespace/provider/error/112B FP lifetime ' + name + ':' + part)
        for kind, baseline in (('CRT-argv', 'native-crt-argv.bin'),
                               ('CRT-environment', 'native-crt-environment.bin')):
            native_name = re.sub(r'^actions-(.*)-return-(\d+)$', r'actions-\1-\2-return', name)
            need((native / ('locale-native-' + native_name + '-' + kind + '.bin')).read_bytes() ==
                 (native / baseline).read_bytes(), 'native complete private vector lifetime ' + name)

    phase('integer-0-entry', entry, a)
    providers = load('integer-native-providers')
    need(set(providers) == set(KINDS), 'actual Linux provider inventory')
    dependency = json.loads((Path(__file__).resolve().parents[1] / 'cases/minimal/integer-native-libc.json').read_text())
    linux_bytes = Path(dependency['path']).read_bytes()
    need(hashlib.sha256(linux_bytes).hexdigest() == dependency['sha256'], 'retained Linux dependency content identity')
    predicates = []
    for i, kind in enumerate(KINDS):
        stem = 'integer-0-api-' + kind
        x, y = reg(stem + '-entry', 17), reg(stem + '-return', 17)
        u, v = api_entries[i], api_ends[i]
        oldarg = oldargs[6][0] if i == 2 else 49
        argument = args[6][0] if i == 2 else 49
        need(x[1] == 0x78000000 + PROVIDERS[i] and x[10:17] ==
             (0x10400000 + RETURNS[i], oldarg, i, 0, oldargs[6][0], 0x78000000, oldarg) and
             y[10:17] == x[10:17] and u['kind'] == v['kind'] == kind and
             u['pc'] == providers[kind] and u['args'] == [argument] and
             u['formal_inventory'] is None and u['caller'] == 'parse_option_integer' and
             u['formal_observation'] == 'one actual raw cdecl argument; external libc has no target DWARF formal claim' and
             u['provider'].endswith('/libc.so.6') and len(bytes.fromhex(u['provider_entry_bytes'])) == 16,
             'real original canonical entry and native external raw cdecl argument/identity ' + kind)
        # Native Linux public import is an actual PLT call, not a target shim.
        site = bytes(d.elf_byte(native, u['return_pc'] - 5 + j) for j in range(5))
        need(site[0] == 0xe8, 'native public direct PLT call ' + kind)
        plt = u['return_pc'] + struct.unpack_from('<i', site, 1)[0]
        need(bytes(d.elf_byte(native, plt + j) for j in range(2)) == b'\xff\x25',
             'actual native public PLT linkage ' + kind)
        maps = u['locale']['maps'].splitlines()
        need(any(int(line.split()[0].split('-')[0], 16) <= u['pc'] <
                 int(line.split()[0].split('-')[1], 16) and 'r-xp' in line and
                 line.endswith('/libc.so.6') for line in maps), 'actual mapped executable Linux libc entry')
        mappings = [line.split() for line in maps if len(line.split()) >= 6 and line.endswith('/libc.so.6')]
        bases = {int(parts[0].split('-')[0], 16) - int(parts[2], 16) for parts in mappings if int(parts[2], 16) == 0}
        need(len(bases) == 1 and u['pc'] == next(iter(bases)) + dependency['symbols'][kind], 'actual named native libc symbol/address identity ' + kind)
        matched_maps = [parts for parts in mappings if int(parts[0].split('-')[0], 16) <= u['pc'] < int(parts[0].split('-')[1], 16)]
        need(len(matched_maps) == 1, 'unique mapped native public provider entry')
        segment = matched_maps[0]
        file_offset = int(segment[2], 16) + u['pc'] - int(segment[0].split('-')[0], 16)
        need(bytes.fromhex(u['provider_entry_bytes']) == linux_bytes[file_offset:file_offset + 16], 'observed native entry bytes match retained named provider ' + kind)
        returned(u, v, x, y)
        need(v['argument_after'] == argument and u['locale']['mb_cur_max'] == 1,
             'actual untouched public slot and finite SBCS1 prestate')
        if i == 2:
            need(u['input'] == v['input'] == b'1\0'.hex() and
                 y[3] == v['eax'] == 1 and all((original / (stem + '-' + p + '-input.bin')).read_bytes() ==
                     b'1\0' for p in ('entry', 'return')), 'actual exact atol signed32 conversion/input')
        else:
            table = raw(stem + '-entry', 'ctype')
            need(len(table) == 514 and y[3] == (struct.unpack_from('<H', table, 100)[0] & (8 if i == 0 else 4)) and
                 bool(v['eax']) == bool(y[3]) == (i == 1),
                 'original raw table mask and native consumed boolean predicate')
        predicates.append({'kind': kind, 'argument_role': 'borrowed Invoke[6]' if i == 2 else 49,
                           'original_raw': y[3], 'native_raw': v['eax'],
                           'comparison': 'exact signed32' if i == 2 else 'zero/nonzero only', 'code_credit': 0})
        phase(stem + '-entry', u, x)
        phase(stem + '-return', v, y)
        need(export_rva(original / 'loaded-msvcrt.bin', kind) == PROVIDERS[i] and
             (original / ('integer-provider-' + kind + '.bin')).read_bytes() ==
             pe_read(original / 'loaded-msvcrt.bin', PROVIDERS[i], 16 if i == 2 else 42),
             'actual loaded canonical public body/export identity ' + kind)
    phase('integer-0-return', end, b)
    need([r[0] for r in chronology] == list(range(a[0], b[0] + 1)),
         'one real numeric call with exactly ordered public entry/return observations')
    text_bytes = (original / 'relocated-text.bin').read_bytes()
    for ret, branch in ((0x39793, 0x85), (0x397a9, 0x84)):
        code = text_bytes[ret - 0x1000:ret - 0x1000 + 11]
        need(code[:7] == bytes([0x83, 0xc4, 4, 0x85, 0xc0, 0x0f, branch]),
             'only consumed TEST EAX/conditional branch for public ctype result')

    def write(name, old_value, value):
        f = fields[name]
        kind = 'data' if f['class'] == 'initialized' else 'bss'
        base = front['storage']['first_invocation'][0] if kind == 'data' else front['bss_address']
        buf = bytearray.fromhex(expected[kind])
        struct.pack_into('<I', buf, front['storage'][name][0] - base, value)
        expected[kind] = buf.hex()
        old = old_data if kind == 'data' else old_bss
        struct.pack_into('<I', old, f['original_address'] - (0x104cc000 if kind == 'data' else 0x104b9000), old_value)

    actions, action_ends = load('actions-entries'), load('actions-returns')
    matches, match_ends = load('actions-matches'), load('actions-match-returns')
    outer, outer_ends = load('actions-outer-entries'), load('actions-outer-returns')
    calls, results = load('actions-signatures'), load('actions-signature-returns')
    need([len(r) for r in (actions, action_ends, matches, match_ends, outer, outer_ends, calls, results)] ==
         ([20,20,21,21,21,21,23 if seed else 24,23 if seed else 24] if append_mode else
          [5, 4, 5, 4, 5, 5, 5 if seed else 6, 5 if seed else 6] if continuation else
          [3, 3, 4, 3, 4, 4, 4 if seed else 5, 4 if seed else 5]),
         'complete subsequent loop/API observations, with no extra hidden calls')
    write('options.warning_level', 1, 1)
    ar = reg('actions-action-return-2')
    phase('actions-action-return-2', action_ends[2], ar)
    returned(actions[2], action_ends[2], reg('actions-action-entry-2'), ar)
    start = 2 if seed else 3
    for offset, slot, ret in ((0, 6, 0x395af), (1, 5, 0x39565)):
        index = start + offset
        check_signature(index, slot, ret, expected, old_bss, old_data, old_saved,
                        calls, results, original, native, names, need, locale, floating,
                        raw, 'integer-0-entry', d, args, oldargs)
        chronology.extend([(reg('actions-signature-%d-entry' % index, 17)[0], 'signature-entry'),
                           (reg('actions-signature-%d-return' % index, 17)[0], 'signature-return')])
        write('source_signature', results[index]['eax'], results[index]['eax'])
    mr = reg('actions-match-return-2')
    phase('actions-match-return-2', match_ends[2], mr)
    returned(matches[2], match_ends[2], reg('actions-match-entry-2'), mr)
    need(match_ends[2]['eax'] == mr[3] == 1, 'actual third matcher signed return1')
    oe, od, me = reg('actions-outer-entry-3'), reg('actions-outer-return-3'), reg('actions-match-entry-3')
    phase('actions-outer-entry-3', outer[3], oe)
    need(oe[1] == 0x10439428 and oe[10] == 0x104390d1 and
         outer[3]['pc'] == names['next_argument'][0] and outer[3]['args'] == [] and
         outer[3]['formal_inventory'] == [] and outer[3]['caller'] == 'process_compiler_options',
         'actual fourth zero-formal callback')
    write('command_argc', 19, 19)
    write('command_argv', original_root + 28, root + 28)
    phase('actions-outer-return-3', outer_ends[3], od)
    returned(outer[3], outer_ends[3], oe, od)
    need(od[3] == oldargs[7][0] and outer_ends[3]['eax'] == args[7][0] and
         outer_ends[3]['value'] == args[7][1].hex() and
         (original / 'actions-outer-3-value.bin').read_bytes() == args[7][1],
         'fourth actual borrowed -Ze full string')
    phase('actions-match-entry-3', matches[3], me)
    rules = load('options-grammar-layout')['option_rules']['address']
    need(me[1] == 0x1043944a and me[10:15] == (0x104390dc, 0x104d6160, oldargs[7][0], 0x10439428, 0) and
         matches[3]['pc'] == names['match_option'][0] and
         matches[3]['args'] == [rules, args[7][0], names['next_argument'][0], 0] and
         matches[3]['formal_inventory'] == matches[2]['formal_inventory'] and
         matches[3]['argument'] == args[7][1].hex(), 'fourth matcher actual four ABI inputs')
    write('current_argument', oldargs[7][0], args[7][0])
    tail = re.findall(r'^trap seq=(\d+) rva=00039757 esp=(\w+).*stack=([0-9a-f,]+)$', text, re.M)
    need(len(tail) == (3 if append_mode else 1) and re.findall(r'^frontier rva=(\w+)$', text, re.M) == (['000397db'] if append_mode else ['000100ea'] if continuation else ['00039757']),
         'actual final pending suffix trap')
    sequence = int(tail[0][0]);stack = [int(v, 16) for v in tail[0][2].split(',')]
    rule = pe_read(original / 'loaded-target.bin', 0xd6160 + 29 * 12, 12)
    pattern = struct.unpack_from('<I', rule)[0]
    need(rule[9] == 35 and stack[:4] == [0x104394cf, pattern + 2, oldargs[7][0] + 2, 35] and
         suffix['pc'] == names['match_option_suffix'][0] and suffix['args'] ==
         [state['args']['pattern'], args[7][0] + 2, 35] and
         suffix['formal_inventory'] == [{'name': k, 'type': t} for k, t in
             [('pattern', 'const char *'), ('argument', 'char *'), ('action', 'unsigned int')]] and
         suffix['caller'] == 'match_option' and suffix['pattern'] == b'*\0'.hex() and
         suffix['argument'] == b'e\0'.hex(), 'genuine suffix pattern/borrowed interior/action35 formal inputs')
    # The native grammar points at the same actual named rule29 literal owner.
    elf = (native / 'native-executable.elf').read_bytes()
    phoff = struct.unpack_from('<I', elf, 28)[0]
    phsize, phcount = struct.unpack_from('<HH', elf, 42)
    segments = [struct.unpack_from('<8I', elf, phoff + i * phsize) for i in range(phcount)]
    address = rules + 29 * 12
    owners = [r for r in segments if r[0] == 1 and r[2] <= address and address + 4 <= r[2] + r[4]]
    need(len(owners) == 1, 'native rule29 file-backed storage ownership')
    owner = owners[0]
    native_pattern = struct.unpack_from('<I', elf, owner[1] + address - owner[2])[0]
    need(suffix['args'][0] == native_pattern + 2, 'native literal rule29+2 identity')
    call(suffix['return_pc'], suffix['pc'])
    need(state['symbol'] == 'match_option_suffix' and state['args'] ==
         dict(zip(('pattern', 'argument', 'action'), suffix['args'])),
         'actual suffix three source formals')
    if continuation:
        need(state['pc'] == suffix['pc'] and state['esp'] == suffix['esp'],
             'real first suffix pre-body checkpoint, no synthetic INT3')
    else:
        need(names['match_option_suffix'][0] < state['pc'] <= sum(names['match_option_suffix']) and
             d.elf_byte(native, state['pc'] - 1) == 0xcc and
             b'Program received signal SIGTRAP' in (native / 'gdb.txt').read_bytes(),
             'real native INT3 suffix boundary')
    need(suffix['guard'] == expected and semantic(suffix['fp']) == floating and
         {k: v for k, v in suffix['locale'].items() if k != 'maps'} == locale and suffix['last_error'] == 203,
         'complete final native state/owners/errors/FP')
    need((original / ('pool-bss-%03d.bin' % (sequence + 1))).read_bytes() == bytes(old_bss) and
         (original / ('frontend-data-%03d.bin' % sequence)).read_bytes() == bytes(old_data) and
         (original / ('frontend-snapshot-%03d.bin' % sequence)).read_bytes() == old_saved and
         semantic(fp((original / ('x87-context-%03d.bin' % sequence)).read_bytes())) == floating,
         'direct final original entire state/unknown neighbors/architectural FP')
    final_front = copy.deepcopy(front)
    final_front.update(command_argc=19, command_argv=root + 28,
                       current_argument=args[7][0], source_signature=results[(3 if seed else 4)]['eax'])
    final_front['options_data']['warning_level'] = 1
    need(state['frontend'] == final_front and state['invocation'] == entry_state['invocation'] and
         state['target_state'] == entry_state['target_state'],
         'whole typed final state, borrowed owner lifetime and unchanged lifecycle')
    need(all(x[0] < y[0] for x, y in zip(chronology, chronology[1:])) and
         chronology[-1][0] < sequence,
         'actual numeric/loop/dependency chronology without copied synthetic events')
    return dict(frontier='_match_option_suffix', candidate='_parse_option_integer', code_bytes=135,
                numeric_input='1', numeric_result=1, public_calls=predicates,
                final_suffix_formals=['pattern', 'argument', 'action'], suffix_action=35,
                actual_matches=4, actual_actions=3, actual_signature_calls=4 if seed else 5,
                unknown_initialized_bytes=6031, unvisited_paths='whitespace/invalid/range source+graph only',
                CRT_code_credit=0, prefix=prefix)
