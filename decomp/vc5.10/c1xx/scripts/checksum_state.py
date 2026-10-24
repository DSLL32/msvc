"""Encountered polynomial tables, reset/overwrite and isolated locale handoff."""
import copy
import importlib.util
import json
from pathlib import Path
import re
import struct

from scaler_probe import symbols
from scaler_state import fp, semantic


def check(s, d, original, native, trial, text, pre, phase, state, seed, final, traps):
    def require(ok, message):
        s.require(ok, 'checksum tables: ' + message)
    def read(name):
        return json.loads((native / ('checksum-' + name + '.json')).read_text())
    def one(name):
        rows = read(name)
        require(len(rows) == 1, 'actual ' + name + ' inventory')
        return rows[0]
    spec = importlib.util.spec_from_file_location('checksum_original_locale', Path(__file__).with_name('check-isolated-locale.py'))
    locale = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(locale)
    original_locale = locale.check(original)
    expected_tables = locale.checksum_tables()
    require(trial['trial'] == '_initialize_checksums' and trial.get('checksum_observers') is not None, 'wrong whole-function candidate/observer contract')
    require(state['symbol'] == 'initialize_multibyte_categories' and state['args'] == {}, 'genuine next named noarg frontier')
    names = symbols(native / 'native-executable.elf')
    binding_entry, binding_return, first_entry = (one(name) for name in ('binding-entries', 'binding-returns', 'first-entries'))
    reference = original_locale['original_receipt_sha256']
    require(binding_entry['args'] == [original_locale['provider_sha256'], reference], 'actual provider/own-receipt enable binding')
    require(binding_return['pc'] == binding_entry['return_pc'] and binding_return['esp'] == binding_entry['esp'] + 4 and binding_entry['guard'] == binding_return['guard'], 'host-only binder RET0 or target-state mutation')
    old_binding = copy.deepcopy(binding_entry['locale'])
    expected_binding = copy.deepcopy(old_binding)
    require(old_binding['selection'] == dict(bound=0, calls=0, reference='', result=0), 'initial selection inventory')
    expected_binding['selection'] = dict(bound=1, calls=0, reference=reference, result=0)
    require(binding_return['locale'] == expected_binding and first_entry['locale']['selection'] == expected_binding['selection'] and first_entry['locale']['lc_codepage'] == 0 and first_entry['locale']['pctype'] == old_binding['pctype'], 'enable changed locale before actual target/API entry')
    entry, returned, before, after, next_entry = (one(name) for name in ('entries', 'returns', 'locale-entries', 'locale-returns', 'next-entries'))
    require(entry['pc'] == names['initialize_checksums'][0] and entry['formal_inventory'] == [] and next_entry['pc'] == names['initialize_multibyte_categories'][0] and next_entry['formal_inventory'] == [], 'actual candidate/next PC and source formals')
    rows = [row for row in traps if row[1] == '000392a8']
    require(len(rows) == 1, 'actual original candidate entry count')
    seq = int(rows[0][0])
    stack = [int(v, 16) for v in rows[0][2].split(',')]
    require(stack[0] == 0x10438fd4, 'actual original caller')
    raw_entry = (original / ('pool-bss-%03d.bin' % (seq + 1))).read_bytes()
    offset = 0xbe970 - 0xb9000
    native_offset = state['frontend']['storage']['checksum_tables'][0] - state['frontend']['bss_address']
    require(native_offset == 1544 and state['frontend']['checksum_native_layout']['size'] == 768 and [state['frontend']['checksum_native_layout']['fields'][name][:2] for name in ('update', 'high', 'low')] == [[0, 256], [256, 256], [512, 256]], 'known semantic table storage/layout')
    initial = bytes([0xa5]) * 768 if seed else bytes(768)
    for snapshot, expected in ((pre, initial), (phase, bytes(768))):
        actual = b''.join(bytes(snapshot['frontend']['checksum_tables'][name]) for name in ('update', 'high', 'low'))
        require(actual == expected, 'new typed initializer input/BSS clear')
    require((original / 'checksum-tables-006.bin').read_bytes() == initial and (original / 'checksum-tables-phase.bin').read_bytes() == bytes(768), 'actual original nonzero/default input and reset')
    require(bytes.fromhex(entry['tables']) == raw_entry[offset:offset + 768] == initial, 'actual helper overwrite input')
    if seed:
        require('checksum-seed point=initializer bytes=768 value=165 code-mock=0\n' in text and 'checksum-seed point=actual-helper-entry bytes=768 value=165 code-mock=0\n' in text, 'explicit data-only auxiliary roles')
        original_preseed = bytearray((original / 'checksum-entry-before-seed-BSS.bin').read_bytes())
        require(original_preseed[offset:offset + 768] == bytes(768), 'real pre-seed handoff was not reset')
        original_preseed[offset:offset + 768] = initial
        require(bytes(original_preseed) == raw_entry, 'auxiliary changed original bytes beyond 768 tables')
        before_seed = copy.deepcopy(entry['before_seed']['guard'])
        bss = bytearray.fromhex(before_seed['bss'])
        require(bss[native_offset:native_offset + 768] == bytes(768), 'native pre-seed real handoff')
        bss[native_offset:native_offset + 768] = initial
        before_seed['bss'] = bss.hex()
        require(before_seed == entry['guard'], 'auxiliary changed native neighboring state')
    else:
        require(entry['before_seed']['guard'] == entry['guard'] and 'checksum-seed ' not in text, 'natural trial gained a data mock')
    expected_guard = copy.deepcopy(entry['guard'])
    bss = bytearray.fromhex(expected_guard['bss'])
    bss[native_offset:native_offset + 768] = expected_tables
    expected_guard['bss'] = bss.hex()
    require(returned['guard'] == before['guard'] == after['guard'] == next_entry['guard'] == expected_guard, 'candidate/API changed native neighbors, padding, snapshot or external state')
    raw_expected = bytearray(raw_entry)
    raw_expected[offset:offset + 768] = expected_tables
    require((original / 'checksum-return-BSS.bin').read_bytes() == bytes(raw_expected), 'actual whole original BSS changed beyond 768 outputs')
    for name in ('frontend-data', 'frontend-snapshot'):
        require((original / (name + '-%03d.bin' % seq)).read_bytes() == (original / (name + '-checksum-return.bin')).read_bytes() == (original / (name + '-locale-next.bin')).read_bytes(), 'whole original initialized/saved unknown bytes changed')
    serialized = b''.join(bytes(state['frontend']['checksum_tables'][name]) for name in ('update', 'high', 'low'))
    require(serialized == expected_tables and all(bytes.fromhex(row['tables']) == expected_tables for row in (returned, before, after, next_entry)), 'all 768 original/native table outputs and lifetime')
    ret = re.findall(r'^checksum-return pc=(\w+) esp=(\w+) eax=(\w+) ebx=(\w+) esi=(\w+) edi=(\w+) ebp=(\w+) error=(\d+)$', text, re.M)
    require(len(ret) == 1, 'actual original return observation')
    ret = ret[0]
    old_esp = int(re.search(r'^trap seq=%d .*esp=(\w+)' % seq, text, re.M)[1], 16)
    regs = re.findall(r'^pool-registers seq=%d ebx=(\w+) esi=(\w+) edi=(\w+) ebp=(\w+)$' % (seq + 1), text, re.M)
    require(int(ret[0], 16) == 0x10438fd4 and int(ret[1], 16) == old_esp + 4 and len(regs) == 1 and tuple(regs[0]) == tuple(ret[3:7]) and int(ret[7]) == 203, 'actual original RET0/nonvolatile/error')
    require(returned['pc'] == entry['return_pc'] and returned['entry_esp'] == entry['esp'] and returned['esp'] == entry['esp'] + 4 and all(returned[r] == entry[r] for r in ('ebx', 'esi', 'edi', 'ebp')), 'native actual RET0/nonvolatile return')
    original_fp = semantic(fp((original / ('x87-context-%03d.bin' % seq)).read_bytes()))
    require(original_fp == semantic(fp((original / 'checksum-return-fp.bin').read_bytes())) and all(semantic(row['fp']) == original_fp and row['last_error'] == 203 for row in (entry, returned, before, after, next_entry)), 'whole actual architectural FP/errors')
    start, length = names['process_compiler_options']
    for row in (entry, before, next_entry):
        ret_pc = row['return_pc']
        code = bytes(d.elf_byte(native, ret_pc - 5 + i) for i in range(5))
        require(start < ret_pc < start + length and code[0] == 0xe8 and ret_pc + struct.unpack_from('<i', code, 1)[0] == row['pc'], 'native actual CALL/target owner')
    require(before['pc'] == names['recon_setlocale'][0] and before['args'][0] == 2 and before['args'][1] and before['locale_bytes'] == '' and before['formal_inventory'] == [dict(name='category', type='int'), dict(name='locale', type='const char *')], 'native actual public locale input/formals')
    require(after['pc'] == before['return_pc'] and after['entry_esp'] == before['esp'] and after['esp'] == before['esp'] + 4 and all(after[r] == before[r] for r in ('ebx', 'esi', 'edi', 'ebp')), 'public cdecl8/RET0/nonvolatile ABI')
    for label, row in (('before', before), ('after', after), ('next', next_entry)):
        old = original_locale['phases'][label]
        current = row['locale']
        require([current['mb_cur_max'], current['lc_codepage'], *current['lc_handle'], current['mb_codepage']] == [old['words'][1], old['words'][2], *old['words'][3:9], old['words'][10]] and old['words'][9] == 0, 'isolated exact locale scalars/MB-zero scope')
        require(current['mb_gate'] == old['words'][9] and current['selection'] == dict(bound=1, calls=0 if label == 'before' else 1, reference=reference, result=0 if label == 'before' else after['eax']), 'actual independent MB gate and once-only receipt/call/result inventory')
        require(current['errors'] == old['errors'], 'native original errno/DOS/LastError')
        for name in ('mbctype', 'ctype', 'old-ctype'):
            require(bytes.fromhex(current[name.replace('-', '_')]) == (original / ('locale-' + label + '-' + name + '.bin')).read_bytes(), 'full current/retained table identity: ' + name)
        require(bytes.fromhex(current['result_bytes']) == (original / ('locale-' + label + '-result.bin')).read_bytes(), 'complete returned string bytes/lifetime')
        for name in ('CRT-argv', 'CRT-environment'):
            require((native / ('locale-native-' + label + '-' + name + '.bin')).read_bytes() == (native / ('native-crt-argv.bin' if name == 'CRT-argv' else 'native-crt-environment.bin')).read_bytes(), 'native actual private ordered vector storage/lifetime')
    old, new, later = (row['locale'] for row in (before, after, next_entry))
    require(old['pctype'] == old['old_pctype'] == old['static_ctype'] + 2 and old['result_pointer'] == 0 and new['pctype'] != old['pctype'] and new['old_pctype'] == old['pctype'] and new['result_pointer'] == after['eax'] and new == later, 'native old/new table and result storage identities/lifetime')
    return dict(code_bytes=236, checksum_semantic_BSS_bytes=768, complete_tables=True, return_value_claimed=False, nonzero_reset_and_overwrite=bool(seed), isolated_locale_original_receipt=original_locale['original_receipt_sha256'], unknown_initialized_bytes=6543)
