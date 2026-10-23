"""Compare encountered grammar/vocabulary data with the original executable."""
import struct
from win32_indexes import indexes, publics

original, rebuilt, cfg, original_records, rebuilt_records = indexes()
names = publics()
state_addresses, state_reverse_addresses = {}, {}


def read(pe, address, size):
    section = next(s for s in pe.sections if
                   s['rva'] <= address-pe.base < s['rva']+s['virtual_size'])
    if section['name'] == '.bss':
        return bytes(size)
    return pe.read(address, size)


def word(pe, address):
    return struct.unpack('<I', read(pe, address, 4))[0]


def vocabulary(pe, address, kind):
    result = []
    while True:
        name = word(pe, address)
        if not name:
            return result
        if kind == 'keyword':
            result.append((pe.string(name), word(pe, address+4)))
            address += 8
        else:
            result.append((pe.string(name), *struct.unpack('<III', read(pe, address+4, 12))))
            address += 16


def grammar(pe, address):
    rows = []
    while True:
        pattern, target, result, action, reserved = struct.unpack('<IIbBH', read(pe, address, 12))
        if not action:
            return rows
        rows.append((pe.string(pattern), target, result, action, reserved))
        address += 12


def compare_grammar(old_address, new_address):
    old_rows = grammar(original, old_address)
    new_rows = grammar(rebuilt, new_address)
    assert len(old_rows) == len(new_rows), 'option rule count differs'
    forward, backward = {}, {}

    def alias(old, new):
        assert forward.setdefault(old, new) == new, 'shared option target was split'
        assert backward.setdefault(new, old) == old, 'distinct option targets were merged'
        for offset in range(4):
            assert state_addresses.setdefault(old+offset, new+offset) == new+offset, 'option/state alias was split'
            assert state_reverse_addresses.setdefault(new+offset, old+offset) == old+offset, 'distinct option/state objects were merged'

    def scalar(old, new):
        alias(old, new)
        assert word(original, old) == word(rebuilt, new), 'option default differs'

    for old, new in zip(old_rows, new_rows):
        assert (old[0], *old[2:]) == (new[0], *new[2:]), 'option grammar differs'
        old_target, new_target, action = old[1], new[1], old[3]
        alias(old_target, new_target)
        if action == 0x22:
            a, b = word(original, old_target), word(rebuilt, new_target)
            assert bool(a) == bool(b), 'option string default nullness differs'
            if a:
                assert original.string(a) == rebuilt.string(b), 'option string default differs'
        elif action == 0x23:
            while True:
                a = struct.unpack('<III', read(original, old_target, 12))
                b = struct.unpack('<III', read(rebuilt, new_target, 12))
                assert a[:2] == b[:2], 'Z option grammar differs'
                if not a[0]:
                    break
                scalar(a[2], b[2])
                old_target += 12
                new_target += 12
        else:
            scalar(old_target, new_target)
    return len(old_rows)


