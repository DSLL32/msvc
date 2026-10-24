"""Bind the actual include spelling to the established filename-buffer tail."""


def validate(name, anchors, native, words):
    assert name == '_search_source_include@4'
    end = anchors['filename_buffer_end']
    assert end['original'] == 0x9fc54
    assert end['bytes'] == end['read_bytes'] == 4 and end['read_succeeded']
    assert end['value'] == 0x90f14 == words[0] + 11, 'original filename tail owner differs'
    assert native['filename_buffer_end'] == native['token_owner'] + 7936
    assert native['raw_filename'] + 11 == native['filename_buffer_end'], 'native filename tail owner differs'
    for index, (offset, width, text) in enumerate(((0, 4, b'regi'), (4, 4, b'stry'),
                                                  (8, 2, b'.h'), (9, 2, b'h\0'))):
        row = anchors['include_spelling_' + str(index)]
        assert row['original'] == words[0] + offset, 'spelling observation has wrong owner'
        assert row['bytes'] == row['read_bytes'] == width and row['read_succeeded']
        assert row['value'] == native['include_spelling_' + str(index)] == int.from_bytes(text, 'little'), 'include spelling differs'
