"""Reached ASCII path comparison: owned caller arrays, CRT bytes and real return."""
from section_state import pairs,returned


def compare(original,state,trap,events):
    calls=[e for e in events if e['event']=='collision_observation'];assert len(calls)==2
    before,after=calls;assert(before['before'],after['before'])==(1,0)
    returned(before,after,(0x44b8cb,0x44b8d0),8)
    native=state['collision_call']
    lists={e['name']:e for e in events if e['event']=='argument_list'}
    left=lists['link_files'];right=state['argument_lists']['link_files']
    assert before['formals']==after['formals']==[trap['output_name_pointer'],left['header'][0]]
    assert native['output']==state['output_name_pointer'] and native['inputs']==right['header'][0]
    assert native['normal_return']is True and native['return_stack']==native['entry_stack']+12
    assert before['stack']==next(e for e in events if e['event']=='pass_one_entry')['stack_pointer']-40
    assert left['header'][2]==right['header'][2]==2
    expected=['hello.exe','hello.obj','R:\\toolchains\\5.10\\lib\\kernel32.lib']
    cwd='S:\\vc5.10\\link\\case-work\\hello-win32'
    outputs=[cwd+'\\hello.exe',cwd+'\\hello.obj',expected[2]]
    pointers=[trap['output_name_pointer']]+[n['words'][0]for n in left['nodes']]
    npointers=[state['output_name_pointer']]+[n['words'][0]for n in right['nodes']]
    observed=list(pairs(events,'collision_fullpath_observation',3))
    native_paths=state['collision_path_calls'];assert len(native_paths)==3
    errors={e['phase']:e for e in events if e['event']=='crt_error_state'}
    dests=[];ndests=[]
    for i,((call,result),ncall)in enumerate(zip(observed,native_paths)):
        sites=(0x461862,0x461864)if i==0 else(0x46189d,0x46189f)
        returned(call,result,sites,0)
        destination,source,capacity=call['arguments']
        assert result['arguments']==call['arguments']
        assert source==pointers[i] and capacity==ncall['capacity']==259
        assert ncall['name']==npointers[i]
        assert call['input']==result['input']==ncall['input']==expected[i]
        assert result['result']==destination and ncall['result']==ncall['output']
        assert result['text']==ncall['text']==outputs[i]
        assert ncall['return_stack']==ncall['entry_stack']+4
        # Original SUB ESP,520 supplies two separate 260-byte stack arrays.
        assert destination==before['stack']-(524 if i==0 else 264)
        assert ncall['local_sizes']==[260,260]
        assert abs(ncall['local_output']-ncall['local_input'])==260
        assert ncall['output']==ncall['local_output'if i==0 else'local_input']
        assert max(ncall['local_output'],ncall['local_input'])+260<=native['entry_stack']
        raw_before=(original/('collision-path-%d-before.bin'%i)).read_bytes()
        raw_after=(original/('collision-path-%d-after.bin'%i)).read_bytes()
        n_before=bytes.fromhex(ncall['before']);n_after=bytes.fromhex(ncall['after'])
        expected_bytes=outputs[i].encode('ascii')+b'\0';used=len(expected_bytes)
        assert len(raw_before)==len(raw_after)==len(n_before)==len(n_after)==260
        assert raw_after[:used]==n_after[:used]==expected_bytes
        # Incoming bytes are uninitialized. Each run must preserve its own tail.
        assert raw_after[used:]==raw_before[used:] and n_after[used:]==n_before[used:]
        for phase in ('before','after'):
            original_errors={k:errors['collision-path-%d-%s'%(i,phase)][k]for k in ('errno','doserrno','last_error')}
            assert original_errors==ncall[phase+'_errors']==dict(errno=2,doserrno=2,last_error=2)
        if i==2:
            assert raw_before==previous_raw and n_before==previous_native
        previous_raw=raw_after;previous_native=n_after
        dests.append(destination);ndests.append(ncall['output'])
    assert dests[1]==dests[2] and dests[0]+260==dests[1]
    assert ndests[1]==ndests[2] and abs(ndests[0]-ndests[1])==260
    comparisons=list(pairs(events,'collision_compare_observation',2))
    assert len(state['collision_compare_calls'])==2
    for i,((call,result),ncall)in enumerate(zip(comparisons,state['collision_compare_calls'])):
        assert(call['site'],result['site'])==(0x4618c4,0x4618c6)
        assert call['stack']==result['stack']
        assert call['arguments']==result['arguments']==[dests[0],dests[i+1]]
        assert(ncall['left'],ncall['right'])==(ndests[0],ndests[i+1])
        assert call['left']==result['left']==ncall['left_text']==outputs[0]
        assert call['right']==result['right']==ncall['right_text']==outputs[i+1]
        assert result['result']==ncall['result']==(-1 if i==0 else 1)
        assert ncall['return_stack']==ncall['entry_stack']+4
        for phase,event in [('before',call),('after',result)]:
            assert(event['codepage'],event['lc_ctype_handle'],event['mb_cur_max'])==(ncall['codepage'],ncall['lc_ctype_handle'],ncall['mb_cur_max'])==(0,0,1)
            original_errors={k:errors['collision-compare-%d-%s'%(i,phase)][k]for k in ('errno','doserrno','last_error')}
            assert original_errors==ncall[phase+'_errors']==dict(errno=2,doserrno=2,last_error=2)
    # The recorded invocation must interleave each comparison with its own input.
    protocol=[(e['event'],e['index'])for e in events if e['event']in ('collision_fullpath_observation','collision_compare_observation')and e['before']]
    assert protocol==[('collision_fullpath_observation',0),('collision_fullpath_observation',1),('collision_compare_observation',0),('collision_fullpath_observation',2),('collision_compare_observation',1)]
