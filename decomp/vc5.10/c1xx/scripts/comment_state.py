"""Encountered generated header text and COM flag, without guessed capacity."""
import json
import re
import struct


def check(s, original, native, trial, text, pre, phase, state, seed, final, symbols, traps):
    names=('com_initialized','generated_comment','comment_value_offset','generated_comment_length')
    zero=[0,[0]*74,0,0];nonzero=[37,[165]*74,23,29]
    expected=[0,list(b'// Created by Microsoft (R) C/C++ Compiler Version 11.00.7307 (XXXXXXXX).\0'),63,73]
    def raw(suffix):
        blob=(original/('generated-comment-'+suffix+'.bin')).read_bytes();s.require(len(blob)==86,'missing complete encountered comment fields');return [struct.unpack_from('<I',blob)[0],list(blob[4:78]),*struct.unpack_from('<2I',blob,78)]
    def values(snapshot):return [snapshot['frontend'][name] for name in names]
    s.require(raw('000')==zero,'loaded comment defaults differ')
    s.require(raw('006')==values(pre)==(nonzero if seed else zero),'comment nonzero BSS input differs')
    s.require(raw('phase')==values(phase)==zero,'comment escaped BSS clearing')
    entry=json.loads((native/'comment-entry.json').read_text());result=json.loads((native/'comment-result.json').read_text())
    rows=[r for r in traps if r[1]=='00038af9'];s.require(len(rows)==1,'missing actual comment constructor entry')
    suffix='%03d'%int(rows[0][0]);aux=bool(trial.get('test_comment_seed'))
    s.require(raw(suffix)==values(entry)==(nonzero if aux else zero),'actual comment constructor input differs')
    start,size=[int(v,16) for v in re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] initialize_generated_comment$',symbols,re.M).groups()]
    wrapper,wsize=[int(v,16) for v in re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] initialize_frontend_39$',symbols,re.M).groups()]
    abi=entry['raw_entry_abi']
    s.require(entry['symbol']=='initialize_generated_comment' and entry['pc']==start and entry['args']=={} and abi['stack_arguments']==[] and abi['esp']==entry['esp'] and wrapper<abi['return_pc']<wrapper+wsize and int(rows[0][2].split(',')[0],16)==0x10438af4,'constructor cdecl(void)/real caller boundary differs')
    if aux:
        s.require(trial['auxiliary_comment_data_seed']=={'owner':'_initialize_generated_comment','point':'actual constructor entry after BSS reset','com_initialized':37,'generated_comment':'74 bytes165','comment_value_offset':23,'generated_comment_length':29,'code_or_allocator_mock':False} and 'comment-seed actual-entry=00038af9 bytes=74 value=165 com=37 offset=23 length=29 code-mock=0\n' in text,'undeclared comment auxiliary')
    else:s.require('auxiliary_comment_data_seed' not in trial and 'comment-seed ' not in text,'hidden comment auxiliary')
    cleanup=[row for row in traps if row[1]=='000386ae'];s.require(len(cleanup)==12,'missing actual post-constructor cleanup registration')
    last=cleanup[-1];s.require(int(last[2].split(',')[1],16)==0x104346d4,'wrong original generated-comment cleanup callback')
    s.require(raw('%03d'%int(last[0]))==values(result)==raw(final)==values(state)==expected,'generated comment value, offset, length or COM reset differs')
    s.require(result['symbol']=='register_cleanup' and result['args']=={'cleanup':int(re.search(r'^([0-9a-f]+) [0-9a-f]+ [Tt] destroy_generated_comment$',symbols,re.M)[1],16)},'native cleanup formal differs')
    registrar=int(re.search(r'^([0-9a-f]+) [0-9a-f]+ [Tt] register_cleanup$',symbols,re.M)[1],16)
    result_abi=result['raw_entry_abi']
    s.require(result['pc']==registrar and result_abi['esp']==result['esp'] and result_abi['stack_arguments']==[result['args']['cleanup']] and wrapper<result_abi['return_pc']<wrapper+wsize,'cleanup registrar actual stack argument/caller differs')
    before=(original/'comment-bss-entry.bin').read_bytes();after=(original/'comment-bss-result.bin').read_bytes();s.require(len(before)==len(after)==0xf69c,'missing whole original BSS neighbor guard')
    ranges=[(0xc6b64,4),(0xc6ba8,74),(0xc6cb8,4),(0xc6c44,4)]
    a=bytearray(before);z=bytearray(after)
    for address,width in ranges:
        off=address-0xb9000;a[off:off+width]=z[off:off+width]=bytes(width)
    s.require(a==z,'constructor changed unknown string tail or unrelated original storage')
    old=dict(entry['frontend']);new=dict(result['frontend'])
    for name in names:old.pop(name);new.pop(name)
    s.require(old==new,'constructor changed other native modeled fields')
    for snapshot in (entry,result):s.require(snapshot['target_state']['last_error']==203 and snapshot['control_word']==0x27f,'comment constructor changed caller state')
