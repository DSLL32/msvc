"""Pure comparator for captured224-byte operand-decoder entry states."""

def compare(observations, actual):
    assert isinstance(observations,list) and len(observations)==1, 'Missing/nonunique original decoder observation'
    expected=observations[0]
    for state in (expected,actual):
        assert set(state)=={'event','words','operand','cursor'}
        assert state['event']=='decoder_state'
        assert isinstance(state['words'],list) and len(state['words'])==56
        assert all(type(word) is int and 0<=word<=0xffffffff for word in state['words'])
        assert state['words'][19]==1, 'Instruction byte pointer must be non-NULL'
        assert state['operand'] in (1,2,3)
        assert type(state['cursor']) is int and 0<=state['cursor']<=0xffffffff
    assert expected==actual, (expected,actual)
    return dict(status='pass',scalar_words=54,pointer_relations=2,operand=expected['operand'],cursor=expected['cursor'])
