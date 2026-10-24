"""Actual initialized tail words of the reached virtual-member thunk keys."""


def validate(function, anchors, native, words):
    assert function == '_equal_tail_word_pairs'
    for index, name in enumerate(('left', 'right')):
        for member, offset in (('kind', 4), ('displacement', 8)):
            key = name + '_' + member
            word = anchors[key]
            assert word['bytes'] == word['read_bytes'] == 4 and word['read_succeeded'], 'missing complete initialized key word'
            assert 0 < words[index] <= 0xffffffff - 12, 'actual key span wraps'
            assert word['original'] == words[index] + offset, 'key word does not belong to actual argument'
            assert word['value'] == native[key], 'actual initialized tail word differs'
