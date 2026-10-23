"""Explicit raw-dump contracts; new boundaries remain fail closed."""
import json
import struct

# Which completed allocations and registrations may be consumed at each trap.
# A pointer is compared by its observed allocation role, never host address.
STAGES = {
    '_initialize_buffer12': (1,0,0), '_allocate_buffer12': (1,0,0),
    '_register_buffer12_cleanup': (1,1,0),
    '_initialize_buffer8': (2,1,1), '_allocate_buffer8': (2,1,1),
    '_register_buffer8_cleanup': (2,2,1),
    '_initialize_buffer4': (3,2,2), '_allocate_buffer4': (3,2,2),
    '_register_buffer4_cleanup': (3,3,2),
    '_initialize_timing': (4,3,3), '_setup_timing': (4,3,3),
    '_register_timing_cleanup': (4,3,3), '_main': (None,3,4), '_initialize_debug@8':(None,3,4), '_initialize_file_cache@16':(None,3,4), '_get_debug_flag@4':(None,3,4), '_zero_allocate@4':(None,3,4), '_new_file_record':(None,3,4), '_initialize_memory':(None,3,4), '_run_linker@8':(None,3,4), '_initialize_repro':(None,3,4), '_collect_file_arguments@16':(None,3,4), '_parse_command_line@16':(None,3,4), '_parse_argument@8':(None,3,4), '_is_libpath@4':(None,3,4), '_append_argument@8':(None,3,4), '_append_argument_record@12':(None,3,4), '_has_switch@4':(None,3,4),
}
MEMORY_READY={ '_run_linker@8','_initialize_repro','_collect_file_arguments@16',
               '_parse_command_line@16','_parse_argument@8','_is_libpath@4','_append_argument@8','_append_argument_record@12','_has_switch@4' }
CACHE_READY=MEMORY_READY | {'_initialize_memory'}
DEBUG_READY=CACHE_READY | {'_get_debug_flag@4','_zero_allocate@4','_new_file_record'}

BUFFERS = [('12',0x424e70,12288,1024,0x424ea0,0x424eb0),
           ('8',0x431d80,4000,500,0x431db0,0x431dc0),
           ('4',0x431df0,4096,1024,0x431e20,0x431e30)]


