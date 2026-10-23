"""Bound the reached first virtual table through its class-owned pointer path."""


def validate(function, formal, anchors, native, words):
    assert function == '_begin_virtual_entries@8'
    assert formal['name'] == 'table' and formal['stack_index'] == 1
    binding = formal['binding']
    assert binding['kind'] == 'class-first-virtual-table'
    assert binding['root_address'] == 0x8c1fc
    names = ['class_root', 'class_info', 'virtual_tables', 'table_items', 'first_table']
    assert binding['path'] == names and binding['offsets'] == [32, 8, 8, 0]
    original_previous = native_previous = None
    for index, name in enumerate(names):
        item = anchors[name]
        assert item['bytes'] == item['read_bytes'] == 4 and item['read_succeeded'], 'missing exact ownership pointer read'
        assert item['value'] and native[name], 'NULL or incomplete table ownership'
        if not index:
            assert item['original'] == binding['root_address'], 'class root address differs'
            assert native[name+'_address'] == native['class_contexts_address'], 'native class slot root differs'
        else:
            offset = binding['offsets'][index - 1]
            assert original_previous <= 0xffffffff-offset and native_previous <= 0xffffffff-offset
            assert item['original'] == original_previous + offset, 'original ownership pointer link differs'
            assert native[name+'_address'] == native_previous + offset, 'native ownership pointer link differs'
        original_previous, native_previous = item['value'], native[name]
    assert words[1] == original_previous, 'actual original table argument differs from class owner'
    assert formal['native_raw'] == native_previous, 'actual native table argument differs from class owner'
