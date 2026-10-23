"""Six persistent PDB members; no provider calls or pointee interpretation."""
import json
import re
import struct

FIELDS=('database','names','types','minimal_rebuild','stream_state','read_mode')
OFFSETS=(0,4,8,12,20,24)

def check(s,original,native,trial,text,pre,phase,state,final,symbols,traps):
    def raw(suffix):
        blob=(original/('pdb-context-01-'+suffix+'.bin')).read_bytes()
        s.require(len(blob)==28,'missing PDB persistent raw state')
        return blob,dict(zip(FIELDS,[struct.unpack_from('<I',blob,i)[0] for i in OFFSETS]))
    def seed(pointer):return dict(zip(FIELDS,[pointer+4*i for i in range(5)]+[37]))
    zero=dict.fromkeys(FIELDS,0)
    s.require(raw('000')[1]==zero,'original PDB defaults differ')
    mode=trial.get('test_pdb_seed');seeded=mode is not None
    s.require(not seeded or trial.get('test_seed') in ('restore-nonzero','restore-zero'),'persistent seed has no owned allocation')
    before=pre['frontend'];copied=phase['frontend']
    for suffix,frame in (('006',before),('phase',copied)):
        expected=seed(frame['saved_state']) if seeded else zero
        s.require(raw(suffix)[1]==(seed(struct.unpack_from('<I',(original/('frontend-state-'+suffix+'.bin')).read_bytes(),4)[0]) if seeded else zero),'original persistent seed/reset differs')
        s.require(frame['input_pdb_context']==expected,'native persistent fields incorrectly cleared by BSS reset')
    entry=json.loads((native/'pdb-entry.json').read_text());registration=json.loads((native/'pdb-registration.json').read_text())
    actual=re.search(r'^([0-9a-f]+) [0-9a-f]+ [Tt] initialize_pdb_context_01$',symbols,re.M)
    s.require(actual is not None and entry['pc']==int(actual[1],16) and entry['args']=={},'false PDB initializer entry')
    rows=[row for row in traps if row[1]=='0003894f'];s.require(len(rows)==1,'missing PDB original constructor boundary')
    suffix='%03d'%int(rows[0][0]);original_entry,old=raw(suffix)
    original_saved=struct.unpack_from('<I',(original/('frontend-state-'+suffix+'.bin')).read_bytes(),4)[0]
    s.require(old==(seed(original_saved) if seeded else zero) and entry['frontend']['input_pdb_context']==(seed(entry['frontend']['saved_state']) if seeded else zero),'PDB constructor input differs')
    reuse=mode if seeded else entry['frontend']['restore_mode']
    s.require(reuse in (0,1),'unknown persistent reuse mode')
    old_restore=struct.unpack_from('<I',(original/('frontend-state-'+suffix+'.bin')).read_bytes(),12)[0]
    s.require(old_restore==entry['frontend']['restore_mode']==reuse,'PDB constructor branch condition differs')
    if seeded:
        s.require(trial['auxiliary_pdb_data_seed']=={'point':'initializer entry before BSS reset; restore_mode overridden at PDB constructor entry','owner':'_initialize_pdb_context_01','pointer_fields':'owned snapshot interiors +0/+4/+8/+12/+16; never dereferenced','read_mode':37,'reuse_mode':mode,'code_or_allocator_mock':False},'undeclared PDB seed scope')
        s.require(re.search(r'^pdb-context-seed allocation=%08x read-mode=37 never-dereferenced=1$'%original_saved,text,re.M) and re.search(r'^pdb-reuse-seed value=%d$'%mode,text,re.M),'raw auxiliary PDB seed absent')
    else:s.require('auxiliary_pdb_data_seed' not in trial and 'pdb-context-seed ' not in text and 'pdb-reuse-seed ' not in text,'hidden PDB data override')
    original_after,old_after=raw(final)
    s.require(old_after==(old if reuse else zero) and state['frontend']['input_pdb_context']==(entry['frontend']['input_pdb_context'] if reuse else zero),'PDB conditional reset result differs')
    s.require(original_after[16:20]==original_entry[16:20],'PDB initializer changed unmodeled original +10 gap')
    s.require(registration['args']=={'cleanup':int(re.search(r'^([0-9a-f]+) [0-9a-f]+ [Tt] destroy_pdb_context_01$',symbols,re.M)[1],16)} and registration['frontend']['cleanup_count']==9 and registration['frontend']['input_pdb_context']==state['frontend']['input_pdb_context'],'PDB cleanup registration differs')
    register=[r for r in traps if r[1]=='000386ae'][-1]
    regline=next(line for line in text.splitlines() if line.startswith('trap seq='+register[0]+' '))
    s.require('eax=00000000 ' in regline,'original PDB initializer cdecl int return differs')
    # register_cleanup is cdecl; its prologue may use EAX, so return ABI is
    # additionally checked structurally by the original-toolchain graph audit.
    for frame in (entry,registration,state):
        s.require(frame['frontend']['pdb_context_layout']=={'size':24,'fields':{name:[i*4,4] for i,name in enumerate(FIELDS)}},'PDB native storage includes unmodeled gaps')
