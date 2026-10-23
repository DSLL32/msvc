"""Linux-only real DLL-pass frontier capture, called by capture-native.py."""
import json
import os
import re
from pathlib import Path
import subprocess
import time


def capture(project,output,binary,trial):
    for item,digest in trial['case_inputs'].items():
        import hashlib
        source=project/item
        if hashlib.sha256(source.read_bytes()).hexdigest()!=digest:raise ValueError('case input changed')
        (output/source.name).write_bytes(source.read_bytes())
    cfg=json.loads((output/'functions.json').read_text())
    names=[]
    for row in cfg['driver']:
        name=row.get('native_symbol') or row['name'].lstrip('_').split('@')[0]
        names.append(name)
    (output/'filter.txt').write_text(''.join(name+'\n' for name in names))
    (output/'seen.txt').write_text('')
    environment=os.environ.copy()
    contract=json.loads((output/'contract.json').read_text())
    environment.update(contract['consumed_environment_seed'])
    environment['C1XX_INVOKE_ARGV']=str(output/'invoke-argv.bin')
    environment['RECON_DRIVE_R']=str(project.parents[2])
    if trial.get('system_info_observers'):
        reference=environment.get('C1XX_SYSTEM_INFO_ORIGINAL')
        if not reference:raise ValueError('provide explicit cached original system-info reference')
        from system_info_reference import prepare as prepare_system_info
        reference_sha=prepare_system_info(project,Path(reference),output,trial)
        environment.update(C1XX_SYSTEM_INFO_PROFILE=str(output/'system-info-profile.bin'),C1XX_SYSTEM_INFO_REFERENCE_SHA256=reference_sha,C1XX_CRT_ARGV=str(output/'original-crt-argv.bin'),C1XX_CRT_ENVIRONMENT=str(output/'original-crt-environment.bin'))
    environment.pop('C1XX_X87_ENTRY_REFERENCE',None)
    if trial.get('scaler_observers'):
        from x87_reference import prepare as prepare_x87
        environment['C1XX_X87_ENTRY_REFERENCE']=prepare_x87(project,Path(environment['C1XX_SYSTEM_INFO_ORIGINAL']),output,trial)
    environment.pop('C1XX_TEST_INIT_RESTORE',None)
    if trial.get('test_seed'):environment['C1XX_TEST_INIT_RESTORE']=trial['test_seed']
    wrapper=output/'exec-wrapper.py'
    wrapper.write_text('#!/usr/bin/python3\nimport os,sys\nfd=os.open('+repr(str(output/'trace.txt'))+',os.O_WRONLY|os.O_CREAT|os.O_TRUNC,0o600)\nos.set_inheritable(fd,True)\nos.environ.update(RECON_TRACE_FD=str(fd),RECON_TRACE_FILTER='+repr(str(output/'filter.txt'))+',RECON_TRACE_SEEN='+repr(str(output/'seen.txt'))+')\nos.execv(sys.argv[1],sys.argv[1:])\n')
    wrapper.chmod(0o755)
    collector=output/'collect.gdb'
    collector.write_text('''python
import gdb,json,struct,time
recon_collect_started=time.perf_counter()
frame=gdb.newest_frame()
value={'symbol':frame.name(),'pc':int(gdb.parse_and_eval('$pc')),'eax':int(gdb.parse_and_eval('(unsigned int)$eax')),'esi':int(gdb.parse_and_eval('(unsigned int)$esi')),'edi':int(gdb.parse_and_eval('(unsigned int)$edi')),'esp':int(gdb.parse_and_eval('(unsigned int)$esp')),
       'control_word':int(gdb.parse_and_eval('$fctrl')),'args':{}}
block=frame.block()
while block is not None and not block.is_global and not block.is_static:
    for symbol in block:
        if symbol.is_argument: value['args'][symbol.name]=int(frame.read_var(symbol))
    block=block.superblock
if frame.name()=='reserve_work_buffer' and value['pc']==int(gdb.parse_and_eval('(unsigned int)&reserve_work_buffer')):
    value['args']={'reserve_bytes':int(gdb.parse_and_eval('*(unsigned int *)($esp+4)')),'commit_bytes':int(gdb.parse_and_eval('*(unsigned int *)($esp+8)'))}
    value['raw_entry_abi']={'esp':value['esp'],'return_pc':int(gdb.parse_and_eval('*(unsigned int *)$esp')),'stack_arguments':[value['args']['reserve_bytes'],value['args']['commit_bytes']]}
if frame.name()=='recon_GetSystemInfo' and value['pc']==int(gdb.parse_and_eval('(unsigned int)&recon_GetSystemInfo')):
    value['args']={'output':int(gdb.parse_and_eval('*(unsigned int *)($esp+4)'))}
if frame.name() in ('initialize_frontend_hook','initialize_compiler_environment','initialize_default_arena') and value['pc']==int(gdb.parse_and_eval('(unsigned int)&'+frame.name())):
    value['raw_entry_abi']={'esp':value['esp'],'return_pc':int(gdb.parse_and_eval('*(unsigned int *)$esp')),'stack_arguments':[]}
if frame.name() in ('initialize_intermediate_files','process_early_option') and value['pc']==int(gdb.parse_and_eval('(unsigned int)&'+frame.name())):
    names=['argv'] if frame.name()=='initialize_intermediate_files' else ['argument','state']
    values=[int(gdb.parse_and_eval('*(unsigned int *)($esp+%d)'%(4+4*i))) for i in range(len(names))]
    value['args']=dict(zip(names,values))
    value['raw_entry_abi']={'esp':value['esp'],'return_pc':int(gdb.parse_and_eval('*(unsigned int *)$esp')),'stack_arguments':values}
    formal_block=gdb.block_for_pc(value['pc'])
    while formal_block and not formal_block.function:formal_block=formal_block.superblock
    value['formal_inventory']=[{'name':item.name,'type':str(item.type)} for item in formal_block if item.is_argument] if formal_block else []
if frame.name() in ('initialize_default_arena','CompilerArena::Reserve','calculate_arena_reservation'):
    value['caller_return_pc']=int(frame.older().pc())
if frame.name()=='initialize_work_buffers':
    value['caller_return_pc']=int(frame.older().pc())
if frame.name()=='c1xx_main' and value['pc']==int(gdb.parse_and_eval('(unsigned int)&c1xx_main')):
    value['args']={'argc':int(gdb.parse_and_eval('*(int *)($esp+4)')),'argv':int(gdb.parse_and_eval('*(unsigned int *)($esp+8)'))}
    value['raw_entry_abi']={'esp':value['esp'],'return_pc':int(gdb.parse_and_eval('*(unsigned int *)$esp')),'stack_arguments':[value['args']['argc'],value['args']['argv']]}
if frame.name()=='register_cleanup' and value['pc']==int(gdb.parse_and_eval('(unsigned int)&register_cleanup')):
    value['args']={'cleanup':int(gdb.parse_and_eval('*(unsigned int *)($esp+4)'))}
    value['raw_entry_abi']={'esp':value['esp'],'return_pc':int(gdb.parse_and_eval('*(unsigned int *)$esp')),'stack_arguments':[value['args']['cleanup']]}
if frame.name()=='initialize_generated_comment' and value['pc']==int(gdb.parse_and_eval('(unsigned int)&initialize_generated_comment')):
    value['raw_entry_abi']={'esp':value['esp'],'return_pc':int(gdb.parse_and_eval('*(unsigned int *)$esp')),'stack_arguments':[]}
if frame.name()=='CompilerPool::Initialize' and value['pc']==int(gdb.parse_and_eval('(unsigned int)&_ZN12CompilerPool10InitializeEj')):
    value['args']={'this':int(gdb.parse_and_eval('(unsigned int)$ecx')),'minimum_pages':int(gdb.parse_and_eval('*(unsigned int *)($esp+4)'))}
    value['raw_entry_abi']={'ecx':value['args']['this'],'esp':value['esp'],'return_pc':int(gdb.parse_and_eval('*(unsigned int *)$esp')),'stack_argument':value['args']['minimum_pages']}
if frame.name()=='PchContext::Initialize' and value['pc']==int(gdb.parse_and_eval('(unsigned int)&_ZN10PchContext10InitializeEv')):
    value['args']={'this':int(gdb.parse_and_eval('(unsigned int)$ecx'))}
    value['raw_entry_abi']={'ecx':value['args']['this'],'edi':value['edi'],'esp':value['esp'],'return_pc':int(gdb.parse_and_eval('*(unsigned int *)$esp')),'stack_arguments':[]}
if frame.name()=='CompilerArena::Reserve' and value['pc']==int(gdb.parse_and_eval('(unsigned int)&_ZN13CompilerArena7ReserveEPK15ArenaDescriptorPh')):
    value['args']={'this':int(gdb.parse_and_eval('(unsigned int)$ecx')),'descriptor':int(gdb.parse_and_eval('*(unsigned int *)($esp+4)')),'address':int(gdb.parse_and_eval('*(unsigned int *)($esp+8)'))}
    value['raw_entry_abi']={'ecx':value['args']['this'],'esp':value['esp'],'return_pc':int(gdb.parse_and_eval('*(unsigned int *)$esp')),'stack_arguments':[value['args']['descriptor'],value['args']['address']]}
    formal_block=gdb.block_for_pc(value['pc'])
    while formal_block and not formal_block.function:formal_block=formal_block.superblock
    value['formal_inventory']=[{'name':item.name,'type':str(item.type)} for item in formal_block if item.is_argument] if formal_block else []
if frame.name()=='calculate_arena_reservation' and value['pc']==int(gdb.parse_and_eval('(unsigned int)&calculate_arena_reservation')):
    value['args']={'descriptor':int(gdb.parse_and_eval('*(unsigned int *)($esp+4)'))}
    value['raw_entry_abi']={'esp':value['esp'],'return_pc':int(gdb.parse_and_eval('*(unsigned int *)$esp')),'stack_arguments':[value['args']['descriptor']]}
    formal_block=gdb.block_for_pc(value['pc'])
    while formal_block and not formal_block.function:formal_block=formal_block.superblock
    value['formal_inventory']=[{'name':item.name,'type':str(item.type)} for item in formal_block if item.is_argument] if formal_block else []
if frame.name()=='CompilerArena::Reset' and value['pc']==int(gdb.parse_and_eval('(unsigned int)&_ZN13CompilerArena5ResetEv')):
    value['args']={'this':int(gdb.parse_and_eval('(unsigned int)$ecx'))}
    value['raw_entry_abi']={'ecx':value['args']['this'],'esp':value['esp'],'return_pc':int(gdb.parse_and_eval('*(unsigned int *)$esp')),'stack_arguments':[]}
value['x87']={name:int(gdb.parse_and_eval(register)) for name,register in [('control','$fctrl'),('status','$fstat'),('tag','$ftag')]}
value['target_state']={name:int(gdb.parse_and_eval('(unsigned int)'+name)) for name in
    ('process_attached','raw_dll_entry','dll_adjust_fdiv','dll_module','onexit_begin','onexit_end')}
value['target_state']['last_error']=int(gdb.parse_and_eval('last_error'))
try:value['target_state']['compiler_mode']=int(gdb.parse_and_eval('compiler_mode'))
except gdb.error:pass
value['frontend']={name:int(gdb.parse_and_eval('(unsigned int)'+expr)) for name,expr in
    [('first_invocation','frontend_data.first_invocation'),('saved_state','frontend_bss.saved_state'),
     ('cleanup_count','frontend_bss.cleanup_count'),('restore_mode','restore_mode')]}
value['frontend']['saved_first']=int(gdb.parse_and_eval('frontend_bss.saved_state->first_invocation')) if value['frontend']['saved_state'] else None
value['frontend']['cleanup_callbacks']=[int(gdb.parse_and_eval('(unsigned int)frontend_bss.cleanup_callbacks[%d]'%n)) for n in range(32)]
value['frontend']['storage']={name:[int(gdb.parse_and_eval('(unsigned int)&'+expr)),int(gdb.parse_and_eval('sizeof('+expr+')'))] for name,expr in
    [('first_invocation','frontend_data.first_invocation'),('saved_state','frontend_bss.saved_state'),
     ('cleanup_count','frontend_bss.cleanup_count'),('cleanup_callbacks','frontend_bss.cleanup_callbacks'),('restore_mode','restore_mode'),('compiler_mode','compiler_mode')]}
value['frontend']['context_stack']=[int(gdb.parse_and_eval('(unsigned int)frontend_bss.context_stack.'+name)) for name in ('origin','position','top')]
value['frontend']['storage']['context_stack']=[int(gdb.parse_and_eval('(unsigned int)&frontend_bss.context_stack')),12]
value['frontend']['saved_context_stack']=[int(gdb.parse_and_eval('(unsigned int)frontend_bss.saved_context_stack.'+name)) for name in ('origin','position','top')]
value['frontend']['storage']['saved_context_stack']=[int(gdb.parse_and_eval('(unsigned int)&frontend_bss.saved_context_stack')),int(gdb.parse_and_eval('sizeof(frontend_bss.saved_context_stack)'))]
value['frontend']['nested_context_stacks']=[int(gdb.parse_and_eval('(unsigned int)frontend_bss.nested_context_stacks[%d].%s'%(i,name))) for i in range(9) for name in ('origin','position','top')]
value['frontend']['storage']['nested_context_stacks']=[int(gdb.parse_and_eval('(unsigned int)&frontend_bss.nested_context_stacks')),int(gdb.parse_and_eval('sizeof(frontend_bss.nested_context_stacks)'))]
value['frontend']['context_array_count']=int(gdb.parse_and_eval('sizeof(frontend_bss.nested_context_stacks)/sizeof(frontend_bss.nested_context_stacks[0])'))
value['frontend']['source_position']=[int(gdb.parse_and_eval('(unsigned int)frontend_bss.source_position.'+name)) for name in ('origin','position')]
value['frontend']['storage']['source_position']=[int(gdb.parse_and_eval('(unsigned int)&frontend_bss.source_position')),int(gdb.parse_and_eval('sizeof(frontend_bss.source_position)'))]
value['frontend']['source_position_fields']=[field.name for field in gdb.lookup_type('FrontendSourcePosition').fields()]
value['frontend']['context_stack_fields']=[field.name for field in gdb.lookup_type('FrontendContextStack').fields()]
value['frontend']['declared_fields']={'initialized':[field.name for field in gdb.lookup_type('FrontendData').fields()], 'bss':[field.name for field in gdb.lookup_type('FrontendBss').fields()]}
value['frontend']['known_sizes']={'initialized':int(gdb.parse_and_eval('sizeof(FrontendData)')),'bss':int(gdb.parse_and_eval('sizeof(FrontendBss)'))}
value['frontend']['output_buffers']=[]
output_fields=('data','cursor','capacity','relocation','relocation_context','file','ok','descriptor','mode')
for i in range(int(gdb.parse_and_eval('sizeof(frontend_bss.output_buffers)/sizeof(frontend_bss.output_buffers[0])'))):
    expression='frontend_bss.output_buffers[%d]'%i
    value['frontend']['output_buffers'].append({name:int(gdb.parse_and_eval(expression+'.'+name)) for name in output_fields})
    for name in output_fields:
        value['frontend']['storage']['output_buffer_%02d.'%(i+1)+name]=[int(gdb.parse_and_eval('(unsigned int)&'+expression+'.'+name)),int(gdb.parse_and_eval('sizeof('+expression+'.'+name+')'))]
value['frontend']['output_native_layout']={'size':int(gdb.parse_and_eval('sizeof(OutputBuffer)')),'fields':{field.name:[field.bitpos//8,int(field.type.sizeof)] for field in gdb.lookup_type('OutputBuffer').fields()}}
value['frontend']['output_count']=int(gdb.parse_and_eval('sizeof(frontend_bss.output_buffers)/sizeof(frontend_bss.output_buffers[0])'))
value['frontend']['bss_address']=int(gdb.parse_and_eval('(unsigned int)&frontend_bss'))
value['aux_owned']=None
if int(gdb.parse_and_eval('(unsigned int)c1xx_aux_new_base')):
    value['aux_owned']={}
    for role in ('old','new'):
        base=int(gdb.parse_and_eval('(unsigned int)c1xx_aux_'+role+'_base'))
        payload=int(gdb.parse_and_eval('(unsigned int)c1xx_aux_'+role+'_payload'))
        value['aux_owned'][role]={'base':base,'payload':payload,'owned_request':8584,'payload_bytes':8520,
            'prefix':bytes(gdb.selected_inferior().read_memory(base,32)).hex(),
            'suffix':bytes(gdb.selected_inferior().read_memory(base+32+8520,32)).hex(),
            'known_first':int(gdb.parse_and_eval('c1xx_aux_'+role+'_payload->first_invocation'))}
    if frame.name()=='checked_malloc':
        value['aux_mock_call']={'callee':'_checked_malloc','request':value['args']['bytes'],
            'genuine_int3_pc':value['pc']-1,'actual_opcode':bytes(gdb.selected_inferior().read_memory(value['pc']-1,1)).hex(),
            'result':value['aux_owned']['new']['payload'],'caller_return_pc':int(frame.older().pc())}
value['frontend']['deferred_work']=int(gdb.parse_and_eval('(unsigned int)frontend_bss.deferred_work'))
value['frontend']['storage']['deferred_work']=[int(gdb.parse_and_eval('(unsigned int)&frontend_bss.deferred_work')),4]
value['frontend']['token_value']=int(gdb.parse_and_eval('frontend_bss.token_value.integer'))
value['frontend']['sentinel_token']={'kind':int(gdb.parse_and_eval('frontend_bss.sentinel_token.kind')),'value':int(gdb.parse_and_eval('frontend_bss.sentinel_token.value.integer'))}
for name,expression in [('token_value','frontend_bss.token_value'),('sentinel_token.kind','frontend_bss.sentinel_token.kind'),('sentinel_token.value','frontend_bss.sentinel_token.value')]:
    value['frontend']['storage'][name]=[int(gdb.parse_and_eval('(unsigned int)&'+expression)),int(gdb.parse_and_eval('sizeof('+expression+')'))]
value['frontend']['token_value_layout']={'size':int(gdb.parse_and_eval('sizeof(TokenValue)')),'is_union':gdb.lookup_type('TokenValue').strip_typedefs().code==gdb.TYPE_CODE_UNION,'fields':{f.name:[f.bitpos//8,int(f.type.sizeof)] for f in gdb.lookup_type('TokenValue').fields()}}
value['frontend']['sentinel_native_layout']={'size':int(gdb.parse_and_eval('sizeof(SentinelToken)')),'fields':{f.name:[f.bitpos//8,int(f.type.sizeof)] for f in gdb.lookup_type('SentinelToken').fields()}}
value['frontend']['sentinel_padding']=bytes(gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('(unsigned int)&frontend_bss.sentinel_token.kind'))+1,3)).hex()
value['frontend']['source_signature']=int(gdb.parse_and_eval('frontend_bss.source_signature'))
value['frontend']['storage']['source_signature']=[int(gdb.parse_and_eval('(unsigned int)&frontend_bss.source_signature')),int(gdb.parse_and_eval('sizeof(frontend_bss.source_signature)'))]
if gdb.lookup_global_symbol('input_pdb_context') is not None:
    fields=('database','names','types','minimal_rebuild','stream_state','read_mode')
    value['frontend']['input_pdb_context']={field:int(gdb.parse_and_eval('(unsigned int)input_pdb_context.'+field)) for field in fields}
    for field in fields:
        value['frontend']['storage']['input_pdb_context.'+field]=[int(gdb.parse_and_eval('(unsigned int)&input_pdb_context.'+field)),4]
    value['frontend']['pdb_context_layout']={'size':int(gdb.parse_and_eval('sizeof(CompilerPdbContext)')),'fields':{f.name:[f.bitpos//8,int(f.type.sizeof)] for f in gdb.lookup_type('CompilerPdbContext').fields()}}
if gdb.lookup_global_symbol('output_pdb_context') is not None:
    fields=('database','names','types','minimal_rebuild','stream_state','read_mode')
    value['frontend']['output_pdb_context']={field:int(gdb.parse_and_eval('(unsigned int)output_pdb_context.'+field)) for field in fields}
    for field in fields:
        value['frontend']['storage']['output_pdb_context.'+field]=[int(gdb.parse_and_eval('(unsigned int)&output_pdb_context.'+field)),4]
if any(f.name=='pools' for f in gdb.lookup_type('FrontendBss').fields()):
    fields=('cursor','remaining','minimum_pages','arena')
    count=int(gdb.parse_and_eval('sizeof(frontend_bss.pools)/sizeof(frontend_bss.pools[0])'))
    value['frontend']['pools']=[{name:int(gdb.parse_and_eval('(unsigned int)frontend_bss.pools[%d].'%i+name)) for name in fields} for i in range(count)]
    for i in range(count):
        for name in fields:value['frontend']['storage']['pool_%02d.'%(i+1)+name]=[int(gdb.parse_and_eval('(unsigned int)&frontend_bss.pools[%d].'%i+name)),4]
    value['frontend']['pool_native_layout']={'size':int(gdb.parse_and_eval('sizeof(CompilerPool)')),'fields':{f.name:[f.bitpos//8,int(f.type.sizeof)] for f in gdb.lookup_type('CompilerPool').fields()}}
if any(f.name=='type_ranks_initialized' for f in gdb.lookup_type('FrontendBss').fields()):
    value['frontend']['type_ranks_initialized']=int(gdb.parse_and_eval('frontend_bss.type_ranks_initialized'))
    value['frontend']['storage']['type_ranks_initialized']=[int(gdb.parse_and_eval('(unsigned int)&frontend_bss.type_ranks_initialized')),4]
if any(f.name=='primitive_type_ranks' for f in gdb.lookup_type('FrontendData').fields()):
    for name,count in [('primitive_type_ranks',8),('complex_type_ranks',9)]:
        value['frontend'][name]=[[int(gdb.parse_and_eval('frontend_data.%s[%d].%s'%(name,i,field))) for field in ('code','rank')] for i in range(count)]
        value['frontend']['saved_'+name]=[[int(gdb.parse_and_eval('frontend_bss.saved_state->%s[%d].%s'%(name,i,field))) for field in ('code','rank')] for i in range(count)] if value['frontend']['saved_state'] else None
        for i in range(count):
            for field,width in [('code',4),('rank',1)]:value['frontend']['storage']['%s[%d].%s'%(name,i,field)]=[int(gdb.parse_and_eval('(unsigned int)&frontend_data.%s[%d].%s'%(name,i,field))),width]
    value['frontend']['type_rank_layout']={'size':int(gdb.parse_and_eval('sizeof(TypeRankMapping)')),'fields':{f.name:[f.bitpos//8,int(f.type.sizeof)] for f in gdb.lookup_type('TypeRankMapping').fields()}}
    for name,count in [('primitive_type_rank',65),('complex_type_rank',69)]:
        value['frontend'][name]=[int(gdb.parse_and_eval('frontend_bss.%s[%d]'%(name,i))) for i in range(count)]
        value['frontend']['storage'][name]=[int(gdb.parse_and_eval('(unsigned int)&frontend_bss.'+name)),int(gdb.parse_and_eval('sizeof(frontend_bss.'+name+')'))]
if any(f.name=='empty_metadata_prototype' for f in gdb.lookup_type('FrontendBss').fields()):
    value['frontend']['empty_metadata_prototype']=list(bytes(gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('(unsigned int)&frontend_bss.empty_metadata_prototype')),20)))
    value['frontend']['storage']['empty_metadata_prototype']=[int(gdb.parse_and_eval('(unsigned int)&frontend_bss.empty_metadata_prototype')),20]
if any(f.name=='pch_context' for f in gdb.lookup_type('FrontendBss').fields()):
    pch_fields=['image_count','root_image','images','stored_file_position','file_name','file_offset','file']+['mapping_handles[%d]'%i for i in range(6)]+['loaded_image_count','scratch_arena','original_pages','mapped_pages']
    value['frontend']['pch_context']={name:int(gdb.parse_and_eval(('' if name in ('image_count','stored_file_position','file_offset','loaded_image_count') else '(unsigned int)')+'frontend_bss.pch_context.'+name)) for name in pch_fields}
    for name in pch_fields:value['frontend']['storage']['pch_context.'+name]=[int(gdb.parse_and_eval('(unsigned int)&frontend_bss.pch_context.'+name)),4]
    value['frontend']['pch_native_layout']={'size':int(gdb.parse_and_eval('sizeof(PchContext)')),'fields':{f.name:[f.bitpos//8,int(f.type.sizeof)] for f in gdb.lookup_type('PchContext').fields()}}
if any(f.name=='pending_group_head' for f in gdb.lookup_type('FrontendBss').fields()):
    for name in ('pending_group_head','active_group'):
        value['frontend'][name]=int(gdb.parse_and_eval('(unsigned int)frontend_bss.'+name))
        value['frontend']['storage'][name]=[int(gdb.parse_and_eval('(unsigned int)&frontend_bss.'+name)),4]
if any(f.name=='com_initialized' for f in gdb.lookup_type('FrontendBss').fields()):
    for name in ('com_initialized','comment_value_offset','generated_comment_length'):
        value['frontend'][name]=int(gdb.parse_and_eval('frontend_bss.'+name))
        value['frontend']['storage'][name]=[int(gdb.parse_and_eval('(unsigned int)&frontend_bss.'+name)),4]
    value['frontend']['generated_comment']=list(bytes(gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('(unsigned int)&frontend_bss.generated_comment')),74)))
    value['frontend']['storage']['generated_comment']=[int(gdb.parse_and_eval('(unsigned int)&frontend_bss.generated_comment')),74]
if any(f.name=='deferred_action_depth' for f in gdb.lookup_type('FrontendBss').fields()):
    for name in ('deferred_action_head','deferred_action_tail','deferred_action_depth'):
        value['frontend'][name]=int(gdb.parse_and_eval(('' if name=='deferred_action_depth' else '(unsigned int)')+'frontend_bss.'+name))
        value['frontend']['storage'][name]=[int(gdb.parse_and_eval('(unsigned int)&frontend_bss.'+name)),4]
if any(f.name=='compiler_phase' for f in gdb.lookup_type('FrontendData').fields()):
    system_fields=['dwOemId','dwPageSize','lpMinimumApplicationAddress','lpMaximumApplicationAddress','dwActiveProcessorMask','dwNumberOfProcessors','dwProcessorType','dwAllocationGranularity','wProcessorLevel','wProcessorRevision']
    for name in ('compiler_phase','option_c9il','compiler_flags'):
        value['frontend'][name]=int(gdb.parse_and_eval('frontend_data.'+name))
        value['frontend']['saved_'+name]=int(gdb.parse_and_eval('frontend_bss.saved_state->'+name)) if value['frontend']['saved_state'] else None
        value['frontend']['storage'][name]=[int(gdb.parse_and_eval('(unsigned int)&frontend_data.'+name)),4]
    for name in ('break_into_debugger','command_argc','command_argv'):
        value['frontend'][name]=int(gdb.parse_and_eval(('' if name in ('break_into_debugger','command_argc') else '(unsigned int)')+'frontend_bss.'+name))
        value['frontend']['storage'][name]=[int(gdb.parse_and_eval('(unsigned int)&frontend_bss.'+name)),4]
    value['frontend']['system_info']=[int(gdb.parse_and_eval('(unsigned int)frontend_data.system_info.'+name)) for name in system_fields]
    value['frontend']['saved_system_info']=[int(gdb.parse_and_eval('(unsigned int)frontend_bss.saved_state->system_info.'+name)) for name in system_fields] if value['frontend']['saved_state'] else None
    value['frontend']['storage']['system_info']=[int(gdb.parse_and_eval('(unsigned int)&frontend_data.system_info')),36]
    value['frontend']['system_info_layout']={'size':int(gdb.parse_and_eval('sizeof(SYSTEM_INFO)')),'fields':{name:[int(gdb.parse_and_eval('(unsigned int)&frontend_data.system_info.'+name))-int(gdb.parse_and_eval('(unsigned int)&frontend_data.system_info')),int(gdb.parse_and_eval('sizeof(frontend_data.system_info.'+name+')'))] for name in system_fields}}
if any(f.name=='option_zb' for f in gdb.lookup_type('FrontendData').fields()):
    for name in ('option_zb','option_bm','option_zm_percent'):
        value['frontend'][name]=int(gdb.parse_and_eval('frontend_data.'+name))
        value['frontend']['saved_'+name]=int(gdb.parse_and_eval('frontend_bss.saved_state->'+name)) if value['frontend']['saved_state'] else None
        value['frontend']['storage'][name]=[int(gdb.parse_and_eval('(unsigned int)&frontend_data.'+name)),4]
if any(f.name=='work_buffers' for f in gdb.lookup_type('FrontendBss').fields()):
    value['frontend']['work_buffers']=[[int(gdb.parse_and_eval('(unsigned int)frontend_bss.work_buffers[%d].%s'%(i,name))) for name in ('data','limit')] for i in range(2)]
    for i in range(2):
        for name in ('data','limit'):value['frontend']['storage']['work_buffer_%02d.%s'%(i+1,name)]=[int(gdb.parse_and_eval('(unsigned int)&frontend_bss.work_buffers[%d].%s'%(i,name))),4]
    value['frontend']['work_buffer_layout']={'size':int(gdb.parse_and_eval('sizeof(CompilerWorkBuffer)')),'fields':{f.name:[f.bitpos//8,int(f.type.sizeof)] for f in gdb.lookup_type('CompilerWorkBuffer').fields()}}
if any(f.name=='arenas' for f in gdb.lookup_type('FrontendBss').fields()):
    arena_address=int(gdb.parse_and_eval('(unsigned int)&frontend_bss.arenas'))
    descriptor_address=int(gdb.parse_and_eval('(unsigned int)&arena_descriptors'))
    value['frontend']['arenas']=list(struct.unpack('<72I',gdb.selected_inferior().read_memory(arena_address,288)))
    value['frontend']['arena_descriptors']=list(struct.unpack('<45I',gdb.selected_inferior().read_memory(descriptor_address,180)))
    value['frontend']['storage']['arenas']=[arena_address,288]
    value['frontend']['storage']['arena_descriptors']=[descriptor_address,180]
    value['frontend']['arena_native_layout']={'size':int(gdb.parse_and_eval('sizeof(CompilerArena)')),'fields':{f.name:[f.bitpos//8,int(f.type.sizeof)] for f in gdb.lookup_type('CompilerArena').fields() if not f.is_base_class and f.bitpos is not None}}
    value['frontend']['descriptor_native_layout']={'size':int(gdb.parse_and_eval('sizeof(ArenaDescriptor)')),'fields':{f.name:[f.bitpos//8,int(f.type.sizeof)] for f in gdb.lookup_type('ArenaDescriptor').fields()}}
value['frontend']['callback_slots']=[int(gdb.parse_and_eval('(unsigned int)frontend_initializers[%d]'%n)) for n in range(43)]
value['target_state']['initializer_words']=[int(gdb.parse_and_eval('(unsigned int)crt_initializers[%d]'%n)) for n in range(2)]
value['target_state']['onexit_first']=int(gdb.parse_and_eval('(unsigned int)*onexit_begin')) if value['target_state']['onexit_begin'] else None
invocation=frame
while invocation is not None and invocation.name()!='InvokeCompilerPass':invocation=invocation.older()
if invocation is None:raise RuntimeError('no actual Invoke caller frame')
argc=int(invocation.read_var('argc'));argv=invocation.read_var('argv');mode=int(invocation.read_var('mode'))
raw=struct.pack('<II',int(argv),argc);strings=[]
for n in range(argc):
    pointer=int(argv[n]);text=argv[n].string(encoding='latin1').encode('latin1')+b'\\0'
    raw+=struct.pack('<II',pointer,len(text))+text;strings.append(text[:-1].decode('latin1'))
value['invocation']={'argc':argc,'mode':mode,'argv_address':int(argv),'argv':strings}
host=frame
while host is not None and host.name()!='main':host=host.older()
if host is None:raise RuntimeError('no actual native DLL host frame')
spec=host.read_var('spec')
value['registration']={key:spec[key].string() for key in ('name','path','sha256')}
value['registration']['flags']=int(spec['flags']);value['registered_token']=int(host.read_var('module'))
recon_serialize_started=time.perf_counter()
open(@@ARGV@@,'wb').write(raw)
open(@@STATE@@,'w').write(json.dumps(value,indent=2)+'\\n')
recon_collect_timings.append({'state_file':@@STATE@@.rsplit('/',1)[-1],'collection_seconds':recon_serialize_started-recon_collect_started,'serialization_io_seconds':time.perf_counter()-recon_serialize_started})
end
'''.replace('@@ARGV@@',repr(str(output/'native-argv.bin'))).replace('@@STATE@@',repr(str(output/'state.json'))))
    commands=output/'commands.gdb'
    pre=output/'collect-pre.gdb'
    pre.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/'pre.json'))).replace(repr(str(output/'native-argv.bin')),repr(str(output/'pre-argv.bin'))))
    pdb_seed=''
    pdb_object='output_pdb_context' if trial['trial']=='_initialize_pdb_context_02' else 'input_pdb_context'
    if trial.get('test_pdb_seed') is not None:
        for index,field in enumerate(('database','names','types','minimal_rebuild','stream_state')):
            pdb_seed+='set variable '+pdb_object+'.'+field+'=(void *)((unsigned char *)frontend_bss.saved_state+%d)\n'%(index*4)
        pdb_seed+='set variable '+pdb_object+'.read_mode=37\n'
    observer='break *initialize_compiler_pass\ncommands\nsilent\n'+pdb_seed+'source '+str(pre)+'\ncontinue\nend\n'
    if trial['trial']=='_checked_malloc':
        allocation=output/'collect-allocation-entry.gdb'
        allocation.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/'allocation-entry.json'))).replace(repr(str(output/'native-argv.bin')),repr(str(output/'allocation-entry-argv.bin'))))
        observer+='break checked_malloc\ncommands\nsilent\nsource '+str(allocation)+'\ncontinue\nend\n'
    model=json.loads((output/'state-model.json').read_text())
    for i in range(model.get('native_padding',{}).get('output_count',0)):
        number=i+1;name='initialize_output_buffer_%02d'%number
        stem='constructor-entry' if number==1 else 'constructor%02d-entry'%number
        entry=output/('collect-'+stem+'.gdb')
        entry.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/(stem+'.json')))).replace(repr(str(output/'native-argv.bin')),repr(str(output/(stem+'-argv.bin')))))
        change=''
        if trial.get('test_constructor_seed') and trial['trial']=='_'+name:
            assignments={'data':'(unsigned char *)frontend_bss.saved_state','cursor':'(unsigned char *)frontend_bss.saved_state+4',
                'capacity':'64','relocation':'(OutputRelocation)frontend_initializers[1]',
                'relocation_context':'(unsigned char *)frontend_bss.saved_state+8','file':'(void *)((unsigned char *)frontend_bss.saved_state+12)',
                'ok':'53','descriptor':'37','mode':'23'}
            change=''.join('set variable frontend_bss.output_buffers[%d].'%i+field+'='+expression+'\n' for field,expression in assignments.items())
        observer+='break *'+name+'\ncommands\nsilent\n'+change+'source '+str(entry)+'\ncontinue\nend\n'
        if '_register_cleanup' in trial['accepted']:
            stem='registration-entry' if number==1 else 'registration%02d-entry'%number
            registration=output/('collect-'+stem+'.gdb')
            registration.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/(stem+'.json')))).replace(repr(str(output/'native-argv.bin')),repr(str(output/(stem+'-argv.bin')))))
            observer+='break register_cleanup if frontend_bss.cleanup_count==%d\ncommands\nsilent\nsource '%i+str(registration)+'\ncontinue\nend\n'
    if model['schema']>=14:
        entry=output/'collect-deferred-entry.gdb'
        entry.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/'deferred-entry.json'))).replace(repr(str(output/'native-argv.bin')),repr(str(output/'deferred-entry-argv.bin'))))
        seed='set variable frontend_bss.deferred_work=(DeferredWorkFrame *)frontend_bss.saved_state\n' if trial.get('test_deferred_seed') else ''
        observer+='break *initialize_deferred_work\ncommands\nsilent\n'+seed+'source '+str(entry)+'\ncontinue\nend\n'
        registration=output/'collect-deferred-registration.gdb'
        registration.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/'deferred-registration.json'))).replace(repr(str(output/'native-argv.bin')),repr(str(output/'deferred-registration-argv.bin'))))
        observer+='break register_cleanup if frontend_bss.cleanup_count==8\ncommands\nsilent\nsource '+str(registration)+'\ncontinue\nend\n'
    if model['schema']>=15:
        entry=output/'collect-value-copy-entry.gdb'
        entry.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/'value-copy-entry.json'))).replace(repr(str(output/'native-argv.bin')),repr(str(output/'value-copy-entry-argv.bin'))))
        seed=''
        if trial.get('test_token_seed'):
            number=0x1122334455667788 if trial['test_token_seed']=='positive' else -0x112233445566778
            seed='set variable frontend_bss.token_value.integer=%d\nset variable frontend_bss.sentinel_token.value.integer=-7\nset variable frontend_bss.sentinel_token.kind=165\n'%number
        observer+='break *initialize_frontend_18\ncommands\nsilent\n'+seed+'source '+str(entry)+'\ncontinue\nend\n'
    if '_initialize_frontend_19' in trial['accepted']:
        result_point=output/'collect-value-copy-result.gdb'
        result_point.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/'value-copy-result.json'))).replace(repr(str(output/'native-argv.bin')),repr(str(output/'value-copy-result-argv.bin'))))
        observer+='break *initialize_frontend_19\ncommands\nsilent\nsource '+str(result_point)+'\ncontinue\nend\n'
    if model['schema']>=23:
        entry=output/'collect-prototype-entry.gdb'
        entry.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/'prototype-entry.json'))).replace(repr(str(output/'native-argv.bin')),repr(str(output/'prototype-entry-argv.bin'))))
        change=''
        if trial.get('test_prototype_seed'):
            change='python\ngdb.selected_inferior().write_memory(int(gdb.parse_and_eval("(unsigned int)&frontend_bss.empty_metadata_prototype")),bytes(range(1,21)))\nend\n'
        observer+='break *initialize_frontend_36\ncommands\nsilent\n'+change+'source '+str(entry)+'\ncontinue\nend\n'
        result=output/'collect-prototype-result.gdb'
        result.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/'prototype-result.json'))).replace(repr(str(output/'native-argv.bin')),repr(str(output/'prototype-result-argv.bin'))))
        observer+='break *initialize_frontend_37\ncommands\nsilent\nsource '+str(result)+'\ncontinue\nend\n'
    if model['schema']>=16:
        signature=output/'collect-signature-entry.gdb'
        signature.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/'signature-entry.json'))).replace(repr(str(output/'native-argv.bin')),repr(str(output/'signature-entry-argv.bin'))))
        observer+='break *initialize_frontend_21\ncommands\nsilent\nsource '+str(signature)+'\ncontinue\nend\n'
    for number in range(1,3 if model['schema']>=18 else 2 if model['schema']>=17 else 1):
        stem='pdb' if number==1 else 'pdb02'
        entry=output/('collect-'+stem+'-entry.gdb')
        entry.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/(stem+'-entry.json')))).replace(repr(str(output/'native-argv.bin')),repr(str(output/(stem+'-entry-argv.bin')))))
        change='' if trial.get('test_pdb_seed') is None or trial['trial']!='_initialize_pdb_context_%02d'%number else 'set variable restore_mode=%d\n'%trial['test_pdb_seed']
        observer+='break *initialize_pdb_context_%02d\ncommands\nsilent\n'%number+change+'source '+str(entry)+'\ncontinue\nend\n'
        registration=output/('collect-'+stem+'-registration.gdb')
        registration.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/(stem+'-registration.json')))).replace(repr(str(output/'native-argv.bin')),repr(str(output/(stem+'-registration-argv.bin')))))
        observer+='break register_cleanup if frontend_bss.cleanup_count==%d\ncommands\nsilent\nsource '%(8+number)+str(registration)+'\ncontinue\nend\n'
    if model['schema']>=20:
        for i in range(model['pool_projection']['count']):
            target=output/('collect-pool-entry-%02d.gdb'%(i+1))
            target.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/('pool-entry-%02d.json'%(i+1))))).replace(repr(str(output/'native-argv.bin')),repr(str(output/('pool-entry-%02d-argv.bin'%(i+1))))))
        dispatch=output/'observe-pool-entries.gdb'
        dispatch.write_text('python\nimport gdb\nreceiver=int(gdb.parse_and_eval("(unsigned int)$ecx"))\nbases=[int(gdb.parse_and_eval("(unsigned int)&frontend_bss.pools[%d]"%i)) for i in range('+str(model['pool_projection']['count'])+')]\nif receiver not in bases:raise RuntimeError("unmodeled pool receiver")\nnumber=bases.index(receiver)+1\nwith open('+repr(str(output/'pool-entry-order.txt'))+',"a") as stream:stream.write("%d\\n"%number)\ngdb.execute("source "+'+repr(str(output))+'+"/collect-pool-entry-%02d.gdb"%number)\nend\n')
        observer+='break *_ZN12CompilerPool10InitializeEj\ncommands\nsilent\nsource '+str(dispatch)+'\ncontinue\nend\n'
    if trial['trial']=='CompilerPool::Initialize':
        entry=output/'collect-pool-entry.gdb'
        entry.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/'pool-entry.json'))).replace(repr(str(output/'native-argv.bin')),repr(str(output/'pool-entry-argv.bin'))))
        result=output/'collect-pool-return.gdb'
        result.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/'pool-return.json'))).replace(repr(str(output/'native-argv.bin')),repr(str(output/'pool-return-argv.bin'))))
        seed=''
        if trial.get('test_pool_seed'):
            seed='set variable frontend_bss.pools[0].cursor=(unsigned char *)frontend_bss.saved_state+4\nset variable frontend_bss.pools[0].remaining=23\nset variable frontend_bss.pools[0].minimum_pages=37\npython\narena=gdb.parse_and_eval("frontend_bss.pools[0].arena")\narena.assign(gdb.Value(int(gdb.parse_and_eval("(unsigned int)frontend_bss.saved_state"))+8).cast(arena.type))\nend\n'
        return_observer=output/'observe-pool-return.gdb'
        return_observer.write_text('python\nimport gdb\nclass PoolReturnObserver(gdb.Breakpoint):\n    def stop(self):\n        gdb.execute("source "+'+repr(str(result))+')\n        return False\npool_return_bp=PoolReturnObserver("*0x%x"%int(gdb.parse_and_eval("*(unsigned int *)$esp")),temporary=True,internal=True)\nend\n')
        observer+='break *_ZN12CompilerPool10InitializeEj\ncommands\nsilent\n'+seed+'source '+str(entry)+'\nsource '+str(return_observer)+'\ncontinue\nend\n'
    if 'CompilerArena::Reserve' in trial['accepted']:
        entry=output/'collect-arena-size-entry.gdb'
        entry.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/'arena-size-entry.json'))).replace(repr(str(output/'native-argv.bin')),repr(str(output/'arena-size-entry-argv.bin'))))
        observer+='set $arena_size_seen = 0\nbreak *calculate_arena_reservation\ncommands\nsilent\nif $arena_size_seen == 0\nsource '+str(entry)+'\nset $arena_size_seen = 1\nend\ncontinue\nend\n'
    if '_initialize_default_arena' in trial['accepted']:
        entry=output/'collect-arena-member-entry.gdb'
        entry.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/'arena-member-entry.json'))).replace(repr(str(output/'native-argv.bin')),repr(str(output/'arena-member-entry-argv.bin'))))
        observer+='set $arena_member_seen = 0\nbreak *_ZN13CompilerArena7ReserveEPK15ArenaDescriptorPh\ncommands\nsilent\nif $arena_member_seen == 0\nsource '+str(entry)+'\nset $arena_member_seen = 1\nend\ncontinue\nend\n'
    if model['schema']>=31:
        entry=output/'collect-arena-entry.gdb'
        entry.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/'arena-entry.json'))).replace(repr(str(output/'native-argv.bin')),repr(str(output/'arena-entry-argv.bin'))))
        observer+='break *initialize_compiler_environment\ncommands\nsilent\nsource '+str(entry)+'\ncontinue\nend\n'
    if model['schema']>=29:
        entry=output/'collect-work-entry.gdb'
        entry.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/'work-entry.json'))).replace(repr(str(output/'native-argv.bin')),repr(str(output/'work-entry-argv.bin'))))
        observer+='break *initialize_work_buffers\ncommands\nsilent\nsource '+str(entry)+'\ncontinue\nend\n'
    if '_initialize_intermediate_files' in trial['accepted']:
        for symbol,stem in [('initialize_intermediate_files','early-options-entry'),('process_early_option','early-option-entry')]:
            entry=output/('collect-'+stem+'.gdb')
            entry.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/(stem+'.json')))).replace(repr(str(output/'native-argv.bin')),repr(str(output/(stem+'-argv.bin')))))
            if symbol!='process_early_option' or '_process_early_option' not in trial['accepted']:
                observer+='break *'+symbol+'\ncommands\nsilent\nsource '+str(entry)+'\ncontinue\nend\n'
    if model['schema']>=28:
        entry=output/'collect-main-entry.gdb'
        entry.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/'main-entry.json'))).replace(repr(str(output/'native-argv.bin')),repr(str(output/'main-entry-argv.bin'))))
        observer+='break *c1xx_main\ncommands\nsilent\nsource '+str(entry)+'\ncontinue\nend\n'
    if model['schema']>=27:
        entry=output/'collect-action-entry.gdb'
        entry.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/'action-entry.json'))).replace(repr(str(output/'native-argv.bin')),repr(str(output/'action-entry-argv.bin'))))
        change=''
        if trial.get('test_action_seed'):
            change='python\nowned=int(gdb.parse_and_eval("(unsigned int)frontend_bss.saved_state"))\n'
            for name,offset in [('deferred_action_head',4),('deferred_action_tail',8)]:change+='value=gdb.parse_and_eval("frontend_bss.'+name+'")\nvalue.assign(gdb.Value(owned+%d).cast(value.type))\n'%offset
            change+='end\nset variable frontend_bss.deferred_action_depth=17\n'
        observer+='break *initialize_frontend_41\ncommands\nsilent\n'+change+'source '+str(entry)+'\ncontinue\nend\n'
        result=output/'collect-action-result.gdb'
        result.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/'action-result.json'))).replace(repr(str(output/'native-argv.bin')),repr(str(output/'action-result-argv.bin'))))
        observer+='break *initialize_frontend_42\ncommands\nsilent\nsource '+str(result)+'\ncontinue\nend\n'
    if model['schema']>=26:
        entry=output/'collect-comment-entry.gdb'
        entry.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/'comment-entry.json'))).replace(repr(str(output/'native-argv.bin')),repr(str(output/'comment-entry-argv.bin'))))
        change=''
        if trial.get('test_comment_seed'):
            change='set variable frontend_bss.com_initialized=37\nset variable frontend_bss.comment_value_offset=23\nset variable frontend_bss.generated_comment_length=29\npython\ngdb.selected_inferior().write_memory(int(gdb.parse_and_eval("(unsigned int)&frontend_bss.generated_comment")),bytes([165])*74)\nend\n'
        observer+='break *initialize_generated_comment\ncommands\nsilent\n'+change+'source '+str(entry)+'\ncontinue\nend\n'
        result=output/'collect-comment-result.gdb'
        result.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/'comment-result.json'))).replace(repr(str(output/'native-argv.bin')),repr(str(output/'comment-result-argv.bin'))))
        observer+='break *register_cleanup if *(unsigned int *)($esp+4)==(unsigned int)&destroy_generated_comment\ncommands\nsilent\nsource '+str(result)+'\ncontinue\nend\n'
    if model['schema']>=25:
        entry=output/'collect-group-entry.gdb'
        entry.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/'group-entry.json'))).replace(repr(str(output/'native-argv.bin')),repr(str(output/'group-entry-argv.bin'))))
        change=''
        if trial.get('test_group_seed'):
            change='python\nowned=int(gdb.parse_and_eval("(unsigned int)frontend_bss.saved_state"))\n'
            for name,offset in [('pending_group_head',4),('active_group',8)]:change+='value=gdb.parse_and_eval("frontend_bss.'+name+'")\nvalue.assign(gdb.Value(owned+%d).cast(value.type))\n'%offset
            change+='end\n'
        observer+='break *initialize_frontend_38\ncommands\nsilent\n'+change+'source '+str(entry)+'\ncontinue\nend\n'
        result=output/'collect-group-result.gdb'
        result.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/'group-result.json'))).replace(repr(str(output/'native-argv.bin')),repr(str(output/'group-result-argv.bin'))))
        observer+='break *initialize_frontend_39\ncommands\nsilent\nsource '+str(result)+'\ncontinue\nend\n'
    if model['schema']>=24:
        entry=output/'collect-pch-entry.gdb' 
        entry.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/'pch-entry.json'))).replace(repr(str(output/'native-argv.bin')),repr(str(output/'pch-entry-argv.bin'))))
        change=''
        if trial.get('test_pch_seed'):
            change='python\nowned=int(gdb.parse_and_eval("(unsigned int)frontend_bss.saved_state"))\n'
            offset=0
            for name,kind in model['pch_projection']['fields']:
                if name in ('image_count','stored_file_position','file_offset','loaded_image_count'):expression=str({'image_count':17,'stored_file_position':23,'file_offset':-11,'loaded_image_count':29}[name])
                else:offset+=4;expression='owned+%d'%offset
                change+='value=gdb.parse_and_eval("frontend_bss.pch_context.'+name+'")\nvalue.assign(gdb.Value('+expression+').cast(value.type))\n'
            change+='end\n'
        ret_observer=''
        if 'PchContext::Initialize' in trial['accepted']:
            result=output/'collect-pch-return.gdb'
            result.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/'pch-return.json'))).replace(repr(str(output/'native-argv.bin')),repr(str(output/'pch-return-argv.bin'))))
            ret_script=output/'observe-pch-return.gdb'
            ret_script.write_text('python\nimport gdb\nclass PchReturnObserver(gdb.Breakpoint):\n    def stop(self):\n        gdb.execute("source "+'+repr(str(result))+')\n        return False\npch_return_bp=PchReturnObserver("*0x%x"%int(gdb.parse_and_eval("*(unsigned int *)$esp")),temporary=True,internal=True)\nend\n')
            ret_observer='source '+str(ret_script)+'\n'
        observer+='break *_ZN10PchContext10InitializeEv\ncommands\nsilent\n'+change+'source '+str(entry)+'\n'+ret_observer+'continue\nend\n'
    if trial.get('validation_observers'):
        phase=output/'collect-phase.gdb'
        phase.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/'phase.json'))).replace(repr(str(output/'native-argv.bin')),repr(str(output/'phase-argv.bin'))))
        lines=(project/'src/frontend.c').read_text().splitlines()
        line=lines.index('    frontend_data.first_invocation = 0;')+1
        observer+='break '+str(project/'src/frontend.c')+':'+str(line)+'\ncommands\nsilent\nsource '+str(phase)+'\ncontinue\nend\n'
    auxiliary=''
    if trial.get('test_seed')=='first-copy-aux':
        auxiliary_collector=output/'collect-aux-call.gdb'
        auxiliary_collector.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/'aux-call.json'))).replace(repr(str(output/'native-argv.bin')),repr(str(output/'aux-call-argv.bin'))))
        auxiliary='bt\nsource '+str(auxiliary_collector)+'\nreturn (void *)c1xx_aux_new_payload\npython\nimport gdb,json\nopen('+repr(str(output/'aux-return.json'))+',"w").write(json.dumps({"eax":int(gdb.parse_and_eval("$eax")),"pc":int(gdb.parse_and_eval("$pc")),"role":"mock-return","helper_accepted":False})+"\\n")\nend\ncontinue\n'
    allocation_finish=''
    if trial.get('allocation_observers'):
        from allocation_probe import setup as setup_allocation
        allocation_observer,allocation_finish=setup_allocation(output,binary,collector,allow_arena=bool(trial.get('scaler_observers')))
        observer+=allocation_observer
    switch_finish=''
    if trial.get('early_switch_observers'):
        from early_switch_probe import setup as setup_switches
        switch_observer,switch_finish=setup_switches(output)
        observer+=switch_observer
    system_finish=''
    if trial.get('system_info_observers'):
        from system_info_probe import setup as setup_system_info
        system_observer,system_finish=setup_system_info(output,collector)
        observer+=system_observer
    scaler_finish=''
    if trial.get('scaler_observers'):
        from scaler_probe import setup as setup_scaler
        scaler_observer,scaler_finish=setup_scaler(output,binary,collector,model,trial)
        observer+=scaler_observer
    from collector_cache import prepare as prepare_collector_cache
    cache_startup=prepare_collector_cache(output,collector)
    def timed_source(match):
        line=match.group(0)
        return 'python\nrecon_source_started=time.perf_counter()\nend\n'+line+'\npython\nrecon_source_timings.append({"command":'+repr(line)+',"seconds":time.perf_counter()-recon_source_started})\nend'
    observer=re.sub(r'^source [^\n]+$',timed_source,observer,flags=re.M)
    timing_start='python\nimport time,json\nrecon_collect_timings=[]\nrecon_source_timings=[]\nrecon_first_command=time.perf_counter()\nend\n'
    timing_before_run='python\nrecon_before_run=time.perf_counter()\nend\n'
    timing_after_run='python\nrecon_after_run=time.perf_counter()\nend\n'
    timing_finish='python\nopen('+repr(str(output/'gdb-timing.json'))+',"w").write(json.dumps({"first_command":recon_first_command,"before_run":recon_before_run,"after_run":recon_after_run,"collectors":recon_collect_timings,"source_commands":recon_source_timings},indent=2)+"\\n")\nend\n'
    commands.write_text(timing_start+cache_startup+'set pagination off\nset confirm off\nset exec-wrapper '+str(wrapper)+'\n'+observer+timing_before_run+'run > '+str(output/'stdout.bin')+' 2> '+str(output/'stderr.bin')+'\n'+timing_after_run+auxiliary+'bt\nsource '+str(collector)+'\n'+allocation_finish+switch_finish+system_finish+scaler_finish+timing_finish+'quit\n')
    started=time.perf_counter()
    result=subprocess.run(['gdb','--batch','-x',str(commands),'--args',str(binary)],cwd=output,env=environment,
                          stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=15)
    elapsed=time.perf_counter()-started
    (output/'gdb.txt').write_bytes(result.stdout)
    if (output/'gdb-timing.json').exists():
        timing=json.loads((output/'gdb-timing.json').read_text());collect=sum(row['collection_seconds'] for row in timing['collectors']);serialize=sum(row['serialization_io_seconds'] for row in timing['collectors'])
        timing['summary']={'source_command_seconds':sum(row['seconds'] for row in timing['source_commands']),'source_command_count':len(timing['source_commands']),'gdb_total_seconds':elapsed,'startup_seconds':timing['first_command']-started,'breakpoint_setup_seconds':timing['before_run']-timing['first_command'],'collection_seconds':collect,'serialization_io_seconds':serialize,'residual_inferior_debugger_seconds':elapsed-(timing['before_run']-started)-collect-serialize,'collector_count':len(timing['collectors']),'scope':'residual includes inferior execution, breakpoint dispatch and GDB teardown; not standalone fixture execution'}
        (output/'gdb-timing.json').write_text(json.dumps(timing,indent=2)+'\n')
    if result.returncode or b'Program received signal SIGTRAP' not in result.stdout:
        raise ValueError('native pass has no actual INT3 frontier; '+result.stdout.decode(errors='replace'))
    return result,elapsed
