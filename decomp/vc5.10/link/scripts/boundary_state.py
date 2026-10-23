"""Explicit raw-dump contracts; new boundaries remain fail closed."""
import json
import struct


def compare(name, original, native, original_state, native_state):
    if name == '_mainCRTStartup':
        # No C arguments or application globals exist at process entry. OS
        # loader register/stack values belong to different native runtimes.
        assert original_state['rva'] == 0x65280
        assert native_state['function'] == 'target_startup'
        return
    if name in ('_initialize_buffer12','_allocate_buffer12'):
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
        assert (original_state['esi']-0x467000)//4 == native_state['initerm_current_index'] == 1
        assert (original_state['edi']-0x467000)//4 == native_state['initerm_end_index'] == 5
        assert original_state['x87_control_word'] == native_state['x87_control_word'] == 0x027f
        for filename, native_name in [('crt-fmode.bin','crt_fmode'),('crt-commode.bin','crt_commode'),('crt-adjust-fdiv.bin','crt_adjust_fdiv')]:
            assert struct.unpack('<i',(original/filename).read_bytes())[0] == native_state[native_name] == 0
        assert original_state['crt_argv'] == native_state['crt_argv'] == ['R:\\toolchains\\5.10\\bin\\LINK.EXE','/?']
        startup = struct.unpack('<9I',(original/'startup-globals.bin').read_bytes())
        assert startup == (0,0,0,0,0,0,0,0xffffffff,0xffffffff)
        for symbol in ['buffer12','buffer8','buffer4','timing']:
            data = (original/(symbol+'.bin')).read_bytes()
            assert data.hex() == native_state['app_globals'][symbol]
            assert not any(data), 'constructor application state is not pristine'
        return
    raise AssertionError('raw state contract not implemented for ' + name)