def compare(name, original, native, original_state, native_state):
    if name == '_mainCRTStartup':
        # No C arguments or application globals exist at process entry. OS
        # loader register/stack values belong to different native runtimes.
        assert original_state['rva'] == 0x65280
        assert native_state['function'] == 'target_startup'
        return
    assert name in STAGES, 'raw state contract not implemented for ' + name
    cursor, allocations, registrations = STAGES[name]
    config = json.loads((original / 'functions.json').read_text())
    names = {record['start']:record['name'].lstrip('_') for record in config['driver']}
    values = list(struct.unpack('<5I',(original / 'initializers.bin').read_bytes()))
    logical = [None if value==0 else names[value] for value in values]
    assert logical == native_state['initializers']
    assert logical == [None,'initialize_buffer12','initialize_buffer8','initialize_buffer4','initialize_timing']
    events = [json.loads(line) for line in (original / 'events.jsonl').read_text().splitlines()]
    calls = [event for event in events if event['event']=='initerm_call']
    assert [(event['site'],event['begin'],event['end']) for event in calls] == [(0x465320,0x467018,0x46701c),(0x46535b,0x467000,0x467014)]
    observed_calls = [[None if value==0 else names[value] for value in event['values']] for event in calls]
    assert observed_calls == [[None],logical] == native_state['initerm_calls']
    if cursor is not None:
        assert (original_state['esi']-0x467000)//4 == native_state['initerm_current_index'] == cursor
        assert (original_state['edi']-0x467000)//4 == native_state['initerm_end_index'] == 5
    assert original_state['x87_control_word'] == native_state['x87_control_word'] == 0x027f
    for filename, native_name in [('crt-fmode.bin','crt_fmode'),('crt-commode.bin','crt_commode'),('crt-adjust-fdiv.bin','crt_adjust_fdiv')]:
        assert struct.unpack('<i',(original/filename).read_bytes())[0] == native_state[native_name] == 0
    assert original_state['crt_argv'] == native_state['crt_argv'] == ['R:\\toolchains\\5.10\\bin\\LINK.EXE','/?']
    startup = struct.unpack('<9I',(original/'startup-globals.bin').read_bytes())
    assert startup == (0,0,0,0,0,0,0,0xffffffff,0xffffffff)
    observations = [event for event in events if event['event']=='operator_new_observation']
    assert len(observations) == 2*allocations
    assert len(native_state.get('operator_new_calls',[])) == allocations
    for index,(suffix,address,size,count,registration,cleanup) in enumerate(BUFFERS):
        symbol='buffer'+suffix
        data=(original/(symbol+'.bin')).read_bytes()
        native_data=bytes.fromhex(native_state['app_globals'][symbol])
        if index >= allocations:
            assert data==native_data and not any(data), 'unconstructed buffer state changed'
            continue
        pointer, remaining, capacity = struct.unpack('<3I', data)
        native_pointer, native_remaining, native_capacity = struct.unpack('<3I',native_data)
        assert pointer and native_pointer, 'allocation unexpectedly failed'
        assert (remaining,capacity)==(native_remaining,native_capacity)==(count,count)
        call,returned=observations[index*2:index*2+2]
        assert (call['name'],call['site'],call['stack_top'])==('observe:new_call_'+suffix,address+5,size)
        assert (returned['name'],returned['site'],returned['eax'])==('observe:new_return_'+suffix,address+10,pointer)
        assert call['new_handler']==returned['new_handler']==0
        assert call['malloc_new_mode']==returned['malloc_new_mode']==0
        assert native_state['operator_new_calls'][index]==dict(bytes=size,result=native_pointer)
    # Uninitialized allocation contents are not consumed before these frontiers.
    timing=(original/'timing.bin').read_bytes()
    if cursor is None or name=='_register_timing_cleanup':
        start,end,output,options,display=struct.unpack('<4IB3x',timing)
        nstart,nend,noutput,noptions,ndisplay=struct.unpack('<4IB3x',bytes.fromhex(native_state['app_globals']['timing']))
        # GetTickCount values are identified nondeterministic clocks only.
        assert start==end and nstart==nend
        assert output==original_state['crt_base']+0x3f590
        assert noutput==native_state['crt_stdout_address']
        assert options==noptions==display==ndisplay==0
        assert timing[17:]==bytes.fromhex(native_state['app_globals']['timing'])[17:]==b'\0'*3
        stdout=(original/'crt-stdout.bin').read_bytes()
        nstdout=bytes.fromhex(native_state['crt_stdout_bytes'])
        if name in MEMORY_READY:
            original_file=struct.unpack('<8I',stdout);native_file=struct.unpack('<8I',nstdout)
            assert original_file[0]==original_file[2] and original_file[0]
            assert native_file[0]==native_file[2] and native_file[0]
            assert original_file[1:2]+original_file[3:]==native_file[1:2]+native_file[3:]==(0,0x40a,1,0,2048,0)
        else:
            assert stdout==nstdout
            assert struct.unpack('<8I',stdout)==(0,0,0,2,1,0,0,0)
    else:
        assert timing.hex()==native_state['app_globals']['timing'] and not any(timing)
    observations=[event for event in events if event['event']=='atexit_observation']
    assert len(observations)==registrations*2
    assert len(native_state.get('atexit_calls',[]))==registrations
    registration_records=BUFFERS+ [('timing',0,0,0,0x444ea0,0x444eb0)]
    for index,(suffix,address,size,count,registration,cleanup) in enumerate(registration_records[:registrations]):
        call,returned=observations[index*2:index*2+2]
        assert (call['name'],call['site'],call['stack_top'])==('observe:atexit_call_'+suffix,registration+5,cleanup)
        assert (returned['name'],returned['site'],returned['eax'])==('observe:atexit_return_'+suffix,registration+10,0)
        assert native_state['atexit_calls'][index]==dict(callback='close_timing' if suffix=='timing' else 'free_buffer'+suffix,result=0)

    if cursor is None and name!='_main':
        observations=[event for event in events if event['event']=='console_observation']
        assert len(observations)==2
        call,returned=observations
        assert (call['name'],call['site'],call['arguments'])==('observe:console_call',0x40ded2,[0x40dca0,1])
        assert (returned['name'],returned['site'],returned['eax'])==('observe:console_return',0x40ded8,1)
        assert native_state['console_calls']==[dict(callback='console_control',add=1,result=1)]
        assert struct.unpack('<I',(original/'diagnostic-output.bin').read_bytes())[0]==original_state['crt_base']+0x3f590
        assert native_state['diagnostic_output']==native_state['crt_stdout_address']
    if name=='_initialize_debug@8':
        assert original_state['debug_options_pointer']==native_state['frontier_arguments']['options']==0
        assert original_state['debug_environment']==native_state['frontier_arguments']['environment']=='LINK_DB'

    if name in DEBUG_READY:
        assert original_state['program_name']==native_state['program_name']=='LINK'
        assert original_state['program_name_pointer']==original_state['main_frame_pointer']-0x23c
        assert native_state['program_name_address']==native_state['basename_address']
        flags=(original/'debug-flags.bin').read_bytes()
        assert flags.hex()==native_state['debug_flags'] and not any(flags)

    if name in DEBUG_READY:
        probes=[event for event in events if event['event']=='module_probe']
        assert [event['name'] for event in probes]==['observe:module_call','observe:module_return','observe:proc_call','observe:proc_return']
        module_call,module_return,proc_call,proc_return=probes
        assert module_call['requested']=='kernel32.dll' and module_call['site']==0x40b568
        module=module_return['eax']
        assert module and module_return['site']==0x40b56e
        assert proc_call['requested']=='IsTNT' and proc_call['arguments'][0]==module and proc_call['site']==0x40b578
        assert proc_return['eax']==0 and proc_return['site']==0x40b57e
        from binary import PE,u32
        module_event,=[event for event in events if event['event']=='module' and event['base']==module]
        module_pe=PE(original/('module-%03d.bin'%module_event['index']))
        exports=module_pe.base+u32(module_pe.data,module_pe.optional+96)
        assert module_pe.string(module_pe.base+u32(module_pe.read(exports+12,4),0)).lower()=='kernel32.dll'
        assert native_state['module_probes']==[dict(kind='module',requested='kernel32.dll',module=0,result=1),dict(kind='proc',requested='IsTNT',module=1,result=0)]
        slots_pointer,newest,count=struct.unpack('<3I',(original/'file-pointers.bin').read_bytes())
        if name not in CACHE_READY:assert newest==count==0
        if name in CACHE_READY or name=='_new_file_record':
            assert slots_pointer and native_state['file_globals']['file_slots']
            array=(original/'file-slots.bin').read_bytes()
            if name=='_new_file_record':assert array.hex()==native_state['file_slots_bytes'] and not any(array)
        else:assert slots_pointer==native_state['file_globals']['file_slots']==0
        assert (original/'file-mapping-enabled.bin').read_bytes()==b'\1'
        assert struct.unpack('<I',(original/'file-window-limit.bin').read_bytes())[0]==(14 if name in CACHE_READY else 0)
        assert struct.unpack('<15I',(original/'file-statistics.bin').read_bytes())==(0,)*15
        assert native_state['file_globals']==dict(file_slots=native_state['file_globals']['file_slots'],file_newest=native_state['file_globals']['file_newest'] if name in CACHE_READY else 0,file_record_count=16 if name in CACHE_READY else 0,file_window_limit=14 if name in CACHE_READY else 0,file_mapping_enabled=1)
        assert native_state['file_statistics']==[0]*15
        if name=='_get_debug_flag@4':
            assert original_state['stack'][1]==native_state['frontier_arguments']['index']==44
        elif name=='_zero_allocate@4':
            assert original_state['stack'][1]==native_state['frontier_arguments']['bytes']==64

        if name in CACHE_READY:
            pointers=struct.unpack('<16I',array)
            native_pointers=struct.unpack('<16I',bytes.fromhex(native_state['file_slots_bytes']))
            assert len(set(pointers))==len(set(native_pointers))==16 and all(pointers) and all(native_pointers)
            assert slots_pointer not in pointers and native_state['file_globals']['file_slots'] not in native_pointers
            assert newest==pointers[-1] and count==16
            assert native_state['file_globals']['file_newest']==native_pointers[-1]
            for index in range(16):
                record=struct.unpack('<10I',(original/('file-record-%02u.bin'%index)).read_bytes())
                native_record=struct.unpack('<10I',bytes.fromhex(native_state['file_records'][index]))
                assert record[:9]==native_record[:9]==(0,0,index,0,0,0,0,0,0)
                assert record[9]==(pointers[index-1] if index else 0)
                assert native_record[9]==(native_pointers[index-1] if index else 0)

    if name in MEMORY_READY:
        system=(original/'system-after.bin').read_bytes();memory=(original/'memory-after.bin').read_bytes()
        calls=native_state['system_calls']
        assert [(call['kind'],call['after']) for call in calls]==[('system',system.hex()),('memory',memory.hex())]
        assert struct.unpack('<I',(original/'allocation-page-size.bin').read_bytes())[0]==native_state['allocation_page_size']==struct.unpack_from('<I',system,4)[0]==4096
        assert struct.unpack('<I',(original/'maximum-physical-memory.bin').read_bytes())[0]==native_state['maximum_physical_memory']==struct.unpack_from('<I',memory,8)[0]==0x7fffffff
        assert struct.unpack('<I',(original/'selected-tool-kind.bin').read_bytes())[0]==native_state['selected_tool_kind']==1
        assert original_state['selected_tool_name']==native_state['selected_tool_name']=='Linker'
        assert native_state['selected_tool_name_address']==native_state['first_tool_name_address']
        from binary import PE,u32
        pe=PE(original/'module-000.bin')
        table_name=u32(pe.read(0x467020,4),0)
        assert original_state['selected_tool_name_pointer']==table_name
        if name not in ('_run_linker@8','_initialize_repro'):
            assert (original/'repro-directory.bin').read_bytes()==bytes(4) and native_state['repro_directory']==0
            assert (original/'repro-response.bin').read_bytes()==bytes(4) and native_state['repro_response']==0
        assert native_state['frontier_arguments']['argc']==2
        if name=='_run_linker@8':
            assert original_state['stack'][1]==2
            assert original_state['stack'][2]==original_state['crt_argv_pointer']
        elif name=='_initialize_repro':
            assert original_state['stack'][6]==2
            assert original_state['stack'][7]==original_state['crt_argv_pointer']
        elif name in ('_collect_file_arguments@16','_parse_command_line@16'):
            assert original_state['stack'][1]==2
            assert original_state['stack'][2]==original_state['crt_argv_pointer']
            assert original_state['stack'][3:5]==[0x408508,0x408510]
            assert native_state['environment_arguments']==['LINK','_LINK_']
            assert struct.unpack('<I',(original/'repro-directory.bin').read_bytes())[0]==native_state['repro_directory']==0
            assert struct.unpack('<I',(original/'repro-response.bin').read_bytes())[0]==native_state['repro_response']==0
        assert native_state['frontier_arguments']['argv_address']==native_state['crt_argv_address']
        assert native_state['frontier_arguments']['argv']==native_state['crt_argv']

    if name=='_parse_command_line@16':
        assert (original/'input-prepass.bin').read_bytes()==b'\1' and native_state['input_prepass']==1
        assert (original/'input-files.bin').read_bytes()==bytes(12)
        assert native_state['input_files']==[0,0,0]

    if name=='_parse_argument@8':
        assert original_state['argument_text']==native_state['argument_detail']['text']=='/?'
        assert original_state['stack'][2]==native_state['argument_detail']['source']==0
        assert original_state['argument_pointer']==original_state['crt_first_argument_pointer']
        assert native_state['argument_detail']['address']==native_state['crt_first_argument_address']
        assert (original/'input-prepass.bin').read_bytes()==b'\1' and native_state['input_prepass']==1
        assert (original/'input-files.bin').read_bytes()==bytes(12) and native_state['input_files']==[0,0,0]
        assert (original/'argument-tail.bin').read_bytes()==bytes(4) and native_state['argument_tail']==0
        frames=[e for e in events if e['event']=='main_frame']
        assert len(frames)==1 and frames[0]['site']==0x40de93 and frames[0]['ebp']==original_state['main_frame_pointer']

    if name=='_is_libpath@4':
        assert original_state['argument_text']==native_state['argument_detail']['text']=='?'
        assert original_state['argument_pointer']==original_state['crt_first_argument_pointer']+1
        assert native_state['argument_detail']['address']==native_state['crt_first_argument_address']+1
        assert (original/'input-prepass.bin').read_bytes()==b'\1' and native_state['input_prepass']==1
        assert (original/'input-files.bin').read_bytes()==bytes(12) and native_state['input_files']==[0,0,0]
        assert (original/'argument-tail.bin').read_bytes()==bytes(4) and native_state['argument_tail']==0

    if name in ('_append_argument@8','_append_argument_record@12'):
        assert original_state['argument_text']==native_state['argument_detail']['text']=='?'
        assert original_state['argument_pointer']==original_state['crt_first_argument_pointer']+1
        assert native_state['argument_detail']['address']==native_state['crt_first_argument_address']+1
        assert original_state['stack'][1]==0x47f040
        assert native_state['argument_detail']['list']==native_state['option_arguments_address']
        assert (original/'option-arguments.bin').read_bytes()==bytes(12) and native_state['option_arguments']==[0,0,0]
        assert (original/'input-prepass.bin').read_bytes()==b'\0' and native_state['input_prepass']==0
        assert (original/'input-files.bin').read_bytes()==bytes(12) and native_state['input_files']==[0,0,0]
        assert (original/'argument-tail.bin').read_bytes()==bytes(4) and native_state['argument_tail']==0
        observed=[e for e in events if e['event']=='mbs_observation']
        assert [e['site'] for e in observed]==[0x45c76b,0x45c771]
        for event in observed:
            assert event['left']=='?' and event['right']=='libpath' and event['arguments'][2]==7
            assert event['codepage']==event['lc_ctype_handle']==event['locale_codepage']==0 and event['mb_cur_max']==1
        assert observed[1]['eax']==0xffffffff
        assert native_state['mbsnbicmp_calls']==[dict(left='?',right='libpath',bytes=7,result=-1)]*2

    if name=='_append_argument_record@12':
        assert original_state['stack'][2]==original_state['stack'][3]
        assert native_state['argument_detail']['address']==native_state['argument_detail']['original_address']

    if name=='_has_switch@4':
        assert original_state['argument_text']==native_state['argument_detail']['text']=='vxd'
        assert original_state['argument_pointer']==0x402b30
        first,last,count=struct.unpack('<3I',(original/'option-arguments.bin').read_bytes())
        nfirst,nlast,ncount=native_state['option_arguments']
        assert first==last and first and nfirst==nlast and nfirst and count==ncount==1
        item=struct.unpack('<6I',(original/'option-node.bin').read_bytes())
        nitem=struct.unpack('<6I',bytes.fromhex(native_state['option_node']))
        assert item[:2]==(original_state['crt_first_argument_pointer']+1,)*2
        assert nitem[:2]==(native_state['crt_first_argument_address']+1,)*2
        assert item[2:]==nitem[2:]==(0,0,0,0)
        assert (original/'input-prepass.bin').read_bytes()==b'\0' and native_state['input_prepass']==0
        assert (original/'input-files.bin').read_bytes()==bytes(12) and native_state['input_files']==[0,0,0]
        assert (original/'argument-tail.bin').read_bytes()==bytes(4) and native_state['argument_tail']==0
        assert native_state['mbsnbicmp_calls']==[dict(left='?',right='libpath',bytes=7,result=-1)]*2
