"""One initialized iterator value forwarded by the actual immediate caller.

This binds a role, not the extent or contents of the generated function symbol.
No extra stack memory is read by the observer.
"""
import re
from binary import PE
from case_paths import ROOT


def validate(function, formal, native, words, receipt, events, config, events_sha):
    assert function == '_emit_declaration_thunk@8'
    assert formal['name'] == 'function' and formal['stack_index'] == 1
    binding = formal['binding']
    assert binding['kind'] == 'forwarded-declaration-thunk'
    assert binding['original_owner_function'] == '_finish_declarations'
    assert binding['original_caller_rva'] == 0x3dd99
    assert binding['iterator_bytes'] == 20 and binding['value_offset'] == 16
    assert binding['load_instruction'] == 0x4dd8a and binding['push_instruction'] == 0x4dd8e
    assert binding['call_instruction'] == 0x4dd94
    assert binding['pointee_credit'] == 'none from forwarding alone'
    for name, extent in (('_finish_declarations', [0x4dd60,0x4ddab]),
                         ('_begin_hash_iteration@8', [0x4ee60,0x4ee7b]),
                         ('_next_hash_entry@4', [0x4ee80,0x4eed3])):
        assert name in config['accepted']
        owner = next(row for row in config['driver'] if row['name'] == name)
        assert [owner['start'],owner['end']] == extent
    assert config['target']['base'] == 0x10000, 'VA/RVA basis changed'
    assert re.findall(r'^caller ([0-9a-f]{8})$', events, re.M) == ['0003dd99']
    projection = receipt['ownership_entry_projection']
    assert '_finish_declarations' in projection['observed'] and projection['events_sha256'] == events_sha
    assert re.search(r'^entry _finish_declarations rva 0003dd60 byte [0-9a-f]{2}$',events,re.M)
    image = PE(ROOT/'toolchains/1.0-nt/bin/C1XX3232.EXE')
    for address, code in ((0x4dd8a,'8b442410508b44241050e817000000'),
                          (0x4eeb2,'8b41045e8b4004894110')):
        start = image.offset(address)
        assert image.data[start:start+len(code)//2].hex() == code, 'actual iterator producer/forwarding code changed'
    assert formal['original_raw'] == words[1]
    assert binding['native_owner_field'] == 'caller_function'
    owner = native['caller_locals']['caller_function']
    assert owner['function'] == 'finish_declarations' and owner['local'] == 'iterator'
    assert owner['fields'] == ['value'], 'wrong initialized iterator field'
    assert owner['value'] == formal['native_raw'] == native[formal['native_field']], 'actual caller iterator.value differs'
