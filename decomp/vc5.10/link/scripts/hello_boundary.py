"""Reached hello-win32 filename search and first option cleanup contracts."""
import json
import struct


def compare_release(original, state, trap, completed_options=False):
    events = [json.loads(line) for line in (original/'events.jsonl').read_text().splitlines()]
    argv = trap['crt_argv_addresses']
    native_argv = state['crt_argv_addresses']
    calls = [event for event in events if event['event']=='library_search_observation']
    assert [event['name'] for event in calls]==['observe:library_search_call','observe:library_search_return']
    before, after = calls
    assert (before['site'], after['site'])==(0x45c6f2,0x45c6f7)
    assert before['arguments']==after['arguments']
    assert before['arguments'][0]==argv[5]
    assert before['arguments'][1]==before['stack_pointer']+16
    assert after['stack_pointer']==before['stack_pointer']+8
    assert before['filename']==after['filename']=='kernel32.lib' and after['result']==0
    raw_before=(original/'library-search-before.bin').read_bytes()
    assert len(raw_before)==260 and raw_before==(original/'library-search-after.bin').read_bytes()
    assert len(state['library_search_calls'])==1
    native_call,=state['library_search_calls']
    assert native_call['name']==native_argv[5] and native_call['text']=='kernel32.lib'
    assert native_call['found'] and native_call['return_stack']==native_call['entry_stack']+12
    assert native_call['result']==0
    assert len(bytes.fromhex(native_call['before']))==260
    assert native_call['before']==native_call['after']
    # Both buffers are uninitialized caller locals: preserve each side's own
    # full contents, rather than compare arbitrary bytes between platforms.
    searches=[event for event in events if event['event']=='searchenv_observation']
    assert [event['name']for event in searches]==['observe:searchenv_call','observe:searchenv_return']
    search_before,search_after=searches
    assert (search_before['site'],search_after['site'])==(0x45c706,0x45c70c)
    assert search_before['arguments']==search_after['arguments']==[argv[5],0x4091fc,before['arguments'][1]]
    directory='R:\\toolchains\\5.10\\lib'
    filename=directory+'\\kernel32.lib'
    for event in searches:
        assert (event['filename'],event['variable'],event['value'])==('kernel32.lib','LIB',directory)
        assert (event['codepage'],event['lc_ctype_handle'],event['mb_cur_max'])==(0,0,1)
    assert search_after['result']==filename
    encoded=filename.encode()+b'\0'
    search_raw_before=(original/'searchenv-before.bin').read_bytes()
    search_raw_after=(original/'searchenv-after.bin').read_bytes()
    assert search_raw_before==raw_before
    assert search_raw_after[:len(encoded)]==encoded
    assert search_raw_after[len(encoded):]==search_raw_before[len(encoded):]
    assert len(state['searchenv_calls'])==1
    search,=state['searchenv_calls']
    assert (search['filename_pointer'],search['output'])==(native_argv[5],native_call['found'])
    assert search['variable_pointer'] and search['variable_pointer'] not in native_argv
    assert (search['filename'],search['variable'],search['value'])==('kernel32.lib','LIB',directory)
    assert (search['codepage'],search['lc_ctype_handle'],search['mb_cur_max'])==(0,0,1)
    native_before=bytes.fromhex(search['before']);native_after=bytes.fromhex(search['after'])
    assert native_before==bytes.fromhex(native_call['after'])
    assert len(native_before)==len(native_after)==260
    assert native_after[:len(encoded)]==encoded and native_after[len(encoded):]==native_before[len(encoded):]
    errors=[event for event in events if event['event']=='crt_error_state'and event['phase'].startswith('searchenv-')]
    assert [event['phase']for event in errors]==['searchenv-before','searchenv-after']
    for event,native_errors in zip(errors,[search['before_errors'],search['after_errors']]):
        assert {key:event[key]for key in ('errno','doserrno','last_error')}==native_errors==dict(errno=2,doserrno=2,last_error=2)
    lists={event['name']:event for event in events if event['event']=='argument_list'}
    assert lists['input_files']['header']==state['argument_lists']['input_files']['header']==[0,0,0]
    assert lists['input_files']['nodes']==state['argument_lists']['input_files']['nodes']==[]
    left,right=lists['option_arguments'],state['argument_lists']['option_arguments']
    assert left['header'][2]==right['header'][2]==3
    assert len(left['nodes'])==len(right['nodes'])==3
    for index,text in enumerate(['nologo','incremental:no','out:hello.exe']):
        for owner,addresses in [(left,argv),(right,native_argv)]:
            node=owner['nodes'][index]
            assert node['text']==node['original']==text
            assert node['words'][:2]==[addresses[index+1]+1]*2
            assert node['words'][2:5]==[0,0,0]
            assert node['words'][5]==(owner['nodes'][index+1]['address']if index<2 else 0)
            assert owner['header'][:2]==[owner['nodes'][0]['address'],owner['nodes'][-1]['address']]
    for owner in [lists['link_files'],state['argument_lists']['link_files']]:
        assert owner['header'][2]==len(owner['nodes'])==2
        for index,text in enumerate(['hello.obj',filename]):
            node=owner['nodes'][index];address=node['words'][0]
            assert address and node['words']==[address,address,0,0,0,owner['nodes'][1]['address']if index==0 else 0]
            assert node['text']==node['original']==text
            assert owner['header'][:2]==[owner['nodes'][0]['address'],owner['nodes'][1]['address']]
    assert struct.unpack('<I',(original/'argument-tail.bin').read_bytes())[0]==lists['link_files']['header'][0]
    assert state['argument_tail']==state['argument_lists']['link_files']['header'][0]
    canonical=state['canonicalized_arguments']
    assert len(canonical)==2
    for index,text in enumerate(['hello.obj',filename]):
        item=canonical[index]
        assert item==dict(input=native_argv[index+4],result=state['argument_lists']['link_files']['nodes'][index]['words'][0],text=text,bytes=(text.encode()+b'\0').hex())
        assert item['result'] not in native_argv
    if completed_options:
        compare_configured(original,state,trap,events,canonical,search)
        return
    original_copy,=[event for event in events if event['event']=='duplicate_return']
    assert original_copy['site']==0x4451ab
    assert trap['stack'][1]==trap['argument_pointer']==original_copy['result']
    assert trap['argument_text']==state['argument_detail']['text']=='nologo'
    address=state['argument_detail']['address']
    assert address==state['duplicate_result'] and address not in native_argv
    assert address not in [item['result']for item in canonical]
    assert (original/'release-argument.bin').read_bytes()==bytes.fromhex(state['release_bytes'])==b'nologo\0'
    assert state['duplicate_calls']==[
        dict(text='hello.obj',input=native_argv[4],result=canonical[0]['result']),
        dict(text=filename,input=search['output'],result=canonical[1]['result']),
        dict(text='nologo',input=native_argv[1]+1,result=address)]
    parser_calls=[event for event in events if event['event']=='parser_allocation']
    allocation=trap['parsed_switch_pointer'];native_allocation=state['parsed_switch_address']
    assert parser_calls==[
        dict(event='parser_allocation',name='observe:parser_alloc_call',site=0x40d149,bytes=15,result=0),
        dict(event='parser_allocation',name='observe:parser_alloc_return',site=0x40d14e,bytes=15,result=allocation)]
    assert state['parser_allocations']==[dict(bytes=15,result=native_allocation)]
    for data,base in [((original/'parsed-switch.bin').read_bytes(),allocation),(bytes.fromhex(state['parsed_switch']),native_allocation)]:
        assert len(data)==15 and struct.unpack_from('<I',data)[0]==base+8
        assert struct.unpack_from('<H',data,4)[0]==0 and data[8:]==b'nologo\0'
        # Reserved WORD6..7 remains uninitialized and is not consumed.
    observations=[event for event in events if event['event']=='configuration_arguments']
    assert observations==[dict(event='configuration_arguments',context=trap['configuration_context'],module=0,file=0)]
    assert state['logo_pending']==0 and (original/'logo-pending.bin').read_bytes()==b'\0'
    from context_heap import compare
    compare(original,state,trap,configuration_context=trap['configuration_context'])


