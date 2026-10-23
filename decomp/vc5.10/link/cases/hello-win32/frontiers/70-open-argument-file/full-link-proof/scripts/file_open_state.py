"""Actual file-record request: all four inputs and initialized caller outputs."""


def compare(original,state,trap,events):
    entry,=[e for e in events if e['event']=='open_file_entry'];native=state['open_file_entry']
    expected=[trap['output_name_pointer'],0x8302,0x180]
    nexpected=[state['output_name_pointer'],0x8302,0x180]
    assert entry['formals']==expected and native['formals']==nexpected
    assert entry['hidden_inputs']==native['hidden_inputs']==[]
    assert trap['esp']==entry['stack_pointer']-44 and trap['stack'][0]==0x40b82e
    current=trap['file_record_formals'];ncurrent=state['file_record_formals']
    for k,value,nvalue in zip(('name','flags','mode'),expected,nexpected):
        assert current[k]==value and ncurrent[k]==nvalue
    assert current['created']==trap['stack'][4]==entry['stack_pointer']-8
    assert current['size_slot']==entry['stack_pointer']-4
    assert ncurrent['created']==ncurrent['local_created']
    assert ncurrent['created']<native['stack_pointer'] and ncurrent['size_slot']+4<=native['stack_pointer']
    assert not(ncurrent['created']>=ncurrent['size_slot']and ncurrent['created']<ncurrent['size_slot']+4)
    for k in ('initial_created','initial_size','file_access_statistics'):
        assert current[k]==ncurrent[k]==0
    assert trap['ebp']==0xffffffff and ncurrent['initial_mapped_result']==-1
    assert current['created_bytes']==ncurrent['created_bytes']==1
    assert current['created']and ncurrent['created']
    assert trap['frontier_inputs']==dict(formals=[current[k]for k in ('name','flags','mode','created')],hidden_register_inputs=[])
    assert state['frontier_inputs']==dict(formals=[ncurrent[k]for k in ('name','flags','mode','created')],hidden_register_inputs=[])
