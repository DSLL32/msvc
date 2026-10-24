"""Reached hello-win32 filename search and first option cleanup contracts."""
import json
import struct


def compare_release(original, state, trap, completed_options=False,native_directory=None):
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
        compare_configured(original,state,trap,events,canonical,search,native_directory=native_directory)
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


def compare_configured(original,state,trap,events,canonical,search,native_directory=None):
    """All formal inputs of validate_debug_options and real option-copy frees."""
    context=trap['validation_context']
    full_link=state['function']in ('LinkContext::FullLink','initialize_output_modules','duplicate_link_string','initialize_contribution_pools','allocate_virtual_memory','initialize_comdats','LinkContext::InitializeSections','create_output_section','find_matching_output_section','adjust_output_section_flags','initialize_section_enumerator','next_output_section','end_section_enumerator','resolve_output_section','create_contribution_group','find_contribution_group','initialize_group_enumerator','next_contribution_group','LinkContext::PassOne','check_output_input_collision','open_argument_file','acquire_file_record','find_cached_file','get_available_file_record','move_file_record','open_mapped_file','initialize_file_mapping','create_library','find_library','initialize_library_enumerator','next_library','LinkContext::LoadInputArgument','read_input_argument','is_archive','mapped_read','is_mac_resource_file','get_file_length')
    assert context==trap['configuration_context']
    if not full_link:assert context==trap['stack'][1]
    assert state['validation_arguments']==dict(context=state['context_this'])
    assert trap['output_name']=='hello.exe'==state['output_name']
    assert trap['output_name_pointer']==trap['crt_argv_addresses'][3]+5
    assert state['output_name_pointer']==state['crt_argv_addresses'][3]+5
    for key,filename,size,expected in [
        ('incremental_requested','incremental-requested.bin',1,0),
        ('incremental_switch_set','incremental-switch-set.bin',1,1),
        ('debug_formats','debug-formats.bin',4,0),
        ('pdb_enabled','pdb-enabled.bin',1,0 if full_link else 1),
        ('debug_option_present','debug-option-present.bin',1,0)]:
        data=(original/filename).read_bytes()
        assert len(data)==size and int.from_bytes(data,'little')==state['option_globals'][key]==expected
    assert state['incremental_enabled']==0 and (original/'incremental-enabled.bin').read_bytes()==b'\0'
    assert state['logo_pending']==0 and (original/'logo-pending.bin').read_bytes()==b'\0'
    source=state['duplicate_calls']
    assert len(source)==(26 if state['function']in ('read_input_argument','is_archive','mapped_read','is_mac_resource_file','get_file_length')else 25 if state['function']in ('open_mapped_file','initialize_file_mapping','create_library','find_library','initialize_library_enumerator','next_library','LinkContext::LoadInputArgument','read_input_argument','is_archive','mapped_read','is_mac_resource_file','get_file_length')else 24 if state['function']in ('LinkContext::PassOne','check_output_input_collision','open_argument_file','acquire_file_record','find_cached_file','get_available_file_record','move_file_record','open_mapped_file','initialize_file_mapping','create_library','find_library','initialize_library_enumerator','next_library','LinkContext::LoadInputArgument','read_input_argument','is_archive','mapped_read','is_mac_resource_file','get_file_length')else 15 if state['function']in ('create_contribution_group','find_contribution_group','initialize_group_enumerator','next_contribution_group','LinkContext::PassOne','check_output_input_collision','open_argument_file','acquire_file_record','find_cached_file','get_available_file_record','move_file_record','open_mapped_file','initialize_file_mapping','create_library','find_library','initialize_library_enumerator','next_library','LinkContext::LoadInputArgument','read_input_argument','is_archive','mapped_read','is_mac_resource_file','get_file_length')else 7 if state['function']in ('initialize_contribution_pools','allocate_virtual_memory','initialize_comdats','LinkContext::InitializeSections','create_output_section','find_matching_output_section','adjust_output_section_flags','initialize_section_enumerator','next_output_section','end_section_enumerator','resolve_output_section')else 5) and len(state['free_calls'])==3
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
    additional=compare_output_modules(original,state,trap,events)if state['function']in ('duplicate_link_string','initialize_contribution_pools','allocate_virtual_memory','initialize_comdats','LinkContext::InitializeSections','create_output_section','find_matching_output_section','adjust_output_section_flags','initialize_section_enumerator','next_output_section','end_section_enumerator','resolve_output_section','create_contribution_group','find_contribution_group','initialize_group_enumerator','next_contribution_group','LinkContext::PassOne','check_output_input_collision','open_argument_file','acquire_file_record','find_cached_file','get_available_file_record','move_file_record','open_mapped_file','initialize_file_mapping','create_library','find_library','initialize_library_enumerator','next_library','LinkContext::LoadInputArgument','read_input_argument','is_archive','mapped_read','is_mac_resource_file','get_file_length')else []
    advanced={}
    if state['function']in ('create_contribution_group','find_contribution_group','initialize_group_enumerator','next_contribution_group','LinkContext::PassOne','check_output_input_collision','open_argument_file','acquire_file_record','find_cached_file','get_available_file_record','move_file_record','open_mapped_file','initialize_file_mapping','create_library','find_library','initialize_library_enumerator','next_library','LinkContext::LoadInputArgument','read_input_argument','is_archive','mapped_read','is_mac_resource_file','get_file_length'):
        from section_state import compare as compare_sections
        advanced=compare_sections(original,state,trap,events,native_directory=native_directory)
    compare(original,state,trap,configuration_context=context,additional_arenas=additional,additional_allocations=compare_section_pools(original,state,trap,events)if state['function']in ('initialize_comdats','LinkContext::InitializeSections','create_output_section','find_matching_output_section','adjust_output_section_flags','initialize_section_enumerator','next_output_section','end_section_enumerator','resolve_output_section','create_contribution_group','find_contribution_group','initialize_group_enumerator','next_contribution_group','LinkContext::PassOne','check_output_input_collision','open_argument_file','acquire_file_record','find_cached_file','get_available_file_record','move_file_record','open_mapped_file','initialize_file_mapping','create_library','find_library','initialize_library_enumerator','next_library','LinkContext::LoadInputArgument','read_input_argument','is_archive','mapped_read','is_mac_resource_file','get_file_length')else [],**advanced)
    if full_link:
        formals=trap['full_link_formals'];native=state['full_link_formals'];entry=state['full_link_entry']
        if state['function']=='LinkContext::FullLink':
            assert formals['this']==trap['ecx']==trap['stack'][5]==context
            assert formals['pack_requested']==trap['stack'][1]==trap['esp']+28
        else:
            original_entry,=[e for e in events if e['event']=='full_link_entry']
            assert formals['this']==original_entry['this']==context
            assert formals['pack_requested']==original_entry['pack_requested']==original_entry['stack_pointer']+28
            assert original_entry['initial']==formals['pack_requested_initial']
            if state['function']=='initialize_output_modules':
                observed=trap['output_module_formals'];native_module=state['output_module_formals']
                assert observed['module']==observed['destination_global']==trap['stack'][1]==0x47f028
                assert observed['initial']==native_module['initial']==0
                assert native_module['module']==native_module['destination_global']!=0
                assert (original/'output-module-before.bin').read_bytes()==(original/'output-module-global.bin').read_bytes()==bytes(4)
        assert (original/'pack-requested-before.bin').read_bytes()==bytes([formals['pack_requested_initial']])
        assert formals['initial_value_contract']=='uninitialized caller local; no cross-platform value equality'
        assert native['this']==entry['this']==state['context_this']
        assert native['pack_requested']==native['caller_output']==entry['pack_requested']
        assert native['initial']==entry['initial'] and len(bytes.fromhex(native['initial']))==1
        assert native['pack_requested'] and not state['context_this']<=native['pack_requested']<state['context_this']+664
        # The caller has not initialized this out-byte. Both raw values stay
        # recorded; only its real pointer/owner/ABI is a boundary invariant.


