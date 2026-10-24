"""One actual immediate-caller adjustment formal forwarded without a register.

The pinned load/PUSH def-use establishes the original role. The fresh raw word
and native actual caller formal bind the pointers; this is no pointee dump or
claim that forwarding alone initializes an IR object.
"""
import re
from binary import PE
from case_paths import ROOT


def validate(function, formal, native, words, receipt, events, config, events_sha):
    assert function == '_make_multiple_inheritance_method_pointer@12'
    assert formal['name'] == 'adjustment' and formal['stack_index'] == 2
    binding = formal['binding']
    assert binding['kind'] == 'forwarded-method-adjustment'
    assert binding['original_owner_function'] == '_make_compact_method_pointer@12'
    assert binding['original_caller_rva'] == 0x3a298
    assert binding['caller_parameter_offset'] == 12
    assert binding['load_instruction'] == 0x4a288 and binding['push_instruction'] == 0x4a28c
    assert binding['call_instruction'] == 0x4a293
    assert binding['pointee_credit'] == 'none from forwarding alone'
    owner_name = binding['original_owner_function']
    assert owner_name in config['accepted']
    owner = next(row for row in config['driver'] if row['name'] == owner_name)
    assert [owner['start'], owner['end']] == [0x4a260, 0x4a29c]
    assert re.findall(r'^caller ([0-9a-f]{8})$', events, re.M) == ['0003a298']
    projection = receipt['ownership_entry_projection']
    assert owner_name in projection['observed'] and projection['events_sha256'] == events_sha
    assert re.search(r'^entry '+re.escape(owner_name)+r' rva 0003a260 byte [0-9a-f]{2}$', events, re.M), 'actual accepted forwarding caller missing'
    image = PE(ROOT/'toolchains/1.0-nt/bin/C1XX3232.EXE')
    start = image.offset(0x4a288)
    assert image.data[start:start+16].hex() == '8b442410508b4424105056e808000000', 'pinned load/PUSH/CALL changed'
    assert formal['original_raw'] == words[2], 'actual original adjustment word differs'
    assert binding['native_owner_field'] == 'caller_adjustment'
    native_owner = native['caller_locals'][binding['native_owner_field']]
    assert native_owner['function'] == 'make_compact_method_pointer'
    assert native_owner['local'] == 'adjustment' and native_owner.get('fields', []) == []
    assert native_owner['value'] == formal['native_raw'] == native[formal['native_field']], 'actual native forwarding formal differs'
