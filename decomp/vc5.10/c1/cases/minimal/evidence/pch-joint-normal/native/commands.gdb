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
            if False:
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
        if False:
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
run
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
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-pch-joint-002/pool-entry-observations.json','w').write(json.dumps(pool_entries,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-pch-joint-002/pool-return-observations.json','w').write(json.dumps(pool_returns,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-pch-joint-002/pch-entry-observations.json','w').write(json.dumps(pch_entries,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-pch-joint-002/pch-return-observations.json','w').write(json.dumps(pch_returns,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-pch-joint-002/allocation-observations.json','w').write(json.dumps(allocations,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-pch-joint-002/state.json','w').write(json.dumps(value,indent=2)+'\n')
end
quit
