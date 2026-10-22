"""Compare encountered grammar/vocabulary data with the original executable."""
import struct
from compare import indexes, publics

original, rebuilt, cfg, _, _ = indexes()
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
    if kind == 'option':
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
    elif kind == 'state':
        for field in table['fields']:
            for offset in range(field['bytes']):
                a, b = field['original'] + offset, names[field['name']] + offset
                assert state_addresses.setdefault(a, b) == b, 'shared frontend state was split: '+field['name']
                assert state_reverse_addresses.setdefault(b, a) == a, 'distinct frontend state was merged: '+field['name']
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
    elif kind == 'configuration':
        assert read(original, old, table['bytes']) == read(rebuilt, new, table['bytes']), 'configuration defaults differ'
        print('%d-byte typed configuration defaults match the original.' % table['bytes'])
    else:
        a, b = vocabulary(original, old, kind), vocabulary(rebuilt, new, kind)
        assert a == b, kind+' vocabulary differs'
        print('%d %s entries match the original.' % (len(a), kind))
