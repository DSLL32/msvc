"""Only real fold_binary_constant locals passed to its floating evaluator."""

def validate(function,formal,owner):
    assert function=='_fold_floating_binary@12'
    index,offset,local={'left':(1,56,'left_value'),'right':(2,68,'right_value')}[formal['name']]
    binding=formal['binding']
    assert formal['stack_index']==index and binding['original_stack_offset']==offset
    assert binding['original_register']=='esp' and binding['native_owner_address']
    assert binding['original_caller_rva']==0x1cfcc and binding['original_owner_function']=='_fold_binary_constant@12'
    assert owner['function']=='fold_binary_constant' and owner['local']==local
    assert owner['value'] is None and binding['address_only'] and not binding.get('output_only'), 'only defined caller input addresses are observed'
    assert 'original_prestate_word_anchor' not in binding
