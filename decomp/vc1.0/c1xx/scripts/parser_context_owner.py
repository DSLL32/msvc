"""The consumed right[-1].context owner for one member-pointer parser action."""
import re

STACK_BASE = 0x94ffc
STACK_ELEMENTS = 220


def validate(function, formal, anchors, native, words, receipt, events, config, events_sha):
    assert function == '_resolve_member_pointer_class@8'
    assert formal['name'] == 'name' and formal['stack_index'] == 1
    binding = formal['binding']
    assert binding['kind'] == 'parser-context-slot'
    registers = [r for r in receipt['ownership_register_observations'] if r['register'] == 'esi']
    assert len(registers) == 1 and registers[0]['read_succeeded'], 'missing parser right owner anchor'
    right = registers[0]['value']
    assert re.findall(r'^ownership-anchor esi ([0-9a-f]{8})$', events, re.M) == ['%08x' % right]
    assert re.findall(r'^caller ([0-9a-f]+)$', events, re.M) == ['00017938']
    owner_name = '_parse_translation_unit'
    assert owner_name in config['accepted']
    projection = receipt['ownership_entry_projection']
    assert owner_name in projection['observed'] and projection['events_sha256'] == events_sha
    owner = next(row for row in config['driver'] if row['name'] == owner_name)
    assert re.search(r'^entry '+owner_name+r' rva %08x byte [0-9a-f]{2}$' % (owner['start']-config['target']['base']), events, re.M)
    assert STACK_BASE + 8 <= right < STACK_BASE + STACK_ELEMENTS * 8 and (right - STACK_BASE) % 8 == 0, 'parser right outside allocated semantic stack'
    word = anchors['parser_context']
    assert word['bytes'] == word['read_bytes'] == 4 and word['read_succeeded'], 'missing exact parser context read'
    assert word['original'] == right - 8, 'parser input is not actual right[-1] slot'
    assert word['value'] == words[1] == formal['original_raw'], 'actual original parser context differs'
    name = native['caller_locals']['caller_name']
    caller_right = native['caller_locals']['caller_right']
    assert name['function'] == caller_right['function'] == 'parse_translation_unit'
    assert name['local'] == caller_right['local'] == 'right'
    assert name['index'] == -1 and name['fields'] == ['context'], 'native parser slot selector differs'
    assert caller_right['fields'] == [] and 'index' not in caller_right
    base = name['stack_base']
    pointer = caller_right['value']
    assert name['stack_elements'] == STACK_ELEMENTS
    assert base + 8 <= pointer < base + STACK_ELEMENTS * 8 and (pointer - base) % 8 == 0, 'native parser right outside allocated semantic stack'
    assert name['slot_address'] == pointer - 8, 'native parser slot owner differs'
    assert formal['native_raw'] == name['value'], 'actual native parser context differs'
    output_registers = [r for r in receipt['ownership_register_observations'] if r['register'] == 'eax']
    assert len(output_registers) == 1 and output_registers[0]['read_succeeded'], 'missing actual output owner anchor'
    output_pointer = output_registers[0]['value']
    assert re.findall(r'^ownership-anchor eax ([0-9a-f]{8})$', events, re.M) == ['%08x' % output_pointer]
    assert words[0] == output_pointer, 'actual output differs from caller local anchor'
    output = native['caller_locals']['caller_result']
    assert output['function'] == 'parse_translation_unit' and output['local'] == 'member_class' and output['fields'] == [], 'native output local owner differs'
    assert output['value'] is None and output['address'] == native['raw_result'], 'unwritten output prestate must remain address-only'
