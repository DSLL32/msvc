set pagination off
set confirm off
python
import gdb,json
allocations=[]
class CompilerAllocation(gdb.Breakpoint):
    def stop(self):
        allocations.append({'size':int(gdb.parse_and_eval('size')),'pc':int(gdb.parse_and_eval('$pc'))})
        return False
CompilerAllocation('allocate_compiler_memory')
end
break initialize_compiler_pass
run
python
import gdb,json
def capture_modeled():
    return {
 'live_addresses':{'initialized':int(gdb.parse_and_eval('(unsigned int)&c1_data')),'bss':int(gdb.parse_and_eval('(unsigned int)&c1_bss'))},
 'initialized':{'first_call':int(gdb.parse_and_eval('c1_data.first_call'))},
 'bss':{'saved_data':int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data')),
        'cleanup_count':int(gdb.parse_and_eval('c1_bss.cleanup_count')),
        'cleanup_callbacks':[int(gdb.parse_and_eval('(unsigned int)c1_bss.cleanup_callbacks[%d]'%i)) for i in range(32)],
        'context':{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.context.'+key)) for key in ('object','value','saved_frame')},
        'current_context':{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.current_context.'+key)) for key in ('object','value','saved_frame')},
        'contexts':[{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.contexts[%d].'%i+key)) for key in ('object','value','saved_frame')} for i in range(6)],
        'output_buffers':[{key:int(gdb.parse_and_eval(('(int)' if key=='file_descriptor' else '(unsigned int)')+'c1_bss.output_buffers[%d].'%i+key)) for key in ('data','cursor','capacity','relocated','relocation_context','stream','good','mode','file_descriptor')} for i in range(7)]},
 'snapshot':({'first_call':int(gdb.parse_and_eval('c1_bss.saved_data->first_call'))} if int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data')) else None),
 'reuse_state':int(gdb.parse_and_eval('c1_reuse_state')),
 'pass_mode':int(gdb.parse_and_eval('c1_pass_mode')),
 'initialization_callbacks':[int(gdb.parse_and_eval('(unsigned int)c1_initialization_callbacks[%d]'%i)) for i in range(34)],
 'native_layout':{'initialized_size':int(gdb.parse_and_eval('sizeof(c1_data)')),'bss_size':int(gdb.parse_and_eval('sizeof(c1_bss)')),
 'first_call':int(gdb.parse_and_eval('(char*)&c1_data.first_call-(char*)&c1_data')),
 'saved_data':int(gdb.parse_and_eval('(char*)&c1_bss.saved_data-(char*)&c1_bss')),
 'cleanup_count':int(gdb.parse_and_eval('(char*)&c1_bss.cleanup_count-(char*)&c1_bss')),
 'cleanup_callbacks':int(gdb.parse_and_eval('(char*)&c1_bss.cleanup_callbacks-(char*)&c1_bss')),
 'context':int(gdb.parse_and_eval('(char*)&c1_bss.context-(char*)&c1_bss')),
 'current_context':int(gdb.parse_and_eval('(char*)&c1_bss.current_context-(char*)&c1_bss')),
 'contexts':int(gdb.parse_and_eval('(char*)&c1_bss.contexts-(char*)&c1_bss')),
 'contexts_stride':int(gdb.parse_and_eval('sizeof(c1_bss.contexts[0])')),
 'output_buffers':int(gdb.parse_and_eval('(char*)&c1_bss.output_buffers-(char*)&c1_bss')),
 'output_buffer_count':int(gdb.parse_and_eval('sizeof(c1_bss.output_buffers)/sizeof(c1_bss.output_buffers[0])')),
 'output_buffer_size':int(gdb.parse_and_eval('sizeof(c1_bss.output_buffers[0])')),
 'buffer_members':{key:int(gdb.parse_and_eval('(char*)&c1_bss.output_buffers[0].'+key+'-(char*)&c1_bss.output_buffers[0]')) for key in ('data','cursor','capacity','relocated','relocation_context','stream','good','mode','file_descriptor')},
 'context_members':{key:int(gdb.parse_and_eval('(char*)&c1_bss.context.'+key+'-(char*)&c1_bss.context')) for key in ('object','value','saved_frame')}}}
def seed_output_buffer(index):
    prefix='c1_bss.output_buffers['+str(index)+'].'
    start='((unsigned char *)c1_bss.saved_data + '+str(index*16)+')'
    gdb.execute('set var '+prefix+'data = '+start)
    gdb.execute('set var '+prefix+'cursor = '+start+' + 4')
    gdb.execute('set var '+prefix+'capacity = '+str(47+index))
    gdb.execute('set var '+prefix+'relocated = (C1BufferRelocator)initialize_state_12365')
    gdb.execute('set var '+prefix+'relocation_context = (void *)'+start)
    gdb.execute('set var '+prefix+'stream = (void *)('+start+' + 8)')
    gdb.execute('set var '+prefix+'good = 9')
    gdb.execute('set var '+prefix+'mode = '+str(165+index))
    gdb.execute('set var '+prefix+'file_descriptor = '+str(57+index))
restore_observations={'pre_seed':capture_modeled(),'buffer_constructors':{}}
gdb.execute('set var c1_bss.saved_data = (CompilerInitializedState *)malloc(6548)')
gdb.execute('set var c1_bss.saved_data->first_call = 0x13579bdf')
gdb.execute('set var c1_data.first_call = 0')
gdb.execute('set var c1_bss.cleanup_count = 7')
gdb.execute('set var c1_bss.cleanup_callbacks[0] = initialize_state_12365')
gdb.execute('set var c1_bss.cleanup_callbacks[1] = initialize_state_12377')
gdb.execute('set var c1_bss.context.object = &c1_data.first_call')
gdb.execute('set var c1_bss.context.value = 23')
gdb.execute('set var c1_bss.context.saved_frame = (void *)c1_bss.saved_data')
gdb.execute('set var c1_bss.current_context.object = &c1_data.first_call')
gdb.execute('set var c1_bss.current_context.value = 29')
gdb.execute('set var c1_bss.current_context.saved_frame = (void *)c1_bss.saved_data')
for i in range(6):
    gdb.execute('set var c1_bss.contexts[%d].object = &c1_data.first_call'%i)
    gdb.execute('set var c1_bss.contexts[%d].value = %d'%(i,31+i))
    gdb.execute('set var c1_bss.contexts[%d].saved_frame = (void *)c1_bss.saved_data'%i)
for i in range(7): seed_output_buffer(i)
gdb.execute('set var c1_pass_mode = 1')
gdb.execute('set var c1_reuse_state = 9')
restore_observations['before']=capture_modeled()
end
break /root/msvc2/decomp/vc5.10/c1/src/pass.c:22
continue
python
restore_observations['post_restore']=capture_modeled()
restore_observations['post_restore_pc']=int(gdb.parse_and_eval('$pc'))
restore_observations['post_restore_line']=gdb.newest_frame().find_sal().line
end
break initialize_state_12365
continue
python
gdb.execute('set var c1_bss.context.object = &c1_data.first_call')
gdb.execute('set var c1_bss.context.value = 23')
gdb.execute('set var c1_bss.context.saved_frame = (void *)c1_bss.saved_data')
restore_observations['before_callback']=capture_modeled()
restore_observations['callback_pc']=int(gdb.parse_and_eval('$pc'))
end
break initialize_state_12377
continue
python
gdb.execute('set var c1_bss.current_context.object = &c1_data.first_call')
gdb.execute('set var c1_bss.current_context.value = 29')
gdb.execute('set var c1_bss.current_context.saved_frame = (void *)c1_bss.saved_data')
restore_observations['before_current_callback']=capture_modeled()
restore_observations['current_callback_pc']=int(gdb.parse_and_eval('$pc'))
end
break initialize_state_12389
continue
python
for i in range(6):
    gdb.execute('set var c1_bss.contexts[%d].object = &c1_data.first_call'%i)
    gdb.execute('set var c1_bss.contexts[%d].value = %d'%(i,31+i))
    gdb.execute('set var c1_bss.contexts[%d].saved_frame = (void *)c1_bss.saved_data'%i)
restore_observations['before_contexts_callback']=capture_modeled()
restore_observations['contexts_callback_pc']=int(gdb.parse_and_eval('$pc'))
end
break initialize_output_buffer_123ae
continue
python
seed_output_buffer(0)
restore_observations['buffer_constructors']['74670']={'before':capture_modeled(),'pc':int(gdb.parse_and_eval('$pc'))}
end
break initialize_output_buffer_12423
continue
python
seed_output_buffer(1)
restore_observations['buffer_constructors']['74787']={'before':capture_modeled(),'pc':int(gdb.parse_and_eval('$pc'))}
end
break initialize_output_buffer_12472
continue
python
seed_output_buffer(2)
restore_observations['buffer_constructors']['74866']={'before':capture_modeled(),'pc':int(gdb.parse_and_eval('$pc'))}
end
break initialize_output_buffer_124c1
continue
python
seed_output_buffer(3)
restore_observations['buffer_constructors']['74945']={'before':capture_modeled(),'pc':int(gdb.parse_and_eval('$pc'))}
end
break initialize_output_buffer_12510
continue
python
seed_output_buffer(4)
restore_observations['buffer_constructors']['75024']={'before':capture_modeled(),'pc':int(gdb.parse_and_eval('$pc'))}
end
break initialize_output_buffer_1255f
continue
python
seed_output_buffer(5)
restore_observations['buffer_constructors']['75103']={'before':capture_modeled(),'pc':int(gdb.parse_and_eval('$pc'))}
end
break initialize_output_buffer_125ae
continue
python
seed_output_buffer(6)
restore_observations['buffer_constructors']['75182']={'before':capture_modeled(),'pc':int(gdb.parse_and_eval('$pc'))}
end
python
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-buffer-constructor-125ae-aux-001/restore-observations.json','w').write(json.dumps(restore_observations,indent=2)+'\n')
end
continue
bt
python
import gdb,json
frame=gdb.newest_frame()
args={name:int(gdb.parse_and_eval('(unsigned int)'+name)) for name in ()}
value={'symbol':frame.name(),'pc':int(gdb.parse_and_eval('$pc')),'args':args}
if frame.name()=='InvokeCompilerPass':
    value['argv']=[gdb.parse_and_eval('argv[%d]'%i).string() for i in range(args['argc'])]
    value['argv_terminator']=int(gdb.parse_and_eval('(unsigned int)argv[%d]'%args['argc']))
ancestor=frame
while ancestor and ancestor.name()!='InvokeCompilerPass': ancestor=ancestor.older()
value['invocation']=None
if ancestor:
    callargs={name:int(ancestor.read_var(name).cast(gdb.lookup_type('unsigned int'))) for name in ('argc','argv','mode')}
    value['invocation']={'symbol':ancestor.name(),'args':callargs,
      'argv':[ancestor.read_var('argv')[i].string() for i in range(callargs['argc'])],
      'argv_terminator':int(ancestor.read_var('argv')[callargs['argc']].cast(gdb.lookup_type('unsigned int')))}
caller=frame.older()
value['caller']={'symbol':caller.name(),'args':{}}
if caller.name()=='InvokeCompilerPass':
    value['caller']['args']={name:int(caller.read_var(name).cast(gdb.lookup_type('unsigned int'))) for name in ('argc','argv','mode')}
    value['caller']['argv']=[caller.read_var('argv')[i].string() for i in range(value['caller']['args']['argc'])]
    value['caller']['argv_terminator']=int(caller.read_var('argv')[value['caller']['args']['argc']].cast(gdb.lookup_type('unsigned int')))
if frame.name()=='recon_initterm':
    value['caller']['args']={name:int(caller.read_var(name).cast(gdb.lookup_type('unsigned int'))) for name in ('module','reason','reserved')}
identity=gdb.parse_and_eval('*c1_host_identity')
value['module_identity']={key:identity[key].string() for key in ('name','path','sha256')}
value['module_identity']['flags']=int(identity['flags'])
value['module_token']=int(gdb.parse_and_eval('(unsigned int)c1_host_module'))
value['compiler_state']={
 'live_addresses':{'initialized':int(gdb.parse_and_eval('(unsigned int)&c1_data')),'bss':int(gdb.parse_and_eval('(unsigned int)&c1_bss'))},
 'initialized':{'first_call':int(gdb.parse_and_eval('c1_data.first_call'))},
 'bss':{'saved_data':int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data')),
        'cleanup_count':int(gdb.parse_and_eval('c1_bss.cleanup_count')),
        'cleanup_callbacks':[int(gdb.parse_and_eval('(unsigned int)c1_bss.cleanup_callbacks[%d]'%i)) for i in range(32)],
        'context':{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.context.'+key)) for key in ('object','value','saved_frame')},
        'current_context':{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.current_context.'+key)) for key in ('object','value','saved_frame')},
        'contexts':[{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.contexts[%d].'%i+key)) for key in ('object','value','saved_frame')} for i in range(6)],
        'output_buffers':[{key:int(gdb.parse_and_eval(('(int)' if key=='file_descriptor' else '(unsigned int)')+'c1_bss.output_buffers[%d].'%i+key)) for key in ('data','cursor','capacity','relocated','relocation_context','stream','good','mode','file_descriptor')} for i in range(7)]},
 'snapshot':({'first_call':int(gdb.parse_and_eval('c1_bss.saved_data->first_call'))} if int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data')) else None),
 'reuse_state':int(gdb.parse_and_eval('c1_reuse_state')),
 'pass_mode':int(gdb.parse_and_eval('c1_pass_mode')),
 'initialization_callbacks':[int(gdb.parse_and_eval('(unsigned int)c1_initialization_callbacks[%d]'%i)) for i in range(34)],
 'native_layout':{'initialized_size':int(gdb.parse_and_eval('sizeof(c1_data)')),'bss_size':int(gdb.parse_and_eval('sizeof(c1_bss)')),
 'first_call':int(gdb.parse_and_eval('(char*)&c1_data.first_call-(char*)&c1_data')),
 'saved_data':int(gdb.parse_and_eval('(char*)&c1_bss.saved_data-(char*)&c1_bss')),
 'cleanup_count':int(gdb.parse_and_eval('(char*)&c1_bss.cleanup_count-(char*)&c1_bss')),
 'cleanup_callbacks':int(gdb.parse_and_eval('(char*)&c1_bss.cleanup_callbacks-(char*)&c1_bss')),
 'context':int(gdb.parse_and_eval('(char*)&c1_bss.context-(char*)&c1_bss')),
 'current_context':int(gdb.parse_and_eval('(char*)&c1_bss.current_context-(char*)&c1_bss')),
 'contexts':int(gdb.parse_and_eval('(char*)&c1_bss.contexts-(char*)&c1_bss')),
 'contexts_stride':int(gdb.parse_and_eval('sizeof(c1_bss.contexts[0])')),
 'output_buffers':int(gdb.parse_and_eval('(char*)&c1_bss.output_buffers-(char*)&c1_bss')),
 'output_buffer_count':int(gdb.parse_and_eval('sizeof(c1_bss.output_buffers)/sizeof(c1_bss.output_buffers[0])')),
 'output_buffer_size':int(gdb.parse_and_eval('sizeof(c1_bss.output_buffers[0])')),
 'buffer_members':{key:int(gdb.parse_and_eval('(char*)&c1_bss.output_buffers[0].'+key+'-(char*)&c1_bss.output_buffers[0]')) for key in ('data','cursor','capacity','relocated','relocation_context','stream','good','mode','file_descriptor')},
 'context_members':{key:int(gdb.parse_and_eval('(char*)&c1_bss.context.'+key+'-(char*)&c1_bss.context')) for key in ('object','value','saved_frame')}}}
value['pass_mode']=int(gdb.parse_and_eval('c1_pass_mode'))
value['crt_attached']=int(gdb.parse_and_eval('c1_crt_attached'))
value['raw_dllmain']=int(gdb.parse_and_eval('(unsigned int)c1_raw_dllmain'))
value['win32_last_error']=int(gdb.parse_and_eval('last_error'))
value['x87_control']=int(gdb.parse_and_eval('$fctrl'))
value['onexit_begin']=int(gdb.parse_and_eval('(unsigned int)c1_onexit_begin'))
value['onexit_end']=int(gdb.parse_and_eval('(unsigned int)c1_onexit_end'))
value['onexit_first']=int(gdb.parse_and_eval('(unsigned int)*c1_onexit_begin')) if value['onexit_begin'] else None
value['crt_adjust_fdiv']=int(gdb.parse_and_eval('c1_crt_adjust_fdiv'))
value['dll_instance']=int(gdb.parse_and_eval('(unsigned int)c1_dll_instance'))
value['initializer_begin']=int(gdb.parse_and_eval('(unsigned int)&c1_initializers[0]'))
value['initializer_end']=int(gdb.parse_and_eval('(unsigned int)&c1_initializers[1]'))
value['initializer_slots']=[int(gdb.parse_and_eval('(unsigned int)c1_initializers[0]'))]
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-buffer-constructor-125ae-aux-001/allocation-observations.json','w').write(json.dumps(allocations,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-buffer-constructor-125ae-aux-001/state.json','w').write(json.dumps(value,indent=2)+'\n')
end
quit
