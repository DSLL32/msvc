"""Bound the first destination-side goto cleanup call to its real owners.

This is one observed caller path, not a general record/heap traversal. The
accepted label writer owns a 24-byte record and initializes previous/+12 and
label/+16 before resolving this goto; the resolver initializes its expression
local to NULL and advances its actual `to` formal once before this first call.
The observation addresses are probes and must match THIS capture's fresh tail.
"""


def validate(function, formals, anchors, native):
    assert function == '_walk_goto_cleanup@16'
    by_name = {item['name']: item for item in formals}
    assert set(by_name) == {'expression', 'record', 'mode', 'label'}
    assert [by_name[n]['stack_index'] for n in ('expression','record','mode','label')] == [0,1,2,3]
    observed = {}
    for name in ('label_tail','tail_previous','tail_label','expression_prestate'):
        word = anchors[name]
        assert word['bytes'] == word['read_bytes'] == 4 and word['read_succeeded'], 'missing complete goto input read: '+name
        assert 0 < word['original'] <= 0xffffffff-3, 'goto input read address wraps'
        observed[name] = word
    root = observed['label_tail']
    assert root['original'] == 0xa0a04, 'wrong fresh goto label-tail global'
    assert 0 < root['value'] <= 0xffffffff-23, 'goto label owner span wraps'
    previous, label = observed['tail_previous'], observed['tail_label']
    assert previous['original'] == root['value']+12, 'goto previous field outside actual fresh label owner'
    assert label['original'] == root['value']+16, 'goto label field outside actual fresh label owner'
    assert by_name['record']['original_raw'] == previous['value'] != 0, 'wrong original destination record owner'
    assert by_name['label']['original_raw'] == label['value'] != 0, 'wrong original destination label owner'
    for name, anchor_name, native_field in (('record','tail_previous','tail_previous'),('label','tail_label','tail_label')):
        binding=by_name[name]['binding']
        assert binding['kind']=='pointer-alias' and binding['original_anchor_name']==anchor_name and binding['native_anchor_field']==native_field
    expression = by_name['expression']
    binding = expression['binding']
    assert binding['kind']=='caller-owned-pointer' and binding['original_register']=='eax' and binding['native_owner_address']
    assert binding['original_caller_rva']==0x31cbb and binding['original_owner_function']=='_resolve_goto_cleanup@12', 'wrong goto cleanup caller'
    assert binding['native_owner_function']=='resolve_goto_cleanup' and binding['native_owner_local']=='expression'
    assert binding['native_owner_field']=='caller_expression' and 'original_stack_offset' not in binding
    assert not binding.get('output_only') and not binding.get('address_only'), 'incoming cleanup expression is a defined input'
    prestate=observed['expression_prestate']
    assert prestate['original']==expression['original_raw'], 'cleanup prestate read is not actual first argument'
    assert 0 < native['label_tail'] <= 0xffffffff-23 and native['label_record_bytes']==24, 'wrong native label owner span'
    assert native['tail_kind'] & 0x3f == 0x35, 'native tail is not actual label record'
    assert native['tail_previous_address']==native['label_tail']+12 and native['tail_label_address']==native['label_tail']+16, 'native label field offsets differ'
    for formal, field, local in (('record','tail_previous','to'),('label','tail_label','label')):
        owner=native['caller_locals']['caller_'+local]
        assert owner['function']=='resolve_goto_cleanup' and owner['local']==local and owner.get('fields',[])==[], 'wrong actual goto caller local'
        assert owner['value']==native[field]==by_name[formal]['native_raw'] != 0, 'native destination field/caller owner differs'
    owner=native['caller_locals']['caller_expression']
    assert owner['function']=='resolve_goto_cleanup' and owner['local']=='expression' and owner.get('fields',[])==[]
    assert owner['address']==expression['native_raw'], 'wrong actual native expression slot'
    assert owner['value']==prestate['value']==native['pointee_expression']==0, 'initialized incoming goto expression differs'
    assert by_name['mode']['original_raw']==by_name['mode']['native_raw']==1, 'this owner proof is the actual destination-mode1 call'
