"""Full ordered own option-action proof through the genuine integer trap."""
import copy
import json
import re
import struct

from allocation_imports import pe_read
from scaler_probe import symbols
from scaler_state import fp, semantic


def check(s, d, original, native, trial, text, state, prefix):
    def need(ok, message):
        s.require(ok, 'option actions: ' + message)

    def rows(name):
        return json.loads((native / ('actions-' + name + '.json')).read_text())

    def regs(tag, index):
        return struct.unpack('<16I', (original / (
            'actions-%s-%d-registers.bin' % (tag, index))).read_bytes())

    def raw(phase, suffix):
        return (original / ('locale-' + phase + '-' + suffix + '.bin')).read_bytes()

    def argv(path):
        data = path.read_bytes()
        root, count = struct.unpack_from('<II', data)
        at, result = 8, []
        for _ in range(count):
            pointer, length = struct.unpack_from('<II', data, at)
            at += 8
            result.append((pointer, data[at:at + length]))
            at += length
        need(count == 26 and at == len(data), 'whole separate Invoke26 input')
        return root, result

    need(trial['trial'] == '_apply_option_action' and trial['option_action_observers'] == {
        'entry_rva': 0x39632, 'frontier_rva': 0x39779, 'matches': 3,
        'actions': 3, 'action_returns': 2, 'outer_calls': 3, 'inner_calls': 3,
        'signature_returns': [0x3909f, 0x395af, 0x39565],
        'scope': 'complete own373B candidate; persistent indexed true argument/matcher/action/public entry+RET inventory, no response replay or per-API run'
    }, 'exact candidate/observer contract')
    need([r for r in trial['ranges'] if r['owner'] == '_apply_option_action'] == [
        {'owner': '_apply_option_action', 'start_rva': 0x39632, 'end_rva': 0x3970d, 'entry': 1},
        {'owner': '_apply_option_action', 'start_rva': 0x72589, 'end_rva': 0x72623, 'entry': 0},
        {'owner': '_apply_option_action', 'start_rva': 0x39710, 'end_rva': 0x39757, 'entry': 0, 'kind': 'text_data'}
    ], 'whole373 code/71 data; three padding bytes excluded')
    seed = bool(trial.get('test_seed'))
    names = symbols(native / 'native-executable.elf')
    oldroot, oldargs = argv(original / 'invoke-entry-argv.bin')
    root, args = argv(native / 'first-action-argv.bin')
    need([v for _, v in oldargs] == [v for _, v in args] and
         [v for _, v in args[1:7]] == [b'-il\0', b'.\\a00436\0', b'-f\0', b'minimal.cpp\0', b'-W\0', b'1\0'],
         'own complete original/native actual strings and borrowed owners')
    matches, matched = rows('matches'), rows('match-returns')
    actions, returned = rows('entries'), rows('returns')
    outer, outer_done = rows('outer-entries'), rows('outer-returns')
    inner, inner_done = rows('inner-entries'), rows('inner-returns')
    calls, results = rows('signatures'), rows('signature-returns')
    integers = rows('integer-entries')
    need([len(v) for v in (matches, matched, actions, returned, outer, outer_done,
                          inner, inner_done, calls, results, integers)] ==
         [3, 2, 3, 2, 3, 3, 3, 3, 2 if seed else 3, 2 if seed else 3, 1],
         'complete ordered entry/return/API inventories')
    first = json.loads((native / 'first-action-state.json').read_text())
    front = first['frontend']
    expected = copy.deepcopy(outer[0]['guard'])
    old_bss = bytearray(raw('actions-outer-entry-0', 'target-BSS'))
    old_data = bytearray((original / 'frontend-data-locale-actions-outer-entry-0.bin').read_bytes())
    old_saved = (original / 'frontend-snapshot-locale-actions-outer-entry-0.bin').read_bytes()
    model = json.loads((native / 'state-model.json').read_text())
    fields = {r['name']: r for r in model['fields']}
    data_base = front['storage']['first_invocation'][0]
    locale = {k: v for k, v in outer[0]['locale'].items() if k != 'maps'}
    original_phase = 'actions-outer-entry-0'
    floating = semantic(outer[0]['fp'])
    chronology = []
    if not seed:
        legacy_entry = json.loads((native / 'vector-public-entries.json').read_text())[0]
        legacy_return = json.loads((native / 'signature-returns.json').read_text())[0]
        for actual, legacy in ((calls[0], legacy_entry), (results[0], legacy_return)):
            need(all(actual[k] == v for k, v in legacy.items()),
                 'indexed first CPP call is the same already validated physical prefix')
        for phase in ('entry', 'return'):
            indexed = struct.unpack('<17I', (original / (
                'actions-signature-0-%s-registers.bin' % phase)).read_bytes())
            legacy = struct.unpack('<16I', (original / (
                'signature-call-%s-registers.bin' % phase)).read_bytes())
            need(indexed[:16] == legacy and indexed[16] == 0,
                 'original indexed first CPP call aliases the real validated prefix')
            tag = 'actions-signature-0-' + phase
            need((original / (tag + '-input.bin')).read_bytes() == b'CPP\0' and
                 (original / (tag + '-table.bin')).read_bytes() ==
                 pe_read(original / 'loaded-mspdb50.bin', 0x21a30, 1024),
                 'indexed first CPP full defined input/table')
            for suffix in ('words', 'mbctype', 'ctype', 'old-ctype', 'result', 'errors',
                           'fp', 'CRT-argv', 'CRT-environment', 'Invoke-argv', 'target-BSS'):
                need(raw(tag, suffix) == raw('signature-' + phase, suffix),
                     'indexed first CPP actual phase/state identity ' + suffix)
        canonical = pe_read(original / 'loaded-mspdb50.bin', 0xf22d, 53)
        need((original / 'actions-signature-0-provider-body.bin').read_bytes() == b'\xcc' + canonical[1:],
             'indexed first CPP body retains exact owned entry guard')

    def write(name, old_value, value):
        field = fields[name]
        kind = 'data' if field['class'] == 'initialized' else 'bss'
        base = data_base if kind == 'data' else front['bss_address']
        buf = bytearray.fromhex(expected[kind])
        struct.pack_into('<I', buf, front['storage'][name][0] - base, value)
        expected[kind] = buf.hex()
        old = old_data if kind == 'data' else old_bss
        oldbase = 0x104cc000 if kind == 'data' else 0x104b9000
        struct.pack_into('<I', old, field['original_address'] - oldbase, old_value)

    def phase(tag, index, row):
        name = 'actions-%s-%d' % (tag, index)
        record = regs(tag, index)
        chronology.append((tag, index, record[0]))
        need(row['guard'] == expected, 'complete native known/padding/external/saved guard ' + name)
        need(raw(name, 'target-BSS') == bytes(old_bss) and
             (original / ('frontend-data-locale-' + name + '.bin')).read_bytes() == bytes(old_data) and
             (original / ('frontend-snapshot-locale-' + name + '.bin')).read_bytes() == old_saved,
             'complete original BSS/8520-byte data/saved including unknown neighbors ' + name)
        need(row['last_error'] == 203 and semantic(row['fp']) == floating and
             {k: v for k, v in row['locale'].items() if k != 'maps'} == locale,
             'native tables/owners/selection/private vectors/errors/architectural FP ' + name)
        for suffix in ('words', 'mbctype', 'ctype', 'old-ctype', 'result', 'errors',
                       'fp', 'CRT-argv', 'CRT-environment', 'Invoke-argv'):
            need(raw(name, suffix) == raw(original_phase, suffix),
                 'original provider/vector/error/full FP lifetime ' + name + ':' + suffix)
        for kind, baseline in (('CRT-argv', 'native-crt-argv.bin'),
                               ('CRT-environment', 'native-crt-environment.bin')):
            native_name = ('actions-%s-%d-return' % (tag[:-7], index)
                           if tag.endswith('-return') else name)
            need((native / ('locale-native-' + native_name + '-' + kind + '.bin')).read_bytes() ==
                 (native / baseline).read_bytes(), 'full native private vector lifetime ' + name)
        return record

    def returned_abi(entry, end, old_entry, old_end, meaningful=False):
        need(end['pc'] == entry['return_pc'] and end['entry_esp'] == entry['esp'] and
             end['esp'] == entry['esp'] + 4 and
             all(end[k] == entry[k] for k in ('ebx', 'esi', 'edi', 'ebp')) and
             old_end[1] == old_entry[10] and old_end[2] == old_entry[2] + 4 and
             old_end[6:10] == old_entry[6:10], 'true RET0/ESP/nonvolatile ABI')
        if meaningful:
            need(end['eax'] == old_end[3] == 1, 'actual matcher signed-result one')

    rules = json.loads((native / 'options-grammar-layout.json').read_text())['option_rules']['address']
    for i in range(3):
        oe = phase('outer-entry', i, outer[i])
        need(oe[1] == 0x10439428 and oe[10] == 0x10400000 + (0x390b3 if not i else 0x390d1) and
             outer[i]['pc'] == names['next_argument'][0] and outer[i]['args'] == [] and
             outer[i]['formal_inventory'] == [] and outer[i]['caller'] == 'process_compiler_options',
             'outer no-formal physical entry/caller')
        write('command_argc', 25 - 2 * i, 25 - 2 * i)
        write('command_argv', oldroot + (1 + 2 * i) * 4, root + (1 + 2 * i) * 4)
        od = phase('outer-return', i, outer_done[i])
        returned_abi(outer[i], outer_done[i], oe, od)
        need(od[3] == oldargs[1 + 2 * i][0] and outer_done[i]['eax'] == args[1 + 2 * i][0] and
             outer_done[i]['value'] == args[1 + 2 * i][1].hex() and
             (original / ('actions-outer-%d-value.bin' % i)).read_bytes() == oldargs[1 + 2 * i][1],
             'borrowed outer result/full NUL string')
        me = phase('match-entry', i, matches[i])
        need(me[1] == 0x1043944a and me[10:15] == (0x10400000 + (0x390be if not i else 0x390dc),
             0x104d6160, oldargs[1 + 2 * i][0], 0x10439428, 0) and
             matches[i]['pc'] == names['match_option'][0] and matches[i]['args'] ==
             [rules, args[1 + 2 * i][0], names['next_argument'][0], 0] and
             matches[i]['argument'] == args[1 + 2 * i][1].hex() and
             matches[i]['formal_inventory'] == [{'name': k, 'type': v} for k, v in
                [('rules', 'CompilerOptionRule *'), ('argument', 'char *'), ('next', 'char *(*)(void)'), ('copy', 'int')]],
             'each actual four-formal matcher entry/borrowed slot')
        write('current_argument', oldargs[1 + 2 * i][0], args[1 + 2 * i][0])
        ie = phase('inner-entry', i, inner[i])
        need(ie[1] == 0x10439428 and ie[10] == 0x10400000 + (0x395bd if i == 1 else 0x3957b) and
             inner[i]['pc'] == names['next_argument'][0] and inner[i]['args'] == [] and
             inner[i]['formal_inventory'] == [] and inner[i]['caller'] == 'match_option',
             'each genuine indirect no-argument callback')
        write('command_argc', 24 - 2 * i, 24 - 2 * i)
        write('command_argv', oldroot + (2 + 2 * i) * 4, root + (2 + 2 * i) * 4)
        ir = phase('inner-return', i, inner_done[i])
        returned_abi(inner[i], inner_done[i], ie, ir)
        need(ir[3] == oldargs[2 + 2 * i][0] and inner_done[i]['eax'] == args[2 + 2 * i][0] and
             inner_done[i]['value'] == args[2 + 2 * i][1].hex() and
             (original / ('actions-inner-%d-value.bin' % i)).read_bytes() == oldargs[2 + 2 * i][1],
             'borrowed inner result/full NUL string')
        ae = phase('action-entry', i, actions[i])
        rule = [31, 12, 23][i]
        need(ae[1] == 0x10439632 and ae[10:14] == (0x10400000 + (0x395cb if i == 1 else 0x39589),
             0x104d6160 + 12 * rule, oldargs[2 + 2 * i][0], 0) and
             actions[i]['pc'] == names['apply_option_action'][0] and actions[i]['args'] ==
             [rules + 12 * rule, args[2 + 2 * i][0], 0] and actions[i]['caller'] == 'match_option' and
             actions[i]['value'] == args[2 + 2 * i][1].hex() and
             actions[i]['formal_inventory'] == [{'name': k, 'type': v} for k, v in
                 [('rule', 'CompilerOptionRule *'), ('value', 'char *'), ('copy', 'int')]],
             'all three real action formals/rule/value/copy inputs')
        if i == 2:
            break
        target = 'options.il_prefix' if not i else 'options.source_file'
        write(target, oldargs[2 + 2 * i][0], args[2 + 2 * i][0])
        ar = phase('action-return', i, returned[i])
        returned_abi(actions[i], returned[i], ae, ar)
        if i == 1:
            for j in range(2):
                api_index = j + (0 if seed else 1)
                if j:
                    result = results[api_index - 1]['eax']
                    write('source_signature', result, result)
                check_signature(api_index, [4, 3][j], [0x395af, 0x39565][j], expected,
                                old_bss, old_data, old_saved, calls, results, original, native, names,
                                need, locale, floating, raw, original_phase, d, args, oldargs)
            result = results[-1]['eax']
            write('source_signature', result, result)
        mr = phase('match-return', i, matched[i])
        returned_abi(matches[i], matched[i], me, mr, meaningful=True)

    integer = integers[0]
    need(integer['pc'] == names['parse_option_integer'][0] and integer['args'] == [args[6][0]] and
         integer['formal_inventory'] == [{'name': 'value', 'type': 'const char *'}] and
         integer['caller'] == 'apply_option_action' and integer['value'] == b'1\0'.hex(),
         'new named stub exact actual physical one-formal ABI')
    tail = re.findall(r'^trap seq=(\d+) rva=00039779 .*stack=([0-9a-f,]+)$', text, re.M)
    need(len(tail) == 1 and [int(v, 16) for v in tail[0][1].split(',')][:2] ==
         [0x10439694, oldargs[6][0]] and re.findall(r'^frontier rva=(\w+)$', text, re.M) == ['00039779'],
         'actual original integer-call frontier and argument')
    need(state['symbol'] == 'parse_option_integer' and state['args'] == {'value': args[6][0]} and
         names['parse_option_integer'][0] < state['pc'] <= sum(names['parse_option_integer']) and
         d.elf_byte(native, state['pc'] - 1) == 0xcc, 'actual native integer INT3 trap')
    need(integer['guard'] == expected and state['frontend']['command_argc'] == 20 and
         state['frontend']['command_argv'] == root + 24 and
         state['frontend']['current_argument'] == args[5][0], 'complete final known state/borrowed cursor')
    final_front = copy.deepcopy(front)
    final_front.update(command_argc=20, command_argv=root + 24,
                       current_argument=args[5][0], source_signature=results[-1]['eax'])
    final_front['options_data']['il_prefix'] = args[2][0]
    final_front['options_bss']['source_file'] = args[4][0]
    need(state['frontend'] == final_front and state['invocation'] == first['invocation'],
         'whole final typed state and actual Invoke storage/strings')
    sequence = int(tail[0][0])
    need((original / ('pool-bss-%03d.bin' % (sequence + 1))).read_bytes() == bytes(old_bss) and
         (original / ('frontend-data-%03d.bin' % sequence)).read_bytes() == bytes(old_data) and
         (original / ('frontend-snapshot-%03d.bin' % sequence)).read_bytes() == old_saved,
         'direct final complete original state/unknown-neighbor guard')
    need(semantic(fp((original / ('x87-context-%03d.bin' % sequence)).read_bytes())) == floating and
         semantic(integer['fp']) == floating, 'actual final architectural FP')
    observed = [(tag, int(index), int(seq)) for tag, index, seq in re.findall(
        r'^option-action phase=(\S+) index=(\d+) seq=(\d+) ', text, re.M)]
    need(observed == chronology and all(a[2] < b[2] for a, b in zip(chronology, chronology[1:])),
         'complete unambiguous chronological action loop')
    data = (original / 'relocated-text.bin').read_bytes()[0x39710 - 0x1000:0x39757 - 0x1000]
    need(data == pe_read(original / 'loaded-target.bin', 0x39710, 71), 'exact restored71-byte switch data')
    return dict(frontier='_parse_option_integer', code_bytes=373, table_data_bytes=71,
                table_code_credit=0, matches=3, actions=3, returned_actions=2,
                outer_callbacks=3, inner_callbacks=3, signature_calls=len(calls),
                final_argument='borrowed Invoke[6] 1 NUL', argc_after=20,
                unknown_initialized_bytes=6031, prefix=prefix)


