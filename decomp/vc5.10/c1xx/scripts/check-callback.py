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
    constructor_match=re.fullmatch(r'_initialize_output_buffer_(\d{2})',trial['trial'] or '')
    constructor_number=int(constructor_match[1]) if constructor_match else 0
    constructor=constructor_number==1
    constructor2=constructor_number>=2
    registrar=trial['trial']=='_register_cleanup'
    allocation_candidate=trial['trial']=='_reserve_work_buffer'
    early_options_candidate=trial['trial']=='_initialize_intermediate_files'
    early_switch_candidate=trial['trial']=='_process_early_option'
    reset_candidate=trial['trial'] in ('CompilerArena::Reset','_initialize_pool_arenas','CompilerPool::BindArena','CompilerPool::Refill','CompilerArena::AllocatePages','CompilerArena::CommitExtent','_process_compiler_options','_push_declaration_context','_initialize_include_aliases','_initialize_checksums','_initialize_multibyte_categories','_initialize_lowercase_table')
    scaler_candidate=reset_candidate or trial['trial']=='_calculate_arena_reservation'
    reserve_arena_candidate=trial['trial']=='CompilerArena::Reserve'
    default_arena_candidate=trial['trial']=='_initialize_default_arena'
    environment_candidate=trial['trial']=='_initialize_compiler_environment'
    hook_candidate=trial['trial']=='_initialize_frontend_hook'
    work_candidate=trial['trial']=='_initialize_work_buffers'
    main_candidate=trial['trial']=='_main'
    comment_constructor=trial['trial']=='_initialize_generated_comment'
    rank_helper=trial['trial']=='_initialize_type_ranks'
    pool_member=trial['trial']=='CompilerPool::Initialize'
    pdb=trial['trial'] in ('_initialize_pdb_context_01','_initialize_pdb_context_02')
    pdb_number=2 if trial['trial']=='_initialize_pdb_context_02' else 1
    deferred=trial['trial']=='_initialize_deferred_work'
    match=re.fullmatch(r'_initialize_frontend_(\d{2})',trial['trial'] or '')
    s.require(scaler_candidate or reserve_arena_candidate or default_arena_candidate or environment_candidate or hook_candidate or early_switch_candidate or early_options_candidate or allocation_candidate or work_candidate or main_candidate or comment_constructor or rank_helper or pool_member or pdb or deferred or constructor_number in (1,2,3,4,5,6,7,8) or registrar or match is not None and 1<=int(match[1])<=42,
              'no reviewed callback decoder for this function')
    index=42 if scaler_candidate or reserve_arena_candidate or default_arena_candidate or environment_candidate or hook_candidate or early_switch_candidate or early_options_candidate or allocation_candidate or work_candidate or main_candidate else 39 if comment_constructor else 34 if rank_helper else 24 if pool_member else 21+pdb_number if pdb else 16 if deferred else 7+constructor_number if constructor_number else 8 if registrar else int(match[1])
    s.require(trial['case']=='minimal' and o['exit']==125 and
              trial.get('test_seed') in (None,'restore-nonzero','restore-zero') and
              'auxiliary_mock' not in trial,'wrong context initializer trial')
    wrapper09=index>=9
    seed=trial.get('test_seed')
    model=json.loads((native/'state-model.json').read_text())
    s.require(model['schema'] in (2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36),'context fields omitted from typed snapshot/BSS model')
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
    path=[]
    def callback(slot):
        return ('initialize_frontend_%02d'%slot,model['initializers'][slot]['original_address']-0x10400000)
    records={row['name']:row for row in trial['driver']}
    if index<8:
        path=[callback(slot) for slot in range(1,index+2)]
    else:
        path=[callback(slot) for slot in range(1,9)]
        completed=8 if deferred or index>=16 else constructor_number if constructor_number else 1 if registrar else index-8
        for number in range(1,completed+1):
            name='initialize_output_buffer_%02d'%number
            path += [(name,records['_'+name]['start']-0x10400000),('register_cleanup',0x386ae)]
            if '_register_cleanup' not in trial['accepted']:break
            path.append(callback(8+number))
        if deferred or index>=17:
            path += [('initialize_deferred_work',0x38904),('register_cleanup',0x386ae),callback(17)]
            if not deferred:path += [callback(slot) for slot in range(18,min(index+2,23))]
            if index>=22:
                path.append(('initialize_pdb_context_01',0x3894f))
                if pdb or index>=23:path += [('register_cleanup',0x386ae),callback(23)]
                if index>=23:
                    path.append(('initialize_pdb_context_02',0x3898f))
                    if pdb and pdb_number==2 or index>=24:path += [('register_cleanup',0x386ae),callback(24)]
                    if index>=24:
                        for number in range(model['pool_projection']['count']):
                            path.append(('CompilerPool::Initialize',0x389d5))
                            if 'CompilerPool::Initialize' in trial['accepted']:path.append(callback(25+number))
        elif not constructor_number and not registrar:
            name='initialize_deferred_work' if index==16 else 'initialize_output_buffer_%02d'%(completed+1)
            path.append((name,records['_'+name]['start']-0x10400000))
    if index>=34:
        path.append(('initialize_type_ranks',0x38a6b))
        if '_initialize_type_ranks' in trial['accepted']:
            for slot in range(35,min(index+2,43)):
                path.append(callback(slot))
                if slot==39 and index>=39:
                    path.append(('initialize_generated_comment',0x38af9))
                    if '_initialize_generated_comment' not in trial['accepted']:break
                    path.append(('register_cleanup',0x386ae))
                if slot==37 and index>=37:
                    path.append(('PchContext::Initialize',0x3bd41))
                    if 'PchContext::Initialize' not in trial['accepted']:break
    if index>=42:path.append(('c1xx_main',0x38bfd))
    if '_main' in trial['accepted']:path.append(('initialize_work_buffers',0x38c86))
    if '_initialize_work_buffers' in trial['accepted']:path.append(('reserve_work_buffer',0x38cc9))
    if '_reserve_work_buffer' in trial['accepted']:path += [('reserve_work_buffer',0x38cc9),('register_cleanup',0x386ae),('initialize_intermediate_files',0x38d07)]
    if '_initialize_intermediate_files' in trial['accepted']:path.append(('process_early_option',0x38d56))
    if '_process_early_option' in trial['accepted']:path += [('process_early_option',0x38d56)]*24+[('initialize_frontend_hook',0x38df0)]
    if '_initialize_frontend_hook' in trial['accepted']:path.append(('initialize_compiler_environment',0x38df1))
    if '_initialize_compiler_environment' in trial['accepted']:path.append(('initialize_default_arena',0x38e33))
    if '_initialize_default_arena' in trial['accepted']:path.append(('CompilerArena::Reserve',0x38e70))
    if 'CompilerArena::Reserve' in trial['accepted']:path.append(('calculate_arena_reservation',0x38ed2))
    if '_calculate_arena_reservation' in trial['accepted']:path += [('support:_ftol',0x38f04),('CompilerArena::Reset',0x38f0a)]
    if 'CompilerArena::Reset' in trial['accepted']:
        path += [('CompilerArena::Reserve',0x38e70),('calculate_arena_reservation',0x38ed2),('support:_ftol',0x38f04),('CompilerArena::Reset',0x38f0a)]*8+[('initialize_pool_arenas',0x38f1b)]
    if '_initialize_pool_arenas' in trial['accepted']:path.append(('CompilerPool::BindArena',0x38f94))
    if 'CompilerPool::BindArena' in trial['accepted']:path.append(('CompilerPool::Refill',0x1ee29))
    if 'CompilerPool::Refill' in trial['accepted']:path.append(('CompilerArena::AllocatePages',0x1ee58))
    if 'CompilerArena::AllocatePages' in trial['accepted']:path.append(('CompilerArena::CommitExtent',0x1d738))
    if 'CompilerArena::CommitExtent' in trial['accepted']:path += [('CompilerPool::BindArena',0x38f94),('CompilerPool::Refill',0x1ee29),('CompilerArena::AllocatePages',0x1ee58)]*9+[('process_compiler_options',0x38fad)]
    if '_process_compiler_options' in trial['accepted']:path.append(('push_declaration_context',0x2b834))
    if '_push_declaration_context' in trial['accepted']:path.append(('initialize_checksums',0x392a8)if seed else('initialize_include_aliases',0x39282))
    if '_initialize_include_aliases'in trial['accepted']and not seed:path.append(('initialize_checksums',0x392a8))
    if '_initialize_checksums'in trial['accepted']:path.append(('initialize_multibyte_categories',0x39394))
    if '_initialize_multibyte_categories'in trial['accepted']:path.append(('initialize_lowercase_table',0x393ca))
    if '_initialize_lowercase_table'in trial['accepted']:path.append(('read_env_arguments',0x39413))
    if trial['trial']=='_initialize_lowercase_table':
        observer=re.findall(r'^path-fold-helper-return seq=(\d+) pc=10438fee esp=(\w+) entry-esp=(\w+) calls=(\d+)$',text,re.M)
        s.require(len(observer)==1 and int(observer[0][1],16)==int(observer[0][2],16)+4 and int(observer[0][3])==(255 if seed else 256),'explicit non-entry helper return state observation')
    # This particular return observer also emits one ordinary save_state. It
    # owns a state sequence number, never a function-entry/coverage event.
    expected += [(str((7 if seed else 8)+i+(1 if trial['trial']=='_initialize_lowercase_table'and i==len(path)-1 else 0)),'%08x'%rva) for i,(_,rva) in enumerate(path)]
    if trial['trial']=='_initialize_lowercase_table':s.require(int(observer[0][0])==int(expected[-1][0])-1,'non-entry return observation sequence differs')
    final='%03d'%int(expected[-1][0]);frontier=path[-1][0]
    expected_frontier=expected[-1][1]
    s.require([row[:2] for row in traps]==expected and re.findall(r'^frontier rva=(\w+)$',text,re.M)==[expected_frontier],
              'original callback did not advance to exact next callback')
    s.require([int(word,16) for word in traps[4][2].split(',')][1:4]==[26,pointer,1],'original raw Invoke ABI differs')
    trace=['DllMain','InvokeCompilerPass','initialize_compiler_pass']+([] if seed else ['checked_malloc'])
    for name,_ in path:
        if name=='support:_ftol':continue
        trace_name=records[name]['native_symbol'] if name in ('CompilerPool::Initialize','CompilerPool::BindArena','CompilerPool::Refill','CompilerArena::AllocatePages','CompilerArena::CommitExtent','PchContext::Initialize','CompilerArena::Reserve','CompilerArena::Reset') else name
        if trace_name not in trace:trace.append(trace_name)
    s.require((native/'trace.txt').read_text()==''.join(name+'\n' for name in trace),'native first-entry callback trace differs')
    registered=sum(name=='register_cleanup' for name,_ in path[:-1])
    s.require(state['symbol']==frontier and (constructor or frontier in ('CompilerPool::Initialize','CompilerPool::BindArena','CompilerPool::Refill','CompilerArena::AllocatePages','CompilerArena::CommitExtent','PchContext::Initialize','CompilerArena::Reserve','CompilerArena::Reset','c1xx_main','reserve_work_buffer','initialize_intermediate_files','process_early_option','calculate_arena_reservation','process_compiler_options','push_declaration_context','initialize_include_aliases','initialize_checksums','initialize_multibyte_categories','initialize_lowercase_table','read_env_arguments') or state['args']=={}) and
              'Program received signal SIGTRAP' in (native/'gdb.txt').read_text(),'not a real native callback frontier')
    if frontier=='read_env_arguments':
        s.require(state['args']=={'arguments':0} and [int(word,16)for word in traps[-1][2].split(',')][:2]==[0x10439021,0],'actual read_env_arguments(NULL) formal/caller differs')
    if frontier=='c1xx_main':
        s.require(struct.unpack_from('<I',(original/('state-'+final+'.bin')).read_bytes(),8)[0]==1,'original mode assignment before main differs')
        s.require(state['args']=={'argc':26,'argv':native_pointer} and [int(v,16) for v in traps[-1][2].split(',')][1:3]==[26,pointer],'actual main argc/argv ABI differs at frontier')
    symbols=subprocess.check_output(['nm','-S','--defined-only',str(native/'native-executable.elf')],text=True)
    native_frontier=records[frontier]['native_symbol'] if frontier in ('CompilerPool::Initialize','CompilerPool::BindArena','CompilerPool::Refill','CompilerArena::AllocatePages','CompilerArena::CommitExtent','PchContext::Initialize','CompilerArena::Reserve','CompilerArena::Reset') else frontier
    match=re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] '+native_frontier+'$',symbols,re.M)
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
    original_cleanup=[];native_cleanup=[]
    for number in range(1,registered+1):
        name='cleanup_main' if number==13 else 'destroy_generated_comment' if number==12 else 'destroy_pdb_context_02' if number==11 else 'destroy_pdb_context_01' if number==10 else 'destroy_deferred_work' if number==9 else 'destroy_output_buffer_%02d'%number
        symbol=re.search(r'^([0-9a-f]+) [0-9a-f]+ [Tt] '+name+'$',symbols,re.M)
        s.require(symbol is not None,'missing encountered native destructor boundary')
        original_cleanup.append(records['_'+name]['start']);native_cleanup.append(int(symbol[1],16))
    for suffix,old,current,flag in (('phase',phase_old,phase,copied),(final,after,state,0)):
        value=current['frontend']
        s.require(old['first_invocation']==value['first_invocation']==flag and old['saved_state'] and value['saved_state'] and
                  old['cleanup_count']==value['cleanup_count']==(registered if suffix==final else 0) and
                  old['cleanup_callbacks']==(original_cleanup+[0]*(32-registered) if suffix==final else [0]*32) and
                  value['cleanup_callbacks']==(native_cleanup+[0]*(32-registered) if suffix==final else [0]*32) and
                  old['restore_mode']==value['restore_mode']==(trial['test_pdb_seed'] if suffix==final and trial.get('test_pdb_seed') is not None else wanted) and value['saved_first']==copied and
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
        'purpose':'observe typed copy/restore before first-flag reset'}]+([{'owner':'_initialize_frontend_24','original_rva':0x389d4,'original_byte':0xc3,'purpose':'actual pool member return-this and RET4 stack effect'}] if pool_member else [])+([{'owner':'_initialize_compiler_pass','original_rva':0x385f3,'original_byte':0xeb,'purpose':'actual PCH member return to inherited caller, RET0 and EDI preservation','activation':'armed only at actual member entry'}] if 'PchContext::Initialize' in trial['accepted'] else []),'undeclared phase observer')
    for snapshot in (pre,phase,state):
        s.require(snapshot['target_state']['process_attached']==1 and snapshot['target_state']['last_error']==203 and
                  snapshot['control_word']==0x27f and snapshot['target_state']['compiler_mode']==(1 if snapshot is state and index>=42 else 0 if not seed or seed=='restore-zero' else 3),
                  'caller/lifecycle state changed')
    if model['schema']>=6:
        check_outputs(original,native,trial,text,pre,phase,state,seed,final,slots,symbols,traps,model)
    if model['schema']>=14:check_deferred(original,native,trial,text,pre,phase,state,seed,final,symbols,traps)
    if model['schema']>=15:check_token_value(original,native,trial,text,pre,phase,state,seed,final,symbols,traps)
    if model['schema']>=19:
        spec=importlib.util.spec_from_file_location('pool_state',Path(__file__).with_name('pool_state.py'));pool=importlib.util.module_from_spec(spec);spec.loader.exec_module(pool)
        pool.check(s,original,native,trial,text,pre,phase,state,seed,final,symbols,traps)
    if model['schema']>=17:
        spec=importlib.util.spec_from_file_location('pdb_state',Path(__file__).with_name('pdb_state.py'));p=importlib.util.module_from_spec(spec);spec.loader.exec_module(p)
        p.check(s,original,native,trial,text,pre,phase,state,final,symbols,traps)
    if model['schema']>=21:
        def ready(suffix):
            blob=(original/('type-ranks-ready-'+suffix+'.bin')).read_bytes();s.require(len(blob)==4,'missing raw rank-ready field');return struct.unpack('<I',blob)[0]
        s.require(ready('000')==0 and ready('006')==pre['frontend']['type_ranks_initialized']==(37 if seed else 0),'rank-ready initial/nonzero state differs')
        s.require(ready('phase')==phase['frontend']['type_ranks_initialized']==0 and ready(final)==state['frontend']['type_ranks_initialized']==(1 if '_initialize_type_ranks' in trial['accepted'] else 0),'rank-ready escaped BSS reset or changed before helper')
    if model['schema']>=22:
        spec=importlib.util.spec_from_file_location('type_rank_state',Path(__file__).with_name('type_rank_state.py'));ranks=importlib.util.module_from_spec(spec);spec.loader.exec_module(ranks)
        ranks.check(s,original,native,trial,text,pre,phase,state,seed,final,symbols,traps)
    if model['schema']>=23:
        spec=importlib.util.spec_from_file_location('prototype_state',Path(__file__).with_name('prototype_state.py'));prototype=importlib.util.module_from_spec(spec);spec.loader.exec_module(prototype)
        prototype.check(s,original,native,trial,text,pre,phase,state,seed,final,symbols,traps)
    if model['schema']>=24:
        spec=importlib.util.spec_from_file_location('pch_state',Path(__file__).with_name('pch_state.py'));pch=importlib.util.module_from_spec(spec);spec.loader.exec_module(pch)
        pch.check(s,original,native,trial,text,pre,phase,state,seed,final,symbols,traps)
    if model['schema']>=25:
        spec=importlib.util.spec_from_file_location('pending_group_state',Path(__file__).with_name('pending_group_state.py'));groups=importlib.util.module_from_spec(spec);spec.loader.exec_module(groups)
        groups.check(s,original,native,trial,text,pre,phase,state,seed,final,symbols,traps)
    if model['schema']>=29:
        spec=importlib.util.spec_from_file_location('work_state',Path(__file__).with_name('work_state.py'));work=importlib.util.module_from_spec(spec);spec.loader.exec_module(work)
        work.check(s,original,native,trial,text,pre,phase,state,seed,final,symbols,traps)
    if model['schema']>=28:
        spec=importlib.util.spec_from_file_location('main_state',Path(__file__).with_name('main_state.py'));main_state=importlib.util.module_from_spec(spec);spec.loader.exec_module(main_state)
        main_state.check(s,original,native,trial,text,pre,phase,state,seed,final,symbols,traps)
    if model['schema']>=27:
        spec=importlib.util.spec_from_file_location('action_state',Path(__file__).with_name('action_state.py'));action=importlib.util.module_from_spec(spec);spec.loader.exec_module(action)
        action.check(s,original,native,trial,text,pre,phase,state,seed,final,symbols,traps)
    if model['schema']>=26:
        spec=importlib.util.spec_from_file_location('comment_state',Path(__file__).with_name('comment_state.py'));comment=importlib.util.module_from_spec(spec);spec.loader.exec_module(comment)
        comment.check(s,original,native,trial,text,pre,phase,state,seed,final,symbols,traps)
    if model['schema']>=16:check_signature(original,native,pre,phase,state,seed,final,symbols,traps)
    s.require(set(re.findall(r'^last-error=(\d+)$',text,re.M))=={'203'} and
              {int(word,16)&0xffff for word in re.findall(r'^control-word=(\w+)$',text,re.M)}=={0x27f},'original caller state changed')
    if '_reserve_work_buffer' in trial['accepted']:
        spec=importlib.util.spec_from_file_location('allocation_state',Path(__file__).with_name('allocation_state.py'));allocation=importlib.util.module_from_spec(spec);spec.loader.exec_module(allocation)
        allocation.check(s,d,original,native,trial,text,state,final,symbols,traps)
    if '_initialize_intermediate_files' in trial['accepted']:
        spec=importlib.util.spec_from_file_location('early_options_state',Path(__file__).with_name('early_options_state.py'));early=importlib.util.module_from_spec(spec);spec.loader.exec_module(early)
        early.check(s,d,original,native,trial,text,state,final,symbols,traps)
    if '_process_early_option' in trial['accepted']:
        spec=importlib.util.spec_from_file_location('early_switches_state',Path(__file__).with_name('early_switches_state.py'));switches=importlib.util.module_from_spec(spec);spec.loader.exec_module(switches)
        switches.check(s,d,original,native,trial,text,pre,phase,state,seed,final,symbols,traps)
    if '_initialize_frontend_hook' in trial['accepted']:
        spec=importlib.util.spec_from_file_location('system_info_state',Path(__file__).with_name('system_info_state.py'));system=importlib.util.module_from_spec(spec);spec.loader.exec_module(system)
        system.check(s,d,original,native,trial,text,state,final,symbols,traps)
    if model['schema']>=31:
        spec=importlib.util.spec_from_file_location('arena_state',Path(__file__).with_name('arena_state.py'));arena=importlib.util.module_from_spec(spec);spec.loader.exec_module(arena)
        arena.check(s,d,original,native,trial,text,pre,phase,state,seed,final,symbols,traps)
    if '_initialize_pool_arenas' in trial['accepted']:
        from pool_bind_state import check as check_pool_binding
        check_pool_binding(s,d,original,native,trial,text,state,final,traps)
    if model['schema']>=32 and '_process_compiler_options' in trial['accepted']:
        from options_state import check as check_options
        check_options(s,d,original,native,trial,text,pre,phase,state,seed,final,traps)
    if model['schema']>=33:
        from language_context_state import check as check_language
        check_language(s,d,original,native,trial,text,pre,phase,state,seed,final,traps)
    if '_initialize_include_aliases'in trial['accepted'] and not seed:
        from include_alias_state import check as check_alias
        check_alias(s,d,original,native,trial,text,state,final,traps)
    if model['schema'] >= 34 and trial['trial'] in ('_initialize_checksums','_initialize_multibyte_categories','_initialize_lowercase_table'):
        from checksum_state import check as check_checksum
        check_checksum(s,d,original,native,trial,text,pre,phase,state,seed,final,traps)
    if model['schema']>=35 and trial['trial'] in ('_initialize_multibyte_categories','_initialize_lowercase_table'):
        from scanner_state import check as check_scanner
        check_scanner(s,d,original,native,trial,text,pre,phase,state,seed,final,traps)
    if require_audit:s.audit(trial,audit_path)
    if model['schema']>=36 and trial['trial']=='_initialize_lowercase_table':
        from path_fold_state import check as check_fold
        check_fold(s,d,original,native,trial,text,pre,phase,state,seed,final,traps)
    return {'trial':trial['trial'],'frontier':'_main' if frontier=='c1xx_main' else frontier if frontier in ('CompilerPool::Initialize','CompilerPool::BindArena','CompilerPool::Refill','CompilerArena::AllocatePages','CompilerArena::CommitExtent','PchContext::Initialize','CompilerArena::Reserve','CompilerArena::Reset') else '_'+frontier,'seed':seed,
            'modeled_context_fields':sum(counts.values()),'original_default_context':[0,0,0],
            'typed_bss_bytes':state['frontend']['known_sizes']['bss']-(model['native_padding']['bytes'] if model['schema']>=6 else 0),
            'native_bss_size':state['frontend']['known_sizes']['bss'],
            'native_c_padding_bytes':model['native_padding']['bytes'] if model['schema']>=6 else 0,
            'output_constructor_fields_checked':9*model.get('native_padding',{}).get('output_count',0),
            'cleanup_registration_checked':registrar or wrapper09,
            'nonzero_output_constructor_input_checked':bool(trial.get('test_constructor_seed')),
            'token_semantic_bytes_checked':17 if model['schema']>=15 else 0,
            'token_pointer_arm_tested':False,
            'token_unknown_tail_bytes_each':32 if model['schema']>=15 else None,
            'deferred_work_pointer_checked':model['schema']>=14,
            'nonzero_deferred_constructor_input_checked':bool(trial.get('test_deferred_seed')),
            'nonzero_expanded_BSS_restore_checked':bool(seed),'original_unmodeled_snapshot_bytes':6031 if model['schema']>=35 else 6543 if model['schema']>=33 else 6547 if model['schema']>=32 else 8371 if model['schema']>=30 else 8383 if model['schema']>=28 else 8431 if model['schema']>=22 else 8516,
            'native_snapshot_byte_equivalence_claimed':False,'whole_mask_bytes_checked':751216,
            'nonzero_seed_scope':'typed pointer identities and exact scalar17' if model['schema']>=5 else 'legacy zeroing/reset evidence only; not middle-scalar equivalence',
            'original_seconds':o['seconds'],'native_seconds':n['seconds']}


