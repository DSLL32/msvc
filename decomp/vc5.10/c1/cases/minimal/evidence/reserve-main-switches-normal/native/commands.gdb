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
 'initialized':{'first_call':int(gdb.parse_and_eval('c1_data.first_call')),'compiler_phase':int(gdb.parse_and_eval('c1_data.compiler_phase')),'c9_il':int(gdb.parse_and_eval('c1_data.c9_il')),'compiler_flags':int(gdb.parse_and_eval('(unsigned int)c1_data.compiler_flags')),'system_info':{key:int(gdb.parse_and_eval('(unsigned int)c1_data.system_info.'+key)) for key in ('dwOemId', 'dwPageSize', 'lpMinimumApplicationAddress', 'lpMaximumApplicationAddress', 'dwActiveProcessorMask', 'dwNumberOfProcessors', 'dwProcessorType', 'dwAllocationGranularity', 'wProcessorLevel', 'wProcessorRevision')}},
 'bss':{'work_buffers':[{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.work_buffers[%d].'%i+key)) for key in ('data','limit')} for i in range(2)],'debug_break':int(gdb.parse_and_eval('c1_bss.debug_break')),'command_argc':int(gdb.parse_and_eval('c1_bss.command_argc')),'command_argv':int(gdb.parse_and_eval('(unsigned int)c1_bss.command_argv')),'pch_state':{key:([int(gdb.parse_and_eval('(unsigned int)c1_bss.pch_state.mapping_handles[%d]'%i)) for i in range(6)] if key=='mapping_handles' else int(gdb.parse_and_eval(('(int)' if key in ('image_count','loaded_image_count') else '(unsigned int)')+'c1_bss.pch_state.'+key))) for key in ('image_count', 'root_image', 'images', 'stored_file_position', 'file_name', 'file_offset', 'file', 'mapping_handles', 'loaded_image_count', 'scratch_arena', 'original_pages', 'mapped_pages')},'empty_record_prototype':[int(gdb.parse_and_eval('c1_bss.empty_record_prototype[%d]'%i)) for i in range(20)],'pools':[{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.pools[%d].'%i+key)) for key in ('cursor','remaining','minimum_pages','arena')} for i in range(9)],'pdb_signature':int(gdb.parse_and_eval('c1_bss.pdb_signature')),'literal':int(gdb.parse_and_eval('c1_bss.literal.integer')),'saved_literal':int(gdb.parse_and_eval('c1_bss.saved_literal.integer')),'saved_literal_available':int(gdb.parse_and_eval('c1_bss.saved_literal_available')),'saved_data':int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data')),
        'cleanup_count':int(gdb.parse_and_eval('c1_bss.cleanup_count')),
        'cleanup_callbacks':[int(gdb.parse_and_eval('(unsigned int)c1_bss.cleanup_callbacks[%d]'%i)) for i in range(32)],
        'context':{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.context.'+key)) for key in ('object','value','saved_frame')},
        'current_context':{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.current_context.'+key)) for key in ('object','value','saved_frame')},
        'contexts':[{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.contexts[%d].'%i+key)) for key in ('object','value','saved_frame')} for i in range(6)],
        'output_buffers':[{key:int(gdb.parse_and_eval(('(int)' if key=='file_descriptor' else '(unsigned int)')+'c1_bss.output_buffers[%d].'%i+key)) for key in ('data','cursor','capacity','relocated','relocation_context','stream','good','mode','file_descriptor')} for i in range(8)]},
 'snapshot':({'first_call':int(gdb.parse_and_eval('c1_bss.saved_data->first_call')),'compiler_phase':int(gdb.parse_and_eval('c1_bss.saved_data->compiler_phase')),'c9_il':int(gdb.parse_and_eval('c1_bss.saved_data->c9_il')),'compiler_flags':int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data->compiler_flags')),'system_info':{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data->system_info.'+key)) for key in ('dwOemId', 'dwPageSize', 'lpMinimumApplicationAddress', 'lpMaximumApplicationAddress', 'dwActiveProcessorMask', 'dwNumberOfProcessors', 'dwProcessorType', 'dwAllocationGranularity', 'wProcessorLevel', 'wProcessorRevision')}} if int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data')) else None),
 'reuse_state':int(gdb.parse_and_eval('c1_reuse_state')),
 'pass_mode':int(gdb.parse_and_eval('c1_pass_mode')),
 'initialization_callbacks':[int(gdb.parse_and_eval('(unsigned int)c1_initialization_callbacks[%d]'%i)) for i in range(34)],
 'native_layout':{'work_buffers':int(gdb.parse_and_eval('(char*)&c1_bss.work_buffers-(char*)&c1_bss')),'work_buffer_count':2,'work_buffer_size':int(gdb.parse_and_eval('sizeof(c1_bss.work_buffers[0])')),'work_buffer_members':{key:int(gdb.parse_and_eval('(char*)&c1_bss.work_buffers[0].'+key+'-(char*)&c1_bss.work_buffers[0]')) for key in ('data','limit')},'initialized_members':{key:int(gdb.parse_and_eval('(char*)&c1_data.'+key+'-(char*)&c1_data')) for key in ('first_call','compiler_phase','c9_il','compiler_flags','system_info')},'system_info_size':int(gdb.parse_and_eval('sizeof(c1_data.system_info)')),'system_info_members':{key:int(gdb.parse_and_eval('(char*)&c1_data.system_info.'+key+'-(char*)&c1_data.system_info')) for key in ('dwOemId', 'dwPageSize', 'lpMinimumApplicationAddress', 'lpMaximumApplicationAddress', 'dwActiveProcessorMask', 'dwNumberOfProcessors', 'dwProcessorType', 'dwAllocationGranularity', 'wProcessorLevel', 'wProcessorRevision')},'command_members':{key:int(gdb.parse_and_eval('(char*)&c1_bss.'+key+'-(char*)&c1_bss')) for key in ('debug_break','command_argc','command_argv')},'pch_state':int(gdb.parse_and_eval('(char*)&c1_bss.pch_state-(char*)&c1_bss')),'pch_state_size':int(gdb.parse_and_eval('sizeof(c1_bss.pch_state)')),'mapping_handle_count':int(gdb.parse_and_eval('sizeof(c1_bss.pch_state.mapping_handles)/sizeof(c1_bss.pch_state.mapping_handles[0])')),'pch_members':{key:int(gdb.parse_and_eval('(char*)&c1_bss.pch_state.'+key+'-(char*)&c1_bss.pch_state')) for key in ('image_count', 'root_image', 'images', 'stored_file_position', 'file_name', 'file_offset', 'file', 'mapping_handles', 'loaded_image_count', 'scratch_arena', 'original_pages', 'mapped_pages')},'empty_record_prototype':int(gdb.parse_and_eval('(char*)&c1_bss.empty_record_prototype-(char*)&c1_bss')),'empty_record_prototype_size':int(gdb.parse_and_eval('sizeof(c1_bss.empty_record_prototype)')),'initialized_size':int(gdb.parse_and_eval('sizeof(c1_data)')),'bss_size':int(gdb.parse_and_eval('sizeof(c1_bss)')),
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
PoolEntry('*0x804bde0')
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
PchEntry('*0x804c1a0')
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
ActualMainEntry('*0x804c2b0')
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
ActualWorkEntry('*0x804c520')
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
            open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-reserve-main-switches-002'+'/work-committed-%03d.bin'%(self.index//2),'wb').write(data)
        work_api_returns.append(row)
        self.enabled=False
        return False
class WorkApiEntry(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame()
        # Host/shim initialization does not call this API in this case. Require
        # the actual application allocator as caller, with no filtered calls.
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
WorkApiEntry('*0x805d990')
WorkReserveEntry('*0x804c5b0')
end
run
bt
python
import gdb,json
frame=gdb.newest_frame()
args={name:int(gdb.parse_and_eval('(unsigned int)'+name)) for name in ('arguments',)}
block=frame.block()
while block and not block.function:block=block.superblock
assert block and block.function
formals=[{'name':symbol.name,'type':str(symbol.type)} for symbol in block if symbol.is_argument]
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
      'argv_terminator':int(ancestor.read_var('argv')[callargs['argc']].cast(gdb.lookup_type('unsigned int')))}
caller=frame.older()
value['caller']={'symbol':caller.name(),'args':{},'pc':int(caller.pc())}
if caller.name()=='InvokeCompilerPass':
    value['caller']['args']={name:int(caller.read_var(name).cast(gdb.lookup_type('unsigned int'))) for name in ('argc','argv','mode')}
    value['caller']['argv']=[caller.read_var('argv')[i].string() for i in range(value['caller']['args']['argc'])]
    value['caller']['argv_terminator']=int(caller.read_var('argv')[value['caller']['args']['argc']].cast(gdb.lookup_type('unsigned int')))
if caller.name()=='c1_main':
    value['caller']['pc']=int(caller.pc())
    value['caller']['args']={name:int(caller.read_var(name).cast(gdb.lookup_type('unsigned int'))) for name in ('argc','argv')}
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
 'initialized':{'first_call':int(gdb.parse_and_eval('c1_data.first_call')),'compiler_phase':int(gdb.parse_and_eval('c1_data.compiler_phase')),'c9_il':int(gdb.parse_and_eval('c1_data.c9_il')),'compiler_flags':int(gdb.parse_and_eval('(unsigned int)c1_data.compiler_flags')),'system_info':{key:int(gdb.parse_and_eval('(unsigned int)c1_data.system_info.'+key)) for key in ('dwOemId', 'dwPageSize', 'lpMinimumApplicationAddress', 'lpMaximumApplicationAddress', 'dwActiveProcessorMask', 'dwNumberOfProcessors', 'dwProcessorType', 'dwAllocationGranularity', 'wProcessorLevel', 'wProcessorRevision')}},
 'bss':{'work_buffers':[{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.work_buffers[%d].'%i+key)) for key in ('data','limit')} for i in range(2)],'debug_break':int(gdb.parse_and_eval('c1_bss.debug_break')),'command_argc':int(gdb.parse_and_eval('c1_bss.command_argc')),'command_argv':int(gdb.parse_and_eval('(unsigned int)c1_bss.command_argv')),'pch_state':{key:([int(gdb.parse_and_eval('(unsigned int)c1_bss.pch_state.mapping_handles[%d]'%i)) for i in range(6)] if key=='mapping_handles' else int(gdb.parse_and_eval(('(int)' if key in ('image_count','loaded_image_count') else '(unsigned int)')+'c1_bss.pch_state.'+key))) for key in ('image_count', 'root_image', 'images', 'stored_file_position', 'file_name', 'file_offset', 'file', 'mapping_handles', 'loaded_image_count', 'scratch_arena', 'original_pages', 'mapped_pages')},'empty_record_prototype':[int(gdb.parse_and_eval('c1_bss.empty_record_prototype[%d]'%i)) for i in range(20)],'pools':[{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.pools[%d].'%i+key)) for key in ('cursor','remaining','minimum_pages','arena')} for i in range(9)],'pdb_signature':int(gdb.parse_and_eval('c1_bss.pdb_signature')),'literal':int(gdb.parse_and_eval('c1_bss.literal.integer')),'saved_literal':int(gdb.parse_and_eval('c1_bss.saved_literal.integer')),'saved_literal_available':int(gdb.parse_and_eval('c1_bss.saved_literal_available')),'saved_data':int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data')),
        'cleanup_count':int(gdb.parse_and_eval('c1_bss.cleanup_count')),
        'cleanup_callbacks':[int(gdb.parse_and_eval('(unsigned int)c1_bss.cleanup_callbacks[%d]'%i)) for i in range(32)],
        'context':{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.context.'+key)) for key in ('object','value','saved_frame')},
        'current_context':{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.current_context.'+key)) for key in ('object','value','saved_frame')},
        'contexts':[{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.contexts[%d].'%i+key)) for key in ('object','value','saved_frame')} for i in range(6)],
        'output_buffers':[{key:int(gdb.parse_and_eval(('(int)' if key=='file_descriptor' else '(unsigned int)')+'c1_bss.output_buffers[%d].'%i+key)) for key in ('data','cursor','capacity','relocated','relocation_context','stream','good','mode','file_descriptor')} for i in range(8)]},
 'snapshot':({'first_call':int(gdb.parse_and_eval('c1_bss.saved_data->first_call')),'compiler_phase':int(gdb.parse_and_eval('c1_bss.saved_data->compiler_phase')),'c9_il':int(gdb.parse_and_eval('c1_bss.saved_data->c9_il')),'compiler_flags':int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data->compiler_flags')),'system_info':{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data->system_info.'+key)) for key in ('dwOemId', 'dwPageSize', 'lpMinimumApplicationAddress', 'lpMaximumApplicationAddress', 'dwActiveProcessorMask', 'dwNumberOfProcessors', 'dwProcessorType', 'dwAllocationGranularity', 'wProcessorLevel', 'wProcessorRevision')}} if int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data')) else None),
 'reuse_state':int(gdb.parse_and_eval('c1_reuse_state')),
 'pass_mode':int(gdb.parse_and_eval('c1_pass_mode')),
 'initialization_callbacks':[int(gdb.parse_and_eval('(unsigned int)c1_initialization_callbacks[%d]'%i)) for i in range(34)],
 'native_layout':{'work_buffers':int(gdb.parse_and_eval('(char*)&c1_bss.work_buffers-(char*)&c1_bss')),'work_buffer_count':2,'work_buffer_size':int(gdb.parse_and_eval('sizeof(c1_bss.work_buffers[0])')),'work_buffer_members':{key:int(gdb.parse_and_eval('(char*)&c1_bss.work_buffers[0].'+key+'-(char*)&c1_bss.work_buffers[0]')) for key in ('data','limit')},'initialized_members':{key:int(gdb.parse_and_eval('(char*)&c1_data.'+key+'-(char*)&c1_data')) for key in ('first_call','compiler_phase','c9_il','compiler_flags','system_info')},'system_info_size':int(gdb.parse_and_eval('sizeof(c1_data.system_info)')),'system_info_members':{key:int(gdb.parse_and_eval('(char*)&c1_data.system_info.'+key+'-(char*)&c1_data.system_info')) for key in ('dwOemId', 'dwPageSize', 'lpMinimumApplicationAddress', 'lpMaximumApplicationAddress', 'dwActiveProcessorMask', 'dwNumberOfProcessors', 'dwProcessorType', 'dwAllocationGranularity', 'wProcessorLevel', 'wProcessorRevision')},'command_members':{key:int(gdb.parse_and_eval('(char*)&c1_bss.'+key+'-(char*)&c1_bss')) for key in ('debug_break','command_argc','command_argv')},'pch_state':int(gdb.parse_and_eval('(char*)&c1_bss.pch_state-(char*)&c1_bss')),'pch_state_size':int(gdb.parse_and_eval('sizeof(c1_bss.pch_state)')),'mapping_handle_count':int(gdb.parse_and_eval('sizeof(c1_bss.pch_state.mapping_handles)/sizeof(c1_bss.pch_state.mapping_handles[0])')),'pch_members':{key:int(gdb.parse_and_eval('(char*)&c1_bss.pch_state.'+key+'-(char*)&c1_bss.pch_state')) for key in ('image_count', 'root_image', 'images', 'stored_file_position', 'file_name', 'file_offset', 'file', 'mapping_handles', 'loaded_image_count', 'scratch_arena', 'original_pages', 'mapped_pages')},'empty_record_prototype':int(gdb.parse_and_eval('(char*)&c1_bss.empty_record_prototype-(char*)&c1_bss')),'empty_record_prototype_size':int(gdb.parse_and_eval('sizeof(c1_bss.empty_record_prototype)')),'initialized_size':int(gdb.parse_and_eval('sizeof(c1_data)')),'bss_size':int(gdb.parse_and_eval('sizeof(c1_bss)')),
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
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-reserve-main-switches-002/pool-entry-observations.json','w').write(json.dumps(pool_entries,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-reserve-main-switches-002/pool-return-observations.json','w').write(json.dumps(pool_returns,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-reserve-main-switches-002/pch-entry-observations.json','w').write(json.dumps(pch_entries,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-reserve-main-switches-002/pch-return-observations.json','w').write(json.dumps(pch_returns,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-reserve-main-switches-002/main-entry-observations.json','w').write(json.dumps(main_entries,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-reserve-main-switches-002/work-entry-observations.json','w').write(json.dumps(work_entries,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-reserve-main-switches-002/work-api-entries.json','w').write(json.dumps(work_api_entries,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-reserve-main-switches-002/work-api-returns.json','w').write(json.dumps(work_api_returns,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-reserve-main-switches-002/work-reserve-entries.json','w').write(json.dumps(work_reserve_entries,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-reserve-main-switches-002/work-reserve-returns.json','w').write(json.dumps(work_reserve_returns,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-reserve-main-switches-002/allocation-observations.json','w').write(json.dumps(allocations,indent=2)+'\n')
open('/root/.ai-tmp/msvc2/vc5.10/c1/native-minimal-reserve-main-switches-002/state.json','w').write(json.dumps(value,indent=2)+'\n')
end
quit
