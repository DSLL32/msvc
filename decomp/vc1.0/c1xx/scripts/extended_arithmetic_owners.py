"""The current multiply call's three real caller-owned Extended pointers."""

def validate(function,formal,owner):
    assert function=='_multiply_extended'
    index,offset,address={'left':(0,108,False),'right':(1,120,False),'result':(2,40,True)}[formal['name']]
    binding=formal['binding']
    assert formal['stack_index']==index and binding['original_stack_offset']==offset
    assert binding['original_register']=='esp' and bool(binding.get('native_owner_address'))==address
    assert binding['original_caller_rva']==0x1d1e4 and binding['original_owner_function']=='_fold_floating_binary@12'
    assert owner['function']=='fold_floating_binary' and owner['local']==formal['name']
    if address:
        assert owner['value'] is None and binding['output_only'], 'result prestate is uncredited'
    else:
        assert not binding.get('output_only') and not binding.get('address_only') and owner['value']!=0, 'actual forwarded caller pointer required'
    assert 'original_prestate_word_anchor' not in binding