def output(directory,suffix,index=1):
    raw=(directory/('output-buffer-%02d-'%index+suffix+'.bin')).read_bytes()
    s.require(len(raw)==33,'missing encountered output-buffer fields')
    names=('data','cursor','capacity','relocation','relocation_context','file','ok','descriptor','mode')
    return dict(zip(names,struct.unpack('<7IiB',raw)))


def output_seed(pointer,callback):
    return {'data':pointer,'cursor':pointer+4,'capacity':64,'relocation':callback,
            'relocation_context':pointer+8,'file':pointer+12,'ok':53,'descriptor':37,'mode':23}


def check_outputs(original,native,trial,text,pre,phase,state,seed,final,slots,symbols,traps,model):
    zero={name:0 for name in ('data','cursor','capacity','relocation','relocation_context','file','ok','descriptor','mode')}
    done=dict(zero,ok=1,descriptor=-1)
    before=d.original_fields(original,'006');records={row['name']:row for row in trial['driver']}
    for number in range(1,model['native_padding']['output_count']+1):
        i=number-1;name='initialize_output_buffer_%02d'%number
        s.require(output(original,'000',number)==zero,'original output fields not initially zero')
        expected_old=output_seed(before['saved_state'],0x10438610) if seed else zero
        expected_new=output_seed(pre['frontend']['saved_state'],slots[1]) if seed else zero
        s.require(output(original,'006',number)==expected_old and pre['frontend']['output_buffers'][i]==expected_new,
                  'nonzero output BSS seed/default differs')
        s.require(output(original,'phase',number)==zero and phase['frontend']['output_buffers'][i]==zero,
                  'output fields escaped typed BSS clearing')
        stem='constructor-entry' if number==1 else 'constructor%02d-entry'%number
        entry=json.loads((native/(stem+'.json')).read_text())
        symbol=re.search(r'^([0-9a-f]+) [0-9a-f]+ [Tt] '+name+'$',symbols,re.M)
        s.require(symbol is not None and entry['pc']==int(symbol[1],16) and entry['symbol']==name and entry['args']=={},
                  'not the actual native constructor entry')
        matches=[(index,row) for index,row in enumerate(traps) if int(row[1],16)==records['_'+name]['start']-0x10400000]
        s.require(len(matches)==1,'missing/ambiguous original constructor entry')
        trap_index,row=matches[0];sequence='%03d'%int(row[0])
        seeded=trial.get('test_constructor_seed') and trial['trial']=='_'+name
        if seeded:
            s.require(trial['auxiliary_data_seed']=={'owner':'_'+name,'point':'entry before target body',
                'data':'owned snapshot+0','cursor':'owned snapshot+4','capacity':64,
                'unused_relocation':'_initialize_frontend_01; never invoked','relocation_context':'owned snapshot+8',
                'file':'owned snapshot+12; never dereferenced','ok':53,'descriptor':37,'mode':23,
                'actual_initializer_table_unchanged':True,'code_or_allocator_mock':False},'undeclared constructor-entry data mocks')
            s.require(output(original,sequence,number)==output_seed(d.original_fields(original,sequence)['saved_state'],0x10438610) and
                      entry['frontend']['output_buffers'][i]==output_seed(entry['frontend']['saved_state'],slots[1]) and
                      ('output-seed object=%d role=constructor-entry '%number in text or number==1 and 'output-seed role=constructor-entry ' in text),
                      'missing actual nonzero constructor input')
        else:
            s.require(output(original,sequence,number)==zero and entry['frontend']['output_buffers'][i]==zero,
                      'ordinary constructor input was mocked')
        s.require(output(original,final,number)==done and state['frontend']['output_buffers'][i]==done,
                  'constructor did not assign all nine encountered fields')
        registration_stem='registration-entry' if number==1 else 'registration%02d-entry'%number
        registration=json.loads((native/(registration_stem+'.json')).read_text()) if '_register_cleanup' in trial['accepted'] else state
        register_row=traps[trap_index+1]
        callback_name='destroy_output_buffer_%02d'%number
        callback=re.search(r'^([0-9a-f]+) [0-9a-f]+ [Tt] '+callback_name+'$',symbols,re.M)
        s.require(register_row[1]=='000386ae' and callback is not None and registration['symbol']=='register_cleanup' and
                  registration['args']=={'cleanup':int(callback[1],16)} and
                  int(register_row[2].split(',')[1],16)==records['_'+callback_name]['start'] and
                  registration['frontend']['cleanup_count']==i and
                  output(original,'%03d'%int(register_row[0]),number)==done and registration['frontend']['output_buffers'][i]==done,
                  'actual destructor-registration input or prestate differs')


