"""Typed context-stack initialization and expanded BSS-reset evidence."""
import importlib.util
import json
from pathlib import Path
import re
import struct
import subprocess

spec=importlib.util.spec_from_file_location('projection',Path(__file__).with_name('check-projection.py'))
d=importlib.util.module_from_spec(spec);spec.loader.exec_module(d)
s=d.s


def context(directory,suffix):
    raw=(directory/('context-stack-'+suffix+'.bin')).read_bytes()
    s.require(len(raw)==12,'missing raw original context fields')
    return list(struct.unpack('<3I',raw))


def check(original,native,require_audit=True,audit_path=None):
    o,n,trial,text,state=s.pair(original,native)
    s.require(trial['trial']=='_initialize_frontend_01' and trial['case']=='minimal' and o['exit']==125 and
              trial.get('test_seed') in (None,'restore-nonzero','restore-zero') and
              'auxiliary_mock' not in trial,'wrong context initializer trial')
    seed=trial.get('test_seed');final='008' if seed else '009'
    model=json.loads((native/'state-model.json').read_text())
    s.require(model['schema']==2,'context fields omitted from typed snapshot/BSS model')
    pre=json.loads((native/'pre.json').read_text());phase=json.loads((native/'phase.json').read_text())
    for snapshot in (pre,phase,state):d.field_map(model,snapshot['frontend'])
    slots=d.callback_table(model,original,native,state)
    contract=json.loads((native/'contract.json').read_text())
    pointer,old=d.b.vector(original/'invoke-entry-argv.bin');native_pointer,new=d.b.vector(native/'native-argv.bin')
    s.require(old==new==contract['original_argv'] and state['invocation']=={
        'argc':26,'mode':1,'argv':new,'argv_address':native_pointer},'captured CL invocation differs')
    traps=re.findall(r'^trap seq=(\d+) rva=([0-9a-f]{8}).*stack=([0-9a-f,]+)$',text,re.M)
    expected=[('0','0002e3c6'),('1','0002e46a'),('2','000384ce'),('3','0002e52a'),('5','000384d4'),('6','00038550')]
    if not seed:expected.append(('7','000233ba'))
    expected += [('7' if seed else '8','00038610'),('8' if seed else '9','00038622')]
    s.require([row[:2] for row in traps]==expected and re.findall(r'^frontier rva=(\w+)$',text,re.M)==['00038622'],
              'original callback did not advance to exact next callback')
    s.require([int(word,16) for word in traps[4][2].split(',')][1:4]==[26,pointer,1],'original raw Invoke ABI differs')
    trace=['DllMain','InvokeCompilerPass','initialize_compiler_pass']+([] if seed else ['checked_malloc'])+['initialize_frontend_01','initialize_frontend_02']
    s.require((native/'trace.txt').read_text()==''.join(name+'\n' for name in trace),'native first-entry callback trace differs')
    s.require(state['symbol']=='initialize_frontend_02' and state['args']=={} and
              'Program received signal SIGTRAP' in (native/'gdb.txt').read_text(),'not a real native callback frontier')
    symbols=subprocess.check_output(['nm','-S','--defined-only',str(native/'native-executable.elf')],text=True)
    match=re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] initialize_frontend_02$',symbols,re.M)
    s.require(match is not None,'missing native callback02 symbol');start,size=[int(word,16) for word in match.groups()]
    s.require(start<state['pc']<=start+size and d.elf_byte(native,state['pc']-1)==0xcc,'false native callback INT3 PC')
    s.require(context(original,'000')==[0]*3 and d.original_fields(original,'000')=={
        'first_invocation':1,'saved_state':0,'cleanup_count':0,'restore_mode':0,'cleanup_callbacks':[0]*32},
        'original context/default field evidence differs')
    before=d.original_fields(original,'006');phase_old=d.original_fields(original,'phase');after=d.original_fields(original,final)
    wanted=1 if seed=='restore-zero' else 0;copied=17 if seed else 1
    if seed:
        s.require(before['first_invocation']==0 and before['saved_state'] and before['cleanup_count']==32 and
                  before['cleanup_callbacks']==[0x10438610]*32 and before['restore_mode']==9 and
                  context(original,'006')==[before['saved_state']]*3 and
                  pre['frontend']['context_stack']==[pre['frontend']['saved_state']]*3 and
                  pre['frontend']['cleanup_callbacks']==[slots[1]]*32 and pre['frontend']['cleanup_count']==32 and
                  pre['frontend']['restore_mode']==9 and pre['frontend']['saved_first']==17,
                  'nonzero expanded BSS prestate is missing')
        s.require(phase_old['saved_state']==before['saved_state'] and
                  phase['frontend']['saved_state']==pre['frontend']['saved_state'],'restore changed snapshot identity')
    else:
        s.require(before['first_invocation']==pre['frontend']['first_invocation']==1 and
                  before['saved_state']==pre['frontend']['saved_state']==0 and context(original,'006')==[0]*3 and
                  pre['frontend']['context_stack']==[0]*3,'fresh context initial state differs')
        s.require(int(traps[6][2].split(',')[1],16)==8520,'actual allocator request differs')
    for suffix,old,current,flag in (('phase',phase_old,phase,copied),(final,after,state,0)):
        value=current['frontend']
        s.require(old['first_invocation']==value['first_invocation']==flag and old['saved_state'] and value['saved_state'] and
                  old['cleanup_count']==value['cleanup_count']==0 and old['cleanup_callbacks']==value['cleanup_callbacks']==[0]*32 and
                  old['restore_mode']==value['restore_mode']==wanted and value['saved_first']==copied and
                  context(original,suffix)==value['context_stack']==[0]*3,
                  'typed copy/expanded BSS clear/context initializer result differs')
        raw=(original/('frontend-snapshot-'+suffix+'.bin')).read_bytes()
        s.require(len(raw)==8520 and struct.unpack_from('<I',raw,0x17e0)[0]==copied,'complete original snapshot missing')
    s.require(phase_old['saved_state']==after['saved_state'] and phase['frontend']['saved_state']==state['frontend']['saved_state'],
              'callback changed snapshot pointer')
    s.require((original/'frontend-data-phase.bin').read_bytes()==(original/'frontend-snapshot-phase.bin').read_bytes(),
              'original opaque snapshot copy/restore differs')
    s.require('restore-phase rva=000385be\n' in text and trial['validation_observers']==[{
        'owner':'_initialize_compiler_pass','original_rva':0x385be,'original_byte':0x89,
        'purpose':'observe typed copy/restore before first-flag reset'}],'undeclared phase observer')
    for snapshot in (pre,phase,state):
        s.require(snapshot['target_state']['process_attached']==1 and snapshot['target_state']['last_error']==203 and
                  snapshot['control_word']==0x27f and snapshot['target_state']['compiler_mode']==(0 if not seed or seed=='restore-zero' else 3),
                  'caller/lifecycle state changed')
    s.require(set(re.findall(r'^last-error=(\d+)$',text,re.M))=={'203'} and
              {int(word,16)&0xffff for word in re.findall(r'^control-word=(\w+)$',text,re.M)}=={0x27f},'original caller state changed')
    if require_audit:s.audit(trial,audit_path)
    return {'trial':trial['trial'],'frontier':'_initialize_frontend_02','seed':seed,
            'new_context_fields':3,'original_default_context':[0,0,0],'typed_bss_bytes':148,
            'nonzero_expanded_BSS_restore_checked':bool(seed),'original_unmodeled_snapshot_bytes':8516,
            'native_snapshot_byte_equivalence_claimed':False,'whole_mask_bytes_checked':751216,
            'original_seconds':o['seconds'],'native_seconds':n['seconds']}
