"""Shared metadata and raw-evidence parsing; no execution or Wine imports."""
import hashlib
import json
from pathlib import Path
import re

HERE=Path(__file__).resolve().parents[1]
ROOT=HERE.parents[2]
CRT_SHA='e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210'

def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def read(path):return json.loads(Path(path).read_text())
def names(config):return config['driver']+config['support']+config['imports']
def symbol(logical):return logical.lstrip('_').split('@')[0]
def source_pins():
    paths=[p for p in (HERE/'src').glob('*') if p.suffix in ['.c','.h','.def']]
    paths += [HERE/'scripts'/name for name in ['native-host.c','native-boundary.c','native-fixture.h']]
    return {str(p.relative_to(HERE)):sha(p) for p in sorted(paths)}
def parse_original(directory,config):
    directory=Path(directory);text=(directory/'events.txt').read_text()
    assert 'error ' not in text and 'exception ' not in text,'Original debugger error/exception'
    fs=re.findall(r'^frontier ([0-9a-f]{8}) .*$',text,re.M)
    exits=re.findall(r'^exit ([0-9]+)$',text,re.M)
    assert len(exits)==1,'Missing/ambiguous actual exit event'
    records={r['start']-0x1000000:r for r in names(config)}
    if fs:
        assert len(fs)==1,'Ambiguous frontier'
        rva=int(fs[0],16);assert rva in records,'Unknown original trap location'
        assert int(exits[0])==124,'Wrong trapped process exit'
        row=records[rva];state={'frontier':row['name'],'completion':False,'status':124}
        stack=re.search(r'^stack((?: [0-9a-f]{8}){12})$',text,re.M)
        assert stack,'Missing raw frontier stack'
        words=[int(s,16) for s in stack[1].split()]
        if row['name']=='_rc_main':
            state['arguments']={'argc':words[1],'argv':hex_records(text,'frontier-argv'),'env':hex_records(text,'frontier-env')}
        elif row['name']=='_rc_diagnostic@12':
            state['arguments']={'window':words[1],'code':words[2],'message':hex_records(text,'frontier-message')[0]}
        elif row['name']=='import:RC':
            assert words[3]==0x1001100,'Wrong diagnostic pointer'
            state['arguments']={'window':words[1],'flags':words[2],'diagnostic':'_rc_diagnostic@12','resource':words[4],'argc':words[5],'argv':hex_records(text,'frontier-rc-argv')}
        elif row['name']=='import:_controlfp':
            state['arguments']={'value':words[1],'mask':words[2]}
        elif row['name']=='import:_initterm':
            begin,end=words[1:3]
            assert begin in [0x1002000,0x1002008] and end==begin+4,'Wrong initializer range'
            values=hex_records(text,'frontier-initializers');assert values==['00000000'],'Unexpected initializer content'
            state['arguments']={'table':'xc' if begin==0x1002000 else 'xi','count':1,'entries':[0]}

    else:
        assert int(exits[0])==0,'Original did not complete successfully'
        state={'frontier':None,'completion':True,'status':0}
    entries=[]
    for rva in re.findall(r'^entry ([0-9a-f]{8})$',text,re.M):
        assert int(rva,16) in records,'Unknown traced entry'
        name=records[int(rva,16)]['name']
        if name in {r['name'] for r in config['driver']} and name not in entries:entries.append(name)
    if fs and state['frontier'] in {r['name'] for r in config['driver']} and state['frontier'] not in entries:entries.append(state['frontier'])
    if config.get('state_contract'):
        raw=bytes.fromhex(hex_records(text,'data')[0]) if fs else None
        if raw is not None:
            import struct
            words=struct.unpack('<14I',raw)
            state['crt_state']={'initializers':list(words[:4]),'wildcard':words[4],'newmode':words[5],'default_math_error':words[6],'commit_mode':words[7],'file_mode':words[8],'onexit_begin':words[11],'onexit_end':words[12],'fdiv_adjust':words[13]}
    state['entries']=entries
    return state

def hex_records(text,tag):
    values=[]
    for count,data in re.findall(r'^'+re.escape(tag)+r' ([0-9]+) ([0-9a-f]*)$',text,re.M):
        b=bytes.fromhex(data);assert len(b)==int(count),'Raw byte length mismatch';values.append(b.hex())
    return values

def parse_native(directory):
    directory=Path(directory)
    raw=(directory/'gdb.stdout').read_text()
    lines=re.findall(r'^RC_RAW_STATE=(\{[^\n]*\})$',raw,re.M)
    assert len(lines)==1,'Missing/ambiguous native raw state'
    state=json.loads(lines[0]);assert state.get('completion') is True or state.get('frontier'),'No actual native frontier/completion'
    if state['completion']:
        assert state.get('frontier') is None and state.get('status')==0,'Invalid native completion state'
        assert len(re.findall(r'^\[Inferior [0-9]+ \(process [0-9]+\) exited normally\]$',raw,re.M))==1,'Missing actual GDB normal-exit event'
    if not state['completion']:
        from native_pc import validate
        validate(state.pop('pc_check',None),state['frontier'],raw)
    state['entries']=[line for line in (directory/'trace.txt').read_text().splitlines()]
    return state
