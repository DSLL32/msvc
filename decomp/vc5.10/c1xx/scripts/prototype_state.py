"""The encountered20-byte empty metadata prototype, without guessed fields."""
import json
import re


def check(s, original, native, trial, text, pre, phase, state, seed, final, symbols, traps):
    name='empty_metadata_prototype'
    def raw(suffix):
        blob=(original/('empty-prototype-'+suffix+'.bin')).read_bytes()
        s.require(len(blob)==20,'missing complete prototype value')
        return list(blob)
    s.require(raw('000')==[0]*20,'original prototype default differs')
    s.require(raw('006')==pre['frontend'][name]==([165]*20 if seed else [0]*20),'prototype nonzero BSS prestate differs')
    s.require(raw('phase')==phase['frontend'][name]==[0]*20,'prototype escaped BSS reset')
    entry=json.loads((native/'prototype-entry.json').read_text())
    result=json.loads((native/'prototype-result.json').read_text())
    rows=[r for r in traps if r[1]=='00038ab7'];s.require(len(rows)==1,'missing actual prototype callback entry')
    suffix='%03d'%int(rows[0][0]);aux=bool(trial.get('test_prototype_seed'));before=list(range(1,21)) if aux else [0]*20
    s.require(raw(suffix)==entry['frontend'][name]==before and entry['symbol']=='initialize_frontend_36' and entry['args']=={},'actual prototype reset input differs')
    native_start=int(re.search(r'^([0-9a-f]+) [0-9a-f]+ [Tt] initialize_frontend_36$',symbols,re.M)[1],16)
    s.require(entry['pc']==native_start,'prototype input not observed at actual entry')
    if aux:
        s.require(trial['auxiliary_prototype_data_seed']=={'owner':'_initialize_frontend_36','point':'actual entry before target body, after BSS reset','bytes':20,'values':'1..20','code_or_allocator_mock':False} and 'prototype-seed actual-entry=00038ab7 bytes=20 values=1..20 code-mock=0\n' in text,'undeclared prototype data auxiliary')
    else:s.require('auxiliary_prototype_data_seed' not in trial and 'prototype-seed ' not in text,'hidden prototype data auxiliary')
    after_suffix='%03d'%int(next(r[0] for r in traps if r[1]=='00038ad8'))
    s.require(raw(after_suffix)==result['frontend'][name]==raw(final)==state['frontend'][name]==[0]*20,'prototype was not completely reset')
    a=(original/'prototype-bss-entry.bin').read_bytes();z=(original/'prototype-bss-result.bin').read_bytes();offset=0xc64a0-0xb9000
    s.require(len(a)==len(z)==0xf69c and a[:offset]==z[:offset] and a[offset+20:]==z[offset+20:] and list(a[offset:offset+20])==before and z[offset:offset+20]==bytes(20),'original reset changed an unrelated BSS byte')
    old=dict(entry['frontend']);new=dict(result['frontend']);old.pop(name);new.pop(name)
    s.require(old==new,'prototype callback changed neighboring native modeled state')
    for frame in (entry,result):
        s.require(frame['target_state']['last_error']==203 and frame['control_word']==0x27f,'prototype reset changed caller state')
