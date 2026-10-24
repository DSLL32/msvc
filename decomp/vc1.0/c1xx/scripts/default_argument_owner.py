"""The reached default argument's one initialized expression-pointer field.

No recursive expression or neighboring field state is observed here. The
20-byte owner is the existing expression-list node, with value.pointer at12.
"""
import re


def validate(function, formal, anchors, native, words, receipt, events, config, events_sha):
    assert function=='_append_default_call_argument@8'
    assert formal['name']=='expression' and formal['stack_index']==1
    binding=formal['binding']
    assert binding['kind']=='default-argument-expression' and binding['original_register']=='ebx'
    assert binding['owner_bytes']==20 and binding['field_offset']==12
    assert binding['original_caller_rva']==0x2ad09 and binding['original_owner_function']=='_convert_call_arguments@16'
    owner_name=binding['original_owner_function']
    assert owner_name in config['accepted']
    registers=[row for row in receipt['ownership_register_observations'] if row['register']=='ebx']
    assert len(registers)==1 and registers[0]['read_succeeded'], 'missing actual default-argument owner'
    root=registers[0]['value']
    assert 0<root<=0xffffffff-19, 'original default argument owner span wraps'
    assert re.findall(r'^ownership-anchor ebx ([0-9a-f]{8})$',events,re.M)==['%08x'%root]
    assert re.findall(r'^caller ([0-9a-f]{8})$',events,re.M)==['0002ad09']
    projection=receipt['ownership_entry_projection']
    assert owner_name in projection['observed'] and projection['events_sha256']==events_sha
    owner_record=next(row for row in config['driver'] if row['name']==owner_name)
    assert re.search(r'^entry '+re.escape(owner_name)+r' rva %08x byte [0-9a-f]{2}$'%(owner_record['start']-config['target']['base']),events,re.M), 'actual accepted default owner entry missing'
    assert binding['original_observation_name']=='default_expression'
    word=anchors[binding['original_observation_name']]
    assert word['bytes']==word['read_bytes']==4 and word['read_succeeded'], 'missing complete default expression read'
    assert word['original']==root+12, 'default expression read is not actual caller owner field'
    assert word['value']==words[1]==formal['original_raw']!=0, 'actual original default argument differs from field'
    assert binding['native_owner_field']=='caller_argument' and binding['native_expression_field']=='caller_expression'
    owner=native['caller_locals'][binding['native_owner_field']]
    field=native['caller_locals'][binding['native_expression_field']]
    for local in (owner,field):
        assert local['function']=='convert_call_arguments' and local['local']=='argument', 'wrong actual native default owner'
    assert owner['fields']==[] and field['fields']==['expression'], 'wrong actual native default expression field'
    assert native['default_argument_bytes']==20 and native['default_argument_offset']==12
    assert 0<owner['value']<=0xffffffff-19, 'native default argument owner span wraps'
    assert field['value']==formal['native_raw']==native[formal['native_field']]!=0, 'actual native argument differs from caller expression field'