def check_deferred(original,native,trial,text,pre,phase,state,seed,final,symbols,traps):
    def pointer(suffix):
        raw=(original/('deferred-work-'+suffix+'.bin')).read_bytes()
        s.require(len(raw)==4,'missing raw deferred-work pointer')
        return struct.unpack('<I',raw)[0]
    s.require(pointer('000')==0,'deferred-work original default differs')
    before=d.original_fields(original,'006')
    s.require(pointer('006')==(before['saved_state'] if seed else 0) and
              pre['frontend']['deferred_work']==(pre['frontend']['saved_state'] if seed else 0),
              'deferred-work nonzero reset/default input differs')
    s.require(pointer('phase')==phase['frontend']['deferred_work']==0,'deferred-work escaped BSS clearing')
    entry=json.loads((native/'deferred-entry.json').read_text())
    symbol=re.search(r'^([0-9a-f]+) [0-9a-f]+ [Tt] initialize_deferred_work$',symbols,re.M)
    s.require(symbol is not None and entry['pc']==int(symbol[1],16) and entry['symbol']=='initialize_deferred_work' and entry['args']=={},
              'not actual deferred-work constructor entry')
    matches=[(i,row) for i,row in enumerate(traps) if row[1]=='00038904']
    s.require(len(matches)==1,'missing/ambiguous deferred constructor raw entry')
    index,row=matches[0];suffix='%03d'%int(row[0])
    if trial.get('test_deferred_seed'):
        s.require(trial['auxiliary_deferred_data_seed']=={'owner':'_initialize_deferred_work','point':'entry before target body',
            'deferred_work':'owned snapshot identity; never dereferenced','code_or_allocator_mock':False} and
                  pointer(suffix)==d.original_fields(original,suffix)['saved_state'] and
                  entry['frontend']['deferred_work']==entry['frontend']['saved_state'] and
                  'deferred-seed role=constructor-entry value=owned-snapshot never-dereferenced=1' in text,
                  'missing/changed explicit deferred constructor data seed')
    else:
        s.require('auxiliary_deferred_data_seed' not in trial and pointer(suffix)==entry['frontend']['deferred_work']==0,
                  'ordinary deferred constructor input mocked')
    s.require(pointer(final)==state['frontend']['deferred_work']==0,'deferred-work constructor did not clear pointer')
    registration=json.loads((native/'deferred-registration.json').read_text())
    destructor=re.search(r'^([0-9a-f]+) [0-9a-f]+ [Tt] destroy_deferred_work$',symbols,re.M)
    register=traps[index+1]
    s.require(register[1]=='000386ae' and int(register[2].split(',')[1],16)==0x104346e4 and destructor is not None and
              registration['symbol']=='register_cleanup' and registration['args']=={'cleanup':int(destructor[1],16)} and
              registration['frontend']['cleanup_count']==8 and registration['frontend']['deferred_work']==0 and
              pointer('%03d'%int(register[0]))==0,'deferred destructor registration input differs')


