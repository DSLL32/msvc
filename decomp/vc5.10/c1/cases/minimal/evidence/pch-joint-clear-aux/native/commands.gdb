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
import struct
pool_entries=[]
pool_returns=[]
class PoolReturn(gdb.Breakpoint):
    def stop(self):
        entry=pool_entries[-1]
        words=struct.unpack("<4I",gdb.selected_inferior().read_memory(entry["ecx"],16).tobytes())
        pool_returns.append({"pc":int(gdb.parse_and_eval("$pc")),"eax":int(gdb.parse_and_eval("$eax")),"esp":int(gdb.parse_and_eval("$esp")),"values":list(words)})
        self.enabled = False
        return False
class PoolEntry(gdb.Breakpoint):
    def stop(self):
        esp=int(gdb.parse_and_eval('$esp'))
        words=struct.unpack('<2I',gdb.selected_inferior().read_memory(esp,8).tobytes())
        receiver=int(gdb.parse_and_eval('$ecx'))
        pool=(receiver-int(gdb.parse_and_eval('(unsigned int)&c1_bss.pools[0]')))//16
        assert 0<=pool<9
        if True:
            if True:
                saved=int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data'))
                gdb.selected_inferior().write_memory(int(gdb.parse_and_eval('$ecx')),struct.pack('<4I',saved+128+pool*16,91+pool,7+pool,saved+512+pool*16))
            PoolReturn('*0x%x'%words[0])
        block=gdb.block_for_pc(int(gdb.parse_and_eval('$pc')))
        while block and not block.function:block=block.superblock
        assert block and block.function
        formals=[{'name':symbol.name,'type':str(symbol.type)} for symbol in block if symbol.is_argument]
        assert {row['name'] for row in formals}=={'this','minimum_pages'}
        values=struct.unpack('<4I',gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('$ecx')),16).tobytes())
        pool_entries.append({'formal_inventory':formals,'index':pool,'values':list(values),'pc':int(gdb.parse_and_eval('$pc')),'ecx':int(gdb.parse_and_eval('$ecx')),'esp':esp,'return':words[0],'argument':words[1]})
        return False
PoolEntry('*0x804be10')
import struct
pch_entries=[]
pch_returns=[]
class PchReturn(gdb.Breakpoint):
    def stop(self):
        receiver=pch_entries[-1]['ecx']
        bss=int(gdb.parse_and_eval('(unsigned int)&c1_bss'))
        pch_returns.append({'pc':int(gdb.parse_and_eval('$pc')),'eax':int(gdb.parse_and_eval('$eax')),'esp':int(gdb.parse_and_eval('$esp')),'edi':int(gdb.parse_and_eval('$edi')),
          'values':list(struct.unpack('<17I',gdb.selected_inferior().read_memory(receiver,68).tobytes())),
          'bss_bytes':list(gdb.selected_inferior().read_memory(bss,int(gdb.parse_and_eval('sizeof(c1_bss)'))).tobytes())})
        self.enabled=False
        return False
