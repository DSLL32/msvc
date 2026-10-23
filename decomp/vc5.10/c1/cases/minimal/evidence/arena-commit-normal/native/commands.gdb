set pagination off
set confirm off
python
import gdb,json
def capture_modeled():
    return {
 'pdb_context':{key:int(gdb.parse_and_eval(('(int)' if key=='read_mode' else '(unsigned int)')+'c1_pdb_context.'+key)) for key in ('database','names','types','minimal_rebuild','rebuild_state','read_mode')},
 'pdb_context_layout':{'address':int(gdb.parse_and_eval('(unsigned int)&c1_pdb_context')),'size':int(gdb.parse_and_eval('sizeof(c1_pdb_context)')),'members':{key:int(gdb.parse_and_eval('(char*)&c1_pdb_context.'+key+'-(char*)&c1_pdb_context')) for key in ('database','names','types','minimal_rebuild','rebuild_state','read_mode')}},
 'output_pdb_context':{key:int(gdb.parse_and_eval(('(int)' if key=='read_mode' else '(unsigned int)')+'c1_output_pdb_context.'+key)) for key in ('database','names','types','minimal_rebuild','rebuild_state','read_mode')},
 'output_pdb_context_layout':{'address':int(gdb.parse_and_eval('(unsigned int)&c1_output_pdb_context')),'size':int(gdb.parse_and_eval('sizeof(c1_output_pdb_context)')),'members':{key:int(gdb.parse_and_eval('(char*)&c1_output_pdb_context.'+key+'-(char*)&c1_output_pdb_context')) for key in ('database','names','types','minimal_rebuild','rebuild_state','read_mode')}},
 'live_addresses':{'initialized':int(gdb.parse_and_eval('(unsigned int)&c1_data')),'bss':int(gdb.parse_and_eval('(unsigned int)&c1_bss'))},
 'initialized':{'first_call':int(gdb.parse_and_eval('c1_data.first_call')),'compiler_phase':int(gdb.parse_and_eval('c1_data.compiler_phase')),'c9_il':int(gdb.parse_and_eval('c1_data.c9_il')),'compiler_flags':int(gdb.parse_and_eval('(unsigned int)c1_data.compiler_flags')),'system_info':{key:int(gdb.parse_and_eval('(unsigned int)c1_data.system_info.'+key)) for key in ('dwOemId', 'dwPageSize', 'lpMinimumApplicationAddress', 'lpMaximumApplicationAddress', 'dwActiveProcessorMask', 'dwNumberOfProcessors', 'dwProcessorType', 'dwAllocationGranularity', 'wProcessorLevel', 'wProcessorRevision')},'internal_bm_flags':int(gdb.parse_and_eval('c1_data.internal_bm_flags')),'internal_zb_value':int(gdb.parse_and_eval('c1_data.internal_zb_value')),'memory_percent':int(gdb.parse_and_eval('c1_data.memory_percent'))},
 'bss':{'arenas':[{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.arenas[%d].'%i+key))for key in ('cursor','committed_limit','base','committed_bytes','descriptor','reserve_bytes','reset_value','mapped_view')}for i in range(6)],'work_buffers':[{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.work_buffers[%d].'%i+key)) for key in ('data','limit')} for i in range(2)],'debug_break':int(gdb.parse_and_eval('c1_bss.debug_break')),'command_argc':int(gdb.parse_and_eval('c1_bss.command_argc')),'command_argv':int(gdb.parse_and_eval('(unsigned int)c1_bss.command_argv')),'pch_state':{key:([int(gdb.parse_and_eval('(unsigned int)c1_bss.pch_state.mapping_handles[%d]'%i)) for i in range(6)] if key=='mapping_handles' else int(gdb.parse_and_eval(('(int)' if key in ('image_count','loaded_image_count') else '(unsigned int)')+'c1_bss.pch_state.'+key))) for key in ('image_count', 'root_image', 'images', 'stored_file_position', 'file_name', 'file_offset', 'file', 'mapping_handles', 'loaded_image_count', 'scratch_arena', 'original_pages', 'mapped_pages')},'empty_record_prototype':[int(gdb.parse_and_eval('c1_bss.empty_record_prototype[%d]'%i)) for i in range(20)],'pools':[{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.pools[%d].'%i+key)) for key in ('cursor','remaining','minimum_pages','arena')} for i in range(9)],'pdb_signature':int(gdb.parse_and_eval('c1_bss.pdb_signature')),'literal':int(gdb.parse_and_eval('c1_bss.literal.integer')),'saved_literal':int(gdb.parse_and_eval('c1_bss.saved_literal.integer')),'saved_literal_available':int(gdb.parse_and_eval('c1_bss.saved_literal_available')),'saved_data':int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data')),
        'cleanup_count':int(gdb.parse_and_eval('c1_bss.cleanup_count')),
        'cleanup_callbacks':[int(gdb.parse_and_eval('(unsigned int)c1_bss.cleanup_callbacks[%d]'%i)) for i in range(32)],
        'context':{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.context.'+key)) for key in ('object','value','saved_frame')},
        'current_context':{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.current_context.'+key)) for key in ('object','value','saved_frame')},
        'contexts':[{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.contexts[%d].'%i+key)) for key in ('object','value','saved_frame')} for i in range(6)],
        'output_buffers':[{key:int(gdb.parse_and_eval(('(int)' if key=='file_descriptor' else '(unsigned int)')+'c1_bss.output_buffers[%d].'%i+key)) for key in ('data','cursor','capacity','relocated','relocation_context','stream','good','mode','file_descriptor')} for i in range(8)]},
 'arena_descriptors':[{key:int(gdb.parse_and_eval('c1_arena_descriptors[%d].'%i+key))for key in ('kind','reserve_bytes','commit_unit','flags','alignment')}for i in range(6)],
 'snapshot':({'first_call':int(gdb.parse_and_eval('c1_bss.saved_data->first_call')),'compiler_phase':int(gdb.parse_and_eval('c1_bss.saved_data->compiler_phase')),'c9_il':int(gdb.parse_and_eval('c1_bss.saved_data->c9_il')),'compiler_flags':int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data->compiler_flags')),'system_info':{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data->system_info.'+key)) for key in ('dwOemId', 'dwPageSize', 'lpMinimumApplicationAddress', 'lpMaximumApplicationAddress', 'dwActiveProcessorMask', 'dwNumberOfProcessors', 'dwProcessorType', 'dwAllocationGranularity', 'wProcessorLevel', 'wProcessorRevision')},'internal_bm_flags':int(gdb.parse_and_eval('c1_bss.saved_data->internal_bm_flags')),'internal_zb_value':int(gdb.parse_and_eval('c1_bss.saved_data->internal_zb_value')),'memory_percent':int(gdb.parse_and_eval('c1_bss.saved_data->memory_percent'))} if int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data')) else None),
 'reuse_state':int(gdb.parse_and_eval('c1_reuse_state')),
 'pass_mode':int(gdb.parse_and_eval('c1_pass_mode')),
 'initialization_callbacks':[int(gdb.parse_and_eval('(unsigned int)c1_initialization_callbacks[%d]'%i)) for i in range(34)],
 'native_layout':{'arenas':int(gdb.parse_and_eval('(char*)&c1_bss.arenas-(char*)&c1_bss')),'arena_count':6,'arena_size':int(gdb.parse_and_eval('sizeof(c1_bss.arenas[0])')),'arena_members':{key:int(gdb.parse_and_eval('(char*)&c1_bss.arenas[0].'+key+'-(char*)&c1_bss.arenas[0]'))for key in ('cursor','committed_limit','base','committed_bytes','descriptor','reserve_bytes','reset_value','mapped_view')},'arena_descriptor_size':int(gdb.parse_and_eval('sizeof(c1_arena_descriptors[0])')),'arena_descriptor_members':{key:int(gdb.parse_and_eval('(char*)&c1_arena_descriptors[0].'+key+'-(char*)&c1_arena_descriptors[0]'))for key in ('kind','reserve_bytes','commit_unit','flags','alignment')},'work_buffers':int(gdb.parse_and_eval('(char*)&c1_bss.work_buffers-(char*)&c1_bss')),'work_buffer_count':2,'work_buffer_size':int(gdb.parse_and_eval('sizeof(c1_bss.work_buffers[0])')),'work_buffer_members':{key:int(gdb.parse_and_eval('(char*)&c1_bss.work_buffers[0].'+key+'-(char*)&c1_bss.work_buffers[0]')) for key in ('data','limit')},'initialized_members':{key:int(gdb.parse_and_eval('(char*)&c1_data.'+key+'-(char*)&c1_data')) for key in ('first_call','compiler_phase','c9_il','compiler_flags','system_info','internal_bm_flags','internal_zb_value','memory_percent')},'system_info_size':int(gdb.parse_and_eval('sizeof(c1_data.system_info)')),'system_info_members':{key:int(gdb.parse_and_eval('(char*)&c1_data.system_info.'+key+'-(char*)&c1_data.system_info')) for key in ('dwOemId', 'dwPageSize', 'lpMinimumApplicationAddress', 'lpMaximumApplicationAddress', 'dwActiveProcessorMask', 'dwNumberOfProcessors', 'dwProcessorType', 'dwAllocationGranularity', 'wProcessorLevel', 'wProcessorRevision')},'command_members':{key:int(gdb.parse_and_eval('(char*)&c1_bss.'+key+'-(char*)&c1_bss')) for key in ('debug_break','command_argc','command_argv')},'pch_state':int(gdb.parse_and_eval('(char*)&c1_bss.pch_state-(char*)&c1_bss')),'pch_state_size':int(gdb.parse_and_eval('sizeof(c1_bss.pch_state)')),'mapping_handle_count':int(gdb.parse_and_eval('sizeof(c1_bss.pch_state.mapping_handles)/sizeof(c1_bss.pch_state.mapping_handles[0])')),'pch_members':{key:int(gdb.parse_and_eval('(char*)&c1_bss.pch_state.'+key+'-(char*)&c1_bss.pch_state')) for key in ('image_count', 'root_image', 'images', 'stored_file_position', 'file_name', 'file_offset', 'file', 'mapping_handles', 'loaded_image_count', 'scratch_arena', 'original_pages', 'mapped_pages')},'empty_record_prototype':int(gdb.parse_and_eval('(char*)&c1_bss.empty_record_prototype-(char*)&c1_bss')),'empty_record_prototype_size':int(gdb.parse_and_eval('sizeof(c1_bss.empty_record_prototype)')),'initialized_size':int(gdb.parse_and_eval('sizeof(c1_data)')),'bss_size':int(gdb.parse_and_eval('sizeof(c1_bss)')),
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
PchEntry('*0x804c440')
main_entries=[]
class ActualMainEntry(gdb.Breakpoint):
    def stop(self):
        esp=int(gdb.parse_and_eval('$esp'))
        words=list(struct.unpack('<3I',gdb.selected_inferior().read_memory(esp,12).tobytes()))
        block=gdb.block_for_pc(int(gdb.parse_and_eval('$pc')))
        while block and not block.function:block=block.superblock
        assert block and block.function
        formals=[{'name':symbol.name,'type':str(symbol.type)} for symbol in block if symbol.is_argument]
        assert {row['name'] for row in formals}=={'argc','argv'}
        assert words[1]<=256
        vector=gdb.Value(words[2]).cast(gdb.lookup_type('char').pointer().pointer())
        actual_argv=[vector[i].string() for i in range(words[1])]
        terminator=int(vector[words[1]].cast(gdb.lookup_type('unsigned int')))
        crt_state={'win32_last_error':int(gdb.parse_and_eval('last_error')),'x87_control':int(gdb.parse_and_eval('$fctrl')),
          'crt_attached':int(gdb.parse_and_eval('c1_crt_attached')),'raw_dllmain':int(gdb.parse_and_eval('(unsigned int)c1_raw_dllmain')),
          'onexit_begin':int(gdb.parse_and_eval('(unsigned int)c1_onexit_begin')),'onexit_end':int(gdb.parse_and_eval('(unsigned int)c1_onexit_end')),
          'onexit_first':int(gdb.parse_and_eval('(unsigned int)*c1_onexit_begin')),'crt_adjust_fdiv':int(gdb.parse_and_eval('c1_crt_adjust_fdiv')),
          'dll_instance':int(gdb.parse_and_eval('(unsigned int)c1_dll_instance')),'module_token':int(gdb.parse_and_eval('(unsigned int)c1_host_module'))}
        main_entries.append({'crt_state':crt_state,'argv':actual_argv,'argv_terminator':terminator,'boundary_rva':0x127a7,'symbol':block.function.name,'pc':int(gdb.parse_and_eval('$pc')),'esp':esp,'return':words[0],
          'args':dict(argc=words[1],argv=words[2]),'formal_inventory':formals,'compiler_state':capture_modeled()})
        return False
ActualMainEntry('*0x804c550')
arena_member_entries=[]
arena_member_returns=[]
class ActualArenaMemberReturn(gdb.Breakpoint):
    def __init__(self,address,index):
        super(ActualArenaMemberReturn,self).__init__('*0x%x'%address);self.index=index
    def stop(self):
        arena_member_returns.append({'index':self.index,'pc':int(gdb.parse_and_eval('(unsigned int)$pc')),'esp':int(gdb.parse_and_eval('(unsigned int)$esp')),'eax':int(gdb.parse_and_eval('(unsigned int)$eax')),'compiler_state':capture_modeled()});self.enabled=False;return False
class ActualArenaMemberEntry(gdb.Breakpoint):
    def stop(self):
        esp=int(gdb.parse_and_eval('(unsigned int)$esp'));pc=int(gdb.parse_and_eval('(unsigned int)$pc'))
        words=list(struct.unpack('<3I',gdb.selected_inferior().read_memory(esp,12).tobytes()))
        block=gdb.block_for_pc(pc)
        while block and not block.function:block=block.superblock
        assert block and block.function
        formals=[{'name':symbol.name,'type':str(symbol.type)}for symbol in block if symbol.is_argument]
        frame=gdb.newest_frame();caller=frame.older();ancestor=frame
        while ancestor and ancestor.name()!='InvokeCompilerPass':ancestor=ancestor.older()
        assert ancestor
        actual={key:int(ancestor.read_var(key).cast(gdb.lookup_type('unsigned int')))for key in ('argc','argv','mode')}
        invocation={'symbol':ancestor.name(),'args':actual,'argv':[ancestor.read_var('argv')[i].string()for i in range(actual['argc'])],
          'argv_pointers':[int(ancestor.read_var('argv')[i].cast(gdb.lookup_type('unsigned int')))for i in range(actual['argc'])],
          'argv_terminator':int(ancestor.read_var('argv')[actual['argc']].cast(gdb.lookup_type('unsigned int')))}
        crt_state={'win32_last_error':int(gdb.parse_and_eval('last_error')),'x87_control':int(gdb.parse_and_eval('$fctrl')),
          'crt_attached':int(gdb.parse_and_eval('c1_crt_attached')),'raw_dllmain':int(gdb.parse_and_eval('(unsigned int)c1_raw_dllmain')),
          'onexit_begin':int(gdb.parse_and_eval('(unsigned int)c1_onexit_begin')),'onexit_end':int(gdb.parse_and_eval('(unsigned int)c1_onexit_end')),
          'onexit_first':int(gdb.parse_and_eval('(unsigned int)*c1_onexit_begin')),'crt_adjust_fdiv':int(gdb.parse_and_eval('c1_crt_adjust_fdiv')),
          'dll_instance':int(gdb.parse_and_eval('(unsigned int)c1_dll_instance')),'module_token':int(gdb.parse_and_eval('(unsigned int)c1_host_module'))}
        arena_member_entries.append({'symbol':frame.name(),'invocation':invocation,'crt_state':crt_state,'caller':{'symbol':caller.name(),'pc':int(caller.pc()),'args':{}},
          'args':{'this':int(gdb.parse_and_eval('(unsigned int)$ecx')),'descriptor':words[1],'address':words[2]},
          'pc':pc,'ecx':int(gdb.parse_and_eval('(unsigned int)$ecx')),'esp':esp,'return':words[0],
          'descriptor':words[1],'address':words[2],'formal_inventory':formals,'compiler_state':capture_modeled()})
        ActualArenaMemberReturn(words[0],len(arena_member_entries)-1)
        return False
ActualArenaMemberEntry('*0x804cd00')
pool_cycle_events=[]
pool_cycle_returns=[]
pool_commit_api_entries=[]
pool_commit_api_returns=[]
def pool_cycle_preserved():
    return {name:int(gdb.parse_and_eval('(unsigned int)$'+name))for name in ('ebx','esi','edi','ebp')}
class PoolCycleReturn(gdb.Breakpoint):
    def __init__(self,kind,index,address,entry_esp):
        super(PoolCycleReturn,self).__init__('*0x%x'%address);self.kind=kind;self.index=index;self.entry_esp=entry_esp
        pool_cycle_events.append({'kind':'enter','function':kind,'index':index})
    def stop(self):
        pool_cycle_events.append({'kind':'return','function':self.kind,'index':self.index})
        pool_cycle_returns.append({'kind':self.kind,'index':self.index,'pc':int(gdb.parse_and_eval('(unsigned int)$pc')),
          'esp':int(gdb.parse_and_eval('(unsigned int)$esp')),'entry_esp':self.entry_esp,'eax':int(gdb.parse_and_eval('(unsigned int)$eax')),
          'last_error':int(gdb.parse_and_eval('last_error')),'preserved':pool_cycle_preserved(),'fp':capture_x87(),'compiler_state':capture_modeled()})
        self.enabled=False;return False
class PoolCommitApiReturn(gdb.Breakpoint):
    def __init__(self,address,index,args):
        super(PoolCommitApiReturn,self).__init__('*0x%x'%address);self.index=index;self.args=args
    def stop(self):
        views=[];view=gdb.parse_and_eval('allocations')
        while int(view):
            item=view.dereference();views.append({'address':int(item['address']),'length':int(item['length'])});view=item['next']
        pool_cycle_events.append({'kind':'return','function':'commit-api','index':self.index})
        pool_commit_api_returns.append({'index':self.index,'pc':int(gdb.parse_and_eval('(unsigned int)$pc')),
          'esp':int(gdb.parse_and_eval('(unsigned int)$esp')),'eax':int(gdb.parse_and_eval('(unsigned int)$eax')),
          'last_error':int(gdb.parse_and_eval('last_error')),'preserved':pool_cycle_preserved(),'fp':capture_x87(),
          'compiler_state':capture_modeled(),'maps':open('/proc/%d/maps'%gdb.selected_inferior().pid).read(),'allocations':views})
        open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/pool-committed-000.bin','wb').write(gdb.selected_inferior().read_memory(self.args[0],self.args[1]).tobytes())
        self.enabled=False;return False
class PoolCommitApiEntry(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame();caller=frame.older()
        if caller.name()!='CompilerArena::CommitExtent':return False
        esp=int(gdb.parse_and_eval('(unsigned int)$esp'));words=list(struct.unpack('<5I',gdb.selected_inferior().read_memory(esp,20).tobytes()))
        block=gdb.block_for_pc(int(gdb.parse_and_eval('(unsigned int)$pc')))
        while block and not block.function:block=block.superblock
        pool_cycle_events.append({'kind':'enter','function':'commit-api','index':len(pool_commit_api_entries)})
        pool_commit_api_entries.append({'pc':int(gdb.parse_and_eval('(unsigned int)$pc')),'esp':esp,'return':words[0],'args':words[1:],
          'formal_inventory':[{'name':symbol.name,'type':str(symbol.type)}for symbol in block if symbol.is_argument],
          'caller':{'symbol':caller.name(),'pc':int(caller.pc()),'args':{'this':int(caller.read_var('this').cast(gdb.lookup_type('unsigned int')))}},
          'last_error':int(gdb.parse_and_eval('last_error')),'preserved':pool_cycle_preserved(),'fp':capture_x87(),'compiler_state':capture_modeled()})
        PoolCommitApiReturn(words[0],len(pool_commit_api_entries)-1,words[1:]);return False
PoolCommitApiEntry("*0x805f470")
default_pool_entries=[]
class ActualDefaultPoolEntry(gdb.Breakpoint):
    def stop(self):
        esp=int(gdb.parse_and_eval('$esp'))
        words=list(struct.unpack('<I',gdb.selected_inferior().read_memory(esp,4).tobytes()))
        block=gdb.block_for_pc(int(gdb.parse_and_eval('$pc')))
        while block and not block.function:block=block.superblock
        assert block and block.function
        formals=[{'name':symbol.name,'type':str(symbol.type)} for symbol in block if symbol.is_argument]
        assert formals==[]
        crt_state={'win32_last_error':int(gdb.parse_and_eval('last_error')),'x87_control':int(gdb.parse_and_eval('$fctrl')),
          'crt_attached':int(gdb.parse_and_eval('c1_crt_attached')),'raw_dllmain':int(gdb.parse_and_eval('(unsigned int)c1_raw_dllmain')),
          'onexit_begin':int(gdb.parse_and_eval('(unsigned int)c1_onexit_begin')),'onexit_end':int(gdb.parse_and_eval('(unsigned int)c1_onexit_end')),
          'onexit_first':int(gdb.parse_and_eval('(unsigned int)*c1_onexit_begin')),'crt_adjust_fdiv':int(gdb.parse_and_eval('c1_crt_adjust_fdiv')),
          'dll_instance':int(gdb.parse_and_eval('(unsigned int)c1_dll_instance')),'module_token':int(gdb.parse_and_eval('(unsigned int)c1_host_module'))}
        frame=gdb.newest_frame();caller=frame.older()
        ancestor=frame
        while ancestor and ancestor.name()!='InvokeCompilerPass':ancestor=ancestor.older()
        assert ancestor
        actual={key:int(ancestor.read_var(key).cast(gdb.lookup_type('unsigned int')))for key in ('argc','argv','mode')}
        invocation={'symbol':ancestor.name(),'args':actual,'argv':[ancestor.read_var('argv')[i].string()for i in range(actual['argc'])],
          'argv_pointers':[int(ancestor.read_var('argv')[i].cast(gdb.lookup_type('unsigned int')))for i in range(actual['argc'])],
          'argv_terminator':int(ancestor.read_var('argv')[actual['argc']].cast(gdb.lookup_type('unsigned int')))}
        default_pool_entries.append({'invocation':invocation,'crt_state':crt_state,'boundary_rva':0x12ac3,'symbol':block.function.name,'pc':int(gdb.parse_and_eval('$pc')),'esp':esp,'return':words[0],
          'args':{},'formal_inventory':formals,'compiler_state':capture_modeled(),'caller':{'symbol':caller.name(),'pc':int(caller.pc()),'args':{}}})
        return False
ActualDefaultPoolEntry('*0x804c1d0')
pool_arena_entries=[]
class ActualPoolArenaEntry(gdb.Breakpoint):
    def stop(self):
        esp=int(gdb.parse_and_eval('$esp'))
        words=list(struct.unpack('<2I',gdb.selected_inferior().read_memory(esp,8).tobytes()))
        block=gdb.block_for_pc(int(gdb.parse_and_eval('$pc')))
        while block and not block.function:block=block.superblock
        assert block and block.function
        formals=[{'name':symbol.name,'type':str(symbol.type)} for symbol in block if symbol.is_argument]
        crt_state={'win32_last_error':int(gdb.parse_and_eval('last_error')),'x87_control':int(gdb.parse_and_eval('$fctrl')),
          'crt_attached':int(gdb.parse_and_eval('c1_crt_attached')),'raw_dllmain':int(gdb.parse_and_eval('(unsigned int)c1_raw_dllmain')),
          'onexit_begin':int(gdb.parse_and_eval('(unsigned int)c1_onexit_begin')),'onexit_end':int(gdb.parse_and_eval('(unsigned int)c1_onexit_end')),
          'onexit_first':int(gdb.parse_and_eval('(unsigned int)*c1_onexit_begin')),'crt_adjust_fdiv':int(gdb.parse_and_eval('c1_crt_adjust_fdiv')),
          'dll_instance':int(gdb.parse_and_eval('(unsigned int)c1_dll_instance')),'module_token':int(gdb.parse_and_eval('(unsigned int)c1_host_module'))}
        frame=gdb.newest_frame();caller=frame.older()
        ancestor=frame
        while ancestor and ancestor.name()!='InvokeCompilerPass':ancestor=ancestor.older()
        assert ancestor
        actual={key:int(ancestor.read_var(key).cast(gdb.lookup_type('unsigned int')))for key in ('argc','argv','mode')}
        invocation={'symbol':ancestor.name(),'args':actual,'argv':[ancestor.read_var('argv')[i].string()for i in range(actual['argc'])],
          'argv_pointers':[int(ancestor.read_var('argv')[i].cast(gdb.lookup_type('unsigned int')))for i in range(actual['argc'])],
          'argv_terminator':int(ancestor.read_var('argv')[actual['argc']].cast(gdb.lookup_type('unsigned int')))}
        pool_arena_entries.append({'invocation':invocation,'crt_state':crt_state,'boundary_rva':0x12b30,'symbol':frame.name(),'pc':int(gdb.parse_and_eval('$pc')),'preserved':pool_cycle_preserved(),'fp':capture_x87(),'esp':esp,'return':words[0],
          'args':{'this':int(gdb.parse_and_eval('(unsigned int)$ecx')),'arena_index':words[1]},'ecx':int(gdb.parse_and_eval('(unsigned int)$ecx')),'arena_index':words[1],'formal_inventory':formals,'compiler_state':capture_modeled(),'caller':{'symbol':caller.name(),'pc':int(caller.pc()),'args':{}}})
        PoolCycleReturn('set-arena',len(pool_arena_entries)-1,words[0],esp)
        return False
ActualPoolArenaEntry('*0x804c330')
pool_refill_entries=[]
class ActualPoolRefillEntry(gdb.Breakpoint):
    def stop(self):
        esp=int(gdb.parse_and_eval('$esp'))
        words=list(struct.unpack('<2I',gdb.selected_inferior().read_memory(esp,8).tobytes()))
        block=gdb.block_for_pc(int(gdb.parse_and_eval('$pc')))
        while block and not block.function:block=block.superblock
        assert block and block.function
        formals=[{'name':symbol.name,'type':str(symbol.type)} for symbol in block if symbol.is_argument]
        crt_state={'win32_last_error':int(gdb.parse_and_eval('last_error')),'x87_control':int(gdb.parse_and_eval('$fctrl')),
          'crt_attached':int(gdb.parse_and_eval('c1_crt_attached')),'raw_dllmain':int(gdb.parse_and_eval('(unsigned int)c1_raw_dllmain')),
          'onexit_begin':int(gdb.parse_and_eval('(unsigned int)c1_onexit_begin')),'onexit_end':int(gdb.parse_and_eval('(unsigned int)c1_onexit_end')),
          'onexit_first':int(gdb.parse_and_eval('(unsigned int)*c1_onexit_begin')),'crt_adjust_fdiv':int(gdb.parse_and_eval('c1_crt_adjust_fdiv')),
          'dll_instance':int(gdb.parse_and_eval('(unsigned int)c1_dll_instance')),'module_token':int(gdb.parse_and_eval('(unsigned int)c1_host_module'))}
        frame=gdb.newest_frame();caller=frame.older()
        ancestor=frame
        while ancestor and ancestor.name()!='InvokeCompilerPass':ancestor=ancestor.older()
        assert ancestor
        actual={key:int(ancestor.read_var(key).cast(gdb.lookup_type('unsigned int')))for key in ('argc','argv','mode')}
        invocation={'symbol':ancestor.name(),'args':actual,'argv':[ancestor.read_var('argv')[i].string()for i in range(actual['argc'])],
          'argv_pointers':[int(ancestor.read_var('argv')[i].cast(gdb.lookup_type('unsigned int')))for i in range(actual['argc'])],
          'argv_terminator':int(ancestor.read_var('argv')[actual['argc']].cast(gdb.lookup_type('unsigned int')))}
        pool_refill_entries.append({'invocation':invocation,'crt_state':crt_state,'boundary_rva':0xaa9c,'symbol':frame.name(),'pc':int(gdb.parse_and_eval('$pc')),'preserved':pool_cycle_preserved(),'fp':capture_x87(),'esp':esp,'return':words[0],
          'args':{'this':int(gdb.parse_and_eval('(unsigned int)$ecx')),'minimum_bytes':words[1]},'ecx':int(gdb.parse_and_eval('(unsigned int)$ecx')),'minimum_bytes':words[1],'formal_inventory':formals,'compiler_state':capture_modeled(),'caller':{'symbol':caller.name(),'pc':int(caller.pc()),'args':{key:int(caller.read_var(key).cast(gdb.lookup_type('unsigned int')))for key in ('this','arena_index')}}})
        PoolCycleReturn('refill',len(pool_refill_entries)-1,words[0],esp)
        return False
ActualPoolRefillEntry('*0x804c3a0')
arena_page_entries=[]
class ActualArenaPageEntry(gdb.Breakpoint):
    def stop(self):
        esp=int(gdb.parse_and_eval('$esp'))
        words=list(struct.unpack('<3I',gdb.selected_inferior().read_memory(esp,12).tobytes()))
        block=gdb.block_for_pc(int(gdb.parse_and_eval('$pc')))
        while block and not block.function:block=block.superblock
        assert block and block.function
        formals=[{'name':symbol.name,'type':str(symbol.type)} for symbol in block if symbol.is_argument]
        crt_state={'win32_last_error':int(gdb.parse_and_eval('last_error')),'x87_control':int(gdb.parse_and_eval('$fctrl')),
          'crt_attached':int(gdb.parse_and_eval('c1_crt_attached')),'raw_dllmain':int(gdb.parse_and_eval('(unsigned int)c1_raw_dllmain')),
          'onexit_begin':int(gdb.parse_and_eval('(unsigned int)c1_onexit_begin')),'onexit_end':int(gdb.parse_and_eval('(unsigned int)c1_onexit_end')),
          'onexit_first':int(gdb.parse_and_eval('(unsigned int)*c1_onexit_begin')),'crt_adjust_fdiv':int(gdb.parse_and_eval('c1_crt_adjust_fdiv')),
          'dll_instance':int(gdb.parse_and_eval('(unsigned int)c1_dll_instance')),'module_token':int(gdb.parse_and_eval('(unsigned int)c1_host_module'))}
        frame=gdb.newest_frame();caller=frame.older()
        ancestor=frame
        while ancestor and ancestor.name()!='InvokeCompilerPass':ancestor=ancestor.older()
        assert ancestor
        actual={key:int(ancestor.read_var(key).cast(gdb.lookup_type('unsigned int')))for key in ('argc','argv','mode')}
        invocation={'symbol':ancestor.name(),'args':actual,'argv':[ancestor.read_var('argv')[i].string()for i in range(actual['argc'])],
          'argv_pointers':[int(ancestor.read_var('argv')[i].cast(gdb.lookup_type('unsigned int')))for i in range(actual['argc'])],
          'argv_terminator':int(ancestor.read_var('argv')[actual['argc']].cast(gdb.lookup_type('unsigned int')))}
        arena_page_entries.append({'invocation':invocation,'crt_state':crt_state,'boundary_rva':0xaacb,'symbol':frame.name(),'pc':int(gdb.parse_and_eval('$pc')),'preserved':pool_cycle_preserved(),'fp':capture_x87(),'esp':esp,'return':words[0],
          'args':{'this':int(gdb.parse_and_eval('(unsigned int)$ecx')),'pages':words[1],'available_bytes':words[2]},'ecx':int(gdb.parse_and_eval('(unsigned int)$ecx')),'pages':words[1],'available_bytes':words[2],'formal_inventory':formals,'compiler_state':capture_modeled(),'caller':{'symbol':caller.name(),'pc':int(caller.pc()),'args':{key:int(caller.read_var(key).cast(gdb.lookup_type('unsigned int')))for key in ('this','minimum_bytes')}}})
        PoolCycleReturn('allocate-pages',len(arena_page_entries)-1,words[0],esp)
        return False
ActualArenaPageEntry('*0x804cf00')
pool_commit_entries=[]
class ActualPoolCommitEntry(gdb.Breakpoint):
    def stop(self):
        esp=int(gdb.parse_and_eval('$esp'))
        words=list(struct.unpack('<I',gdb.selected_inferior().read_memory(esp,4).tobytes()))
        block=gdb.block_for_pc(int(gdb.parse_and_eval('$pc')))
        while block and not block.function:block=block.superblock
        assert block and block.function
        formals=[{'name':symbol.name,'type':str(symbol.type)} for symbol in block if symbol.is_argument]
        crt_state={'win32_last_error':int(gdb.parse_and_eval('last_error')),'x87_control':int(gdb.parse_and_eval('$fctrl')),
          'crt_attached':int(gdb.parse_and_eval('c1_crt_attached')),'raw_dllmain':int(gdb.parse_and_eval('(unsigned int)c1_raw_dllmain')),
          'onexit_begin':int(gdb.parse_and_eval('(unsigned int)c1_onexit_begin')),'onexit_end':int(gdb.parse_and_eval('(unsigned int)c1_onexit_end')),
          'onexit_first':int(gdb.parse_and_eval('(unsigned int)*c1_onexit_begin')),'crt_adjust_fdiv':int(gdb.parse_and_eval('c1_crt_adjust_fdiv')),
          'dll_instance':int(gdb.parse_and_eval('(unsigned int)c1_dll_instance')),'module_token':int(gdb.parse_and_eval('(unsigned int)c1_host_module'))}
        frame=gdb.newest_frame();caller=frame.older()
        ancestor=frame
        while ancestor and ancestor.name()!='InvokeCompilerPass':ancestor=ancestor.older()
        assert ancestor
        actual={key:int(ancestor.read_var(key).cast(gdb.lookup_type('unsigned int')))for key in ('argc','argv','mode')}
        invocation={'symbol':ancestor.name(),'args':actual,'argv':[ancestor.read_var('argv')[i].string()for i in range(actual['argc'])],
          'argv_pointers':[int(ancestor.read_var('argv')[i].cast(gdb.lookup_type('unsigned int')))for i in range(actual['argc'])],
          'argv_terminator':int(ancestor.read_var('argv')[actual['argc']].cast(gdb.lookup_type('unsigned int')))}
        pool_commit_entries.append({'invocation':invocation,'crt_state':crt_state,'boundary_rva':0x9883,'symbol':frame.name(),'pc':int(gdb.parse_and_eval('$pc')),'preserved':pool_cycle_preserved(),'fp':capture_x87(),'esp':esp,'return':words[0],
          'args':{'this':int(gdb.parse_and_eval('(unsigned int)$ecx'))},'ecx':int(gdb.parse_and_eval('(unsigned int)$ecx')),'formal_inventory':formals,'compiler_state':capture_modeled(),'caller':{'symbol':caller.name(),'pc':int(caller.pc()),'args':{key:int(caller.read_var(key).cast(gdb.lookup_type('unsigned int')))for key in ('this','pages','available_bytes')}}})
        PoolCycleReturn('commit',len(pool_commit_entries)-1,words[0],esp)
        return False
ActualPoolCommitEntry('*0x804cfb0')
work_entries=[]
class ActualWorkEntry(gdb.Breakpoint):
    def stop(self):
        esp=int(gdb.parse_and_eval('$esp'))
        words=list(struct.unpack('<I',gdb.selected_inferior().read_memory(esp,4).tobytes()))
        block=gdb.block_for_pc(int(gdb.parse_and_eval('$pc')))
        while block and not block.function:block=block.superblock
        assert block and block.function
        formals=[{'name':symbol.name,'type':str(symbol.type)} for symbol in block if symbol.is_argument]
        assert not formals
        crt_state={'win32_last_error':int(gdb.parse_and_eval('last_error')),'x87_control':int(gdb.parse_and_eval('$fctrl')),
          'crt_attached':int(gdb.parse_and_eval('c1_crt_attached')),'raw_dllmain':int(gdb.parse_and_eval('(unsigned int)c1_raw_dllmain')),
          'onexit_begin':int(gdb.parse_and_eval('(unsigned int)c1_onexit_begin')),'onexit_end':int(gdb.parse_and_eval('(unsigned int)c1_onexit_end')),
          'onexit_first':int(gdb.parse_and_eval('(unsigned int)*c1_onexit_begin')),'crt_adjust_fdiv':int(gdb.parse_and_eval('c1_crt_adjust_fdiv')),
          'dll_instance':int(gdb.parse_and_eval('(unsigned int)c1_dll_instance')),'module_token':int(gdb.parse_and_eval('(unsigned int)c1_host_module'))}
        frame=gdb.newest_frame();caller=frame.older()
        work_entries.append({'crt_state':crt_state,'boundary_rva':0x12830,'symbol':block.function.name,'pc':int(gdb.parse_and_eval('$pc')),'esp':esp,'return':words[0],
          'args':{},'formal_inventory':formals,'compiler_state':capture_modeled(),'caller':{'symbol':caller.name(),'pc':int(caller.pc()),'args':{key:int(caller.read_var(key).cast(gdb.lookup_type('unsigned int'))) for key in ('argc','argv')}}})
        return False
ActualWorkEntry('*0x804c740')
scanner_entries=[]
class ActualScannerEntry(gdb.Breakpoint):
    def stop(self):
        esp=int(gdb.parse_and_eval('$esp'))
        words=list(struct.unpack('<2I',gdb.selected_inferior().read_memory(esp,8).tobytes()))
        block=gdb.block_for_pc(int(gdb.parse_and_eval('$pc')))
        while block and not block.function:block=block.superblock
        assert block and block.function
        formals=[{'name':symbol.name,'type':str(symbol.type)} for symbol in block if symbol.is_argument]
        assert formals==[{'name':'arguments','type':'char **'}]
        crt_state={'win32_last_error':int(gdb.parse_and_eval('last_error')),'x87_control':int(gdb.parse_and_eval('$fctrl')),
          'crt_attached':int(gdb.parse_and_eval('c1_crt_attached')),'raw_dllmain':int(gdb.parse_and_eval('(unsigned int)c1_raw_dllmain')),
          'onexit_begin':int(gdb.parse_and_eval('(unsigned int)c1_onexit_begin')),'onexit_end':int(gdb.parse_and_eval('(unsigned int)c1_onexit_end')),
          'onexit_first':int(gdb.parse_and_eval('(unsigned int)*c1_onexit_begin')),'crt_adjust_fdiv':int(gdb.parse_and_eval('c1_crt_adjust_fdiv')),
          'dll_instance':int(gdb.parse_and_eval('(unsigned int)c1_dll_instance')),'module_token':int(gdb.parse_and_eval('(unsigned int)c1_host_module'))}
        frame=gdb.newest_frame();caller=frame.older()
        ancestor=frame
        while ancestor and ancestor.name()!='InvokeCompilerPass':ancestor=ancestor.older()
        assert ancestor
        actual={key:int(ancestor.read_var(key).cast(gdb.lookup_type('unsigned int')))for key in ('argc','argv','mode')}
        invocation={'symbol':ancestor.name(),'args':actual,'argv':[ancestor.read_var('argv')[i].string()for i in range(actual['argc'])],
          'argv_pointers':[int(ancestor.read_var('argv')[i].cast(gdb.lookup_type('unsigned int')))for i in range(actual['argc'])],
          'argv_terminator':int(ancestor.read_var('argv')[actual['argc']].cast(gdb.lookup_type('unsigned int')))}
        scanner_entries.append({'invocation':invocation,'crt_state':crt_state,'boundary_rva':0x128b1,'symbol':block.function.name,'pc':int(gdb.parse_and_eval('$pc')),'esp':esp,'return':words[0],
          'args':dict(arguments=words[1]),'formal_inventory':formals,'compiler_state':capture_modeled(),'caller':{'symbol':caller.name(),'pc':int(caller.pc()),'args':{key:int(caller.read_var(key).cast(gdb.lookup_type('unsigned int'))) for key in ('argc','argv')}}})
        return False
ActualScannerEntry('*0x804c890')
empty_entries=[]
class ActualEmptyEntry(gdb.Breakpoint):
    def stop(self):
        esp=int(gdb.parse_and_eval('$esp'))
        words=list(struct.unpack('<I',gdb.selected_inferior().read_memory(esp,4).tobytes()))
        block=gdb.block_for_pc(int(gdb.parse_and_eval('$pc')))
        while block and not block.function:block=block.superblock
        assert block and block.function
        formals=[{'name':symbol.name,'type':str(symbol.type)} for symbol in block if symbol.is_argument]
        assert formals==[]
        crt_state={'win32_last_error':int(gdb.parse_and_eval('last_error')),'x87_control':int(gdb.parse_and_eval('$fctrl')),
          'crt_attached':int(gdb.parse_and_eval('c1_crt_attached')),'raw_dllmain':int(gdb.parse_and_eval('(unsigned int)c1_raw_dllmain')),
          'onexit_begin':int(gdb.parse_and_eval('(unsigned int)c1_onexit_begin')),'onexit_end':int(gdb.parse_and_eval('(unsigned int)c1_onexit_end')),
          'onexit_first':int(gdb.parse_and_eval('(unsigned int)*c1_onexit_begin')),'crt_adjust_fdiv':int(gdb.parse_and_eval('c1_crt_adjust_fdiv')),
          'dll_instance':int(gdb.parse_and_eval('(unsigned int)c1_dll_instance')),'module_token':int(gdb.parse_and_eval('(unsigned int)c1_host_module'))}
        frame=gdb.newest_frame();caller=frame.older()
        ancestor=frame
        while ancestor and ancestor.name()!='InvokeCompilerPass':ancestor=ancestor.older()
        assert ancestor
        actual={key:int(ancestor.read_var(key).cast(gdb.lookup_type('unsigned int')))for key in ('argc','argv','mode')}
        invocation={'symbol':ancestor.name(),'args':actual,'argv':[ancestor.read_var('argv')[i].string()for i in range(actual['argc'])],
          'argv_pointers':[int(ancestor.read_var('argv')[i].cast(gdb.lookup_type('unsigned int')))for i in range(actual['argc'])],
          'argv_terminator':int(ancestor.read_var('argv')[actual['argc']].cast(gdb.lookup_type('unsigned int')))}
        empty_entries.append({'invocation':invocation,'crt_state':crt_state,'boundary_rva':0x12998,'symbol':block.function.name,'pc':int(gdb.parse_and_eval('$pc')),'esp':esp,'return':words[0],
          'args':{},'formal_inventory':formals,'compiler_state':capture_modeled(),'caller':{'symbol':caller.name(),'pc':int(caller.pc()),'args':{key:int(caller.read_var(key).cast(gdb.lookup_type('unsigned int'))) for key in ('argc','argv')}}})
        return False
ActualEmptyEntry('*0x804c680')
arena_entries=[]
class ActualArenaEntry(gdb.Breakpoint):
    def stop(self):
        esp=int(gdb.parse_and_eval('$esp'))
        words=list(struct.unpack('<I',gdb.selected_inferior().read_memory(esp,4).tobytes()))
        block=gdb.block_for_pc(int(gdb.parse_and_eval('$pc')))
        while block and not block.function:block=block.superblock
        assert block and block.function
        formals=[{'name':symbol.name,'type':str(symbol.type)} for symbol in block if symbol.is_argument]
        assert formals==[]
        crt_state={'win32_last_error':int(gdb.parse_and_eval('last_error')),'x87_control':int(gdb.parse_and_eval('$fctrl')),
          'crt_attached':int(gdb.parse_and_eval('c1_crt_attached')),'raw_dllmain':int(gdb.parse_and_eval('(unsigned int)c1_raw_dllmain')),
          'onexit_begin':int(gdb.parse_and_eval('(unsigned int)c1_onexit_begin')),'onexit_end':int(gdb.parse_and_eval('(unsigned int)c1_onexit_end')),
          'onexit_first':int(gdb.parse_and_eval('(unsigned int)*c1_onexit_begin')),'crt_adjust_fdiv':int(gdb.parse_and_eval('c1_crt_adjust_fdiv')),
          'dll_instance':int(gdb.parse_and_eval('(unsigned int)c1_dll_instance')),'module_token':int(gdb.parse_and_eval('(unsigned int)c1_host_module'))}
        frame=gdb.newest_frame();caller=frame.older()
        ancestor=frame
        while ancestor and ancestor.name()!='InvokeCompilerPass':ancestor=ancestor.older()
        assert ancestor
        actual={key:int(ancestor.read_var(key).cast(gdb.lookup_type('unsigned int')))for key in ('argc','argv','mode')}
        invocation={'symbol':ancestor.name(),'args':actual,'argv':[ancestor.read_var('argv')[i].string()for i in range(actual['argc'])],
          'argv_pointers':[int(ancestor.read_var('argv')[i].cast(gdb.lookup_type('unsigned int')))for i in range(actual['argc'])],
          'argv_terminator':int(ancestor.read_var('argv')[actual['argc']].cast(gdb.lookup_type('unsigned int')))}
        arena_entries.append({'invocation':invocation,'crt_state':crt_state,'boundary_rva':0x12999,'symbol':block.function.name,'pc':int(gdb.parse_and_eval('$pc')),'esp':esp,'return':words[0],
          'args':{},'formal_inventory':formals,'compiler_state':capture_modeled(),'caller':{'symbol':caller.name(),'pc':int(caller.pc()),'args':{key:int(caller.read_var(key).cast(gdb.lookup_type('unsigned int'))) for key in ('argc','argv')}}})
        return False
ActualArenaEntry('*0x804cb90')
primary_entries=[]
class ActualPrimaryEntry(gdb.Breakpoint):
    def stop(self):
        esp=int(gdb.parse_and_eval('$esp'))
        words=list(struct.unpack('<I',gdb.selected_inferior().read_memory(esp,4).tobytes()))
        block=gdb.block_for_pc(int(gdb.parse_and_eval('$pc')))
        while block and not block.function:block=block.superblock
        assert block and block.function
        formals=[{'name':symbol.name,'type':str(symbol.type)} for symbol in block if symbol.is_argument]
        assert formals==[]
        crt_state={'win32_last_error':int(gdb.parse_and_eval('last_error')),'x87_control':int(gdb.parse_and_eval('$fctrl')),
          'crt_attached':int(gdb.parse_and_eval('c1_crt_attached')),'raw_dllmain':int(gdb.parse_and_eval('(unsigned int)c1_raw_dllmain')),
          'onexit_begin':int(gdb.parse_and_eval('(unsigned int)c1_onexit_begin')),'onexit_end':int(gdb.parse_and_eval('(unsigned int)c1_onexit_end')),
          'onexit_first':int(gdb.parse_and_eval('(unsigned int)*c1_onexit_begin')),'crt_adjust_fdiv':int(gdb.parse_and_eval('c1_crt_adjust_fdiv')),
          'dll_instance':int(gdb.parse_and_eval('(unsigned int)c1_dll_instance')),'module_token':int(gdb.parse_and_eval('(unsigned int)c1_host_module'))}
        frame=gdb.newest_frame();caller=frame.older()
        ancestor=frame
        while ancestor and ancestor.name()!='InvokeCompilerPass':ancestor=ancestor.older()
        assert ancestor
        actual={key:int(ancestor.read_var(key).cast(gdb.lookup_type('unsigned int')))for key in ('argc','argv','mode')}
        invocation={'symbol':ancestor.name(),'args':actual,'argv':[ancestor.read_var('argv')[i].string()for i in range(actual['argc'])],
          'argv_pointers':[int(ancestor.read_var('argv')[i].cast(gdb.lookup_type('unsigned int')))for i in range(actual['argc'])],
          'argv_terminator':int(ancestor.read_var('argv')[actual['argc']].cast(gdb.lookup_type('unsigned int')))}
        primary_entries.append({'invocation':invocation,'crt_state':crt_state,'boundary_rva':0x129db,'symbol':block.function.name,'pc':int(gdb.parse_and_eval('$pc')),'esp':esp,'return':words[0],
          'args':{},'formal_inventory':formals,'compiler_state':capture_modeled(),'caller':{'symbol':caller.name(),'pc':int(caller.pc()),'args':{}}})
        return False
ActualPrimaryEntry('*0x804cc50')
scaler_fp_records=[]
scaler_entries=[]
scaler_returns=[]
def capture_x87():
    cw=int(gdb.parse_and_eval('$fctrl'))&0xffff;sw=int(gdb.parse_and_eval('$fstat'))&0xffff;tag=int(gdb.parse_and_eval('$ftag'))&0xffff
    top=(sw>>11)&7
    registers=[gdb.parse_and_eval('$st%d'%i).bytes.hex()for i in range(8)]
    return {'cw':cw,'sw':sw,'tag':tag,'top':top,'registers':registers,'live':[{'logical':i,'tag':(tag>>(2*((top+i)&7)))&3,'raw80':registers[i]}for i in range(8)if ((tag>>(2*((top+i)&7)))&3)!=3]}
class ScalerFloatPoint(gdb.Breakpoint):
    def __init__(self,address,label):
        super(ScalerFloatPoint,self).__init__('*0x%x'%address);self.label=label
    def stop(self):
        value={'label':self.label,'pc':int(gdb.parse_and_eval('(unsigned int)$pc')),'esp':int(gdb.parse_and_eval('(unsigned int)$esp')),'ebp':int(gdb.parse_and_eval('(unsigned int)$ebp')),
          'fp':capture_x87(),'compiler_state':capture_modeled()}
        if self.label=='converted':
            address=value['ebp']+-24
            value['qword_address']=address;value['qword_bytes']=gdb.selected_inferior().read_memory(address,8).tobytes().hex()
        scaler_fp_records.append(value)
        return False
class ScalerReturn(gdb.Breakpoint):
    def __init__(self,address):super(ScalerReturn,self).__init__('*0x%x'%address)
    def stop(self):
        scaler_returns.append({'pc':int(gdb.parse_and_eval('(unsigned int)$pc')),'esp':int(gdb.parse_and_eval('(unsigned int)$esp')),'eax':int(gdb.parse_and_eval('(unsigned int)$eax')),
          'fp':capture_x87(),'compiler_state':capture_modeled()});self.enabled=False;return False
class ScalerEntry(gdb.Breakpoint):
    def stop(self):
        esp=int(gdb.parse_and_eval('(unsigned int)$esp'));words=list(struct.unpack('<2I',gdb.selected_inferior().read_memory(esp,8).tobytes()))
        block=gdb.block_for_pc(int(gdb.parse_and_eval('$pc')))
        while block and not block.function:block=block.superblock
        assert block and block.function
        frame=gdb.newest_frame();caller=frame.older();ancestor=frame
        while ancestor and ancestor.name()!='InvokeCompilerPass':ancestor=ancestor.older()
        assert ancestor
        actual={key:int(ancestor.read_var(key).cast(gdb.lookup_type('unsigned int')))for key in ('argc','argv','mode')}
        invocation={'symbol':ancestor.name(),'args':actual,'argv':[ancestor.read_var('argv')[i].string()for i in range(actual['argc'])],
          'argv_pointers':[int(ancestor.read_var('argv')[i].cast(gdb.lookup_type('unsigned int')))for i in range(actual['argc'])],
          'argv_terminator':int(ancestor.read_var('argv')[actual['argc']].cast(gdb.lookup_type('unsigned int')))}
        crt_state={'win32_last_error':int(gdb.parse_and_eval('last_error')),'x87_control':int(gdb.parse_and_eval('$fctrl')),
          'crt_attached':int(gdb.parse_and_eval('c1_crt_attached')),'raw_dllmain':int(gdb.parse_and_eval('(unsigned int)c1_raw_dllmain')),
          'onexit_begin':int(gdb.parse_and_eval('(unsigned int)c1_onexit_begin')),'onexit_end':int(gdb.parse_and_eval('(unsigned int)c1_onexit_end')),
          'onexit_first':int(gdb.parse_and_eval('(unsigned int)*c1_onexit_begin')),'crt_adjust_fdiv':int(gdb.parse_and_eval('c1_crt_adjust_fdiv')),
          'dll_instance':int(gdb.parse_and_eval('(unsigned int)c1_dll_instance')),'module_token':int(gdb.parse_and_eval('(unsigned int)c1_host_module'))}
        caller_args={key:int(caller.read_var(key).cast(gdb.lookup_type('unsigned int')))for key in ('this','descriptor','address')}
        scaler_entries.append({'symbol':frame.name(),'invocation':invocation,'crt_state':crt_state,'caller':{'symbol':caller.name(),'pc':int(caller.pc()),'args':caller_args},
          'args':{'descriptor':words[1]},'pc':int(gdb.parse_and_eval('(unsigned int)$pc')),'esp':esp,'return':words[0],'descriptor':words[1],
          'formal_inventory':[{'name':symbol.name,'type':str(symbol.type)}for symbol in block if symbol.is_argument],'fp':capture_x87(),'compiler_state':capture_modeled()})
        ScalerReturn(words[0]);return False
initial_x87_records=[]
class InitialX87(gdb.Breakpoint):
    def stop(self):
        esp=int(gdb.parse_and_eval('(unsigned int)$esp'));words=list(struct.unpack('<4I',gdb.selected_inferior().read_memory(esp,16).tobytes()))
        initial_x87_records.append({'pc':int(gdb.parse_and_eval('(unsigned int)$pc')),'esp':esp,'return':words[0],'args':words[1:],'fp':capture_x87(),
          'profile':{'reference':gdb.parse_and_eval('native_x87_empty_entry.reference_sha256').string(),'applications':int(gdb.parse_and_eval('native_x87_empty_entry.applications'))}});return False
InitialX87("*0x804a900")
ScalerEntry("*0x804cde0")
ScalerFloatPoint(134532622,'instruction-0')
ScalerFloatPoint(134532633,'instruction-1')
ScalerFloatPoint(134532636,'instruction-2')
ScalerFloatPoint(134532638,'instruction-3')
ScalerFloatPoint(134532644,'instruction-4')
ScalerFloatPoint(134532646,'instruction-5')
ScalerFloatPoint(134532662,'instruction-6')
ScalerFloatPoint(134532665,'instruction-7')
ScalerFloatPoint(134532668,'instruction-8')
ScalerFloatPoint(134532671,"converted")
arena_reset_entries=[]
arena_reset_returns=[]
class ArenaResetReturn(gdb.Breakpoint):
    def __init__(self,address,index):
        super(ArenaResetReturn,self).__init__('*0x%x'%address);self.index=index
    def stop(self):
        arena_reset_returns.append({'index':self.index,'pc':int(gdb.parse_and_eval('(unsigned int)$pc')),'esp':int(gdb.parse_and_eval('(unsigned int)$esp')),'fp':capture_x87(),'compiler_state':capture_modeled()});self.enabled=False;return False
class ArenaResetEntry(gdb.Breakpoint):
    def stop(self):
        pc=int(gdb.parse_and_eval('(unsigned int)$pc'));esp=int(gdb.parse_and_eval('(unsigned int)$esp'))
        words=list(struct.unpack('<2I',gdb.selected_inferior().read_memory(esp,8).tobytes()))
        block=gdb.block_for_pc(pc)
        while block and not block.function:block=block.superblock
        assert block and block.function
        before=capture_modeled();index=len(arena_reset_entries);ecx=int(gdb.parse_and_eval('(unsigned int)$ecx'))
        if False:
            assert index<6 and ecx==before['live_addresses']['bss']+804+index*32
            base=before['bss']['arenas'][index]['base'];assert base
            for offset,value in [(0,base+0x1000),(4,base+0x2000),(12,117+index),(24,37+index)]:
                gdb.selected_inferior().write_memory(ecx+offset,struct.pack('<I',value))
        arena_reset_entries.append({'index':index,'pc':pc,'esp':esp,'return':words[0],'ecx':ecx,'seeded':False,'pre_seed':before,
          'formal_inventory':[{'name':symbol.name,'type':str(symbol.type)}for symbol in block if symbol.is_argument],
          'fp':capture_x87(),'compiler_state':capture_modeled()})
        ArenaResetReturn(words[0],index);return False
ArenaResetEntry("*0x804cea0")
arena_api_entries=[]
arena_api_returns=[]
class ArenaApiReturn(gdb.Breakpoint):
    def __init__(self,address):super(ArenaApiReturn,self).__init__('*0x%x'%address)
    def stop(self):
        arena_api_returns.append({'pc':int(gdb.parse_and_eval('(unsigned int)$pc')),'esp':int(gdb.parse_and_eval('(unsigned int)$esp')),'eax':int(gdb.parse_and_eval('(unsigned int)$eax')),
          'last_error':int(gdb.parse_and_eval('last_error')),'fp':capture_x87(),'compiler_state':capture_modeled(),'maps':open('/proc/%d/maps'%gdb.selected_inferior().pid).read(),
          'preserved':{name:int(gdb.parse_and_eval('(unsigned int)$'+name))for name in ('ebx','esi','edi','ebp')}});self.enabled=False;return False
work_api_entries=[]
work_api_returns=[]
work_reserve_entries=[]
work_reserve_returns=[]
class WorkApiReturn(gdb.Breakpoint):
    def __init__(self,address,index):
        super(WorkApiReturn,self).__init__('*0x%x'%address)
        self.index=index
    def stop(self):
        before=work_api_entries[self.index]
        result=int(gdb.parse_and_eval('(unsigned int)$eax'))
        row={'index':self.index,'pc':int(gdb.parse_and_eval('$pc')),'esp':int(gdb.parse_and_eval('(unsigned int)$esp')),'eax':result,'edi':int(gdb.parse_and_eval('(unsigned int)$edi')),'esi':int(gdb.parse_and_eval('(unsigned int)$esi')),'ebx':int(gdb.parse_and_eval('(unsigned int)$ebx')),
             'last_error':int(gdb.parse_and_eval('last_error')),'fpu':int(gdb.parse_and_eval('$fctrl')),
             'maps':open('/proc/%d/maps'%gdb.selected_inferior().pid).read()}
        if self.index%2:
            assert result
            data=gdb.selected_inferior().read_memory(result,0x4000).tobytes()
            open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002'+'/work-committed-%03d.bin'%(self.index//2),'wb').write(data)
        work_api_returns.append(row)
        self.enabled=False
        return False
class WorkApiEntry(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame()
        # Host/shim initialization does not call this API in this case. Require
        # the actual application allocator as caller, with no filtered calls.
        if frame.older().name()=='CompilerArena::Initialize':
            pc=int(gdb.parse_and_eval('(unsigned int)$pc'));esp=int(gdb.parse_and_eval('(unsigned int)$esp'))
            words=list(struct.unpack('<5I',gdb.selected_inferior().read_memory(esp,20).tobytes()))
            block=gdb.block_for_pc(pc)
            while block and not block.function:block=block.superblock
            formals=[{'name':symbol.name,'type':str(symbol.type)}for symbol in block if symbol.is_argument]
            arena_api_entries.append({'pc':pc,'esp':esp,'return':words[0],'args':words[1:],'formal_inventory':formals,'last_error':int(gdb.parse_and_eval('last_error')),'fp':capture_x87(),'compiler_state':capture_modeled(),
              'preserved':{name:int(gdb.parse_and_eval('(unsigned int)$'+name))for name in ('ebx','esi','edi','ebp')}})
            ArenaApiReturn(words[0]);return False
        # The separate strict commit observer owns this exact additional caller.
        if frame.older().name()=='CompilerArena::CommitExtent':return False
        assert frame.older().name()=='reserve_work_buffer'
        pc=int(gdb.parse_and_eval('$pc'));esp=int(gdb.parse_and_eval('(unsigned int)$esp'))
        words=list(struct.unpack('<5I',gdb.selected_inferior().read_memory(esp,20).tobytes()))
        block=gdb.block_for_pc(pc)
        while block and not block.function:block=block.superblock
        assert block and block.function
        formals=[{'name':symbol.name,'type':str(symbol.type)} for symbol in block if symbol.is_argument]
        assert {row['name']for row in formals}=={'address','bytes','kind','protection'}
        index=len(work_api_entries)
        work_api_entries.append({'index':index,'pc':pc,'esp':esp,'return':words[0],'args':words[1:],'formal_inventory':formals,
          'edi':int(gdb.parse_and_eval('(unsigned int)$edi')),'esi':int(gdb.parse_and_eval('(unsigned int)$esi')),'ebx':int(gdb.parse_and_eval('(unsigned int)$ebx')),'last_error':int(gdb.parse_and_eval('last_error')),'fpu':int(gdb.parse_and_eval('$fctrl'))})
        WorkApiReturn(words[0],index)
        return False
class WorkReserveReturn(gdb.Breakpoint):
    def __init__(self,address,index):
        super(WorkReserveReturn,self).__init__('*0x%x'%address)
        self.index=index
    def stop(self):
        work_reserve_returns.append({'index':self.index,'pc':int(gdb.parse_and_eval('$pc')),'esp':int(gdb.parse_and_eval('(unsigned int)$esp')),'eax':int(gdb.parse_and_eval('(unsigned int)$eax')),
          'edi':int(gdb.parse_and_eval('(unsigned int)$edi')),'esi':int(gdb.parse_and_eval('(unsigned int)$esi')),'compiler_state':capture_modeled()})
        self.enabled=False
        return False
class WorkReserveEntry(gdb.Breakpoint):
    def stop(self):
        pc=int(gdb.parse_and_eval('$pc'));esp=int(gdb.parse_and_eval('(unsigned int)$esp'))
        words=list(struct.unpack('<3I',gdb.selected_inferior().read_memory(esp,12).tobytes()))
        block=gdb.block_for_pc(pc)
        while block and not block.function:block=block.superblock
        assert block and block.function
        formals=[{'name':symbol.name,'type':str(symbol.type)} for symbol in block if symbol.is_argument]
        index=len(work_reserve_entries)
        crt_state={'win32_last_error':int(gdb.parse_and_eval('last_error')),'x87_control':int(gdb.parse_and_eval('$fctrl')),
          'crt_attached':int(gdb.parse_and_eval('c1_crt_attached')),'raw_dllmain':int(gdb.parse_and_eval('(unsigned int)c1_raw_dllmain')),
          'onexit_begin':int(gdb.parse_and_eval('(unsigned int)c1_onexit_begin')),'onexit_end':int(gdb.parse_and_eval('(unsigned int)c1_onexit_end')),
          'onexit_first':int(gdb.parse_and_eval('(unsigned int)*c1_onexit_begin')),'crt_adjust_fdiv':int(gdb.parse_and_eval('c1_crt_adjust_fdiv')),
          'dll_instance':int(gdb.parse_and_eval('(unsigned int)c1_dll_instance')),'module_token':int(gdb.parse_and_eval('(unsigned int)c1_host_module'))}
        work_reserve_entries.append({'index':index,'pc':pc,'esp':esp,'return':words[0],'raw_args':words[1:],'args':dict(reserve_bytes=words[1],commit_bytes=words[2]),'formal_inventory':formals,'boundary_rva':0x12873,'symbol':block.function.name,'crt_state':crt_state,
          'caller':{'symbol':gdb.newest_frame().older().name(),'pc':int(gdb.newest_frame().older().pc())},
          'edi':int(gdb.parse_and_eval('(unsigned int)$edi')),'esi':int(gdb.parse_and_eval('(unsigned int)$esi')),'compiler_state':capture_modeled()})
        WorkReserveReturn(words[0],index)
        return False
WorkApiEntry('*0x805f470')
WorkReserveEntry('*0x804c7d0')
internal_parser_entries=[]
internal_parser_returns=[]
internal_getenv_entries=[]
internal_getenv_returns=[]
def observed_crt_vectors():
    def save_vector(expression,name,expected=None):
        values=gdb.parse_and_eval(expression);address=int(values.cast(gdb.lookup_type('unsigned int')));assert address
        rows=[]
        for i in range(16384):
            pointer=int(values[i].cast(gdb.lookup_type('unsigned int')))
            if not pointer:break
            text=values[i].string(encoding='latin1').encode('latin1')+b'\0'
            assert len(text)<=32768;rows.append((pointer,text))
        else:raise AssertionError('unterminated actual bound CRT vector')
        assert expected is None or len(rows)==expected
        raw=struct.pack('<II',address,len(rows))+b''.join(struct.pack('<II',pointer,len(text))+text for pointer,text in rows)
        open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002'+'/'+name,'wb').write(raw)
        return {'address':address,'count':len(rows)}
    argc=int(gdb.parse_and_eval('recon_crt_argc'))
    return {'bound':int(gdb.parse_and_eval('recon_crt_arguments_bound')),'argc':argc,
      'argv':save_vector('recon_crt_argv','native-crt-argv.bin',argc),
      'environment':save_vector('recon_crt_environment','native-crt-environment.bin')}
class InternalParserReturn(gdb.Breakpoint):
    def __init__(self,address,index):
        super(InternalParserReturn,self).__init__('*0x%x'%address)
        self.index=index
    def stop(self):
        internal_parser_returns.append({'index':self.index,'pc':int(gdb.parse_and_eval('$pc')),'esp':int(gdb.parse_and_eval('(unsigned int)$esp')),'eax':int(gdb.parse_and_eval('(unsigned int)$eax')),'compiler_state':capture_modeled(),
          'last_error':int(gdb.parse_and_eval('last_error')),'fpu':int(gdb.parse_and_eval('$fctrl'))})
        self.enabled=False
        return False
class InternalParserEntry(gdb.Breakpoint):
    def stop(self):
        pc=int(gdb.parse_and_eval('$pc'));esp=int(gdb.parse_and_eval('(unsigned int)$esp'))
        words=list(struct.unpack('<3I',gdb.selected_inferior().read_memory(esp,12).tobytes()))
        argument=gdb.Value(words[1]).cast(gdb.lookup_type('char').pointer()).string()
        block=gdb.block_for_pc(pc)
        while block and not block.function:block=block.superblock
        formals=[{'name':symbol.name,'type':str(symbol.type)}for symbol in block if symbol.is_argument]
        index=len(internal_parser_entries)
        internal_parser_entries.append({'index':index,'pc':pc,'esp':esp,'return':words[0],'argument':words[1],'mode':words[2],'formal_inventory':formals,
          'bytes':list(gdb.selected_inferior().read_memory(words[1],len(argument)+1).tobytes()),'compiler_state':capture_modeled()})
        InternalParserReturn(words[0],index)
        return False
class InternalGetenvReturn(gdb.Breakpoint):
    def stop(self):
        internal_getenv_returns.append({'pc':int(gdb.parse_and_eval('$pc')),'esp':int(gdb.parse_and_eval('(unsigned int)$esp')),'eax':int(gdb.parse_and_eval('(unsigned int)$eax')),
          'compiler_state':capture_modeled(),'last_error':int(gdb.parse_and_eval('last_error')),'fpu':int(gdb.parse_and_eval('$fctrl')),'crt':{'bound':int(gdb.parse_and_eval('recon_crt_arguments_bound')),'environment':int(gdb.parse_and_eval('(unsigned int)recon_crt_environment'))}})
        self.enabled=False
        return False
class InternalGetenvEntry(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame();assert frame.older().name()=='scan_internal_switches'
        pc=int(gdb.parse_and_eval('$pc'));esp=int(gdb.parse_and_eval('(unsigned int)$esp'))
        words=list(struct.unpack('<2I',gdb.selected_inferior().read_memory(esp,8).tobytes()))
        name=gdb.Value(words[1]).cast(gdb.lookup_type('char').pointer()).string()
        block=gdb.block_for_pc(pc)
        while block and not block.function:block=block.superblock
        formals=[{'name':symbol.name,'type':str(symbol.type)}for symbol in block if symbol.is_argument]
        internal_getenv_entries.append({'pc':pc,'esp':esp,'return':words[0],'name':name,'crt':observed_crt_vectors(),'formal_inventory':formals,'compiler_state':capture_modeled()})
        InternalGetenvReturn('*0x%x'%words[0])
        return False
InternalParserEntry('*0x804c990')
InternalGetenvEntry('*0x804efb0')
system_entries=[]
system_returns=[]
class SystemInfoReturn(gdb.Breakpoint):
    def stop(self):
        entry=system_entries[-1]
        system_returns.append({'pc':int(gdb.parse_and_eval('$pc')),'esp':int(gdb.parse_and_eval('(unsigned int)$esp')),
          'profile':{'bound':int(gdb.parse_and_eval('native_system_profile.bound')),'system_calls':int(gdb.parse_and_eval('native_system_profile.system_calls')),'memory_calls':int(gdb.parse_and_eval('native_system_profile.memory_calls')),'reference':gdb.parse_and_eval('native_system_profile.reference_sha256').string()},'bytes':list(gdb.selected_inferior().read_memory(entry['argument'],36).tobytes()),'compiler_state':capture_modeled(),
          'last_error':int(gdb.parse_and_eval('last_error')),'fpu':int(gdb.parse_and_eval('$fctrl'))})
        self.enabled=False
        return False
class SystemInfoEntry(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame();assert frame.older().name()=='c1_main'
        pc=int(gdb.parse_and_eval('$pc'));esp=int(gdb.parse_and_eval('(unsigned int)$esp'))
        words=list(struct.unpack('<2I',gdb.selected_inferior().read_memory(esp,8).tobytes()))
        block=gdb.block_for_pc(pc)
        while block and not block.function:block=block.superblock
        formals=[{'name':symbol.name,'type':str(symbol.type)}for symbol in block if symbol.is_argument]
        system_entries.append({'pc':pc,'esp':esp,'return':words[0],'argument':words[1],'formal_inventory':formals,
          'profile':{'bound':int(gdb.parse_and_eval('native_system_profile.bound')),'system_calls':int(gdb.parse_and_eval('native_system_profile.system_calls')),'memory_calls':int(gdb.parse_and_eval('native_system_profile.memory_calls')),'reference':gdb.parse_and_eval('native_system_profile.reference_sha256').string()},'bytes':list(gdb.selected_inferior().read_memory(words[1],36).tobytes()),'compiler_state':capture_modeled(),
          'last_error':int(gdb.parse_and_eval('last_error')),'fpu':int(gdb.parse_and_eval('$fctrl'))})
        SystemInfoReturn('*0x%x'%words[0])
        return False
SystemInfoEntry('*0x805c700')
end
run
bt
python
import gdb,json
frame=gdb.newest_frame()
expected_argument_names=('argc', 'argv')
block=frame.block()
while block and not block.function:block=block.superblock
assert block and block.function
formals=[{'name':symbol.name,'type':str(symbol.type)} for symbol in block if symbol.is_argument]
args={row['name']:int(frame.read_var(row['name']).cast(gdb.lookup_type('unsigned int')))for row in formals}
value={'symbol':frame.name(),'pc':int(gdb.parse_and_eval('$pc')),'args':args,'formal_inventory':formals}
assert {row['name'] for row in formals}==set(args), 'omitted or invented native formal'

if frame.name() in ('InvokeCompilerPass','c1_main'):
    value['argv']=[gdb.parse_and_eval('argv[%d]'%i).string() for i in range(args['argc'])]
    value['argv_terminator']=int(gdb.parse_and_eval('(unsigned int)argv[%d]'%args['argc']))
ancestor=frame
while ancestor and ancestor.name()!='InvokeCompilerPass': ancestor=ancestor.older()
value['invocation']=None
if ancestor:
    callargs={name:int(ancestor.read_var(name).cast(gdb.lookup_type('unsigned int'))) for name in ('argc','argv','mode')}
    value['invocation']={'symbol':ancestor.name(),'args':callargs,
      'argv':[ancestor.read_var('argv')[i].string() for i in range(callargs['argc'])],
      'argv_pointers':[int(ancestor.read_var('argv')[i].cast(gdb.lookup_type('unsigned int'))) for i in range(callargs['argc'])],
      'argv_terminator':int(ancestor.read_var('argv')[callargs['argc']].cast(gdb.lookup_type('unsigned int')))}
if frame.name()=='parse_internal_switch':
    value['argument_bytes']=list(gdb.selected_inferior().read_memory(args['argument'],len(gdb.parse_and_eval('argument').string())+1).tobytes())
caller=frame.older()
value['caller']={'symbol':caller.name(),'args':{},'pc':int(caller.pc())}
if caller.name()=='InvokeCompilerPass':
    value['caller']['args']={name:int(caller.read_var(name).cast(gdb.lookup_type('unsigned int'))) for name in ('argc','argv','mode')}
    value['caller']['argv']=[caller.read_var('argv')[i].string() for i in range(value['caller']['args']['argc'])]
    value['caller']['argv_terminator']=int(caller.read_var('argv')[value['caller']['args']['argc']].cast(gdb.lookup_type('unsigned int')))
if caller.name()=='c1_main':
    value['caller']['pc']=int(caller.pc())
    value['caller']['args']={name:int(caller.read_var(name).cast(gdb.lookup_type('unsigned int'))) for name in ('argc','argv')}
if caller.name()=='CompilerArena::AllocatePages':
    value['caller']['args']={name:int(caller.read_var(name).cast(gdb.lookup_type('unsigned int')))for name in ('this','pages','available_bytes')}
if caller.name()=='CompilerPool::Refill':
    value['caller']['args']={name:int(caller.read_var(name).cast(gdb.lookup_type('unsigned int')))for name in ('this','minimum_bytes')}
if caller.name()=='CompilerPool::SetArena':
    value['caller']['args']={name:int(caller.read_var(name).cast(gdb.lookup_type('unsigned int')))for name in ('this','arena_index')}
if caller.name()=='CompilerArena::Initialize':
    value['caller']['args']={name:int(caller.read_var(name).cast(gdb.lookup_type('unsigned int')))for name in ('this','descriptor','address')}
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
 'initialized':{'first_call':int(gdb.parse_and_eval('c1_data.first_call')),'compiler_phase':int(gdb.parse_and_eval('c1_data.compiler_phase')),'c9_il':int(gdb.parse_and_eval('c1_data.c9_il')),'compiler_flags':int(gdb.parse_and_eval('(unsigned int)c1_data.compiler_flags')),'system_info':{key:int(gdb.parse_and_eval('(unsigned int)c1_data.system_info.'+key)) for key in ('dwOemId', 'dwPageSize', 'lpMinimumApplicationAddress', 'lpMaximumApplicationAddress', 'dwActiveProcessorMask', 'dwNumberOfProcessors', 'dwProcessorType', 'dwAllocationGranularity', 'wProcessorLevel', 'wProcessorRevision')},'internal_bm_flags':int(gdb.parse_and_eval('c1_data.internal_bm_flags')),'internal_zb_value':int(gdb.parse_and_eval('c1_data.internal_zb_value')),'memory_percent':int(gdb.parse_and_eval('c1_data.memory_percent'))},
 'bss':{'arenas':[{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.arenas[%d].'%i+key))for key in ('cursor','committed_limit','base','committed_bytes','descriptor','reserve_bytes','reset_value','mapped_view')}for i in range(6)],'work_buffers':[{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.work_buffers[%d].'%i+key)) for key in ('data','limit')} for i in range(2)],'debug_break':int(gdb.parse_and_eval('c1_bss.debug_break')),'command_argc':int(gdb.parse_and_eval('c1_bss.command_argc')),'command_argv':int(gdb.parse_and_eval('(unsigned int)c1_bss.command_argv')),'pch_state':{key:([int(gdb.parse_and_eval('(unsigned int)c1_bss.pch_state.mapping_handles[%d]'%i)) for i in range(6)] if key=='mapping_handles' else int(gdb.parse_and_eval(('(int)' if key in ('image_count','loaded_image_count') else '(unsigned int)')+'c1_bss.pch_state.'+key))) for key in ('image_count', 'root_image', 'images', 'stored_file_position', 'file_name', 'file_offset', 'file', 'mapping_handles', 'loaded_image_count', 'scratch_arena', 'original_pages', 'mapped_pages')},'empty_record_prototype':[int(gdb.parse_and_eval('c1_bss.empty_record_prototype[%d]'%i)) for i in range(20)],'pools':[{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.pools[%d].'%i+key)) for key in ('cursor','remaining','minimum_pages','arena')} for i in range(9)],'pdb_signature':int(gdb.parse_and_eval('c1_bss.pdb_signature')),'literal':int(gdb.parse_and_eval('c1_bss.literal.integer')),'saved_literal':int(gdb.parse_and_eval('c1_bss.saved_literal.integer')),'saved_literal_available':int(gdb.parse_and_eval('c1_bss.saved_literal_available')),'saved_data':int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data')),
        'cleanup_count':int(gdb.parse_and_eval('c1_bss.cleanup_count')),
        'cleanup_callbacks':[int(gdb.parse_and_eval('(unsigned int)c1_bss.cleanup_callbacks[%d]'%i)) for i in range(32)],
        'context':{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.context.'+key)) for key in ('object','value','saved_frame')},
        'current_context':{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.current_context.'+key)) for key in ('object','value','saved_frame')},
        'contexts':[{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.contexts[%d].'%i+key)) for key in ('object','value','saved_frame')} for i in range(6)],
        'output_buffers':[{key:int(gdb.parse_and_eval(('(int)' if key=='file_descriptor' else '(unsigned int)')+'c1_bss.output_buffers[%d].'%i+key)) for key in ('data','cursor','capacity','relocated','relocation_context','stream','good','mode','file_descriptor')} for i in range(8)]},
 'arena_descriptors':[{key:int(gdb.parse_and_eval('c1_arena_descriptors[%d].'%i+key))for key in ('kind','reserve_bytes','commit_unit','flags','alignment')}for i in range(6)],
 'snapshot':({'first_call':int(gdb.parse_and_eval('c1_bss.saved_data->first_call')),'compiler_phase':int(gdb.parse_and_eval('c1_bss.saved_data->compiler_phase')),'c9_il':int(gdb.parse_and_eval('c1_bss.saved_data->c9_il')),'compiler_flags':int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data->compiler_flags')),'system_info':{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data->system_info.'+key)) for key in ('dwOemId', 'dwPageSize', 'lpMinimumApplicationAddress', 'lpMaximumApplicationAddress', 'dwActiveProcessorMask', 'dwNumberOfProcessors', 'dwProcessorType', 'dwAllocationGranularity', 'wProcessorLevel', 'wProcessorRevision')},'internal_bm_flags':int(gdb.parse_and_eval('c1_bss.saved_data->internal_bm_flags')),'internal_zb_value':int(gdb.parse_and_eval('c1_bss.saved_data->internal_zb_value')),'memory_percent':int(gdb.parse_and_eval('c1_bss.saved_data->memory_percent'))} if int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data')) else None),
 'reuse_state':int(gdb.parse_and_eval('c1_reuse_state')),
 'pass_mode':int(gdb.parse_and_eval('c1_pass_mode')),
 'initialization_callbacks':[int(gdb.parse_and_eval('(unsigned int)c1_initialization_callbacks[%d]'%i)) for i in range(34)],
 'native_layout':{'arenas':int(gdb.parse_and_eval('(char*)&c1_bss.arenas-(char*)&c1_bss')),'arena_count':6,'arena_size':int(gdb.parse_and_eval('sizeof(c1_bss.arenas[0])')),'arena_members':{key:int(gdb.parse_and_eval('(char*)&c1_bss.arenas[0].'+key+'-(char*)&c1_bss.arenas[0]'))for key in ('cursor','committed_limit','base','committed_bytes','descriptor','reserve_bytes','reset_value','mapped_view')},'arena_descriptor_size':int(gdb.parse_and_eval('sizeof(c1_arena_descriptors[0])')),'arena_descriptor_members':{key:int(gdb.parse_and_eval('(char*)&c1_arena_descriptors[0].'+key+'-(char*)&c1_arena_descriptors[0]'))for key in ('kind','reserve_bytes','commit_unit','flags','alignment')},'work_buffers':int(gdb.parse_and_eval('(char*)&c1_bss.work_buffers-(char*)&c1_bss')),'work_buffer_count':2,'work_buffer_size':int(gdb.parse_and_eval('sizeof(c1_bss.work_buffers[0])')),'work_buffer_members':{key:int(gdb.parse_and_eval('(char*)&c1_bss.work_buffers[0].'+key+'-(char*)&c1_bss.work_buffers[0]')) for key in ('data','limit')},'initialized_members':{key:int(gdb.parse_and_eval('(char*)&c1_data.'+key+'-(char*)&c1_data')) for key in ('first_call','compiler_phase','c9_il','compiler_flags','system_info','internal_bm_flags','internal_zb_value','memory_percent')},'system_info_size':int(gdb.parse_and_eval('sizeof(c1_data.system_info)')),'system_info_members':{key:int(gdb.parse_and_eval('(char*)&c1_data.system_info.'+key+'-(char*)&c1_data.system_info')) for key in ('dwOemId', 'dwPageSize', 'lpMinimumApplicationAddress', 'lpMaximumApplicationAddress', 'dwActiveProcessorMask', 'dwNumberOfProcessors', 'dwProcessorType', 'dwAllocationGranularity', 'wProcessorLevel', 'wProcessorRevision')},'command_members':{key:int(gdb.parse_and_eval('(char*)&c1_bss.'+key+'-(char*)&c1_bss')) for key in ('debug_break','command_argc','command_argv')},'pch_state':int(gdb.parse_and_eval('(char*)&c1_bss.pch_state-(char*)&c1_bss')),'pch_state_size':int(gdb.parse_and_eval('sizeof(c1_bss.pch_state)')),'mapping_handle_count':int(gdb.parse_and_eval('sizeof(c1_bss.pch_state.mapping_handles)/sizeof(c1_bss.pch_state.mapping_handles[0])')),'pch_members':{key:int(gdb.parse_and_eval('(char*)&c1_bss.pch_state.'+key+'-(char*)&c1_bss.pch_state')) for key in ('image_count', 'root_image', 'images', 'stored_file_position', 'file_name', 'file_offset', 'file', 'mapping_handles', 'loaded_image_count', 'scratch_arena', 'original_pages', 'mapped_pages')},'empty_record_prototype':int(gdb.parse_and_eval('(char*)&c1_bss.empty_record_prototype-(char*)&c1_bss')),'empty_record_prototype_size':int(gdb.parse_and_eval('sizeof(c1_bss.empty_record_prototype)')),'initialized_size':int(gdb.parse_and_eval('sizeof(c1_data)')),'bss_size':int(gdb.parse_and_eval('sizeof(c1_bss)')),
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
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/pool-entry-observations.json','w').write(json.dumps(pool_entries,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/pool-return-observations.json','w').write(json.dumps(pool_returns,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/pch-entry-observations.json','w').write(json.dumps(pch_entries,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/pch-return-observations.json','w').write(json.dumps(pch_returns,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/main-entry-observations.json','w').write(json.dumps(main_entries,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/work-entry-observations.json','w').write(json.dumps(work_entries,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/arena-entry-observations.json','w').write(json.dumps(arena_entries,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/primary-entry-observations.json','w').write(json.dumps(primary_entries,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/arena-member-entry-observations.json','w').write(json.dumps(arena_member_entries,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/arena-member-returns.json','w').write(json.dumps(arena_member_returns,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/default-pools-entry.json','w').write(json.dumps(default_pool_entries,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/pool-arena-entries.json','w').write(json.dumps(pool_arena_entries,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/pool-refill-entries.json','w').write(json.dumps(pool_refill_entries,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/arena-page-entries.json','w').write(json.dumps(arena_page_entries,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/pool-cycle-events.json','w').write(json.dumps(pool_cycle_events,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/pool-cycle-returns.json','w').write(json.dumps(pool_cycle_returns,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/pool-commit-entries.json','w').write(json.dumps(pool_commit_entries,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/pool-commit-api-entries.json','w').write(json.dumps(pool_commit_api_entries,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/pool-commit-api-returns.json','w').write(json.dumps(pool_commit_api_returns,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/pool-final-committed-000.bin','wb').write(gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('(unsigned int)c1_bss.arenas[0].base')),int(gdb.parse_and_eval('c1_bss.arenas[0].committed_bytes'))).tobytes())
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/arena-reset-returns.json','w').write(json.dumps(arena_reset_returns,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/arena-reset-entries.json','w').write(json.dumps(arena_reset_entries,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/initial-x87.json','w').write(json.dumps(initial_x87_records,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/scaler-fpu.json','w').write(json.dumps(scaler_fp_records,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/scaler-entries.json','w').write(json.dumps(scaler_entries,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/scaler-returns.json','w').write(json.dumps(scaler_returns,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/arena-api-entries.json','w').write(json.dumps(arena_api_entries,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/arena-api-returns.json','w').write(json.dumps(arena_api_returns,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/work-api-entries.json','w').write(json.dumps(work_api_entries,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/work-api-returns.json','w').write(json.dumps(work_api_returns,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/work-reserve-entries.json','w').write(json.dumps(work_reserve_entries,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/work-reserve-returns.json','w').write(json.dumps(work_reserve_returns,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/scanner-entry-observations.json','w').write(json.dumps(scanner_entries,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/internal-parser-entries.json','w').write(json.dumps(internal_parser_entries,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/internal-parser-returns.json','w').write(json.dumps(internal_parser_returns,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/internal-getenv-entries.json','w').write(json.dumps(internal_getenv_entries,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/internal-getenv-returns.json','w').write(json.dumps(internal_getenv_returns,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/empty-entry-observations.json','w').write(json.dumps(empty_entries,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/system-info-entries.json','w').write(json.dumps(system_entries,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/system-info-returns.json','w').write(json.dumps(system_returns,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/allocation-observations.json','w').write(json.dumps(allocations,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-commit-002/state.json','w').write(json.dumps(value,indent=2)+'\n')
end
quit
