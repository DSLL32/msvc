"""Own112B suffix body and real continuation to pending vector append."""
import copy
import json
import re
import struct

from allocation_imports import pe_read
from option_action_state import check_signature
from scaler_probe import symbols
from scaler_state import fp, semantic


def check(s, d, original, native, trial, text, state, prefix):
    def need(ok, message):
        s.require(ok, 'option suffix: ' + message)

    def load(name):
        return json.loads((native / (name + '.json')).read_text())

    def reg(name):
        return struct.unpack('<16I', (original / ('actions-' + name + '-registers.bin')).read_bytes())

    def raw(phase, part):
        return (original / ('locale-' + phase + '-' + part + '.bin')).read_bytes()

    def argv(path):
        data = path.read_bytes();root, count = struct.unpack_from('<II', data)
        at, values = 8, []
        for _ in range(count):
            ptr, length = struct.unpack_from('<II', data, at);at += 8
            values.append((ptr, data[at:at + length]));at += length
        need(count == 26 and at == len(data), 'whole separate Invoke input')
        return root, values

    continuation=trial['trial']=='_append_option_vector' and bool(trial.get('append_observers'))
    need(trial['trial'] in ('_match_option_suffix','_append_option_vector') and trial['suffix_observers'] == ({
        'entry_rva': 0x39757, 'return_rva': 0x394cf, 'calls': 1,
        'arguments': [{'rule': 29, 'Invoke_slot': 7, 'offset': 2, 'action': 35}],
        'frontier_rva': 0x100ea, 'frontier_formals': ['CompilerVector *vector', 'char *value'],
        'scope': 'whole112B suffix candidate, one actual empty suffix; cold path source+graph only'
    } if not continuation else dict(entry_rva=0x39757,return_rva=0x394cf,calls=3,arguments=[{'rule':29,'Invoke_slot':7,'offset':2,'action':35},{'rule':28,'Invoke_slot':10,'offset':3,'action':36},{'rule':48,'Invoke_slot':23,'offset':3,'action':34}],frontier_rva=0x397db,frontier_formals=['char *output','const char *input','unsigned capacity'],scope='three actual empty suffixes in accepted helper; whole43B append candidate')), 'exact bounded candidate/actual input contract')
    need([r for r in trial['ranges'] if r['owner'] == '_match_option_suffix'] == [
        {'owner': '_match_option_suffix', 'start_rva': 0x39757, 'end_rva': 0x39779, 'entry': 1},
        {'owner': '_match_option_suffix', 'start_rva': 0x72623, 'end_rva': 0x72671, 'entry': 0}
    ], 'whole112B logical body including unexecuted78B cold suffix comparison')
    names = symbols(native / 'native-executable.elf');seed = bool(trial.get('test_seed'))
    first = load('first-suffix-state');original_root, oldargs = argv(original / 'invoke-entry-argv.bin')
    root, args = argv(native / 'first-suffix-argv.bin')
    need([v for _, v in oldargs] == [v for _, v in args] and args[7][1] == b'-Ze\0' and
         args[8][1] == b'-D_MSC_EXTENSIONS\0', 'actual argument ownership/full strings')
    entries, ends, append = load('suffix-entries'), load('suffix-returns'), load('suffix-append-entries')
    need([len(r) for r in (entries, ends, append)] == ([3,3,1] if continuation else [1,1,1]), 'one actual suffix and pending append')
    entry, end, final = entries[0], ends[0], append[0]
    previous = load('integer-suffix-entries')
    need(len(previous) == (3 if continuation else 1) and all(entry[k] == v for k, v in previous[0].items()),
         'same real first suffix checkpoint and already validated owners/namespace/FP/state')
    a, b = reg('suffix-entry-0'), reg('suffix-return-0')
    need(a[1] == 0x10439757 and a[10:14] ==
         (0x104394cf, 0x104da5fa, oldargs[7][0] + 2, 35) and
         entry['pc'] == names['match_option_suffix'][0] and
         entry['args'] == [first['args']['pattern'], args[7][0] + 2, 35] and
         entry['formal_inventory'] == [{'name': k, 'type': t} for k, t in
             [('pattern', 'const char *'), ('argument', 'char *'), ('action', 'unsigned int')]] and
         entry['caller'] == 'match_option' and entry['pattern'] == end['pattern'] == b'*\0'.hex() and
         entry['argument'] == end['argument'] == b'e\0'.hex(),
         'actual complete three-slot ABI and borrowed literal/interior string inputs')
    need((original / 'suffix-0-pattern.bin').read_bytes() == b'*\0' and
         (original / 'suffix-0-argument.bin').read_bytes() == b'e\0', 'whole original input strings')
    need(first['pc'] == entry['pc'] and first['esp'] == entry['esp'] and
         first['args'] == dict(zip(('pattern', 'argument', 'action'), entry['args'])),
         'typed collector is the actual pre-prologue entry, not DWARF physical values')

    def call(pc, target):
        data = bytes(d.elf_byte(native, pc - 5 + j) for j in range(5))
        need(data[0] == 0xe8 and pc + struct.unpack_from('<i', data, 1)[0] == target,
             'actual native direct callsite/target')

    def returned(before, after, old_before, old_after):
        need(old_after[1] == old_before[10] and old_after[2] == old_before[2] + 4 and
             old_after[6:10] == old_before[6:10] and after['pc'] == before['return_pc'] and
             after['entry_esp'] == before['esp'] and after['esp'] == before['esp'] + 4 and
             all(after[k] == before[k] for k in ('ebx', 'esi', 'edi', 'ebp')),
             'actual cdecl RET0/ESP/nonvolatile ABI')

    call(entry['return_pc'], entry['pc']);returned(entry, end, a, b)
    need(end['eax'] == b[3] == 1 and b[10:13] == a[11:14] and
         end['argument_slots_after'] == entry['args'], 'empty suffix return1 and unchanged all three slots')
    expected = copy.deepcopy(entry['guard']);front = first['frontend']
    old_bss = bytearray(raw('actions-suffix-entry-0', 'target-BSS'))
    old_data = bytearray((original / 'frontend-data-locale-actions-suffix-entry-0.bin').read_bytes())
    old_saved = (original / 'frontend-snapshot-locale-actions-suffix-entry-0.bin').read_bytes()
    fields = {r['name']: r for r in load('state-model')['fields']}
    locale = {k: v for k, v in entry['locale'].items() if k != 'maps'}
    floating = semantic(entry['fp']);chronology = []

    def phase(name, row, record, native_name=None):
        chronology.append((record[0], name))
        need(row['guard'] == expected and raw(name, 'target-BSS') == bytes(old_bss) and
             (original / ('frontend-data-locale-' + name + '.bin')).read_bytes() == bytes(old_data) and
             (original / ('frontend-snapshot-locale-' + name + '.bin')).read_bytes() == old_saved,
             'complete known/padding/saved/native and original unknown-neighbor guard ' + name)
        need(row['last_error'] == 203 and semantic(row['fp']) == floating and
             {k: v for k, v in row['locale'].items() if k != 'maps'} == locale,
             'native full namespace/owner/locale/error/architectural FP ' + name)
        for part in ('words', 'mbctype', 'ctype', 'old-ctype', 'result', 'errors', 'fp',
                     'CRT-argv', 'CRT-environment', 'Invoke-argv'):
            need(raw(name, part) == raw('actions-suffix-entry-0', part),
                 'original full provider/vector/error/112B FP lifetime ' + name + ':' + part)
        native_name = native_name or re.sub(r'^actions-(.*)-return-(\d+)$', r'actions-\1-\2-return', name)
        for kind, baseline in (('CRT-argv', 'native-crt-argv.bin'),
                               ('CRT-environment', 'native-crt-environment.bin')):
            need((native / ('locale-native-' + native_name + '-' + kind + '.bin')).read_bytes() ==
                 (native / baseline).read_bytes(), 'full native private vector lifetime ' + name)

    def write(name, old_value, value):
        field = fields[name];kind = 'data' if field['class'] == 'initialized' else 'bss'
        base = front['storage']['first_invocation'][0] if kind == 'data' else front['bss_address']
        buf = bytearray.fromhex(expected[kind]);struct.pack_into('<I', buf, front['storage'][name][0] - base, value)
        expected[kind] = buf.hex()
        old = old_data if kind == 'data' else old_bss
        struct.pack_into('<I', old, field['original_address'] - (0x104cc000 if kind == 'data' else 0x104b9000), old_value)

    phase('actions-suffix-entry-0', entry, a)
    phase('actions-suffix-return-0', end, b)
    actions, action_ends = load('actions-entries'), load('actions-returns')
    matches, matched = load('actions-matches'), load('actions-match-returns')
    outer, outer_ends = load('actions-outer-entries'), load('actions-outer-returns')
    calls, results = load('actions-signatures'), load('actions-signature-returns')
    need([len(r) for r in (actions, action_ends, matches, matched, outer, outer_ends, calls, results)] ==
         ([20,20,21,21,21,21,23 if seed else 24,23 if seed else 24] if continuation else [5,4,5,4,5,5,5 if seed else 6,5 if seed else 6]), 'complete actual continuation inventory')
    rules = load('options-grammar-layout')['option_rules']['address']
    for index, rule_index, slot, offset in ((3, 29, 7, 2), (4, 6, 8, 2)):
        ae = reg('action-entry-%u' % index)
        phase('actions-action-entry-%u' % index, actions[index], ae)
        need(ae[1] == 0x10439632 and ae[10:14] ==
             (0x1043953c, 0x104d6160 + 12 * rule_index, oldargs[slot][0] + offset, 0) and
             actions[index]['pc'] == names['apply_option_action'][0] and
             actions[index]['args'] == [rules + 12 * rule_index, args[slot][0] + offset, 0] and
             actions[index]['formal_inventory'] == actions[2]['formal_inventory'] and
             actions[index]['caller'] == 'match_option' and
             actions[index]['value'] == args[slot][1][offset:].hex(), 'actual wildcard/direct-value action inputs')
        if index == 4:break
        write('options.language_extensions', 1, 1)
        ar = reg('action-return-3');phase('actions-action-return-3', action_ends[3], ar)
        returned(actions[3], action_ends[3], ae, ar)
        signature_index = 4 if seed else 5
        check_signature(signature_index, 7, 0x39565, expected, old_bss, old_data, old_saved,
                        calls, results, original, native, names, need, locale, floating,
                        raw, 'actions-suffix-entry-0', d, args, oldargs)
        for label in ('entry', 'return'):
            r = struct.unpack('<17I', (original / ('actions-signature-%u-%s-registers.bin' % (signature_index, label))).read_bytes())
            chronology.append((r[0], 'signature-' + label))
        write('source_signature', results[signature_index]['eax'], results[signature_index]['eax'])
        mr = reg('match-return-3');phase('actions-match-return-3', matched[3], mr)
        returned(matches[3], matched[3], reg('match-entry-3'), mr)
        need(mr[3] == matched[3]['eax'] == 1, 'fourth real matcher result1')
        oe, od, me = reg('outer-entry-4'), reg('outer-return-4'), reg('match-entry-4')
        phase('actions-outer-entry-4', outer[4], oe)
        need(oe[1] == 0x10439428 and oe[10] == 0x104390d1 and
             outer[4]['pc'] == names['next_argument'][0] and outer[4]['args'] == [] and
             outer[4]['formal_inventory'] == [] and outer[4]['caller'] == 'process_compiler_options',
             'fifth real no-formal callback')
        write('command_argc', 18, 18);write('command_argv', original_root + 32, root + 32)
        phase('actions-outer-return-4', outer_ends[4], od);returned(outer[4], outer_ends[4], oe, od)
        need(od[3] == oldargs[8][0] and outer_ends[4]['eax'] == args[8][0] and
             outer_ends[4]['value'] == args[8][1].hex() and
             (original / 'actions-outer-4-value.bin').read_bytes() == args[8][1], 'actual fifth borrowed full -D argument')
        phase('actions-match-entry-4', matches[4], me)
        need(me[1] == 0x1043944a and me[10:15] == (0x104390dc, 0x104d6160, oldargs[8][0], 0x10439428, 0) and
             matches[4]['pc'] == names['match_option'][0] and matches[4]['args'] ==
             [rules, args[8][0], names['next_argument'][0], 0] and
             matches[4]['formal_inventory'] == matches[3]['formal_inventory'] and
             matches[4]['argument'] == args[8][1].hex(), 'actual fifth four-formal matcher input')
        write('current_argument', oldargs[8][0], args[8][0])
    # The pending vector's representation and complete32KiB allocation were
    # proved at the real three-vector prefix; no append semantics are credited.
    nr = reg('append-entry-0');phase('actions-append-entry-0', final, nr, 'suffix-next')
    old_vector = struct.unpack_from('<I', old_bss, 0xc06c4 - 0xb9000)[0]
    need(nr[1] == 0x104100ea and nr[10:13] == (0x104396bc, old_vector, oldargs[8][0] + 2) and
         final['pc'] == names['append_option_vector'][0] and final['args'] ==
         [front['options_bss']['defines'], args[8][0] + 2] and final['formal_inventory'] ==
         [{'name': 'vector', 'type': 'CompilerVector *'}, {'name': 'value', 'type': 'char *'}] and
         final['caller'] == 'apply_option_action' and final['value'] == b'_MSC_EXTENSIONS\0'.hex() and
         (original / 'suffix-append-input.bin').read_bytes() == b'_MSC_EXTENSIONS\0',
         'actual pending append true entry/two complete ABI inputs/borrowed owner')
    call(final['return_pc'], final['pc'])
    header = (original / 'suffix-append-header.bin').read_bytes()
    count, capacity, arena, items = struct.unpack('<hhiI', header)
    native_header = bytes.fromhex(final['vector_header'])
    ncount, ncapacity, narena, nitems = struct.unpack('<hhiI', native_header)
    need((count, capacity, arena) == (ncount, ncapacity, narena) == (0, 40, 1) and
         items == old_vector + 12 and nitems == final['args'][0] + 12 and
         (original / 'suffix-append-items.bin').read_bytes() == bytes.fromhex(final['items']) == bytes(160),
         'actual already-modeled12B header/160B zero pointer array and storage identity')
    need((original / 'suffix-append-committed.bin').read_bytes() ==
         (original / 'vector-2-committed.bin').read_bytes() and bytes.fromhex(final['committed']) ==
         (native / 'vector-2-committed.bin').read_bytes(), 'entire owned32KiB unchanged from proved vector prefix')
    tail = re.findall(r'^trap seq=(\d+) rva=000100ea esp=(\w+).*stack=([0-9a-f,]+)$', text, re.M)
    need(len(tail) == (6 if continuation else 1) and int(tail[0][0]) + (0 if continuation else 1) == nr[0] and int(tail[0][1], 16) == nr[2] and
         [int(v, 16) for v in tail[0][2].split(',')][:3] == list(nr[10:13]) and
         re.findall(r'^frontier rva=(\w+)$', text, re.M) == (['000397db'] if continuation else ['000100ea']), 'direct final original actual stack frontier')
    # The generic trap saves/increments the observer sequence before this
    # same stopped context is recorded by check_suffix_stop; no target runs.
    sequence = int(tail[0][0])
    context = re.findall(r'^trap seq=%u rva=000100ea esp=(\w+) eax=(\w+) ecx=(\w+) edi=(\w+) stack=' % sequence, text, re.M)
    need(len(context) == 1 and [int(v, 16) for v in context[0]] == [nr[2], nr[3], nr[4], nr[8]],
         'same actual final stopped context despite observer counter increment')
    need((original / ('pool-bss-%03d.bin' % (sequence + 1))).read_bytes() == bytes(old_bss) and
         (original / ('frontend-data-%03d.bin' % sequence)).read_bytes() == bytes(old_data) and
         (original / ('frontend-snapshot-%03d.bin' % sequence)).read_bytes() == old_saved and
         semantic(fp((original / ('x87-context-%03d.bin' % sequence)).read_bytes())) == floating,
         'whole original direct final state/unknown bytes/architectural FP')
    expected_front = copy.deepcopy(front)
    expected_front.update(command_argc=18, command_argv=root + 32,
                          current_argument=args[8][0], source_signature=results[4 if seed else 5]['eax'])
    expected_front['options_data']['language_extensions'] = 1
    need(state['symbol'] == 'append_option_vector' and state['args'] ==
         dict(zip(('vector', 'value'), final['args'])) and ((state['pc']==final['pc'] and state['esp']==final['esp']) if continuation else
          (names['append_option_vector'][0] < state['pc'] <= sum(names['append_option_vector']) and d.elf_byte(native,state['pc']-1)==0xcc and b'Program received signal SIGTRAP' in (native/'gdb.txt').read_bytes())) and
         state['frontend'] == expected_front and state['invocation'] == first['invocation'] and
         state['target_state'] == first['target_state'], 'actual named native trap and entire final modeled state/lifetime')
    need(all(x[0] < y[0] for x, y in zip(chronology, chronology[1:])), 'complete ordered suffix/option continuation')
    return dict(frontier='_append_option_vector',candidate='_match_option_suffix',code_bytes=112,
                actual_suffix_calls=1,pattern='star NUL',argument='borrowed Invoke[7]+2 e NUL',action_slot=35,
                action_slot_consumed=False,result=1,appended=False,append_formals=2,
                actual_match_calls=5,actual_action_entries=5,actual_action_returns=4,actual_signature_calls=len(calls),
                unvisited_scope='78B cold suffix comparison source+graph only',unknown_initialized_bytes=6031,
                dependency_credit=0,prefix=prefix)
