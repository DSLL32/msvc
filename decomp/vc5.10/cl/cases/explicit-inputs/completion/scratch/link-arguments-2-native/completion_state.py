"""Decode read-only native CRT observations at real API call boundaries."""
import json
from pathlib import Path
import struct
from frontier_state import Memory,dependency_crt

SCALARS={'fmode':'recon_fmode','commode':'recon_commode','mb_cur_max':'recon_mb_cur_max',
    'mb_codepage':'recon_mb_codepage','lc_codepage':'recon_lc_codepage','lc_handles':'recon_lc_handle',
    'mbctype':'recon_mbctype','pctype_slot':'recon_pctype','environment_slot':'recon_crt_environment',
    'initenv_slot':'recon_crt_initenv','os_environment_slot':'recon_crt_os_environment'}


def observe(directory,gdb,phase,status=None):
    from gdb_mktemp import errors
    value=lambda expression:int(gdb.parse_and_eval(expression))&0xffffffff
    addresses={name:value('(unsigned)&'+expression)for name,expression in SCALARS.items()}
    addresses['initial_pctype']=value('(unsigned)(character_flags+1)')
    ranges=[]
    def read(address,length):
        data=bytes(gdb.selected_inferior().read_memory(address,length))
        ranges.append({'address':address,'data':data.hex()});return data
    dependency_crt(Memory(read,0),addresses,after_child=True)
    record={'phase':phase,'pid':gdb.selected_inferior().pid,'addresses':addresses,'ranges':ranges,
        'errors':errors(),'x87_control_word':value('$fctrl'),'status':status,
        'mre_calls':value('mre_call_count')}
    (Path(directory)/('native-'+phase+'-gdb.json')).write_text(json.dumps(record,indent=2)+'\n')
    from gdb_streams import capture
    capture(directory,phase)


def decode(directory,phase,response,pid):
    directory=Path(directory);raw=json.loads((directory/('native-'+phase+'-gdb.json')).read_text())
    assert raw['phase']==phase and raw['pid']==pid and raw['mre_calls']==0
    ranges=iter(raw['ranges']);backing={}
    def read(address,length):
        item=next(ranges);data=bytes.fromhex(item['data'])
        assert item['address']==address and len(data)==length,'completion memory decoder requests differ'
        for offset,byte in enumerate(data):
            key=address+offset
            assert key not in backing or backing[key]==byte
            backing[key]=byte
        return data
    addresses=raw['addresses'];memory=Memory(read,0)
    state=dependency_crt(memory,addresses,after_child=True)
    assert next(ranges,None)is None,'unused completion memory records'
    assert state=={'fmode':0,'commode':0,'mb_cur_max':1,'mb_codepage':1252,'lc_codepage':1252,
        'lc_handles':[0,0,1033,0,0,0],'mbctype_hex':response['post_mbctype_hex'],'ctype_hex':response['post_ctype_hex'],
        'environment':response['child']['environment'],'initial_environment':response['initial_environment'],
        'environment_vector_moved':True,'initial_strings_copied':True,'os_environment':response['child']['environment']}
    word=lambda address:struct.unpack('<I',bytes(backing[address+i]for i in range(4)))[0]
    current=word(addresses['environment_slot'])
    from dependency_native import decode as request_decode
    free=request_decode((directory/'dependency-native/request-3-3.bin').read_bytes())
    previous=free['environment']
    assert current!=previous['address'] and memory.vector_slots[current][:78]==previous['pointers']
    assert len(memory.vector_slots[current])==79 and memory.vector_slots[current][-1]not in previous['pointers']
    expected=response['child']['after']if phase=='postchild'else response['exit']['errors']
    assert raw['errors']==expected and raw['x87_control_word']==639
    assert raw['status']==(None if phase=='postchild'else 0),'actual exit formal differs'
    streams=json.loads((directory/('streams-'+phase+'.json')).read_text())
    assert streams['pid']==pid and streams['mre_calls']==0
    assert len(streams['crt'])==len(streams['win32'])==len(streams['osfile'])==3
    assert list(struct.unpack('<24I',(directory/('streams-'+phase+'-iob.bin')).read_bytes()))==free['file_words'],'actual FILE fields/ownership changed after child'
    for index,old in enumerate(free['streams']):
        assert streams['osfile'][index]==old['osfile'],'actual CRT descriptor flags changed'
        for kind in ('crt','win32'):
            expected=dict(old[kind])
            if expected['type']==2:expected['position']=None
            observed=streams[kind][index]
            assert {name:observed[name]for name in expected}==expected,'actual standard handle identity/position changed after child'
    assert streams['errors']=={**raw['errors'],'x87_control_word':raw['x87_control_word']}
    return state