def compare_output_modules(original,state,trap,events):
    """Actual new text argument plus two complete allocated records/pages."""
    completed=state['function']in ('initialize_contribution_pools','allocate_virtual_memory','initialize_comdats','LinkContext::InitializeSections','create_output_section','find_matching_output_section','adjust_output_section_flags','initialize_section_enumerator','next_output_section','end_section_enumerator','resolve_output_section','create_contribution_group','find_contribution_group','initialize_group_enumerator','next_contribution_group','LinkContext::PassOne','check_output_input_collision','open_argument_file','acquire_file_record','find_cached_file','get_available_file_record','move_file_record','open_mapped_file','initialize_file_mapping','create_library','find_library','initialize_library_enumerator','next_library','LinkContext::LoadInputArgument','read_input_argument','is_archive','mapped_read','is_mac_resource_file','get_file_length')
    if completed:
        formals=[0,4000,0x3000,4]if state['function']=='allocate_virtual_memory'else []
        if state['function']in ('create_output_section','find_matching_output_section','adjust_output_section_flags','initialize_section_enumerator','next_output_section','end_section_enumerator','resolve_output_section'):
            compare_section_creation(original,state,trap,events)
        elif state['function']not in ('create_contribution_group','find_contribution_group','initialize_group_enumerator','next_contribution_group','LinkContext::PassOne','check_output_input_collision','open_argument_file','acquire_file_record','find_cached_file','get_available_file_record','move_file_record','open_mapped_file','initialize_file_mapping','create_library','find_library','initialize_library_enumerator','next_library','LinkContext::LoadInputArgument','read_input_argument','is_archive','mapped_read','is_mac_resource_file','get_file_length'):assert trap['frontier_inputs']==state['frontier_inputs']==dict(formals=formals,hidden_register_inputs=[])
        if formals:
            assert trap['stack'][1:5]==formals
            for name,filename,size in [('module_section_map','section-map-global.bin',4),('module_section_headers','section-headers-global.bin',4),('module_section_capacity','section-capacity.bin',4),('module_section_pools_ready','section-pools-ready.bin',1)]:
                assert (original/filename).read_bytes()==bytes(size)and state['module_section_pools'][name]==0
    else:
        assert trap['argument_text']==state['argument_detail']['text']==' linker defined module'
        assert trap['argument_pointer']==trap['stack'][1]==0x401854
        assert state['argument_detail']['address']and state['argument_detail']['address']not in state['crt_argv_addresses']
    left=trap['module_constructor'];right=state['module_constructor']
    entry,=[e for e in events if e['event']=='output_modules_entry']
    assert left['result_slot']==entry['result_slot']==0x47f028
    assert left['initial']==entry['initial']==0
    assert right['current']==(right['module']if completed else 0)
    assert struct.unpack('<I',(original/'output-module-global.bin').read_bytes())[0]==(left['module']if completed else 0)
    assert right['result_slot']==right['global_slot']and right['result_slot']
    assert left['module']and left['library']
    if not completed:assert left['module']==trap['esi']
    module=bytearray((original/'output-module.bin').read_bytes());nmodule=bytearray.fromhex(right['module_raw'])
    assert len(module)==len(nmodule)==156
    assert struct.unpack_from('<I',module,100)[0]==left['library']
    assert struct.unpack_from('<I',nmodule,100)[0]==right['library']
    library=bytearray((original/'output-library.bin').read_bytes());nlibrary=bytearray.fromhex(right['library_raw'])
    assert len(library)==len(nlibrary)==68
    if completed:
        calls=[e for e in events if e['event']=='output_name_observation']
        assert len(calls)==4 and len(state['link_string_calls'])==(19 if state['function']in ('LinkContext::PassOne','check_output_input_collision','open_argument_file','acquire_file_record','find_cached_file','get_available_file_record','move_file_record','open_mapped_file','initialize_file_mapping','create_library','find_library','initialize_library_enumerator','next_library','LinkContext::LoadInputArgument','read_input_argument','is_archive','mapped_read','is_mac_resource_file','get_file_length')else 10 if state['function']in ('create_contribution_group','find_contribution_group','initialize_group_enumerator','next_contribution_group','LinkContext::PassOne','check_output_input_collision','open_argument_file','acquire_file_record','find_cached_file','get_available_file_record','move_file_record','open_mapped_file','initialize_file_mapping','create_library','find_library','initialize_library_enumerator','next_library','LinkContext::LoadInputArgument','read_input_argument','is_archive','mapped_read','is_mac_resource_file','get_file_length')else 2)
        for index,(label,offset,raw,nraw,address)in enumerate([
            ('module',4,module,nmodule,0x401854),('library',0,library,nlibrary,0x40183c)]):
            text=' linker defined '+label;expected=text.encode()+b'\0'
            original_name=trap['output_names'][index];native=state['output_names'][index]
            assert struct.unpack_from('<I',raw,offset)[0]==original_name and original_name
            assert struct.unpack_from('<I',nraw,offset)[0]==native['address']and native['address']
            assert (original/('output-'+label+'-name.bin')).read_bytes()==bytes.fromhex(native['bytes'])==expected
            before,after=calls[index*2:index*2+2]
            assert before['kind']==after['kind']==index and before['before']==1 and after['before']==0
            assert (before['site'],after['site'])==[(0x40e3f3,0x40e3f8),(0x40e400,0x40e405)][index]
            assert before['argument']==after['argument']==address and before['text']==after['text']==text
            assert before['stack']==before['entry_stack']==after['entry_stack']and after['stack']==before['stack']+4
            assert after['eax']==original_name
            wrapper=state['link_string_calls'][index]
            assert wrapper['return_stack']==wrapper['entry_stack']+8 and wrapper['result']==native['address']
            assert wrapper['text']==text
            call=state['duplicate_calls'][5+index]
            assert call['input']==wrapper['input']
            assert call['result']==native['address']and call['text']==text and call['input']
            assert call['input']not in state['crt_argv_addresses']and call['input']!=call['result']
            raw[offset:offset+4]=nraw[offset:offset+4]=bytes(4)
        assert trap['output_names'][0]!=trap['output_names'][1]
        assert state['output_names'][0]['address']!=state['output_names'][1]['address']
        for names in (trap['output_names'],[item['address']for item in state['output_names']]):
            assert names[0]+23<=names[1]or names[1]+24<=names[0]
        original_canonical=next(e for e in events if e['event']=='argument_list'and e['name']=='link_files')['nodes']
        for index,name in enumerate(trap['output_names']):
            for address,data in zip(trap['crt_argv_addresses'],trap['crt_argv']):assert name+23+index<=address or address+len(data)+1<=name
            for node in original_canonical:
                address=node['words'][0];assert name+23+index<=address or address+len(node['text'])+1<=name
        for index,name in enumerate(state['output_names']):
            for address,data in zip(state['crt_argv_addresses'],state['crt_argv']):assert name['address']+23+index<=address or address+len(data)+1<=name['address']
            for node in state['canonicalized_arguments']:
                assert name['address']+23+index<=node['result']or node['result']+len(node['text'])+1<=name['address']
    module[100:104]=nmodule[100:104]=bytes(4)
    assert module==nmodule==bytes(156)and library==nlibrary==bytes(68)
    assert len(state['output_module_arenas'])==2
    additional=[]
    for index,(name,size,slot) in enumerate([('module',156,0x467590),('library',68,0x46758c)]):
        original_arena=struct.unpack('<I',(original/('output-'+name+'-arena-pointer.bin')).read_bytes())[0]
        arena=struct.unpack('<4I',(original/('output-'+name+'-arena.bin')).read_bytes());native=state['output_module_arenas'][index];nheader=struct.unpack('<4I',bytes.fromhex(native['raw']))
        assert original_arena and native['address']and native['slot']
        used=136 if index==1 and state['function']in ('LinkContext::LoadInputArgument','read_input_argument','is_archive','mapped_read','is_mac_resource_file','get_file_length')else size
        assert arena[:3]==nheader[:3]==(0,used,4096-used)
        assert arena[3]==left[name]and nheader[3]==right[name]and right[name]
        raw=bytearray((original/('output-'+name+'-page.bin')).read_bytes());nraw=bytearray.fromhex(native['data'])
        assert len(raw)==len(nraw)==4096
        if not index:
            assert struct.unpack_from('<I',raw,100)[0]==left['library']
            assert struct.unpack_from('<I',nraw,100)[0]==right['library']
            raw[100:104]=nraw[100:104]=bytes(4)
        if completed:
            offset=0 if index else 4
            assert struct.unpack_from('<I',raw,offset)[0]==trap['output_names'][index]
            assert struct.unpack_from('<I',nraw,offset)[0]==state['output_names'][index]['address']
            raw[offset:offset+4]=nraw[offset:offset+4]=bytes(4)
            for name,size in zip(trap['output_names'],[23,24]):assert not arena[3]<=name<arena[3]+4096
            for name in state['output_names']:assert not nheader[3]<=name['address']<nheader[3]+4096
        if index==1 and state['function']in ('LinkContext::LoadInputArgument','read_input_argument','is_archive','mapped_read','is_mac_resource_file','get_file_length'):
            from library_creation_state import current_record
            expected,nexpected=current_record(original,state,trap)
            assert trap['command_line_library']['address']==arena[3]+68 and state['command_line_library']['address']==nheader[3]+68
            assert raw[68:136]==expected and nraw[68:136]==nexpected
            raw[68:136]=nraw[68:136]=bytes(68)
        assert raw==nraw==bytes(4096)
        additional.append(dict(original_arena=original_arena,native_arena=native['address'],original_base=arena[3],native_base=nheader[3],bytes=4096))
    return additional


