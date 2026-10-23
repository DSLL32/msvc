"""Decode read-only original/native stream observations, retaining distinct slots."""
import json
from pathlib import Path
import struct
from mktemp_state import original_states


def files(raw):
    assert len(raw)==96
    rows=[]
    for i in range(3):
        ptr,count,base,flags,descriptor,charbuf,size,temporary=struct.unpack_from('<8I',raw,32*i)
        assert not temporary,'FILE temporary-name ownership requires explicit proof'
        if ptr or base:
            assert ptr==base and not count,'live FILE bytes require explicit byte capture'
            buffer={'present':True,'position':0,'capacity':size,'live_bytes':''}
        else:
            assert not size and not count,'inconsistent unbuffered FILE state'
            buffer=None
        rows.append({'count':count,'flags':flags,'descriptor':descriptor,
            'character_buffer':charbuf,'buffer_size':size,'buffer':buffer,'temporary':None})
    return rows


def original(directory,phase):
    directory=Path(directory);prefix='streams-'+phase
    record=json.loads((directory/(prefix+'.json')).read_text())
    teb=(directory/(prefix+'-teb.bin')).read_bytes()
    peb=(directory/(prefix+'-peb.bin')).read_bytes()
    parameters=(directory/(prefix+'-parameters.bin')).read_bytes()
    lowio=(directory/(prefix+'-lowio.bin')).read_bytes()
    assert len(teb)==4096 and len(peb)==32 and len(parameters)==36 and len(lowio)==108
    assert struct.unpack_from('<I',teb,0x18)[0]==record['teb']
    assert struct.unpack_from('<I',teb,0x30)[0]==record['peb']
    assert struct.unpack_from('<I',peb,0x10)[0]==record['parameters']
    assert struct.unpack('<I',(directory/(prefix+'-pioinfo-slot.bin')).read_bytes())[0]==record['lowio']
    crt_base=struct.unpack_from('<I',(directory/'crt-state.bin').read_bytes())[0]
    assert record['iob']==crt_base+0x3f570,'canonical provider _iob identity differs'
    assert teb==(directory/('mktemp-'+phase+'-teb.bin')).read_bytes(),'stream/error TEB snapshots differ'
    def identity(item):return (item['volume'],item['index_high'],item['index_low'])
    known={identity(item):name for item,name in zip(record['inherited'][1:],('stdout','stderr'))}
    assert len(known)==2
    def query(item,handle):
        assert item['handle']==handle
        if item['type']==2:
            assert handle==record['inherited'][0]['handle'] and not item['information_valid']
            role='stdin';position=None
        else:
            assert item['type']==1 and item['information_valid']==1
            role=known[identity(item)];position=(item['position_high']<<32)|item['position_low']
        return {'type':item['type'],'role':role,'position':position}
    crt_handles=[struct.unpack_from('<I',lowio,36*i)[0]for i in range(3)]
    win_handles=list(struct.unpack_from('<3I',parameters,0x18))
    if phase=='entry':
        inherited=[item['handle']for item in record['inherited']]
        assert crt_handles==win_handles==inherited,'initial child slots differ from actual inherited startup handles'
    context=(directory/('main-entry-context.bin'if phase=='entry'else'context.bin')).read_bytes()
    errors=original_states(directory,[phase])[phase]
    errors['x87_control_word']=struct.unpack_from('<I',context,28)[0]&0xffff
    return {'crt':[query(item,handle)for item,handle in zip(record['crt'],crt_handles)],
        'win32':[query(item,handle)for item,handle in zip(record['win32'],win_handles)],
        'crt_win32_slot_alias':[a==b for a,b in zip(crt_handles,win_handles)],
        'osfile':[lowio[36*i+4]for i in range(3)],
        'files':files((directory/(prefix+'-iob.bin')).read_bytes()),'errors':errors}


def native(directory,phase):
    directory=Path(directory);prefix='streams-'+phase
    record=json.loads((directory/(prefix+'.json')).read_text())
    assert record['pid']==json.loads((directory/'native-process.json').read_text())['pid']
    assert len(record['crt'])==len(record['win32'])==len(record['osfile'])==3
    return {'crt':[{key:item[key]for key in ('type','role','position')}for item in record['crt']],
        'win32':[{key:item[key]for key in ('type','role','position')}for item in record['win32']],
        'crt_win32_slot_alias':[a['handle']==b['handle']for a,b in zip(record['crt'],record['win32'])],
        'osfile':record['osfile'],'files':files((directory/(prefix+'-iob.bin')).read_bytes()),'errors':record['errors']}
