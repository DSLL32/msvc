"""The first output-buffer allocation is a new typed generation, never a root waiver."""
import struct


def validate(config,data,original,native):
    rows={row['name']:row for row in data}
    def root(name):return int.from_bytes(original['memory'][rows[name]['original']],'little')
    def other(name):return int.from_bytes(bytes.fromhex(native['globals'][name]),'little')
    subsections=bool(config.get('observe_subsection_output'))
    if config.get('observe_public_writer_prefix'):
        assert original['address']==0x40f960 and original['stack'][0]==0x40d62f and native['frontier_caller']=='write_publics' and native['parameters']=={'buckets':dict(null=False,global_name='type_table_global'),'reserve':4},'output buffer next publicwriter child ABI differs'
    elif subsections:
        assert original['address']==0x40d600 and original['stack'][0]==0x406c5a and native['frontier_caller']=='flush_packed_file' and native['parameters']['offset']==5056,'output buffer next publics consumer ABI differs'
    else:assert original['address']==0x4068a0 and original['stack'][0]==0x406c17 and original['stack'][1]==0 and native['frontier_caller']=='flush_packed_file' and native['parameters']=={'index':0},'output buffer next real consumer ABI differs'
    old=original['output_buffer_allocations'];new=native['output_buffer_allocations']
    assert len(old)==len(new)==1,'output buffer allocation attempt inventory differs'
    a,b=old[0],new[0]
    assert a['sequence']==b['sequence']==0 and a['size']==b['size']==65512 and a['address'] and b['address'],'output buffer actual allocation size/result differs'
    assert a['provider']==a['provider_after']==original['module_bases']['msvcrt.dll']+0x1498 and a['entry_esp']==a['exit_esp'] and a['parent_return']==0x406b84,'output buffer actual canonical malloc/caller ABI differs'
    assert b['owner']=='begin_buffered_output' and b['formal_inventory']=={'file':'int'} and b['exit_esp']==b['entry_esp']+4,'output buffer native allocation/formal ABI differs'
    assert a['file']==b['file']==root('input_file')==other('input_file')==15,'output buffer allocation input file differs'
    for left,right in [(a['before'],a['after'])]:
        assert left['nonvolatile']==right['nonvolatile'],'output malloc changed original nonvolatile registers'
    assert b['before']['nonvolatile']==b['after']['nonvolatile'],'output malloc changed native nonvolatile registers'
    for state,allocation in [(original,a),(native,b)]:
        baseline=state['sort_environments']['after']
        environments=[allocation['before'],allocation['after'],state['output_buffer_state']['environment']]
        for env in environments:
            assert env['fp']==baseline['fp'],'output initialization changed own raw x87 state'
            if state is original:
                assert all(env[name]==baseline[name] for name in ('raw_errors','teb','tid','tls_index','tls_slot','ptd')),'output initialization changed original error identity/state'
            else:
                assert all(env[name]==baseline[name] for name in ('errno','doserrno','last_error','errno_address','doserrno_address','last_error_address')),'output initialization changed native error identity/state'
    out=original['output_buffer_state'];actual=native['output_buffer_state']
    assert out['address']==a['address']==root('output_buffer') and actual['address']==b['address']==other('output_buffer'),'output buffer new generation/global alias differs'
    for key,name,value in [('capacity','output_capacity',65512),('used','output_used',5056 if subsections else 8),('file','output_file',15)]:
        assert out[key]==actual[key]==root(name)==other(name)==value,'output buffer defined field differs: '+name
    header=b'NB11'+bytes(4)
    assert original['memory'][rows['packed_signature']['original']]==bytes.fromhex(native['globals']['packed_signature'])==header,'output buffer contextual header source differs'
    assert len(out['bytes'])==len(bytes.fromhex(actual['bytes']))==(5056 if subsections else 8) and out['bytes'][:8]==bytes.fromhex(actual['bytes'])[:8]==header,'output buffer complete initialized header differs'
    assert root('image_read_mode')==other('image_read_mode')==root('codeview_file_offset')+(5056 if subsections else 8)==(10000 if subsections else 4952),'output buffer writer position/used result differs'
    overlaps=[]
    for side,state,allocation in [('original',original,a),('native',native,b)]:
        base=allocation['address'];size=allocation['size']
        for key in ('memory','strings'):
            for location,payload in state[key].items():
                payload=payload if side=='original' else bytes.fromhex(payload);location=int(location)
                assert not max(base,location)<min(base+size,location+len(payload)),'new output buffer must remain a typed generation namespace'
        for owner,records in [('main-free',state['main_free_calls']),('source-temporary-free',state['source_family_frees'])]:
            for index,retired in enumerate(records):
                if retired['address'] and max(base,retired['address'])<min(base+size,retired['address']+retired['size']):
                    overlaps.append(dict(side=side,retired_owner=owner,retired_index=index,new_owner='begin_buffered_output',new_address=base,new_size=size,old_address=retired['address'],old_size=retired['size']))
    return dict(allocation_attempts=1,new_owner='begin_buffered_output',requested_capacity=65512,initialized_prefix_bytes=5056 if subsections else 8,own_unwritten_capacity_bytes=60456 if subsections else 65504,header=header.hex(),file_index=15,writer_position=10000 if subsections else 4952,actual_old_generation_address_reuse=overlaps,retired_payload_reopened=False,own_full_FP_and_error_baseline_preserved=True,next_consumer='insert_symbol_page_padding' if config.get('observe_public_writer_prefix') else 'write_publics' if subsections else 'write_subsection(index0)')