def compare_section_pools(original,state,trap,events):
    left=trap['module_section_pools'];right=state['module_section_pools']
    assert right['module_section_capacity']==500 and right['module_section_pools_ready']==1
    assert (original/'section-capacity.bin').read_bytes()==struct.pack('<I',500)
    assert (original/'section-pools-ready.bin').read_bytes()==b'\1'
    assert (original/'comdat-root.bin').read_bytes()==bytes(4)
    if state['function']=='LinkContext::InitializeSections':
        assert state['comdat_arena']==0
        formals=trap['section_initializer_formals'];native=state['section_initializer_formals']
        assert formals==dict(this=trap['ecx'],stack_formals=[],other_hidden_inputs=[])
        assert formals['this']==trap['validation_context']
        assert native==dict(this=state['context_this'],stack_formals=[],other_hidden_inputs=[])
    calls=[e for e in events if e['event']=='section_allocation_observation']
    assert len(calls)==4 and len(state['virtual_memory_calls'])==2
    result=[]
    for index,(key,member,size)in enumerate([('map','module_section_map',4000),('headers','module_section_headers',20000)]):
        pointer=left[key];native=right[member];assert pointer and native
        assert (original/('section-'+key+'-global.bin')).read_bytes()==struct.pack('<I',pointer)
        assert (original/('section-'+key+'.bin')).read_bytes()==bytes.fromhex(state['module_section_bytes'][index])==bytes(size)
        before,after=calls[index*2:index*2+2];arguments=[0,size,0x3000,4]
        assert before['kind']==after['kind']==index and(before['before'],after['before'])==(1,0)
        assert (before['site'],after['site'])==[(0x40e36e,0x40e373),(0x40e386,0x40e38b)][index]
        assert before['arguments']==after['arguments']==arguments
        assert before['stack']==before['entry_stack']==after['entry_stack']and after['stack']==before['stack']+16
        assert after['eax']==pointer
        call=state['virtual_memory_calls'][index]
        assert call['arguments']==arguments and call['return_stack']==call['entry_stack']+20 and call['result']==native
        result.append(dict(original_base=pointer,native_base=native,bytes=size,sites=[0x44f5c5,0x44f5cb]))
    return result


