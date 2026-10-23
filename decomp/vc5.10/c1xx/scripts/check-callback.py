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


def context(directory,suffix,name='context_stack'):
    raw=(directory/(name.replace('_','-')+'-'+suffix+'.bin')).read_bytes()
    count=27 if name=='nested_context_stacks' else 2 if name=='source_position' else 3
    s.require(len(raw)==4*count,'missing raw original context fields')
    return list(struct.unpack('<%dI'%count,raw))


def check(original,native,require_audit=True,audit_path=None):
    o,n,trial,text,state=s.pair(original,native)
    constructor=trial['trial']=='_initialize_output_buffer_01'
    registrar=trial['trial']=='_register_cleanup'
    match=re.fullmatch(r'_initialize_frontend_(\d{2})',trial['trial'] or '')
    s.require(constructor or registrar or match is not None and 1<=int(match[1])<=8,'no reviewed callback decoder for this function')
    index=8 if constructor or registrar else int(match[1])
    s.require(trial['case']=='minimal' and o['exit']==125 and
              trial.get('test_seed') in (None,'restore-nonzero','restore-zero') and
              'auxiliary_mock' not in trial,'wrong context initializer trial')
    seed=trial.get('test_seed');final='%03d'%((7 if seed else 8)+index+int(constructor)+2*int(registrar))
    model=json.loads((native/'state-model.json').read_text())
    s.require(model['schema'] in (2,3,4,5,6),'context fields omitted from typed snapshot/BSS model')
    context_names=['context_stack']+(['saved_context_stack'] if model['schema']>=3 else [])
    if model['schema']>=4:context_names.append('nested_context_stacks')
    if model['schema']>=5:context_names.append('source_position')
    counts={name:27 if name=='nested_context_stacks' else 2 if name=='source_position' else 3 for name in context_names}
    def seeded(name,pointer):
        if model['schema']<5:return [pointer]*counts[name]
        return [pointer,17] if name=='source_position' else [pointer,17,pointer]*(9 if name=='nested_context_stacks' else 1)
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
    expected += [(str((7 if seed else 8)+slot-1),'%08x'%(model['initializers'][slot]['original_address']-0x10400000)) for slot in range(1,index+2)]
    if index==8:expected[-1]=(expected[-1][0],'00038666')
    if constructor or registrar:expected.append((str(int(expected[-1][0])+1),'000386ae'))
    if registrar:expected.append((str(int(expected[-1][0])+1),'000386d4'))
    expected_frontier=expected[-1][1]
    s.require([row[:2] for row in traps]==expected and re.findall(r'^frontier rva=(\w+)$',text,re.M)==[expected_frontier],
              'original callback did not advance to exact next callback')
    s.require([int(word,16) for word in traps[4][2].split(',')][1:4]==[26,pointer,1],'original raw Invoke ABI differs')
    trace=['DllMain','InvokeCompilerPass','initialize_compiler_pass']+([] if seed else ['checked_malloc'])+['initialize_frontend_%02d'%slot for slot in range(1,index+2)]
    if index==8:trace[-1]='initialize_output_buffer_01'
    if constructor or registrar:trace.append('register_cleanup')
    if registrar:trace.append('initialize_frontend_09')
    s.require((native/'trace.txt').read_text()==''.join(name+'\n' for name in trace),'native first-entry callback trace differs')
    frontier='initialize_frontend_%02d'%(index+1)
    if index==8:frontier='initialize_output_buffer_01'
    if constructor:frontier='register_cleanup'
    if registrar:frontier='initialize_frontend_09'
    s.require(state['symbol']==frontier and (constructor or state['args']=={}) and
              'Program received signal SIGTRAP' in (native/'gdb.txt').read_text(),'not a real native callback frontier')
    symbols=subprocess.check_output(['nm','-S','--defined-only',str(native/'native-executable.elf')],text=True)
    match=re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] '+frontier+'$',symbols,re.M)
    s.require(match is not None,'missing native callback02 symbol');start,size=[int(word,16) for word in match.groups()]
    s.require(start<state['pc']<=start+size and d.elf_byte(native,state['pc']-1)==0xcc,'false native callback INT3 PC')
    s.require(all(context(original,'000',name)==[0]*counts[name] for name in context_names) and d.original_fields(original,'000')=={
        'first_invocation':1,'saved_state':0,'cleanup_count':0,'restore_mode':0,'cleanup_callbacks':[0]*32},
        'original context/default field evidence differs')
    before=d.original_fields(original,'006');phase_old=d.original_fields(original,'phase');after=d.original_fields(original,final)
    wanted=1 if seed=='restore-zero' else 0;copied=17 if seed else 1
    if seed:
        s.require(before['first_invocation']==0 and before['saved_state'] and before['cleanup_count']==32 and
                  before['cleanup_callbacks']==[0x10438610]*32 and before['restore_mode']==9 and
                  all(context(original,'006',name)==seeded(name,before['saved_state']) and
                      pre['frontend'][name]==seeded(name,pre['frontend']['saved_state']) for name in context_names) and
                  pre['frontend']['cleanup_callbacks']==[slots[1]]*32 and pre['frontend']['cleanup_count']==32 and
                  pre['frontend']['restore_mode']==9 and pre['frontend']['saved_first']==17,
                  'nonzero expanded BSS prestate is missing')
        s.require(phase_old['saved_state']==before['saved_state'] and
                  phase['frontend']['saved_state']==pre['frontend']['saved_state'],'restore changed snapshot identity')
    else:
        s.require(before['first_invocation']==pre['frontend']['first_invocation']==1 and
                  before['saved_state']==pre['frontend']['saved_state']==0 and
                  all(context(original,'006',name)==pre['frontend'][name]==[0]*counts[name] for name in context_names),
                  'fresh context initial state differs')
        s.require(int(traps[6][2].split(',')[1],16)==8520,'actual allocator request differs')
    destructor=re.search(r'^([0-9a-f]+) [0-9a-f]+ [Tt] destroy_output_buffer_01$',symbols,re.M)
    s.require(destructor is not None,'missing native destructor boundary')
    for suffix,old,current,flag in (('phase',phase_old,phase,copied),(final,after,state,0)):
        value=current['frontend']
        s.require(old['first_invocation']==value['first_invocation']==flag and old['saved_state'] and value['saved_state'] and
                  old['cleanup_count']==value['cleanup_count']==int(registrar and suffix==final) and
                  old['cleanup_callbacks']==([0x10434748]+[0]*31 if registrar and suffix==final else [0]*32) and
                  value['cleanup_callbacks']==([int(destructor[1],16)]+[0]*31 if registrar and suffix==final else [0]*32) and
                  old['restore_mode']==value['restore_mode']==wanted and value['saved_first']==copied and
                  all(context(original,suffix,name)==value[name]==[0]*counts[name] for name in context_names),
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
    if constructor or registrar:
        check_output(original,native,trial,text,pre,phase,state,seed,final,slots,symbols,traps)
    s.require(set(re.findall(r'^last-error=(\d+)$',text,re.M))=={'203'} and
              {int(word,16)&0xffff for word in re.findall(r'^control-word=(\w+)$',text,re.M)}=={0x27f},'original caller state changed')
    if require_audit:s.audit(trial,audit_path)
    return {'trial':trial['trial'],'frontier':'_'+frontier,'seed':seed,
            'modeled_context_fields':sum(counts.values()),'original_default_context':[0,0,0],
            'typed_bss_bytes':state['frontend']['known_sizes']['bss']-(3 if model['schema']>=6 else 0),
            'native_bss_size':state['frontend']['known_sizes']['bss'],
            'native_c_padding_bytes':3 if model['schema']>=6 else 0,
            'output_constructor_fields_checked':9 if constructor or registrar else 0,
            'cleanup_registration_checked':registrar,
            'nonzero_output_constructor_input_checked':bool(trial.get('test_constructor_seed')),
            'nonzero_expanded_BSS_restore_checked':bool(seed),'original_unmodeled_snapshot_bytes':8516,
            'native_snapshot_byte_equivalence_claimed':False,'whole_mask_bytes_checked':751216,
            'nonzero_seed_scope':'typed pointer identities and exact scalar17' if model['schema']>=5 else 'legacy zeroing/reset evidence only; not middle-scalar equivalence',
            'original_seconds':o['seconds'],'native_seconds':n['seconds']}


def output(directory,suffix):
    raw=(directory/('output-buffer-01-'+suffix+'.bin')).read_bytes()
    s.require(len(raw)==33,'missing encountered output-buffer fields')
    names=('data','cursor','capacity','relocation','relocation_context','file','ok','descriptor','mode')
    return dict(zip(names,struct.unpack('<7IiB',raw)))


def output_seed(pointer,callback):
    return {'data':pointer,'cursor':pointer+4,'capacity':64,'relocation':callback,
            'relocation_context':pointer+8,'file':pointer+12,'ok':53,'descriptor':37,'mode':23}


def check_output(original,native,trial,text,pre,phase,state,seed,final,slots,symbols,traps):
    zero={name:0 for name in ('data','cursor','capacity','relocation','relocation_context','file','ok','descriptor','mode')}
    done=dict(zero,ok=1,descriptor=-1)
    s.require(output(original,'000')==zero,'original output fields not initially zero')
    before=d.original_fields(original,'006')
    expected_old=output_seed(before['saved_state'],0x10438610) if seed else zero
    expected_new=output_seed(pre['frontend']['saved_state'],slots[1]) if seed else zero
    s.require(output(original,'006')==expected_old and pre['frontend']['output_buffers']==[expected_new],
              'nonzero output BSS seed/default differs')
    s.require(output(original,'phase')==zero and phase['frontend']['output_buffers']==[zero],
              'output fields escaped typed BSS clearing')
    entry=json.loads((native/'constructor-entry.json').read_text())
    entry_symbol=re.search(r'^([0-9a-f]+) [0-9a-f]+ [Tt] initialize_output_buffer_01$',symbols,re.M)
    s.require(entry_symbol is not None and entry['pc']==int(entry_symbol[1],16),'not the actual native constructor entry')
    registrar=trial['trial']=='_register_cleanup'
    sequence='%03d'%(int(final)-(2 if registrar else 1))
    if trial.get('test_constructor_seed'):
        s.require(trial['auxiliary_data_seed']=={'owner':'_initialize_output_buffer_01','point':'entry before target body',
            'data':'owned snapshot+0','cursor':'owned snapshot+4','capacity':64,
            'unused_relocation':'_initialize_frontend_01; never invoked','relocation_context':'owned snapshot+8',
            'file':'owned snapshot+12; never dereferenced','ok':53,'descriptor':37,'mode':23,
            'actual_initializer_table_unchanged':True,'code_or_allocator_mock':False},'undeclared constructor-entry data mocks')
        old_pointer=d.original_fields(original,sequence)['saved_state'];new_pointer=entry['frontend']['saved_state']
        s.require(output(original,sequence)==output_seed(old_pointer,0x10438610) and
                  entry['frontend']['output_buffers']==[output_seed(new_pointer,slots[1])] and
                  'output-seed role=constructor-entry ' in text,'missing actual nonzero constructor input')
    else:
        s.require('auxiliary_data_seed' not in trial and output(original,sequence)==zero and
                  entry['frontend']['output_buffers']==[zero],'ordinary constructor input was mocked')
    s.require(entry['symbol']=='initialize_output_buffer_01' and entry['args']=={} and
              output(original,final)==done and state['frontend']['output_buffers']==[done],
              'constructor did not assign all nine encountered fields')
    callback=re.search(r'^([0-9a-f]+) [0-9a-f]+ [Tt] destroy_output_buffer_01$',symbols,re.M)
    registration=json.loads((native/'registration-entry.json').read_text()) if registrar else state
    row=traps[-2] if registrar else traps[-1]
    s.require(callback is not None and registration['args']=={'cleanup':int(callback[1],16)} and
              int(row[2].split(',')[1],16)==0x10434748,'actual destructor-registration input differs')
    if registrar:
        s.require(registration['symbol']=='register_cleanup' and registration['frontend']['cleanup_count']==0 and
                  registration['frontend']['cleanup_callbacks']==[0]*32 and
                  output(original,'%03d'%(int(final)-1))==done and registration['frontend']['output_buffers']==[done],
                  'registration entry state differs')