class PchEntry(gdb.Breakpoint):
    def stop(self):
        esp=int(gdb.parse_and_eval('$esp'));receiver=int(gdb.parse_and_eval('$ecx'))
        returned=struct.unpack('<I',gdb.selected_inferior().read_memory(esp,4).tobytes())[0]
        if True:
            saved=int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data'))
            words=[0]*17
            for offset,value in [(0,37),(52,19),(12,0x11223344),(20,0x55667788)]:words[offset//4]=value
            for i,offset in enumerate([4,8,16,24,28,32,36,40,44,48,56,60,64]):words[offset//4]=saved+1024+i*8
            gdb.selected_inferior().write_memory(receiver,struct.pack('<17I',*words))
        if True:PchReturn('*0x%x'%returned)
        block=gdb.block_for_pc(int(gdb.parse_and_eval('$pc')))
        while block and not block.function:block=block.superblock
        assert block and block.function
        formals=[{'name':symbol.name,'type':str(symbol.type)} for symbol in block if symbol.is_argument]
        assert {row['name'] for row in formals}=={'this'}
        bss=int(gdb.parse_and_eval('(unsigned int)&c1_bss'))
        pch_entries.append({'pc':int(gdb.parse_and_eval('$pc')),'ecx':receiver,'esp':esp,'return':returned,'formal_inventory':formals,'edi':int(gdb.parse_and_eval('$edi')),
          'values':list(struct.unpack('<17I',gdb.selected_inferior().read_memory(receiver,68).tobytes())),
          'bss_bytes':list(gdb.selected_inferior().read_memory(bss,int(gdb.parse_and_eval('sizeof(c1_bss)'))).tobytes())})
        return False
PchEntry('*0x804c1d0')
end
break initialize_compiler_pass
run
python
import gdb,json
def capture_modeled():
    return {
 'pdb_context':{key:int(gdb.parse_and_eval(('(int)' if key=='read_mode' else '(unsigned int)')+'c1_pdb_context.'+key)) for key in ('database','names','types','minimal_rebuild','rebuild_state','read_mode')},
 'pdb_context_layout':{'address':int(gdb.parse_and_eval('(unsigned int)&c1_pdb_context')),'size':int(gdb.parse_and_eval('sizeof(c1_pdb_context)')),'members':{key:int(gdb.parse_and_eval('(char*)&c1_pdb_context.'+key+'-(char*)&c1_pdb_context')) for key in ('database','names','types','minimal_rebuild','rebuild_state','read_mode')}},
 'output_pdb_context':{key:int(gdb.parse_and_eval(('(int)' if key=='read_mode' else '(unsigned int)')+'c1_output_pdb_context.'+key)) for key in ('database','names','types','minimal_rebuild','rebuild_state','read_mode')},
 'output_pdb_context_layout':{'address':int(gdb.parse_and_eval('(unsigned int)&c1_output_pdb_context')),'size':int(gdb.parse_and_eval('sizeof(c1_output_pdb_context)')),'members':{key:int(gdb.parse_and_eval('(char*)&c1_output_pdb_context.'+key+'-(char*)&c1_output_pdb_context')) for key in ('database','names','types','minimal_rebuild','rebuild_state','read_mode')}},
 'live_addresses':{'initialized':int(gdb.parse_and_eval('(unsigned int)&c1_data')),'bss':int(gdb.parse_and_eval('(unsigned int)&c1_bss'))},
 'initialized':{'first_call':int(gdb.parse_and_eval('c1_data.first_call'))},
 'bss':{'pch_state':{key:([int(gdb.parse_and_eval('(unsigned int)c1_bss.pch_state.mapping_handles[%d]'%i)) for i in range(6)] if key=='mapping_handles' else int(gdb.parse_and_eval(('(int)' if key in ('image_count','loaded_image_count') else '(unsigned int)')+'c1_bss.pch_state.'+key))) for key in ('image_count', 'root_image', 'images', 'stored_file_position', 'file_name', 'file_offset', 'file', 'mapping_handles', 'loaded_image_count', 'scratch_arena', 'original_pages', 'mapped_pages')},'empty_record_prototype':[int(gdb.parse_and_eval('c1_bss.empty_record_prototype[%d]'%i)) for i in range(20)],'pools':[{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.pools[%d].'%i+key)) for key in ('cursor','remaining','minimum_pages','arena')} for i in range(9)],'pdb_signature':int(gdb.parse_and_eval('c1_bss.pdb_signature')),'literal':int(gdb.parse_and_eval('c1_bss.literal.integer')),'saved_literal':int(gdb.parse_and_eval('c1_bss.saved_literal.integer')),'saved_literal_available':int(gdb.parse_and_eval('c1_bss.saved_literal_available')),'saved_data':int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data')),
        'cleanup_count':int(gdb.parse_and_eval('c1_bss.cleanup_count')),
        'cleanup_callbacks':[int(gdb.parse_and_eval('(unsigned int)c1_bss.cleanup_callbacks[%d]'%i)) for i in range(32)],
        'context':{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.context.'+key)) for key in ('object','value','saved_frame')},
        'current_context':{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.current_context.'+key)) for key in ('object','value','saved_frame')},
        'contexts':[{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.contexts[%d].'%i+key)) for key in ('object','value','saved_frame')} for i in range(6)],
        'output_buffers':[{key:int(gdb.parse_and_eval(('(int)' if key=='file_descriptor' else '(unsigned int)')+'c1_bss.output_buffers[%d].'%i+key)) for key in ('data','cursor','capacity','relocated','relocation_context','stream','good','mode','file_descriptor')} for i in range(8)]},
 'snapshot':({'first_call':int(gdb.parse_and_eval('c1_bss.saved_data->first_call'))} if int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data')) else None),
 'reuse_state':int(gdb.parse_and_eval('c1_reuse_state')),
 'pass_mode':int(gdb.parse_and_eval('c1_pass_mode')),
 'initialization_callbacks':[int(gdb.parse_and_eval('(unsigned int)c1_initialization_callbacks[%d]'%i)) for i in range(34)],
 'native_layout':{'pch_state':int(gdb.parse_and_eval('(char*)&c1_bss.pch_state-(char*)&c1_bss')),'pch_state_size':int(gdb.parse_and_eval('sizeof(c1_bss.pch_state)')),'mapping_handle_count':int(gdb.parse_and_eval('sizeof(c1_bss.pch_state.mapping_handles)/sizeof(c1_bss.pch_state.mapping_handles[0])')),'pch_members':{key:int(gdb.parse_and_eval('(char*)&c1_bss.pch_state.'+key+'-(char*)&c1_bss.pch_state')) for key in ('image_count', 'root_image', 'images', 'stored_file_position', 'file_name', 'file_offset', 'file', 'mapping_handles', 'loaded_image_count', 'scratch_arena', 'original_pages', 'mapped_pages')},'empty_record_prototype':int(gdb.parse_and_eval('(char*)&c1_bss.empty_record_prototype-(char*)&c1_bss')),'empty_record_prototype_size':int(gdb.parse_and_eval('sizeof(c1_bss.empty_record_prototype)')),'initialized_size':int(gdb.parse_and_eval('sizeof(c1_data)')),'bss_size':int(gdb.parse_and_eval('sizeof(c1_bss)')),
 'pools':int(gdb.parse_and_eval('(char*)&c1_bss.pools-(char*)&c1_bss')),'pool_count':int(gdb.parse_and_eval('sizeof(c1_bss.pools)/sizeof(c1_bss.pools[0])')),'pool_size':int(gdb.parse_and_eval('sizeof(c1_bss.pools[0])')),'pool_members':{key:int(gdb.parse_and_eval('(char*)&c1_bss.pools[0].'+key+'-(char*)&c1_bss.pools[0]')) for key in ('cursor','remaining','minimum_pages','arena')},
 'pdb_signature':int(gdb.parse_and_eval('(char*)&c1_bss.pdb_signature-(char*)&c1_bss')),
 'literal':int(gdb.parse_and_eval('(char*)&c1_bss.literal-(char*)&c1_bss')),'saved_literal':int(gdb.parse_and_eval('(char*)&c1_bss.saved_literal-(char*)&c1_bss')),'saved_literal_available':int(gdb.parse_and_eval('(char*)&c1_bss.saved_literal_available-(char*)&c1_bss')),'value_size':int(gdb.parse_and_eval('sizeof(c1_bss.literal)')),
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
for i in range(8): seed_output_buffer(i)
for i in range(20):gdb.execute('set var c1_bss.empty_record_prototype[%d] = %d'%(i,1+i))
for key,value in [('image_count',37),('loaded_image_count',19),('stored_file_position',0x11223344),('file_offset',0x55667788)]:gdb.execute('set var c1_bss.pch_state.'+key+' = '+str(value))
pch_pointers=['root_image','images','file_name','file']+['mapping_handles[%d]'%i for i in range(6)]+['scratch_arena','original_pages','mapped_pages']
for i,key in enumerate(pch_pointers):gdb.execute('set var c1_bss.pch_state.'+key+' = (void *)((char *)c1_bss.saved_data + '+str(1024+i*8)+')')
gdb.execute('set var c1_bss.literal.integer = -154024284667738249')
gdb.execute('set var c1_bss.saved_literal.integer = 3694171492931080482')
gdb.execute('set var c1_bss.saved_literal_available = 165')
gdb.execute('set var c1_bss.pdb_signature = 0x13572468')
for i,key in enumerate(('database','names','types','minimal_rebuild','rebuild_state')):
    gdb.execute('set var c1_pdb_context.'+key+' = (void *)((char *)c1_bss.saved_data + '+str(i*8)+')')
gdb.execute('set var c1_pdb_context.read_mode = 37')
for i,key in enumerate(('database','names','types','minimal_rebuild','rebuild_state')):
    gdb.execute('set var c1_output_pdb_context.'+key+' = (void *)((char *)c1_bss.saved_data + '+str(64+i*8)+')')
gdb.execute('set var c1_output_pdb_context.read_mode = 37')
for i in range(9):
    prefix='c1_bss.pools['+str(i)+'].'
    gdb.execute('set var '+prefix+'cursor = (unsigned char *)c1_bss.saved_data + '+str(128+i*16))
    gdb.execute('set var '+prefix+'remaining = '+str(91+i))
    gdb.execute('set var '+prefix+'minimum_pages = '+str(7+i))
    gdb.execute('set var '+prefix+'arena = (void *)((char *)c1_bss.saved_data + '+str(512+i*16)+')')
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
break initialize_output_buffer_125fd
continue
python
seed_output_buffer(7)
restore_observations['buffer_constructors']['75261']={'before':capture_modeled(),'pc':int(gdb.parse_and_eval('$pc'))}
end
break initialize_state_12646
continue
python
gdb.execute('set var c1_bss.literal.integer = 1234605616436508552')
gdb.execute('set var c1_bss.saved_literal.integer = 6153737367135073092')
gdb.execute('set var c1_bss.saved_literal_available = 165')
restore_observations['before_value_copy']=capture_modeled()
restore_observations['value_copy_pc']=int(gdb.parse_and_eval('$pc'))
end
break initialize_state_12663
continue
python
restore_observations['after_value_copy']=capture_modeled()
restore_observations['after_value_copy_pc']=int(gdb.parse_and_eval('$pc'))
end
break initialize_state_12664
continue
python
gdb.execute('set var c1_bss.pdb_signature = 0x24681357')
restore_observations['before_signature']=capture_modeled()
restore_observations['signature_pc']=int(gdb.parse_and_eval('$pc'))
end
break initialize_state_1266f
continue
python
restore_observations['after_signature']=capture_modeled()
end
break initialize_pdb_context_12676
continue
python
gdb.execute('set var c1_reuse_state = 0')
restore_observations['before_pdb_context']=capture_modeled()
restore_observations['pdb_context_pc']=int(gdb.parse_and_eval('$pc'))
end
break initialize_state_126af
continue
python
restore_observations['after_pdb_context']=capture_modeled()
restore_observations['after_pdb_context_pc']=int(gdb.parse_and_eval('$pc'))
end
break initialize_pdb_context_126b6
continue
python
restore_observations['before_output_pdb_context']=capture_modeled()
restore_observations['output_pdb_context_pc']=int(gdb.parse_and_eval('$pc'))
end
break initialize_state_126ef
continue
python
restore_observations['after_output_pdb_context']=capture_modeled()
restore_observations['after_output_pdb_context_pc']=int(gdb.parse_and_eval('$pc'))
end
break initialize_state_1277b
continue
python
for i in range(20):gdb.execute('set var c1_bss.empty_record_prototype[%d] = %d'%(i,0xa0+i))
restore_observations['before_prototype']=capture_modeled()
restore_observations['prototype_pc']=int(gdb.parse_and_eval('$pc'))
end
python
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-pch-joint-clear-aux-001/restore-observations.json','w').write(json.dumps(restore_observations,indent=2)+'\n')
end
continue
bt
python
import gdb,json
frame=gdb.newest_frame()
args={name:int(gdb.parse_and_eval('(unsigned int)'+name)) for name in ()}
block=frame.block()
while block and not block.function:block=block.superblock
assert block and block.function
formals=[{'name':symbol.name,'type':str(symbol.type)} for symbol in block if symbol.is_argument]
value={'symbol':frame.name(),'pc':int(gdb.parse_and_eval('$pc')),'args':args,'formal_inventory':formals}
assert {row['name'] for row in formals}==set(args), 'omitted or invented native formal'

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
 'pdb_context':{key:int(gdb.parse_and_eval(('(int)' if key=='read_mode' else '(unsigned int)')+'c1_pdb_context.'+key)) for key in ('database','names','types','minimal_rebuild','rebuild_state','read_mode')},
 'pdb_context_layout':{'address':int(gdb.parse_and_eval('(unsigned int)&c1_pdb_context')),'size':int(gdb.parse_and_eval('sizeof(c1_pdb_context)')),'members':{key:int(gdb.parse_and_eval('(char*)&c1_pdb_context.'+key+'-(char*)&c1_pdb_context')) for key in ('database','names','types','minimal_rebuild','rebuild_state','read_mode')}},
 'output_pdb_context':{key:int(gdb.parse_and_eval(('(int)' if key=='read_mode' else '(unsigned int)')+'c1_output_pdb_context.'+key)) for key in ('database','names','types','minimal_rebuild','rebuild_state','read_mode')},
 'output_pdb_context_layout':{'address':int(gdb.parse_and_eval('(unsigned int)&c1_output_pdb_context')),'size':int(gdb.parse_and_eval('sizeof(c1_output_pdb_context)')),'members':{key:int(gdb.parse_and_eval('(char*)&c1_output_pdb_context.'+key+'-(char*)&c1_output_pdb_context')) for key in ('database','names','types','minimal_rebuild','rebuild_state','read_mode')}},
 'live_addresses':{'initialized':int(gdb.parse_and_eval('(unsigned int)&c1_data')),'bss':int(gdb.parse_and_eval('(unsigned int)&c1_bss'))},
 'initialized':{'first_call':int(gdb.parse_and_eval('c1_data.first_call'))},
 'bss':{'pch_state':{key:([int(gdb.parse_and_eval('(unsigned int)c1_bss.pch_state.mapping_handles[%d]'%i)) for i in range(6)] if key=='mapping_handles' else int(gdb.parse_and_eval(('(int)' if key in ('image_count','loaded_image_count') else '(unsigned int)')+'c1_bss.pch_state.'+key))) for key in ('image_count', 'root_image', 'images', 'stored_file_position', 'file_name', 'file_offset', 'file', 'mapping_handles', 'loaded_image_count', 'scratch_arena', 'original_pages', 'mapped_pages')},'empty_record_prototype':[int(gdb.parse_and_eval('c1_bss.empty_record_prototype[%d]'%i)) for i in range(20)],'pools':[{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.pools[%d].'%i+key)) for key in ('cursor','remaining','minimum_pages','arena')} for i in range(9)],'pdb_signature':int(gdb.parse_and_eval('c1_bss.pdb_signature')),'literal':int(gdb.parse_and_eval('c1_bss.literal.integer')),'saved_literal':int(gdb.parse_and_eval('c1_bss.saved_literal.integer')),'saved_literal_available':int(gdb.parse_and_eval('c1_bss.saved_literal_available')),'saved_data':int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data')),
        'cleanup_count':int(gdb.parse_and_eval('c1_bss.cleanup_count')),
        'cleanup_callbacks':[int(gdb.parse_and_eval('(unsigned int)c1_bss.cleanup_callbacks[%d]'%i)) for i in range(32)],
        'context':{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.context.'+key)) for key in ('object','value','saved_frame')},
        'current_context':{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.current_context.'+key)) for key in ('object','value','saved_frame')},
        'contexts':[{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.contexts[%d].'%i+key)) for key in ('object','value','saved_frame')} for i in range(6)],
        'output_buffers':[{key:int(gdb.parse_and_eval(('(int)' if key=='file_descriptor' else '(unsigned int)')+'c1_bss.output_buffers[%d].'%i+key)) for key in ('data','cursor','capacity','relocated','relocation_context','stream','good','mode','file_descriptor')} for i in range(8)]},
 'snapshot':({'first_call':int(gdb.parse_and_eval('c1_bss.saved_data->first_call'))} if int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data')) else None),
 'reuse_state':int(gdb.parse_and_eval('c1_reuse_state')),
 'pass_mode':int(gdb.parse_and_eval('c1_pass_mode')),
 'initialization_callbacks':[int(gdb.parse_and_eval('(unsigned int)c1_initialization_callbacks[%d]'%i)) for i in range(34)],
 'native_layout':{'pch_state':int(gdb.parse_and_eval('(char*)&c1_bss.pch_state-(char*)&c1_bss')),'pch_state_size':int(gdb.parse_and_eval('sizeof(c1_bss.pch_state)')),'mapping_handle_count':int(gdb.parse_and_eval('sizeof(c1_bss.pch_state.mapping_handles)/sizeof(c1_bss.pch_state.mapping_handles[0])')),'pch_members':{key:int(gdb.parse_and_eval('(char*)&c1_bss.pch_state.'+key+'-(char*)&c1_bss.pch_state')) for key in ('image_count', 'root_image', 'images', 'stored_file_position', 'file_name', 'file_offset', 'file', 'mapping_handles', 'loaded_image_count', 'scratch_arena', 'original_pages', 'mapped_pages')},'empty_record_prototype':int(gdb.parse_and_eval('(char*)&c1_bss.empty_record_prototype-(char*)&c1_bss')),'empty_record_prototype_size':int(gdb.parse_and_eval('sizeof(c1_bss.empty_record_prototype)')),'initialized_size':int(gdb.parse_and_eval('sizeof(c1_data)')),'bss_size':int(gdb.parse_and_eval('sizeof(c1_bss)')),
 'pools':int(gdb.parse_and_eval('(char*)&c1_bss.pools-(char*)&c1_bss')),'pool_count':int(gdb.parse_and_eval('sizeof(c1_bss.pools)/sizeof(c1_bss.pools[0])')),'pool_size':int(gdb.parse_and_eval('sizeof(c1_bss.pools[0])')),'pool_members':{key:int(gdb.parse_and_eval('(char*)&c1_bss.pools[0].'+key+'-(char*)&c1_bss.pools[0]')) for key in ('cursor','remaining','minimum_pages','arena')},
 'pdb_signature':int(gdb.parse_and_eval('(char*)&c1_bss.pdb_signature-(char*)&c1_bss')),
 'literal':int(gdb.parse_and_eval('(char*)&c1_bss.literal-(char*)&c1_bss')),'saved_literal':int(gdb.parse_and_eval('(char*)&c1_bss.saved_literal-(char*)&c1_bss')),'saved_literal_available':int(gdb.parse_and_eval('(char*)&c1_bss.saved_literal_available-(char*)&c1_bss')),'value_size':int(gdb.parse_and_eval('sizeof(c1_bss.literal)')),
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
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-pch-joint-clear-aux-001/pool-entry-observations.json','w').write(json.dumps(pool_entries,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-pch-joint-clear-aux-001/pool-return-observations.json','w').write(json.dumps(pool_returns,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-pch-joint-clear-aux-001/pch-entry-observations.json','w').write(json.dumps(pch_entries,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-pch-joint-clear-aux-001/pch-return-observations.json','w').write(json.dumps(pch_returns,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-pch-joint-clear-aux-001/allocation-observations.json','w').write(json.dumps(allocations,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-pch-joint-clear-aux-001/state.json','w').write(json.dumps(value,indent=2)+'\n')
end
quit
