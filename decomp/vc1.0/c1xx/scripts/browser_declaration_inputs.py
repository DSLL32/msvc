"""Bind only the reached declaration's initialized four-byte header."""

def validate(name, anchors, checkpoint, words):
    assert name == '_fill_browser_declaration@8'
    header = anchors['browser_symbol_header']
    assert header['original'] == words[1], 'header does not belong to actual symbol formal'
    assert header['bytes'] == header['read_bytes'] == 4 and header['read_succeeded'], 'missing complete symbol header'
    assert header['value'] == checkpoint['browser_symbol_header'] == 7, 'current typed operator-new header differs'
    assert checkpoint['browser_symbol_kind'] == 7
    assert checkpoint['browser_symbol_name'] == checkpoint['operator_new_name'], 'native declaration owner differs'
