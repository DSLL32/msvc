"""Actual file-record request: all four inputs and initialized caller outputs."""


def compare(original,state,trap,events):
    entry,=[e for e in events if e['event']=='open_file_entry'];native=state['open_file_entry']
    expected=[trap['output_name_pointer'],0x8302,0x180]
    nexpected=[state['output_name_pointer'],0x8302,0x180]
    assert entry['formals']==expected and native['formals']==nexpected
    assert entry['hidden_inputs']==native['hidden_inputs']==[]
    available=state['function']=='get_available_file_record'
    finder=state['function']=='find_cached_file'or available
    if finder:
        acquire,=[e for e in events if e['event']=='file_record_entry']
        nacquire=state['file_record_entry']
        assert acquire['stack_pointer']==entry['stack_pointer']-44
        assert trap['esp']==acquire['stack_pointer']-(16 if available else 20)
        assert trap['stack'][0]==(0x40b88a if available else 0x40b87d)
        assert acquire['hidden_inputs']==nacquire['hidden_inputs']==[]
    else:
        assert trap['esp']==entry['stack_pointer']-44 and trap['stack'][0]==0x40b82e
    current=trap['file_record_formals'];ncurrent=state['file_record_formals']
    for k,value,nvalue in zip(('name','flags','mode'),expected,nexpected):
        assert current[k]==value and ncurrent[k]==nvalue
    assert current['created']==entry['stack_pointer']-8
    if not finder:assert current['created']==trap['stack'][4]
    assert current['size_slot']==entry['stack_pointer']-4
    assert ncurrent['created']==ncurrent['local_created']
    assert ncurrent['created']<native['stack_pointer'] and ncurrent['size_slot']+4<=native['stack_pointer']
    assert not(ncurrent['created']>=ncurrent['size_slot']and ncurrent['created']<ncurrent['size_slot']+4)
    for k in ('initial_created','initial_size','file_access_statistics'):
        assert current[k]==ncurrent[k]==0
    assert trap['ebp']==0xffffffff and ncurrent['initial_mapped_result']==-1
    assert current['created_bytes']==ncurrent['created_bytes']==1
    assert current['created']and ncurrent['created']
    if finder:
        assert acquire['formals']==[current[k]for k in ('name','flags','mode','created')]
        assert nacquire['formals']==[ncurrent[k]for k in ('name','flags','mode','created')]
        if available:
            observed=[e for e in events if e['event']=='file_lookup_observation'];assert len(observed)==2
            before,after=observed;assert(before['before'],after['before'])==(1,0)
            assert(before['site'],after['site'])==(0x40b878,0x40b87d)
            assert before['stack']==before['entry_stack']==after['entry_stack']==acquire['stack_pointer']-16
            assert after['stack']==before['stack']+4
            assert before['name']==after['name']==current['name'] and after['result']==0
            lookup,=state['file_lookup_calls']
            assert lookup['name']==ncurrent['name'] and lookup['result']==0 and lookup['normal_return']is True
            assert lookup['return_stack']==lookup['entry_stack']+8
            empty=dict(cached_files=0,cached_file_tail=0,pending_files=0,cached_file_count=0)
            assert trap['file_cache_lists']==state['file_cache_lists']==empty
            assert trap['frontier_inputs']==state['frontier_inputs']==dict(formals=[],hidden_register_inputs=[])
        else:
            assert trap['stack'][1]==current['name']
            assert trap['frontier_inputs']==dict(formals=[current['name']],hidden_register_inputs=[])
            assert state['frontier_inputs']==dict(formals=[ncurrent['name']],hidden_register_inputs=[])
    else:
        assert trap['frontier_inputs']==dict(formals=[current[k]for k in ('name','flags','mode','created')],hidden_register_inputs=[])
        assert state['frontier_inputs']==dict(formals=[ncurrent[k]for k in ('name','flags','mode','created')],hidden_register_inputs=[])
