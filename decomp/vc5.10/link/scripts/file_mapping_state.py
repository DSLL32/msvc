"""Created output handle and complete16-slot mapping registry at its real callee."""
import functools
import hashlib
import json
import struct
import subprocess
from pathlib import Path


@functools.lru_cache(maxsize=8)
def mapping_symbol(path,digest):
    assert hashlib.sha256(Path(path).read_bytes()).hexdigest()==digest
    rows=[line.split()for line in subprocess.check_output(['nm','-S','-n','--defined-only',str(path)],text=True).splitlines()]
    row,=[r for r in rows if len(r)==4 and r[3]=='mapped_files']
    assert int(row[1],16)==64 and row[2]in ('B','D')
    return int(row[0],16)


def compare_created(original,state,trap,events,name,newname,oslots,nslots):
    before,after=[e for e in events if e['event']=='mapped_create_observation']
    call=state['mapped_create_call']
    assert(before['before'],after['before'])==(1,0)
    assert(before['site'],after['site'])==(0x40c914,0x40c91a)
    assert before['arguments']==after['arguments']==[name,0xc0000000,0,0,2,0x80,0]
    assert call['arguments']==[newname,0xc0000000,0,0,2,0x80,0]
    assert before['text']==after['text']==call['text']=='hello.exe'
    assert before['stack']==before['entry_stack']==after['entry_stack']and after['stack']==before['stack']+28
    assert call['normal_return']is True and call['return_stack']==call['entry_stack']+32
    handle=after['result'];nhandle=call['result']
    assert handle not in (0,0xffffffff)and nhandle not in (0,0xffffffff)
    errors={e['phase']:e for e in events if e['event']=='crt_error_state'}
    for phase,last in [('before',2),('after',0)]:
        assert {k:errors['mapped-create-'+phase][k]for k in ('errno','doserrno','last_error')}==call[phase+'_errors']==dict(errno=2,doserrno=2,last_error=last)
    assert struct.unpack('<16I',(original/'mapped-file-slots.bin').read_bytes())==(oslots[15],)+(0,)*15
    assert state['mapped_file_slots']==[nslots[15]]+[0]*15
    assert bytes.fromhex(state['mapped_file_slots_bytes'])==struct.pack('<16I',*state['mapped_file_slots'])
    assert state['mapped_file_slots_address'] # Bound to real BSS symbol in the enclosing compare.
    completed=state['function']=='create_library'
    current=trap['mapping_formals'];ncurrent=state['mapping_formals'];physical=state['mapping_entry']
    assert physical['hidden_inputs']==[]and physical['raw_formals']==[nslots[15],0,state['mapped_open_formals']['size']]
    assert current==dict(record=oslots[15],resize=0,raw_resize_word=0,error=trap['mapped_open_formals']['size'],initial_error=0)
    assert ncurrent==dict(record=nslots[15],resize=0,raw_resize_word=0,error=state['mapped_open_formals']['size'],initial_error=0)
    if completed:
        observed,=[e for e in events if e['event']=='mapping_entry']
        assert observed['hidden_inputs']==[]and observed['raw_formals']==[current['record'],0,current['error']]
    else:assert trap['stack'][1:4]==[current['record'],0,current['error']]
    if not completed:
        for metadata,formals in ((trap,current),(state,ncurrent)):
            assert metadata['frontier_inputs']==dict(formals=[formals['record'],0,formals['error']],hidden_register_inputs=[])
        assert trap['ebp']==current['error']
    return handle,nhandle
