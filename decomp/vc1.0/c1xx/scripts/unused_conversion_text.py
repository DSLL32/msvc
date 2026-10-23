"""One reached C1XX writer branch where a conversion-only formal is unused.

Keep its raw stack word separate: no equality is claimed for unwritten bytes.
This is not a general unknown-input exemption.
"""


def validate(function, formal, anchors, native, words):
    assert function == '_write_debug_member_name@20'
    assert formal['name'] == 'conversion_text' and formal['stack_index'] == 3
    binding = formal['binding']
    assert binding['kind'] == 'unused-conversion-text'
    assert binding['read_instruction'] == 0x6eced
    assert binding['required_absent_flag'] == 0x4000
    flags = anchors[binding['original_flags_anchor']]
    assert flags['bytes'] == flags['read_bytes'] == 2 and flags['read_succeeded'], 'missing exact member flags read'
    assert flags['original'] == words[1] + 2, 'flags do not belong to actual symbol argument'
    assert flags['value'] == native[binding['native_flags_field']], 'actual member flags differ'
    assert not flags['value'] & 0x4000, 'conversion branch may consume this formal'
    return 'uncredited-unused'