def compare_section_creation(original,state,trap,events):
    left=trap['section_creation_formals'];right=state['section_creation_formals']
    keys=('module','name','flags','sections','header')
    if state['function']=='adjust_output_section_flags':
        lflags=trap['section_flag_formals'];rflags=state['section_flag_formals']
        assert lflags['flags']==trap['stack'][1]==trap['esp']+0x20
        assert lflags['owner_stack']==trap['esp']+0x23c
        assert list(struct.unpack('<5I',(original/'section-lookup-inputs.bin').read_bytes()))==[left[k]for k in keys]
        assert lflags['before']==rflags['before']==0xc0000080
        assert (original/'section-normalized-before.bin').read_bytes()==bytes.fromhex(rflags['raw'])==struct.pack('<I',0xc0000080)
        assert lflags['name']==left['name']and lflags['header']==left['header']
        assert rflags['name']==right['name']and rflags['header']==right['header']
        assert rflags['flags']==rflags['owner_local']and rflags['flags']
        assert not state['context_this']<=rflags['flags']<state['context_this']+664
        assert trap['frontier_inputs']==dict(formals=[lflags[k]for k in ('flags','name','header')],hidden_register_inputs=[])
        assert trap['stack'][1:4]==trap['frontier_inputs']['formals']
        assert state['frontier_inputs']==dict(formals=[rflags[k]for k in ('flags','name','header')],hidden_register_inputs=[])
    elif state['function']in ('initialize_section_enumerator','next_output_section','end_section_enumerator','resolve_output_section'):
        compare_section_enumerator_input(original,state,trap,events,left,right)
    else:
        assert trap['frontier_inputs']==dict(formals=[left[k]for k in keys],hidden_register_inputs=[])
        assert state['frontier_inputs']==dict(formals=[right[k]for k in keys],hidden_register_inputs=[])
        assert trap['stack'][1:6]==[left[k]for k in keys]
    entry,=[e for e in events if e['event']=='section_initializer_entry']
    assert entry['this']==trap['section_initializer_this']==trap['validation_context']
    assert state['section_initializer_this']==state['context_this']
    assert left['module']==right['module']==0
    assert left['name']==0x401cc8 and right['name']
    assert left['text']==right['text']=='.bss'and left['flags']==right['flags']==0xc0000080
    assert left['sections']==trap['validation_context']+0x1b4 and right['sections']==state['context_this']+0x1b4
    assert left['header']==trap['validation_context']+0x44 and right['header']==state['context_this']+0x44
    assert trap['section_handles']==state['section_handles']==[0]*19
    assert state['comdat_arena']==0


