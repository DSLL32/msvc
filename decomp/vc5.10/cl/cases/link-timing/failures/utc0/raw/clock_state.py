"""Derive fresh native timing effects from addressed GDB bytes/registers."""
import json
from pathlib import Path
import struct
from link_state import decode as link_state

def verify(directory,response,identity,pid):
    directory=Path(directory);clock=response['clock'];count=clock['expected_calls']
    raw=json.loads((directory/'native-clock-gdb.json').read_bytes())
    assert len(raw['calls'])==count and count in(1,2)
    started_address=None;first_result=None
    for index,record in enumerate(raw['calls'],1):
        assert record['sequence']==index and record['caller']==('start_timer','stop_timer')[index-1]
        slot=bytes.fromhex(record['argument_slot_hex']);assert len(slot)==4
        argument=struct.unpack('<I',slot)[0];assert record['argument_slot']!=argument
        observations={}
        for when in('before','after'):
            value=record[when];assert value['pid']==pid and value['pointer']==argument
            backing={}
            for span in value['ranges']:
                for offset,byte in enumerate(bytes.fromhex(span['data'])):
                    address=span['address']+offset
                    assert address not in backing or backing[address]==byte
                    backing[address]=byte
            read=lambda address,length:bytes(backing[address+i]for i in range(length))
            symbols=value['symbols'];target=clock['calls'][index-1][when]
            assert argument==symbols['started'if index==1 else'stopped']
            if started_address is None:started_address=symbols['started']
            else:assert started_address==symbols['started']
            buffer=read(argument,12);started=read(symbols['started'],12)
            assert buffer.hex()==target['timeb']['raw_hex']==value['timeb_hex']
            assert started.hex()==value['started_hex']
            if index==2:assert started==first_result
            errors={name:struct.unpack('<I',read(symbols[name],4))[0]for name in('errno','doserrno','last_error')}
            assert errors==target['errors']
            counter=struct.unpack('<Q',read(symbols['counter'],8))[0]
            assert counter==index-(when=='before')==value['counter']
            assert struct.unpack('<I',read(symbols['timezone'],4))[0]==0
            assert struct.unpack('<I',read(symbols['daylight'],4))[0]==0
            registers=value['x87_registers'];assert len(registers)==16
            assert int.from_bytes(bytes.fromhex(registers['fctrl']),'little')==value['fctrl']==639
            observations[when]=registers
            if index==1 and when=='after':first_result=buffer
        assert observations['before']==observations['after'],'native _ftime changed live x87 state'
    phases=('link-before','link-after')if count==1 else('link-before','link-after','link-postchild','link-exit')
    states={phase:link_state(directory,phase,response,identity,pid)for phase in phases}
    before=states['link-before']
    for phase,state in states.items():
        for key in('environment_address','environment_pointers','os_address','pctype','file_words'):
            assert state[key]==before[key],'native timing changed state: '+key
        streams=json.loads(json.dumps(state['streams']))
        if phase in('link-postchild','link-exit'):
            for family in('crt','win32'):
                for index in(1,2):streams[family][index]['position']=before['streams'][family][index]['position']
        assert streams==before['streams'],'native timing changed stream aliases'
    assert states['link-after']['argv']==before['argv']
    if count==1:
        # The real next target frontier occurs after the child returned. Its
        # independent stopped stream capture must retain the same FILE/handles.
        stream=json.loads((directory/'streams-frontier.json').read_bytes())
        assert stream['pid']==pid and {k:stream[k]for k in('crt','win32','osfile')}==before['streams']
        assert list(struct.unpack('<24I',(directory/'streams-frontier-iob.bin').read_bytes()))==before['file_words']
        assert stream['errors']=={**response['parent']['phases']['spawn-after']['errors'],'x87_control_word':639}
    else:
        unlink=json.loads((directory/'native-link-unlink-gdb.json').read_bytes())
        expected='.\\'+response['child']['argv'][2][3:]
        assert unlink['pid']==pid and unlink['argument_address']==unlink['caller_buffer_address']
        assert bytes.fromhex(unlink['filename_bytes'])==expected.encode('ascii')+b'\0'
        assert unlink['present_before']is True and unlink['present_after']is False and unlink['result']==0
        after={name:struct.unpack('<I',bytes.fromhex(row['bytes']))[0]for name,row in unlink['after_raw'].items()}
        assert after==states['link-postchild']['errors']==states['link-exit']['errors']
    return states