def token_raw(directory,suffix):
    source=(directory/('token-source-'+suffix+'.bin')).read_bytes()
    payload=(directory/('sentinel-payload-'+suffix+'.bin')).read_bytes()
    header=(directory/('sentinel-header-'+suffix+'.bin')).read_bytes()
    s.require(len(source)==len(payload)==40 and len(header)==8,'missing complete original token payload/header')
    return source,payload,header


def check_token_value(original,native,trial,text,pre,phase,state,seed,final,symbols,traps):
    def values(raw):return struct.unpack_from('<q',raw[0])[0],{'kind':raw[2][0],'value':struct.unpack_from('<q',raw[1])[0]}
    zero=(0,{'kind':0,'value':0})
    s.require(values(token_raw(original,'000'))==zero,'token prefix/kind defaults differ')
    before=token_raw(original,'006');expected=(0x1122334455667788,{'kind':165,'value':-7}) if seed else zero
    s.require(values(before)==(pre['frontend']['token_value'],pre['frontend']['sentinel_token'])==expected,
              'nonzero token prefix/kind reset inputs differ')
    s.require(values(token_raw(original,'phase'))==(phase['frontend']['token_value'],phase['frontend']['sentinel_token'])==zero,
              'token prefix/kind escaped BSS clear')
    entry=json.loads((native/'value-copy-entry.json').read_text())
    symbol=re.search(r'^([0-9a-f]+) [0-9a-f]+ [Tt] initialize_frontend_18$',symbols,re.M)
    s.require(symbol is not None and entry['pc']==int(symbol[1],16) and entry['symbol']=='initialize_frontend_18' and entry['args']=={},
              'not actual token copy entry')
    rows=[row for row in traps if row[1]=='0003891e'];s.require(len(rows)==1,'missing token copy raw entry')
    suffix='%03d'%int(rows[0][0]);old=token_raw(original,suffix)
    variant=trial.get('test_token_seed')
    number=0x1122334455667788 if variant=='positive' else -0x112233445566778 if variant=='negative' else 0
    wanted=(number,{'kind':165,'value':-7}) if variant else zero
    s.require(values(old)==(entry['frontend']['token_value'],entry['frontend']['sentinel_token'])==wanted,
              'token copy integer-arm input differs')
    if variant:
        s.require(trial['auxiliary_token_data_seed']=={'owner':'_initialize_frontend_18','point':'entry before target body','arm':'integer',
            'source':number,'old_cache':-7,'old_kind':165,'unknown_tails_seeded':False,'pointer_arm_tested':False,'code_or_allocator_mock':False} and
                  'token-seed role=callback-entry arm=integer sign='+variant+' unknown-tails-seeded=0 pointer-arm-tested=0' in text,
                  'undeclared token integer auxiliary roles')
    else:s.require('auxiliary_token_data_seed' not in trial,'ordinary token input mocked')
    copy_index=next(i for i,row in enumerate(traps) if row[1]=='0003891e')
    s.require(traps[copy_index+1][1]=='0003893b','missing immediate post-copy original boundary')
    after=token_raw(original,'%03d'%int(traps[copy_index+1][0]))
    native_after=json.loads((native/'value-copy-result.json').read_text()) if (native/'value-copy-result.json').exists() else state
    if (native/'value-copy-result.json').exists():
        point=re.search(r'^([0-9a-f]+) [0-9a-f]+ [Tt] initialize_frontend_19$',symbols,re.M)
        s.require(point is not None and native_after['pc']==int(point[1],16) and native_after['symbol']=='initialize_frontend_19' and native_after['args']=={},'false immediate post-copy native boundary')
    s.require(after[0]==old[0] and after[1]==old[0] and after[2][0]==1 and after[2][1:]==old[2][1:],
              'complete ORIGINAL-to-ORIGINAL40-byte internal copy/source/header preservation differs')
    s.require((native_after['frontend']['token_value'],native_after['frontend']['sentinel_token'])==(number,{'kind':1,'value':number}) and
              entry['frontend']['sentinel_padding']==native_after['frontend']['sentinel_padding'],
              'typed token prefix/kind copy or native own padding preservation differs')
    s.require(token_raw(original,final)==after and state['frontend']['token_value']==native_after['frontend']['token_value'] and state['frontend']['sentinel_token']==native_after['frontend']['sentinel_token'],'later initializer unexpectedly changed token projection before consumer frontier')
    for key,value in entry['frontend'].items():
        if key not in ('sentinel_token',):s.require(native_after['frontend'][key]==value,'token copy changed adjacent modeled native state '+key)
    # The unknown original tails are compared only with ORIGINAL source bytes.
    # No native object or integer conversion represents those bytes or pointers.


