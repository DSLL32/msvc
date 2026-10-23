"""Two encountered opaque queue pointers; original tail slot remains unmodeled."""
import json
import re
import struct


def check(s, original, native, trial, text, pre, phase, state, seed, final, symbols, traps):
    names=('pending_group_head','active_group')
    def raw(suffix):
        data=(original/('pending-groups-'+suffix+'.bin')).read_bytes();s.require(len(data)==12,'missing original group pointers and untouched tail guard');a,tail,b=struct.unpack('<3I',data);return [a,b]
    def values(snapshot):return [snapshot['frontend'][name] for name in names]
    owned=struct.unpack_from('<I',(original/'frontend-state-006.bin').read_bytes(),4)[0]
    s.require(raw('000')==[0,0],'loaded original group pointer defaults differ')
    s.require(raw('006')==([owned+4,owned+8] if seed else [0,0]) and values(pre)==([pre['frontend']['saved_state']+4,pre['frontend']['saved_state']+8] if seed else [0,0]),'group pointer nonzero BSS input differs')
    s.require(raw('phase')==values(phase)==[0,0],'group pointers escaped BSS clearing')
    entry=json.loads((native/'group-entry.json').read_text());result=json.loads((native/'group-result.json').read_text())
    rows=[r for r in traps if r[1]=='00038ae2'];s.require(len(rows)==1,'missing actual group callback entry')
    suffix='%03d'%int(rows[0][0]);old_owned=struct.unpack_from('<I',(original/('frontend-state-'+suffix+'.bin')).read_bytes(),4)[0];new_owned=entry['frontend']['saved_state'];aux=bool(trial.get('test_group_seed'))
    s.require(raw(suffix)==([old_owned+4,old_owned+8] if aux else [0,0]) and values(entry)==([new_owned+4,new_owned+8] if aux else [0,0]),'actual group callback input identities differ')
    start=int(re.search(r'^([0-9a-f]+) [0-9a-f]+ [Tt] initialize_frontend_38$',symbols,re.M)[1],16)
    s.require(entry['symbol']=='initialize_frontend_38' and entry['pc']==start and entry['args']=={},'group input not observed at actual cdecl(void) entry')
    if aux:
        s.require(trial['auxiliary_group_data_seed']=={'owner':'_initialize_frontend_38','point':'actual entry after BSS reset','head':'owned snapshot+4','active':'owned snapshot+8','tail_untouched':True,'pointees_dereferenced':False,'code_or_allocator_mock':False} and 'group-seed actual-entry=00038ae2 head=snapshot+4 active=snapshot+8 tail-untouched=1 code-mock=0\n' in text,'undeclared group pointer auxiliary')
    else:s.require('auxiliary_group_data_seed' not in trial and 'group-seed ' not in text,'hidden group pointer auxiliary')
    result_suffix='%03d'%int(next(row[0] for row in traps if row[1]=='00038aef'))
    s.require(raw(result_suffix)==values(result)==raw(final)==values(state)==[0,0],'group pointers were not reset')
    before=(original/'group-bss-entry.bin').read_bytes();after=(original/'group-bss-result.bin').read_bytes();head=0xc6b40-0xb9000;active=0xc6b48-0xb9000
    s.require(len(before)==len(after)==0xf69c and before[:head]==after[:head] and before[head+4:active]==after[head+4:active] and before[active+4:]==after[active+4:] and after[head:head+4]==after[active:active+4]==bytes(4),'group callback changed original tail or unrelated BSS bytes')
    old=dict(entry['frontend']);new=dict(result['frontend'])
    for name in names:old.pop(name);new.pop(name)
    s.require(old==new,'group callback changed other native modeled state')
    for snapshot in (entry,result):s.require(snapshot['target_state']['last_error']==203 and snapshot['control_word']==0x27f,'group callback changed caller state')
