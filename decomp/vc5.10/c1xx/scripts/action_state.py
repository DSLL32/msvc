"""Deferred action head/tail pointers and signed nesting depth; nodes opaque."""
import json
import re
import struct


def check(s, original, native, trial, text, pre, phase, state, seed, final, symbols, traps):
    names=('deferred_action_head','deferred_action_tail','deferred_action_depth')
    def raw(suffix):
        data=(original/('deferred-actions-'+suffix+'.bin')).read_bytes();s.require(len(data)==12,'missing original deferred action pointers/depth');return list(struct.unpack('<2Ii',data))
    def values(snapshot):return [snapshot['frontend'][name] for name in names]
    owned=struct.unpack_from('<I',(original/'frontend-state-006.bin').read_bytes(),4)[0]
    s.require(raw('000')==[0,0,0],'loaded original deferred action defaults differ')
    s.require(raw('006')==([owned+4,owned+8,17] if seed else [0,0,0]) and values(pre)==([pre['frontend']['saved_state']+4,pre['frontend']['saved_state']+8,17] if seed else [0,0,0]),'deferred action nonzero BSS input differs')
    s.require(raw('phase')==values(phase)==[0,0,0],'deferred actions escaped BSS clearing')
    entry=json.loads((native/'action-entry.json').read_text());result=json.loads((native/'action-result.json').read_text())
    rows=[r for r in traps if r[1]=='00038bea'];s.require(len(rows)==1,'missing actual deferred action callback entry')
    suffix='%03d'%int(rows[0][0]);old_owned=struct.unpack_from('<I',(original/('frontend-state-'+suffix+'.bin')).read_bytes(),4)[0];new_owned=entry['frontend']['saved_state'];aux=bool(trial.get('test_action_seed'))
    s.require(raw(suffix)==([old_owned+4,old_owned+8,17] if aux else [0,0,0]) and values(entry)==([new_owned+4,new_owned+8,17] if aux else [0,0,0]),'actual deferred action callback input identities differ')
    start=int(re.search(r'^([0-9a-f]+) [0-9a-f]+ [Tt] initialize_frontend_41$',symbols,re.M)[1],16)
    s.require(entry['symbol']=='initialize_frontend_41' and entry['pc']==start and entry['args']=={},'deferred action input not observed at actual cdecl(void) entry')
    if aux:
        s.require(trial['auxiliary_action_data_seed']=={'owner':'_initialize_frontend_41','point':'actual entry after BSS reset','head':'owned snapshot+4','tail':'owned snapshot+8','depth':17,'pointees_dereferenced':False,'code_or_allocator_mock':False} and 'action-seed actual-entry=00038bea head=snapshot+4 tail=snapshot+8 depth=17 code-mock=0\n' in text,'undeclared deferred action auxiliary')
    else:s.require('auxiliary_action_data_seed' not in trial and 'action-seed ' not in text,'hidden deferred action auxiliary')
    result_suffix='%03d'%int(next(row[0] for row in traps if row[1]=='00038bfc'))
    s.require(raw(result_suffix)==values(result)==raw(final)==values(state)==[0,0,0],'deferred actions were not reset')
    before=(original/'action-bss-entry.bin').read_bytes();after=(original/'action-bss-result.bin').read_bytes();offset=0xc6d28-0xb9000
    s.require(len(before)==len(after)==0xf69c and before[:offset]==after[:offset] and before[offset+12:]==after[offset+12:] and after[offset:offset+12]==bytes(12),'action callback changed unrelated original BSS bytes')
    old=dict(entry['frontend']);new=dict(result['frontend'])
    for name in names:old.pop(name);new.pop(name)
    s.require(old==new,'deferred action callback changed other native modeled state')
    for snapshot in (entry,result):s.require(snapshot['target_state']['last_error']==203 and snapshot['control_word']==0x27f,'deferred action callback changed caller state')
