"""Typed actual compiler-pass state at normal show_help entry, not a frontier."""
import hashlib
import json
from pathlib import Path
import re
import struct

CONTROL=b'CL_PASS_STATE_V1:preprocessor-pass-options\n'

def decode(directory,platform):
    directory=Path(directory);assert(directory/'pass-state-observer.txt').read_bytes()==CONTROL
    data=(directory/'pass-state.bin').read_bytes();magic,base,width=struct.unpack_from('<3I',data)
    assert magic==0x31535043 and width==180 and len(data)>=192
    rows=[];position=192;identities={};strings={}
    for i in range(5):
        words=struct.unpack_from('<9I',data,12+36*i)
        assert words[5:8]==(0,0,0),'unexpected DLL lifetime in help pass table'
        text={};aliases={}
        for slot,key in ((0,'name'),(1,'path'),(2,'environment'),(8,'options')):
            pointer,length=struct.unpack_from('<2I',data,position);position+=8
            assert pointer==words[slot]and 0<=length<=1024
            if not pointer:assert length==0;value=None;identity=None
            else:
                assert length;raw=data[position:position+length];position+=length
                assert len(raw)==length and raw[-1:]==b'\0'and b'\0'not in raw[:-1]
                value=raw[:-1].decode('ascii');identity=identities.setdefault(pointer,len(identities)+1)
                assert pointer not in strings or strings[pointer]==raw
                strings[pointer]=raw
            text[key]=value;aliases[key]=identity
        prefix=struct.pack('<I',words[3]);assert prefix[2:]==b'\0\0'
        rows.append({**text,'prefix':prefix[:2].decode('ascii'),'flags':words[4],
            'module':None,'invoke':None,'abort':None,'string_aliases':aliases})
    assert position==len(data),'extra pass snapshot payload'
    if platform=='original':
        events=(directory/'events.txt').read_text();records=re.findall(r'^pass-state event=(\d+) site=([0-9a-f]{8}) count=(\d+)$',events,re.M)
        assert len(records)==1 and records[0][1:]==('004052a5','1')and base==0x4090a8
        assert 'entry event='+records[0][0]+' address=004052a5 name=show_help\n'in events
    else:
        report=json.loads((directory/'pass-state-gdb.json').read_text())
        assert report=={'symbol':'show_help','count':1,'base':base,'bytes':len(data)}
    # These are the actual modeled fields, not inferred allocation tail bytes.
    return {'phase':'actual show_help entry','record_count':5,'record_width':36,
        'pointer_and_int_width':4,'structure_padding':'none; finaltwochar-array bytes are initializedzero',
        'records':rows}

def expected(fields):
    rows=fields['records']
    assert [r['flags']for r in rows]==[0,1,1,0,0]
    assert [r['options']for r in rows]==[None,'.pre',None,None,None]
    assert [r['prefix']for r in rows]==['D-','1-','P-','2-','C/']
    assert [r['name']for r in rows]==[None,'c1','c1xx','c2','link.exe']
    assert all(r['path']is None for r in rows)
    assert [r['environment']for r in rows]==[None,'MSC_CMD_FLAGS=','MSC_CMD_FLAGS=','MSC_CMD_FLAGS=',None]
