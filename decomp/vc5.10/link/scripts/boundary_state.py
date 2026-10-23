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
    '_register_timing_cleanup': (4,3,3), '_main': (None,3,4), '_initialize_debug@8':(None,3,4), '_initialize_messages@16':(None,3,4),
}
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
    if name in ('_register_timing_cleanup','_main','_initialize_debug@8','_initialize_messages@16'):
        start,end,output,options,display=struct.unpack('<4IB3x',timing)
        nstart,nend,noutput,noptions,ndisplay=struct.unpack('<4IB3x',bytes.fromhex(native_state['app_globals']['timing']))
        # GetTickCount values are identified nondeterministic clocks only.
        assert start==end and nstart==nend
        assert output==original_state['crt_base']+0x3f590
        assert noutput==native_state['crt_stdout_address']
        assert options==noptions==display==ndisplay==0
        assert timing[17:]==bytes.fromhex(native_state['app_globals']['timing'])[17:]==b'\0'*3
        stdout=(original/'crt-stdout.bin').read_bytes()
        assert stdout.hex()==native_state['crt_stdout_bytes']
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

    if name in ('_initialize_debug@8','_initialize_messages@16'):
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

    if name=='_initialize_messages@16':
        assert original_state['stack'][1:5]==list(native_state['frontier_arguments'].values())==[16,14,18,16]
        assert original_state['program_name']==native_state['program_name']=='LINK'
        assert original_state['program_name_pointer']==original_state['main_frame_pointer']-0x23c
        assert native_state['program_name_address']==native_state['basename_address']
        flags=(original/'debug-flags.bin').read_bytes()
        assert flags.hex()==native_state['debug_flags'] and not any(flags)
