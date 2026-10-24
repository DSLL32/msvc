"""Actual file-record request: all four inputs and initialized caller outputs."""


import functools
import hashlib
import json
import subprocess
from pathlib import Path


@functools.lru_cache(maxsize=16)
def slot_symbols(path,expected_sha256):
    path=Path(path)
    assert hashlib.sha256(path.read_bytes()).hexdigest()==expected_sha256
    output=subprocess.check_output(['nm','-S','-n','--defined-only',str(path)],text=True)
    symbols={}
    for line in output.splitlines():
        fields=line.split()
        if len(fields)==4 and fields[3]in ('file_newest','pending_files'):
            address,size,kind,name=fields
            assert kind in ('B','D')and int(size,16)==4
            assert name not in symbols
            symbols[name]=int(address,16)
    assert set(symbols)=={'file_newest','pending_files'}
    return symbols


def compare(original,state,trap,events,native_directory=None):
    if state['function']in ('open_mapped_file','initialize_file_mapping','create_library','find_library','initialize_library_enumerator','next_library','LinkContext::LoadInputArgument','read_input_argument','is_archive','mapped_read','is_mac_resource_file','get_file_length','seek_file','seek_mapped_file','read_big_endian_dword','byte_swap_dword','read_object_file','check_input_machine'):return # Full completed mutation contract is in file_move_state.
    entry,=[e for e in events if e['event']=='open_file_entry'];native=state['open_file_entry']
    expected=[trap['output_name_pointer'],0x8302,0x180]
    nexpected=[state['output_name_pointer'],0x8302,0x180]
    assert entry['formals']==expected and native['formals']==nexpected
    assert entry['hidden_inputs']==native['hidden_inputs']==[]
    moving=state['function']=='move_file_record'
    available=state['function']=='get_available_file_record'or moving
    finder=state['function']=='find_cached_file'or available
    if finder:
        acquire,=[e for e in events if e['event']=='file_record_entry']
        nacquire=state['file_record_entry']
        assert acquire['stack_pointer']==entry['stack_pointer']-44
        assert trap['esp']==acquire['stack_pointer']-(48 if moving else 16 if available else 20)
        assert trap['stack'][0]==(0x40b956 if moving else 0x40b88a if available else 0x40b87d)
        assert acquire['hidden_inputs']==nacquire['hidden_inputs']==[]
    else:
        assert trap['esp']==entry['stack_pointer']-44 and trap['stack'][0]==0x40b82e
    current=trap['file_record_formals'];ncurrent=state['file_record_formals']
    for k,value,nvalue in zip(('name','flags','mode'),expected,nexpected):
        assert current[k]==value and ncurrent[k]==nvalue
    assert current['created']==entry['stack_pointer']-8
    if not finder:assert current['created']==trap['stack'][4]
    assert current['size_slot']==entry['stack_pointer']-4
    assert ncurrent['created']==ncurrent['local_created']
    assert ncurrent['created']<native['stack_pointer'] and ncurrent['size_slot']+4<=native['stack_pointer']
    assert not(ncurrent['created']>=ncurrent['size_slot']and ncurrent['created']<ncurrent['size_slot']+4)
    for k in ('initial_created','initial_size','file_access_statistics'):
        assert current[k]==ncurrent[k]==0
    assert trap['ebp']==0xffffffff and ncurrent['initial_mapped_result']==-1
    assert current['created_bytes']==ncurrent['created_bytes']==1
    assert current['created']and ncurrent['created']
    if finder:
        assert acquire['formals']==[current[k]for k in ('name','flags','mode','created')]
        assert nacquire['formals']==[ncurrent[k]for k in ('name','flags','mode','created')]
        if available:
            observed=[e for e in events if e['event']=='file_lookup_observation'];assert len(observed)==2
            before,after=observed;assert(before['before'],after['before'])==(1,0)
            assert(before['site'],after['site'])==(0x40b878,0x40b87d)
            assert before['stack']==before['entry_stack']==after['entry_stack']==acquire['stack_pointer']-16
            assert after['stack']==before['stack']+4
            assert before['name']==after['name']==current['name'] and after['result']==0
            lookup,=state['file_lookup_calls']
            assert lookup['name']==ncurrent['name'] and lookup['result']==0 and lookup['normal_return']is True
            assert lookup['return_stack']==lookup['entry_stack']+8
            empty=dict(cached_files=0,cached_file_tail=0,pending_files=0,cached_file_count=0)
            assert trap['file_cache_lists']==state['file_cache_lists']==empty
            if moving:
                import struct
                source=trap['file_move_formals'];destination=state['file_move_formals']
                opointers=struct.unpack('<16I',(original/'file-slots.bin').read_bytes())
                npointers=struct.unpack('<16I',bytes.fromhex(state['file_slots_bytes']))
                assert source['record']==source['source_value']==opointers[15]
                assert destination['record']==destination['source_value']==npointers[15]
                assert source['source_first']==0x46753c
                assert source['destination_first']==0x4674d8
                assert native_directory is not None
                receipt=json.loads((native_directory/'receipt.json').read_text())
                symbols=slot_symbols(str(native_directory/'compiled/native-LINK.EXE'),receipt['native_sha256'])
                assert state['file_cache_slots']==symbols
                assert destination['source_first']==symbols['file_newest']
                assert destination['destination_first']==symbols['pending_files']
                assert destination['source_first']and destination['destination_first']
                assert destination['source_first']!=destination['destination_first']
                for key in ('source_last','destination_last','append','destination_value'):
                    assert source[key]==destination[key]==0
                assert source['slot_bytes']==destination['slot_bytes']==4
                keys=('record','source_first','source_last','destination_first','destination_last','append')
                assert trap['stack'][1:7]==[source[k]for k in keys]
                assert trap['frontier_inputs']==dict(formals=[source[k]for k in keys],hidden_register_inputs=[])
                assert state['frontier_inputs']==dict(formals=[destination[k]for k in keys],hidden_register_inputs=[])
            else:
                assert trap['frontier_inputs']==state['frontier_inputs']==dict(formals=[],hidden_register_inputs=[])
        else:
            assert trap['stack'][1]==current['name']
            assert trap['frontier_inputs']==dict(formals=[current['name']],hidden_register_inputs=[])
            assert state['frontier_inputs']==dict(formals=[ncurrent['name']],hidden_register_inputs=[])
    else:
        assert trap['frontier_inputs']==dict(formals=[current[k]for k in ('name','flags','mode','created')],hidden_register_inputs=[])
        assert state['frontier_inputs']==dict(formals=[ncurrent[k]for k in ('name','flags','mode','created')],hidden_register_inputs=[])
