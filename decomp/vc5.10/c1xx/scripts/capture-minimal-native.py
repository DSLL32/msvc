"""Linux-only real DLL-pass frontier capture, called by capture-native.py."""
import json
import os
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
    environment.pop('C1XX_TEST_INIT_RESTORE',None)
    if trial.get('test_seed'):environment['C1XX_TEST_INIT_RESTORE']=trial['test_seed']
    wrapper=output/'exec-wrapper.py'
    wrapper.write_text('#!/usr/bin/python3\nimport os,sys\nfd=os.open('+repr(str(output/'trace.txt'))+',os.O_WRONLY|os.O_CREAT|os.O_TRUNC,0o600)\nos.set_inheritable(fd,True)\nos.environ.update(RECON_TRACE_FD=str(fd),RECON_TRACE_FILTER='+repr(str(output/'filter.txt'))+',RECON_TRACE_SEEN='+repr(str(output/'seen.txt'))+')\nos.execv(sys.argv[1],sys.argv[1:])\n')
    wrapper.chmod(0o755)
    collector=output/'collect.gdb'
    collector.write_text('''python
import gdb,json,struct
frame=gdb.newest_frame()
value={'symbol':frame.name(),'pc':int(gdb.parse_and_eval('$pc')),
       'control_word':int(gdb.parse_and_eval('$fctrl')),'args':{}}
block=frame.block()
while block is not None and not block.is_global and not block.is_static:
    for symbol in block:
        if symbol.is_argument: value['args'][symbol.name]=int(frame.read_var(symbol))
    block=block.superblock
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
open(@@ARGV@@,'wb').write(raw)
value['invocation']={'argc':argc,'mode':mode,'argv_address':int(argv),'argv':strings}
host=frame
while host is not None and host.name()!='main':host=host.older()
if host is None:raise RuntimeError('no actual native DLL host frame')
spec=host.read_var('spec')
value['registration']={key:spec[key].string() for key in ('name','path','sha256')}
value['registration']['flags']=int(spec['flags']);value['registered_token']=int(host.read_var('module'))
open(@@STATE@@,'w').write(json.dumps(value,indent=2)+'\\n')
end
'''.replace('@@ARGV@@',repr(str(output/'native-argv.bin'))).replace('@@STATE@@',repr(str(output/'state.json'))))
    commands=output/'commands.gdb'
    pre=output/'collect-pre.gdb'
    pre.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/'pre.json'))).replace(repr(str(output/'native-argv.bin')),repr(str(output/'pre-argv.bin'))))
    observer='break *initialize_compiler_pass\ncommands\nsilent\nsource '+str(pre)+'\ncontinue\nend\n'
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
    if trial['trial']=='_initialize_frontend_21':
        signature=output/'collect-signature-entry.gdb'
        signature.write_text(collector.read_text().replace(repr(str(output/'state.json')),repr(str(output/'signature-entry.json'))).replace(repr(str(output/'native-argv.bin')),repr(str(output/'signature-entry-argv.bin'))))
        observer+='break *initialize_frontend_21\ncommands\nsilent\nsource '+str(signature)+'\ncontinue\nend\n'
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
    commands.write_text('set pagination off\nset confirm off\nset exec-wrapper '+str(wrapper)+'\n'+observer+'run > '+str(output/'stdout.bin')+' 2> '+str(output/'stderr.bin')+'\n'+auxiliary+'bt\nsource '+str(collector)+'\nquit\n')
    started=time.perf_counter()
    result=subprocess.run(['gdb','--batch','-x',str(commands),'--args',str(binary)],cwd=output,env=environment,
                          stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=15)
    elapsed=time.perf_counter()-started
    (output/'gdb.txt').write_bytes(result.stdout)
    if result.returncode or b'Program received signal SIGTRAP' not in result.stdout:
        raise ValueError('native pass has no actual INT3 frontier; '+result.stdout.decode(errors='replace'))
    return result,elapsed