def check_signature(index, slot, caller, expected, old_bss, old_data, old_saved,
                    calls, results, original, native, names, need, locale,
                    floating, raw, original_phase, d, args, oldargs):
    a, b = [struct.unpack('<17I', (original / (
        'actions-signature-%d-%s-registers.bin' % (index, phase))).read_bytes())
            for phase in ('entry', 'return')]
    entry, end = calls[index], results[index]
    value = args[slot][1]
    seed = struct.unpack_from('<I', old_bss, 0xc0834 - 0xb9000)[0]
    need(a[1] == a[11] + 0xf22d and a[10] == b[1] == b[10] == 0x10400000 + caller and
         a[12:15] == (oldargs[slot][0], len(value) - 1, seed) and
         a[16] == b[16] == index and b[2] == a[2] + 4 and b[6:10] == a[6:10] and
         b[12:14] == a[12:14] and b[14] == b[3], 'original indexed public ABI/RET0/private third-slot update')
    need(entry['pc'] == names['recon_mspdb_SigForPbCb'][0] and
         entry['args'] == [args[slot][0], len(value) - 1, seed] and
         entry['input'] == end['input'] == value.hex() and entry['caller'] == 'match_option' and
         entry['formal_inventory'] == [{'name': k, 'type': v} for k, v in
            [('bytes', 'const void *'), ('byte_count', 'long'), ('signature', 'unsigned long')]],
         'native indexed actual public formals/input/seed')
    call = bytes(d.elf_byte(native, entry['return_pc'] - 5 + i) for i in range(5))
    need(call[0] == 0xe8 and entry['return_pc'] + struct.unpack_from('<i', call, 1)[0] == entry['pc'] and
         end['pc'] == entry['return_pc'] and end['esp'] == entry['esp'] + 4 and
         end['entry_esp'] == entry['esp'] and end['eax'] == b[3] and
         all(end[k] == entry[k] for k in ('ebx', 'esi', 'edi', 'ebp')) and
         end['argument_slots_after'] == [args[slot][0], len(value) - 1, end['eax']],
         'actual native call encoding/RET0/result/nonvolatiles/private slots')
    need(pe_read(original / 'loaded-target.bin', caller, 8) == bytes.fromhex('83c40ca334084c10'),
         'actual caller discards private slots and consumes only EAX')
    canonical = pe_read(original / 'loaded-mspdb50.bin', 0xf22d, 53)
    body = (original / ('actions-signature-%d-provider-body.bin' % index)).read_bytes()
    need(body == b'\xcc' + canonical[1:], 'whole actual provider body with owned first-byte guard')
    for phase, row in (('entry', entry), ('return', end)):
        name = 'actions-signature-%d-%s' % (index, phase)
        need((original / (name + '-input.bin')).read_bytes() == value and
             (original / (name + '-table.bin')).read_bytes() == pe_read(original / 'loaded-mspdb50.bin', 0x21a30, 1024),
             'full original defined input/table lifetime')
        need(row['guard'] == expected and raw(name, 'target-BSS') == bytes(old_bss) and
             (original / ('frontend-data-locale-' + name + '.bin')).read_bytes() == bytes(old_data) and
             (original / ('frontend-snapshot-locale-' + name + '.bin')).read_bytes() == old_saved,
             'whole indexed public target/unknown-neighbor preservation')
        need(row['last_error'] == 203 and semantic(row['fp']) == floating and
             {k: v for k, v in row['locale'].items() if k != 'maps'} == locale,
             'native indexed provider owner/vector/error/FP preservation')
        for suffix in ('words', 'mbctype', 'ctype', 'old-ctype', 'result', 'errors',
                       'fp', 'CRT-argv', 'CRT-environment', 'Invoke-argv'):
            need(raw(name, suffix) == raw(original_phase, suffix),
                 'original indexed namespace/provider/error/full FP ' + suffix)
        for kind, baseline in (('CRT-argv', 'native-crt-argv.bin'),
                               ('CRT-environment', 'native-crt-environment.bin')):
            need((native / ('locale-native-' + name + '-' + kind + '.bin')).read_bytes() ==
                 (native / baseline).read_bytes(), 'indexed native full private vector preservation')
    need(entry['vector'] == end['vector'] and
         (original / ('actions-signature-%d-entry-vector.bin' % index)).read_bytes() ==
         (original / ('actions-signature-%d-return-vector.bin' % index)).read_bytes(),
         'complete owned32KiB vector storage preserved across public call')