for table in cfg.get('tables', []):
    old, new, kind = table['original'], names[table['name']], table['kind']
    if kind == 'token_membership':
        values = list(read(rebuilt, new, table['count']))
        assert values[-1] == 0 and all(values[:-1]) and len(set(values[:-1])) == len(values)-1, 'token membership sentinel/duplicates differ'
        expected = {index for index,value in enumerate(read(original, old, table['domain'])) if value & table['mask']}
        assert set(values[:-1]) == expected, 'encountered lexical property membership differs: '+table['name']
        print('%d contextual operator-name token memberships and sentinel match the original.' % len(expected))
    elif kind == 'option':
        count = compare_grammar(old, new)
        print('%d option rules and their aliases/defaults match the original.' % count)
    elif kind == 'identifiers':
        forward, backward = {}, {}
        count = 0
        while word(original, old):
            a, b = word(original, old), word(rebuilt, new)
            assert b, 'special identifier table ended early'
            assert original.string(word(original, old+4)) == rebuilt.string(word(rebuilt, new+4)), \
                'special identifier spelling differs'
            assert forward.setdefault(a, b) == b and backward.setdefault(b, a) == a, \
                'special identifier target identity differs'
            assert read(original, a, 4) == read(rebuilt, b, 4), 'special identifier default differs'
            count += 1
            old += 8
            new += 8
        assert read(rebuilt, new, 8) == bytes(8), 'special identifier sentinel differs'
        print('%d special identifiers and their target identities match the original.' % count)
    elif kind == 'directives':
        groups = read(original, table['groups'], 29)
        rows = []
        for index in range(table['count']):
            prefixes = [chr(ch) for ch in range(ord('_'), ord('z')+1)
                        if groups[ch-ord('_')] <= index < groups[ch-ord('_')+1]]
            assert len(prefixes) == 1, 'directive suffix has no unique first-character group'
            spelling = prefixes[0] + original.string(word(original, old+index*4))
            token = word(original, table['ids']+index*4)
            actual = rebuilt.string(word(rebuilt, new+index*8))
            assert actual == spelling, 'directive spelling differs: '+spelling
            assert read(rebuilt, new+index*8+4, 1)[0] == token, 'directive kind differs: '+spelling
            rows.append(spelling)
        for ch in range(ord('_'), ord('z')+1):
            lower = sum(name[0] < chr(ch) for name in rows)
            upper = sum(name[0] <= chr(ch) for name in rows)
            assert [lower, upper] == list(groups[ch-ord('_'):ch-ord('_')+2]), 'directive group bounds differ'
        print('%d logical preprocessor directive spellings, kinds and first-character groups match the original.' % len(rows))
    elif kind == 'bytes':
        assert read(original, old, table['bytes']) == read(rebuilt, new, table['bytes']), \
            'byte vocabulary differs: '+table['name']
        print('%d bytes of %s match the original.' % (table['bytes'], table['name']))
    elif kind == 'string_pointers':
        forward, backward = {}, {}
        for index in range(table['count']):
            a, b = word(original, old+index*4), word(rebuilt, new+index*4)
            assert original.string(a) == rebuilt.string(b), 'CRT string default differs'
            assert forward.setdefault(a, b) == b and backward.setdefault(b, a) == a, \
                'CRT string identity differs'
        print('%d CRT string defaults and identities match the original.' % table['count'])
    elif kind == 'function_pointers':
        records = {record['name']: record for record in original_records}
        for index, function in enumerate(table['functions']):
            a, b = word(original, old+index*4), word(rebuilt, new+index*4)
            if function is None:
                assert a == b == 0, 'callback table sentinel differs: '+table['name']
            else:
                assert a == records[function]['start'] and b == names[function], \
                    'callback identity differs: '+table['name']+' '+function
        print('%d contextual function pointers/sentinel match the original.' % len(table['functions']))
    elif kind == 'state':
        for field in table['fields']:
            for offset in range(field['bytes']):
                a, b = field['original'] + offset, names[field['name']] + offset
                assert state_addresses.setdefault(a, b) == b, 'shared frontend state was split: %s at original %x (expected %x, previously mapped %x)' % (field['name'], a, b, state_addresses[a])
                assert state_reverse_addresses.setdefault(b, a) == a, 'distinct frontend state was merged: '+field['name']
            if 'pointer_to' in field:
                assert field['bytes'] == 4
                assert word(original, field['original']) == field['original_target'], \
                    'original frontend pointer target differs: '+field['name']
                assert word(rebuilt, names[field['name']]) == names[field['pointer_to']] + field.get('target_offset', 0), \
                    'rebuilt frontend pointer target differs: '+field['name']
            else:
                assert read(original, field['original'], field['bytes']) == \
                       read(rebuilt, names[field['name']], field['bytes']), \
                    'initial frontend state differs: '+field['name']
        print('%d shared frontend state defaults match the original.' % len(table['fields']))
    elif kind == 'tokens':
        for index in range(table['count']):
            a, b = word(original, old+index*4), word(rebuilt, new+index*4)
            assert bool(a) == bool(b), 'token spelling nullness differs'
            if a:
                assert original.string(a) == rebuilt.string(b), 'token spelling differs: %d' % index
        print('%d token spelling slots match the original.' % table['count'])
    elif kind == 'schemas':
        for index in range(table['count']):
            a = word(original, old + index*table['original_stride'])
            b = word(rebuilt, new + index*table['rebuilt_stride'])
            assert bool(a) == bool(b), 'IR schema nullness differs'
            if a:
                assert original.string(a) == rebuilt.string(b), 'IR schema differs: %02x' % index
        print('%d IR record field schemas match the original.' % table['count'])
    elif kind == 'runtime_functions':
        for index in range(table['count']):
            a, b = old + index*12, new + index*12
            assert original.string(word(original, a)) == rebuilt.string(word(rebuilt, b)), 'runtime function name differs'
            assert read(original, a+4, 8) == read(rebuilt, b+4, 8), 'runtime function attributes differ'
        print('%d runtime function specifications match the original.' % table['count'])
    elif kind == 'packed_options':
        for index in range(table['count']):
            a, b = word(original, old+index*4), word(rebuilt, new+index*4)
            assert bool(a) == bool(b), 'packed-option slot nullness differs'
            if a:
                for offset in range(4):
                    assert state_addresses.setdefault(a+offset, b+offset) == b+offset, 'packed-option state was split'
                    assert state_reverse_addresses.setdefault(b+offset, a+offset) == a+offset, 'packed-option state was merged'
                assert read(original, a, 4) == read(rebuilt, b, 4), 'packed-option default differs: %d' % index
        print('%d packed options, aliases and defaults match the original.' % table['count'])
    elif kind == 'declaration_codes':
        seen = set()
        for index in range(table['count']):
            token, value = struct.unpack('<II', read(rebuilt, new+index*8, 8))
            assert token not in seen, 'duplicate declaration code'
            seen.add(token)
            assert word(original, old+token*4) == value, 'declaration code differs: %d' % token
        print('%d contextual declaration codes match the original vocabulary.' % table['count'])
    elif kind == 'expression_codes':
        declarations = names['_declaration_codes']
        count = word(rebuilt, names['_declaration_code_count'])
        meanings = dict(struct.unpack('<II', read(rebuilt, declarations+i*8, 8))
                        for i in range(count))
        count = word(rebuilt, names['_expression_record_rule_count'])
        seen = set()
        for index in range(count):
            token, value = struct.unpack('<II', read(rebuilt, new+index*8, 8))
            assert token not in seen, 'duplicate expression record override'
            seen.add(token)
            meanings[token] = value
        for token in range(table['count']):
            assert meanings.get(token, 0x9e) == word(original, old+token*4), \
                'expression record differs for token %02x' % token
        for token in (3, 0x46, 0x67, 0x6e, 0x76, 0x7a, 0x81, 0x8f, 0xa8):
            assert read(original, 0xa1d47+token, 1)[0] & 0x80, \
                'expression special handler lacks its native flag'
        print('%d contextual expression/declaration record meanings match the original.' % table['count'])
    elif kind == 'configuration':
        for offset in range(table['bytes']):
            assert state_addresses.setdefault(old+offset, new+offset) == new+offset, 'configuration/state alias was split'
            assert state_reverse_addresses.setdefault(new+offset, old+offset) == old+offset, 'distinct configuration/state objects were merged'
        assert read(original, old, table['bytes']) == read(rebuilt, new, table['bytes']), 'configuration defaults differ'
        print('%d-byte typed configuration defaults match the original.' % table['bytes'])
    else:
        a, b = vocabulary(original, old, kind), vocabulary(rebuilt, new, kind)
        assert a == b, kind+' vocabulary differs'
        print('%d %s entries match the original.' % (len(a), kind))
