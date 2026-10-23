"""Exact direct-call observations, separate from public compiler fixtures."""
import json
from pathlib import Path
import re
import struct

def original_state(text, boundary, buffers):
    memory={int(a,16):bytes.fromhex(h)for a,n,h in re.findall(r'^memory ([0-9a-f]+) (\d+) ([0-9a-f]+)$',text,re.M)}
    streams={}
    for i,kind,h in re.findall(r'^buffer (\d+) (unopened|\d+)(?: ([0-9a-f]*))?$',text,re.M):
        streams[buffers[int(i)]]=None if kind=='unopened'else h
    match=re.search(r'^asm-state '+boundary+r' (.+)$',text,re.M)
    assert match,'Missing original direct operand state'
    return dict(error_count=int.from_bytes(memory[0x4b228],'little'),
                asm_current_token=memory[0x4b148][0],
                current_token_value=memory[0x4b148][8:16].hex(),
                repair_control=memory[0x610f4].hex(),
                type_0000=int.from_bytes(memory[0x5b700],'little'),
                asm_state=[int(x)for x in match[1].split()],
                asm_name=re.search(r'^asm-name '+boundary+r' (.*)$',text,re.M)[1],
                buffers=streams)

def compare_state(original,native):
    for key in ['error_count','asm_current_token','repair_control','asm_state','asm_name','buffers']:
        assert original[key]==native[key],('direct state differs',key,original[key],native[key])
    assert original['type_0000'] and native['type_0000'],'uninitialized type owner'
    # The helpers only read this pointer when returning the known type owner;
    # the raw union at current_token_value is not consumed by the newline path.

def validate(original_directory,native_directory,settings,config):
    original_directory=Path(original_directory);native_directory=Path(native_directory)
    text=(original_directory/'events').read_text();native=json.loads((native_directory/'state.json').read_text());names=[r['name'][1:]for r in config['buffered_streams']]
    returned='direct-return eax 'in text
    split=text.index('direct-return eax 'if returned else'exception 80000003')
    before=original_state(text[:split],'direct-entry',names);compare_state(before,native['before'])
    assert settings['arguments']==native['arguments']==native['formals'],'direct inputs differ'
    target=next(r for r in config['driver']if r['name']==settings['logical'])
    start=re.search(r'^direct-start target ([0-9a-f]+) marker ([0-9a-f]+) stack ([0-9a-f]+) count (\d+) args ([0-9a-f]+) ([0-9a-f]+)$',text,re.M)
    assert int(start[1],16)==target['start'] and int(start[4])==len(settings['arguments'])
    assert [int(x,16)for x in start.groups()[4:4+len(settings['arguments'])]]==settings['arguments']
    if not returned:
        block=text[split:];exception=re.search(r'^exception 80000003 rva ([0-9a-f]+) base ([0-9a-f]+)',block,re.M)
        assert int(exception[1],16)+int(exception[2],16)==target['start']
        slots={int(i):int(x,16)for i,x in re.findall(r'^stack (\d+) ([0-9a-f]+)$',block,re.M)}
        assert [slots[i+1]for i in range(len(settings['arguments']))]==settings['arguments']
        assert native['status']=='entry'
        return dict(status='pass',kind='fresh matching direct entry',public_reachability=False)
    assert native['status']=='returned'
    assert native['after']['registers']['esp']==native['stack']+4+4*len(settings['arguments']), 'native direct cleanup differs'
    assert native['canary']=='a5'*16, 'native direct canary differs'
    for register in ['ebx','esi','edi','ebp','fctrl']:
        assert native['before']['registers'][register]==native['after']['registers'][register], ('native direct preserved register differs',register)
    after=original_state(text[split:],'direct-return',names);compare_state(after,native['after'])
    result=re.search(r'^direct-return eax ([0-9a-f]+) edx ([0-9a-f]+) esp ([0-9a-f]+) expected ([0-9a-f]+) nonvolatile (\d+) fctrl ([0-9a-f]+) before ([0-9a-f]+)$',text,re.M)
    assert result and result[3]==result[4] and result[5]=='1' and result[6]==result[7]
    assert re.search(r'^direct-canary (.+)$',text,re.M)[1]=='a5a5a5a5 '*3+'a5a5a5a5'
    raw=dict(eax=int(result[1],16),edx=int(result[2],16))
    if settings['logical']=='_asm_recovery_value@4':
        token=settings['arguments'][0]
        if token==43:
            assert raw['edx']==after['type_0000']
            assert native['after']['registers']['edx']==native['after']['type_0000']
            defined=dict(eax_mask=0,edx_mask=0xffffffff,edx_owner='type_0000',provenance='4c9a8..4c9af writes only local+4;4c9bb..4c9c6 loads EAX/EDX then RET4')
        else:
            value=0 if token in [39,40,41,42,44,45,47,48,49]else token
            assert raw['eax']==native['after']['registers']['eax']==value
            defined=dict(eax_mask=0xffffffff,edx_mask=0,expected_eax=value,provenance='4c9b3 writes only local+0 zero or4c9a2 writes only token;4c9bb..4c9c6 loads EAX/EDX then RET4; table ownership independently audited')
    else:
        defined=dict(eax_mask=0,edx_mask=0,provenance='void result: caller consumes neither EAX nor EDX')
    for name in ['stdout','stderr']:
        assert (original_directory/name).read_bytes()==(native_directory/name).read_bytes(),('direct stream differs',name)
    original_work=original_directory/'work';native_work=native_directory/'vc1.0/c1/asm-parser-repair'
    original_files={p.name:p.read_bytes()for p in original_work.iterdir()if p.is_file()}
    native_files={p.name:p.read_bytes()for p in native_work.iterdir()if p.is_file()}
    assert original_files==native_files,('direct artifact set/bytes differ',original_files.keys(),native_files.keys())
    return dict(status='pass',kind='direct body/return',public_reachability=False,defined_result=defined,original_raw_result=raw,native_raw_result={r:native['after']['registers'][r]for r in ['eax','edx']},original_before=before,original_after=after)
