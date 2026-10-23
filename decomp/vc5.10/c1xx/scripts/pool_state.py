"""Encountered four-field POD and actual ECX/stack thiscall frontier."""
import re
import struct

FIELDS=('cursor','remaining','minimum_pages','arena')

def check(s,original,native,trial,text,pre,phase,state,seed,final,symbols,traps):
    def raw(suffix):
        blob=(original/('pool-01-'+suffix+'.bin')).read_bytes()
        s.require(len(blob)==16,'missing raw pool object')
        return dict(zip(FIELDS,struct.unpack('<4I',blob)))
    zero=dict.fromkeys(FIELDS,0)
    s.require(raw('000')==zero,'original pool defaults differ')
    old_saved=struct.unpack_from('<I',(original/'frontend-state-006.bin').read_bytes(),4)[0]
    new_saved=pre['frontend']['saved_state']
    s.require(raw('006')==(dict(zip(FIELDS,[old_saved+4,23,37,old_saved+8])) if seed else zero),'original nonzero pool input differs')
    s.require(pre['frontend']['pools']==[dict(zip(FIELDS,[new_saved+4,23,37,new_saved+8])) if seed else zero],'native nonzero pool input differs')
    s.require(raw('phase')==zero and phase['frontend']['pools']==[zero],'pool escaped original/native BSS clearing')
    s.require(raw(final)==zero and state['frontend']['pools']==[zero],'pool wrapper changed pre-constructor state')
    s.require(state['symbol']=='CompilerPool::Initialize' and state['args']=={'this':state['frontend']['storage']['pool_01.cursor'][0],'minimum_pages':1},'actual native member arguments differ')
    line=next(line for line in text.splitlines() if line.startswith('trap seq='+traps[-1][0]+' '))
    s.require('ecx=104c0ac8 ' in line and int(traps[-1][2].split(',')[1],16)==1,'original thiscall ECX/stack argument differs')
    s.require(trial['driver'][-1]['name']=='CompilerPool::Initialize' and trial['driver'][-1]['symbol']=='?Initialize@CompilerPool@@QAEPAU1@I@Z' and trial['driver'][-1]['native_symbol']=='_ZN12CompilerPool10InitializeEj','unbound member symbol projection')