def compare_configured(original,state,trap,events,canonical,search):
    """All formal inputs of validate_debug_options and real option-copy frees."""
    context=trap['validation_context']
    assert context==trap['stack'][1]==trap['configuration_context']
    assert state['validation_arguments']==dict(context=state['context_this'])
    assert trap['output_name']=='hello.exe'==state['output_name']
    assert trap['output_name_pointer']==trap['crt_argv_addresses'][3]+5
    assert state['output_name_pointer']==state['crt_argv_addresses'][3]+5
    for key,filename,size,expected in [
        ('incremental_requested','incremental-requested.bin',1,0),
        ('incremental_switch_set','incremental-switch-set.bin',1,1),
        ('debug_formats','debug-formats.bin',4,0),
        ('pdb_enabled','pdb-enabled.bin',1,1),
        ('debug_option_present','debug-option-present.bin',1,0)]:
        data=(original/filename).read_bytes()
        assert len(data)==size and int.from_bytes(data,'little')==state['option_globals'][key]==expected
    assert state['incremental_enabled']==0 and (original/'incremental-enabled.bin').read_bytes()==b'\0'
    assert state['logo_pending']==0 and (original/'logo-pending.bin').read_bytes()==b'\0'
    source=state['duplicate_calls']
    assert len(source)==5 and len(state['free_calls'])==3
    assert source[:2]==[
        dict(text='hello.obj',input=state['crt_argv_addresses'][4],result=canonical[0]['result']),
        dict(text=canonical[1]['text'],input=search['output'],result=canonical[1]['result'])]
    observations=[event for event in events if event['event']=='free_observation']
    assert len(observations)==6
    for index,text in enumerate(['nologo','incremental:no','out:hello.exe']):
        call,returned=observations[index*2:index*2+2]
        assert (call['name'],returned['name'])==('observe:free_call','observe:free_return')
        assert (call['site'],returned['site'])==(0x44fe35,0x44fe3b)
        assert call['index']==returned['index']==index and call['text']==text
        assert call['argument']==returned['argument'] and call['argument']
        assert call['argument'] not in trap['crt_argv_addresses']
        assert call['stack_pointer']==returned['stack_pointer']
        raw=(original/('free-%d-before.bin'%index)).read_bytes()
        native=state['free_calls'][index]
        assert raw==bytes.fromhex(native['before'])==text.encode()+b'\0'
        assert native['text']==text and native['normal_return'] is True
        assert native['return_stack']==native['entry_stack']+4
        assert source[index+2]==dict(text=text,input=state['crt_argv_addresses'][index+1]+1,result=native['argument'])
        assert native['argument'] not in state['crt_argv_addresses']+[item['result']for item in canonical]
        error_events=[event for event in events if event['event']=='crt_error_state'and event['phase'].startswith('free-%d-'%index)]
        assert [event['phase']for event in error_events]==['free-%d-before'%index,'free-%d-after'%index]
        for event,observed in zip(error_events,[native['before_errors'],native['after_errors']]):
            assert {key:event[key]for key in ('errno','doserrno','last_error')}==observed==dict(errno=2,doserrno=2,last_error=2)
    assert struct.unpack_from('<I',bytes.fromhex(state['link_context']),0x150)[0]==0
    assert bytes.fromhex(state['link_context'])[0x124]==1
    from context_heap import compare
    compare(original,state,trap,configuration_context=context)