def check_signature(original,native,pre,phase,state,seed,final,symbols,traps):
    def raw(suffix):
        data=(original/('source-signature-'+suffix+'.bin')).read_bytes()
        s.require(len(data)==4,'missing original signature bytes')
        return struct.unpack('<I',data)[0]
    s.require(raw('000')==0,'original signature default differs')
    s.require(raw('006')==pre['frontend']['source_signature']==(37 if seed else 0),'signature reset input differs')
    s.require(raw('phase')==phase['frontend']['source_signature']==0,'signature escaped BSS reset')
    rows=[row for row in traps if row[1]=='0003893d'];s.require(len(rows)==1,'missing signature initializer raw entry')
    entry=json.loads((native/'signature-entry.json').read_text())
    symbol=re.search(r'^([0-9a-f]+) [0-9a-f]+ [Tt] initialize_frontend_21$',symbols,re.M)
    s.require(symbol is not None and entry['symbol']=='initialize_frontend_21' and entry['args']=={} and entry['pc']==int(symbol[1],16) and
              raw('%03d'%int(rows[0][0]))==entry['frontend']['source_signature']==0,'actual signature entry input differs')
    s.require(raw(final)==state['frontend']['source_signature']==0xffffffff,'signature initializer did not seed unsigned32 all-ones')