def compare_section_enumerator_input(original,state,trap,events,left,right):
    l=trap['section_enumerator_formals'];r=state['section_enumerator_formals']
    initialized=state['function']in ('next_output_section','end_section_enumerator','resolve_output_section')
    resolving=state['function']=='resolve_output_section'
    assert trap['frontier_inputs']==dict(formals=[trap['resolution_formals']['section']]if resolving else [l['enumerator']]if initialized else [l['enumerator'],l['sections']],hidden_register_inputs=[])
    assert state['frontier_inputs']==dict(formals=[state['resolution_formals']['section']]if resolving else [r['enumerator']]if initialized else [r['enumerator'],r['sections']],hidden_register_inputs=[])
    assert trap['stack'][1:1+len(trap['frontier_inputs']['formals'])]==trap['frontier_inputs']['formals']
    assert l['enumerator']==trap['esp']+(0x1c if initialized else 0x20)and l['owner_stack']==trap['esp']+(0x234 if initialized else 0x238)
    assert l['normalized_pointer']==trap['esp']+(0x18 if initialized else 0x1c)
    assert list(struct.unpack('<5I',(original/'section-lookup-inputs.bin').read_bytes()))==[left[k]for k in ('module','name','flags','sections','header')]
    assert l['sections']==left['sections']and r['sections']==right['sections']
    assert r['enumerator']==r['owner_local']and r['enumerator']
    if initialized:
        enum_observations=[e for e in events if e['event']=='section_enum_observation']
        assert len(enum_observations)==2 and len(state['section_enum_calls'])==1
        call,returned=enum_observations;native=state['section_enum_calls'][0]
        assert call['index']==returned['index']==0 and(call['before'],returned['before'])==(1,0)
        assert (call['site'],returned['site'])==(0x40f16e,0x40f173)
        assert call['arguments']==returned['arguments']==[l['enumerator'],l['sections']]
        assert call['entry_stack']==returned['entry_stack']==call['stack']and returned['stack']==call['stack']+8
        assert native['enumerator']==r['enumerator']and native['sections']==r['sections']
        assert native['return_stack']==native['entry_stack']+12
        assert len((original/'section-enumerator-before-0.bin').read_bytes())==len(bytes.fromhex(native['before']))==16
        assert (original/'section-enumerator-after-0.bin').read_bytes()==(original/'section-enumerator.bin').read_bytes()==struct.pack('<4I',0x40fc00,0x40fd90,0,0)
        expected=struct.pack('<4I',state['section_callback_addresses']['next_output_section'],state['section_callback_addresses']['end_section_enumerator'],0,0).hex()
        assert native['after']==r['before']==expected
        if state['function']in ('end_section_enumerator','resolve_output_section'):
            observations=[e for e in events if e['event']=='section_next_observation']
            assert len(observations)==2 and len(state['section_next_calls'])==1
            before,after=observations;call=state['section_next_calls'][0]
            assert before['index']==after['index']==0 and(before['before'],after['before'])==(1,0)
            assert(before['site'],after['site'])==(0x40f178,0x40f17d)
            assert before['argument']==after['argument']==l['enumerator']
            assert before['entry_stack']==after['entry_stack']==before['stack']and after['stack']==before['stack']+4
            assert after['result']==(after['eax']&255)==call['result']==0
            assert (original/'section-next-before-0.bin').read_bytes()==(original/'section-next-after-0.bin').read_bytes()==(original/'section-enumerator.bin').read_bytes()
            assert call['enumerator']==r['enumerator']and call['before']==call['after']==expected
            assert call['return_stack']==call['entry_stack']+8
            if resolving:
                lres=trap['resolution_formals'];rres=state['resolution_formals']
                assert lres['section']==rres['section']==rres['caller_current']==0
                assert lres['caller_current_slot']==l['enumerator']+12 and rres['caller_current_slot']==r['enumerator']+12
                observations=[e for e in events if e['event']=='section_end_observation']
                assert len(observations)==2 and len(state['section_end_calls'])==1
                before,after=observations;call=state['section_end_calls'][0]
                assert before['index']==after['index']==0 and(before['before'],after['before'])==(1,0)
                assert(before['site'],after['site'])==(0x40f1d6,0x40f1db)
                assert before['argument']==after['argument']==l['enumerator']
                assert before['entry_stack']==after['entry_stack']==before['stack']and after['stack']==before['stack']+4
                assert (original/'section-end-before-0.bin').read_bytes()==(original/'section-end-after-0.bin').read_bytes()==(original/'section-enumerator.bin').read_bytes()
                assert call['enumerator']==r['enumerator']and call['before']==call['after']==expected
                assert call['return_stack']==call['entry_stack']+8 and call['normal_return']is True
    else:assert len((original/'section-enumerator-before.bin').read_bytes())==len(bytes.fromhex(r['before']))==16
    assert l['initial_value_contract']==r['initial_value_contract']==('fully initialized enumerator record'if initialized else 'uninitialized output record; no incoming bytes consumed')
    assert l['normalized']==r['normalized']==0xc0000080
    assert r['normalized_pointer']+4<=r['enumerator']or r['enumerator']+16<=r['normalized_pointer']
    observations=[e for e in events if e['event']=='section_flags_observation']
    assert len(observations)==2 and len(state['section_flag_calls'])==1
    before,after=observations;call=state['section_flag_calls'][0]
    assert before['index']==after['index']==0 and before['kind']==after['kind']==0
    assert (before['before'],after['before'])==(1,0)and(before['site'],after['site'])==(0x40f15a,0x40f15f)
    assert before['arguments']==after['arguments']==[l['normalized_pointer'],left['name'],left['header']]
    assert before['entry_stack']==after['entry_stack']==before['stack']and after['stack']==before['stack']+12
    assert before['value']==after['value']==call['before']==call['after']==l['normalized']
    assert call['flags']==r['normalized_pointer']and call['name']==right['name']and call['header']==right['header']
    assert before['text']==after['text']==call['text']=='.bss'
    assert before['initialize_bss']==after['initialize_bss']==call['initialize_bss']==0
    assert before['subsystem']==after['subsystem']==call['subsystem']==0
    assert call['return_stack']==call['entry_stack']+16
