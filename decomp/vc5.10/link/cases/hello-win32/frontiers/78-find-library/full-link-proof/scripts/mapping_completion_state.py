"""Mapped view ownership, complete backing bytes and real return-chain proof."""
import struct


def compare(original,state,trap,events,oslots,nslots,handle,nhandle,native_directory):
    observed=[e for e in events if e['event']=='mapping_api_observation']
    assert[(e['kind'],e['before'])for e in observed]==[(0,1),(0,0),(1,1),(1,0),(2,1),(2,0)]
    calls=state['mapping_api_calls'];assert[c['kind']for c in calls]==['create','view','close']
    errors={e['phase']:e for e in events if e['event']=='crt_error_state'}
    for i,(kind,before,after,call)in enumerate(zip(('create','view','close'),observed[::2],observed[1::2],calls)):
        count=1 if kind=='close'else 6
        assert(before['site'],after['site'])==[(0x40cd44,0x40cd4a),(0x40cd95,0x40cd9b),(0x40cdc9,0x40cdcf)][i]
        assert before['stack']==before['entry_stack']==after['entry_stack']and after['stack']==before['stack']+count*4
        assert before['arguments']==after['arguments']
        assert call['normal_return']is True and call['return_stack']==call['entry_stack']+(count+1)*4
        for phase in ('before','after'):
            assert {k:errors['mapping-'+kind+'-'+phase][k]for k in ('errno','doserrno','last_error')}==call[phase+'_errors']==dict(errno=2,doserrno=2,last_error=0)
    assert observed[0]['arguments']==[handle,0,4,0,0x80000,0]
    assert calls[0]['arguments']==[nhandle,0,4,0,0x80000,0]
    mapping=observed[1]['result'];nmapping=calls[0]['result']
    assert mapping and nmapping and mapping!=handle and nmapping!=nhandle
    assert observed[2]['arguments']==[mapping,0xf001f,0,0,0x80000,0]
    assert calls[1]['arguments']==[nmapping,0xf001f,0,0,0x80000,0]
    view=observed[3]['result'];nview=calls[1]['result'];assert view and nview
    assert state['created_view']==dict(address=nview,bytes=0x80000)
    assert observed[4]['arguments']==[mapping]and calls[2]['arguments']==[nmapping]
    assert observed[5]['result']==calls[2]['result']==1
    original_view=b''.join((original/('created-mapping-view-%02d.bin'%i)).read_bytes()for i in range(8))
    assert len(original_view)==0x80000 and not any(original_view)
    # This exact view was read only AFTER successful close of its mapping handle.
    from pathlib import Path
    directory=Path(native_directory)
    native_view=(directory/'created-mapping-view.bin').read_bytes()
    assert native_view==original_view==(original/'artifacts/hello.exe').read_bytes()==(directory/'artifacts/hello.exe').read_bytes()
    returns=[e for e in events if e['event']=='file_open_return']
    assert[e['kind']for e in returns]==['mapping','mapped','output']
    for e,key,site,cleanup,result in zip(returns,('mapping_return','mapped_open_return','output_open_return'),(0x40c9cf,0x40b84a,0x44b8e5),(12,16,12),(1,0,15)):
        n=state[key]
        assert e['site']==site and e['stack']==e['entry_stack']+cleanup+4
        assert (e['result']&255 if key=='mapping_return'else e['result'])==result
        assert n['normal_return']is True and n['return_stack']==n['entry_stack']+cleanup+4 and n['result']==result
        if key!='output_open_return':assert e['error']==n['error']==0 and e['created']==1
    assert state['mapped_open_return']['created']==1
    assert state['file_record_formals']==state['mapped_open_return']['caller_formals']
    mapped,=[e for e in events if e['event']=='mapped_open_entry'];m=state['mapped_open_entry']
    assert returns[1]['entry_stack']==mapped['stack_pointer']and state['mapped_open_return']['entry_stack']==m['stack_pointer']
    entry,=[e for e in events if e['event']=='open_file_entry'];assert returns[2]['entry_stack']==entry['stack_pointer']
    assert state['output_open_return']['entry_stack']==state['open_file_entry']['stack_pointer']
    assert state['mapping_return']['arguments']==state['mapping_entry']['raw_formals']
    assert state['mapped_open_return']['arguments']==m['raw_formals']
    assert state['output_open_return']['arguments']==state['open_file_entry']['formals']
    assert trap['file_input_lifetime']==state['file_input_lifetime']=='captured at mapped-opener return before caller locals expire'
    context=trap['validation_context'];ncontext=state['context_this']
    assert trap['library_create_formals']==dict(name=0,path=0,libraries=context+0x1bc)
    assert state['library_create_formals']==dict(name=0,path=0,libraries=ncontext+0x1bc)
    assert (original/'library-list-input.bin').read_bytes()==struct.pack('<4I',0,context+0x1bc,0,0)
    assert bytes.fromhex(state['library_list_input'])==struct.pack('<4I',0,ncontext+0x1bc,0,0)
    if state['function']in ('find_library','initialize_library_enumerator'):
        parent,=[e for e in events if e['event']=='library_create_entry']
        assert parent['formals']==[0,0,context+0x1bc]and parent['hidden_inputs']==[]
        assert trap['esp']==parent['stack_pointer']-(848 if state['function']=='initialize_library_enumerator'else 28)
        assert trap['find_library_formals']==state['find_library_formals']==dict(name=0,first=0,before=0,raw_before_word=0)
        assert state['library_find_entry']['raw_formals']==[0,0,0]and state['library_find_entry']['hidden_inputs']==[]
        if state['function']=='find_library':
            assert trap['stack'][1:4]==[0,0,0]
            assert trap['frontier_inputs']==state['frontier_inputs']==dict(formals=[0,0,0],hidden_register_inputs=[])
        else:
            finder,=[e for e in events if e['event']=='library_find_entry']
            assert finder['formals']==[0,0,0]and finder['hidden_inputs']==[]
            assert finder['stack_pointer']==parent['stack_pointer']-28
            assert trap['esp']==finder['stack_pointer']-820 and trap['stack'][0]==0x40f674
            o=trap['library_enum_input'];n=state['library_enum_input']
            assert o==dict(address=trap['esp']+32,bytes=16,formals=[trap['esp']+32,0],hidden_inputs=[])
            assert trap['stack'][1:3]==o['formals']
            assert state['library_enum_entry']==dict(stack_pointer=state['library_enum_entry']['stack_pointer'],formals=n['formals'],hidden_inputs=[])
            assert state['library_enum_entry']['stack_pointer']<n['address']
            assert n['bytes']==16 and n['address']and n['formals']==[n['address'],0]and n['hidden_inputs']==[]
            assert n['address']<state['library_find_entry']['stack_pointer']and n['address']+16<=state['library_find_entry']['stack_pointer']
            # Both records are uninitialized caller output locals. Preserve raw bytes;
            # constructor overwrites all16 bytes and reads none, so no value equality.
            assert len((original/'library-enum-before.bin').read_bytes())==len(bytes.fromhex(n['raw_before']))==16
            assert trap['frontier_inputs']==dict(formals=o['formals'],hidden_register_inputs=[])
            assert state['frontier_inputs']==dict(formals=n['formals'],hidden_register_inputs=[])
    else:
        assert trap['stack'][1:4]==[0,0,context+0x1bc]
        assert trap['frontier_inputs']==dict(formals=[0,0,context+0x1bc],hidden_register_inputs=[])
        assert state['frontier_inputs']==dict(formals=[0,0,ncontext+0x1bc],hidden_register_inputs=[])
    assert state['library_create_entry']['formals']==[0,0,ncontext+0x1bc]and state['library_create_entry']['hidden_inputs']==[]
    assert trap['output_file']==state['output_file']==15
    return view,nview
