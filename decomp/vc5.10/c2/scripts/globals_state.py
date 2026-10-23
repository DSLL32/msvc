"""Typed reached-data view after this case's global IL has been consumed.

This walks symbol/section/filename data, not machine control flow. It preserves
all4096 arena bytes, normalizing only documented pointer fields. Unknown kinds,
pools, out-of-arena pointers or string identities fail rather than disappear.
"""
import struct

ROOTS = {
    'pools': (0x48e020, 'c2_pools'), 'free_blocks': (0x48e000, 'c2_free_blocks'),
    'types': (0x497fd8, 'c2_types'), 'scope': (0x48fb68, 'global_scope'),
    'section_head': (0x497dd0, 'section_head'), 'section_tail': (0x497db8, 'section_tail'),
    'filenames': (0x4973b8, 'filenames'), 'pending': (0x4973d4, 'pending_functions'),
    'insertion': (0x4973f0, 'pending_insertion'), 'context': (0x497f58, 'section_context'),
    'template': (0x497308, 'symbol_template'), 'input_name': (0x496a00, 'symbol_name'),
    'decorated_name': (0x497460, 'decorated_name'), 'source_pool': (0x48e050, 'c2_pools[4]'),
}


def complete_globals(read, addresses):
    def u32(address): return struct.unpack('<I', read(address, 4))[0]
    pool_words = struct.unpack('<12I', read(addresses['pools'], 48))
    head, cursor, remaining = pool_words[:3]
    assert head and cursor - head == 1892 and remaining == 2204
    assert pool_words[3:] == (0,) * 9 and read(addresses['source_pool'], 12) == bytes(12)
    assert u32(addresses['free_blocks']) == 0 and u32(addresses['types']) == head + 8
    block = bytearray(read(head, 4096))
    assert block[:8] == struct.pack('<II', 0, 4088)
    context = u32(addresses['context'])
    assert context and not head <= context < head + 4096
    strings, string_pointers = [], {}

    def arena_pointer(pointer):
        if not pointer: return 0
        assert head + 8 <= pointer < cursor, 'pointer outside allocated arena'
        return 0x10000000 + pointer - head

    def string(pointer):
        assert pointer
        value = bytearray()
        for i in range(2059):
            byte = read(pointer + i, 1)
            if byte == b'\0': break
            value += byte
        else: raise AssertionError('unbounded reached string')
        text = value.decode('ascii')
        if head <= pointer < cursor:
            return arena_pointer(pointer), text
        if pointer not in string_pointers:
            string_pointers[pointer] = 0x20000000 + len(strings)
            strings.append(text)
        return string_pointers[pointer], text

    def patch(at, value): struct.pack_into('<I', block, at - head, value)
    scope = bytearray(read(addresses['scope'], 4100))
    assert scope[:4] == bytes(4)
    objects = {}
    for bucket in range(1024):
        pointer = struct.unpack_from('<I', scope, 4 + bucket * 4)[0]
        struct.pack_into('<I', scope, 4 + bucket * 4, arena_pointer(pointer))
        chain = set()
        while pointer:
            assert pointer not in chain and pointer not in objects, 'duplicate/cyclic symbol'
            chain.add(pointer)
            kind = read(pointer + 4, 1)[0]
            assert kind in (4, 9), 'new symbol kind needs an explicit data model'
            size = 86 if kind == 9 or not (u32(pointer + 54) & 0x4000) else 110
            assert pointer + size <= cursor and u32(pointer + 40) & 1023 == bucket
            objects[pointer] = (kind, size)
            pointer = u32(pointer)
    assert len(objects) == 17
    inventory = []
    for pointer in sorted(objects):
        kind, size = objects[pointer]
        patch(pointer, arena_pointer(u32(pointer)))
        name_pointer, name = string(u32(pointer + 24)); patch(pointer + 24, name_pointer)
        row = dict(offset=pointer-head, kind=kind, size=size, serial=u32(pointer+40), name=name)
        if kind == 9:
            class_pointer, class_name = string(u32(pointer+54)); patch(pointer+54, class_pointer)
            patch(pointer+50, arena_pointer(u32(pointer+50)))
            combine = read(pointer+67, 1)[0]
            if combine == 7:
                group = u32(pointer+36)
                assert read(group,12) == bytes(12), 'COMDAT group reserved/count state differs'
                patch(pointer+36, arena_pointer(group))
                patch(pointer+74, arena_pointer(u32(pointer+74)))
                patch(pointer+78, arena_pointer(u32(pointer+78)))
            else:
                assert combine == 2 and u32(pointer+36) == context
                patch(pointer+36, 0xf0000001)
            row['class_name'] = class_name
        elif size == 110:
            patch(pointer+102, arena_pointer(u32(pointer+102)))
        inventory.append(row)
    expected = [
        (72,9,86,5,'_TEXT'),(160,9,86,2,'_DATA'),(248,9,86,3,'CONST'),
        (336,9,86,4,'_BSS'),(424,9,86,9,'_TLS'),
        (548,4,110,85,'_mix_unsigned'),(676,9,86,146,'_mix_unsigned'),
        (776,4,110,92,'_mix_signed'),(900,9,86,147,'_mix_signed'),
        (1000,4,110,100,'_nested_loops'),(1128,9,86,148,'_nested_loops'),
        (1228,4,110,122,'_pointer_aliasing'),(1360,9,86,149,'_pointer_aliasing'),
        (1460,4,110,135,'_floating_point'),(1588,9,86,150,'_floating_point'),
        (1688,4,86,80,'_floating_call'),(1792,4,86,79,'_observe')]
    assert [(r['offset'],r['kind'],r['size'],r['serial'],r['name'])for r in inventory] == expected
    sections, pointer = [], u32(addresses['section_head'])
    while pointer:
        assert len(sections) < 10 and objects[pointer][0] == 9
        sections.append(pointer-head); pointer = u32(pointer+50)
    assert sections == [72,160,248,336,424,676,900,1128,1360,1588]
    assert u32(addresses['section_tail']) == head+1588
    pending, pointer = [], u32(addresses['pending'])
    while pointer:
        assert len(pending) < 5 and objects[pointer] == (4,110)
        pending.append((pointer-head,u32(pointer+86)));pointer=u32(pointer+102)
    assert pending == [(548,1132),(776,1288),(1000,1453),(1228,1910),(1460,2413)]
    assert u32(addresses['insertion']) == head+1330
    pointer = u32(addresses['filenames'])
    assert pointer == head+536 and u32(pointer) == 86 and u32(pointer+8) == 0
    name_pointer, name = string(u32(pointer+4)); assert name == 'numeric-control-flow.c'
    patch(pointer+4,name_pointer);patch(pointer+8,0)
    template = bytearray(read(addresses['template'],64))
    pointer = struct.unpack_from('<I',template,24)[0]
    struct.pack_into('<I',template,24,string(pointer)[0])
    # All unaddressed padding, reserved fields, string bytes and unused arena
    # tail remain in this normalized4096-byte view and must match exactly.
    return dict(pool_cursor=1892,pool_remaining=2204,pool_bytes=block.hex(),scope_bytes=scope.hex(),
        objects=inventory,external_strings=strings,sections=sections,pending=pending,
        insertion_offset=1330,filename_offset=536,template_bytes=template.hex(),
        input_bytes=read(addresses['input_name'],2058).hex(),
        decorated_bytes=read(addresses['decorated_name'],2059).hex())
