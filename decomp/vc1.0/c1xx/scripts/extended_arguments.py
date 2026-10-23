"""The actual twelve-byte by-value Extended formal of its arena allocator."""

def validate(function,formal,native,words):
    assert function=='_save_extended_in_arena@16' and formal['name']=='value' and formal['stack_index']==0
    assert formal['binding']==dict(kind='extended-stack-value',stack_indices=[0,1,2],native_fields=['value_low','value_high','value_tail'])
    original=[words[i] for i in [0,1,2]]
    actual=[native[field] for field in ['value_low','value_high','value_tail']]
    assert original==actual, 'actual twelve-byte Extended value differs'
    return dict(original=original,native=actual)
