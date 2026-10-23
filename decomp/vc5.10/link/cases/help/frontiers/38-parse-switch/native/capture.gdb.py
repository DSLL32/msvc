
import gdb,json,struct
state={'entries':[],'initerm_calls':[],'operator_new_calls':[],'atexit_calls':[],'console_calls':[],'module_probes':[],'system_calls':[],'mbsnbicmp_calls':[],'virtual_alloc_calls':[],'parser_allocations':[]}
class NewReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,size):
        super().__init__(frame,internal=True);self.size=size
    def stop(self):
        state['operator_new_calls'].append({'bytes':self.size,'result':int(self.return_value)})
        return False
class NewEntry(gdb.Breakpoint):
    def stop(self):
        size=int(gdb.parse_and_eval('((unsigned*)$esp)[1]'))
        NewReturned(gdb.newest_frame(),size)
        return False
NewEntry('*recon_cpp_new',internal=True)
class MbsReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,left,right,count):
        super().__init__(frame,internal=True);self.left=left;self.right=right;self.count=count
    def stop(self):
        state['mbsnbicmp_calls'].append(dict(left=self.left,right=self.right,bytes=self.count,result=int(self.return_value)))
        return False
class MbsEntry(gdb.Breakpoint):
    def stop(self):
        arguments=gdb.parse_and_eval('(unsigned*)$esp')
        text=gdb.lookup_type('char').pointer()
        MbsReturned(gdb.newest_frame(),gdb.Value(int(arguments[1])).cast(text).string(),gdb.Value(int(arguments[2])).cast(text).string(),int(arguments[3]))
        return False
MbsEntry('*recon_mbsnbicmp',internal=True)
class AtExitReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,callback):
        super().__init__(frame,internal=True);self.callback=callback
    def stop(self):
        state['atexit_calls'].append({'callback':self.callback,'result':int(gdb.parse_and_eval('(int)$eax'))})
        return False
class AtExitEntry(gdb.Breakpoint):
    def stop(self):
        callback=int(gdb.parse_and_eval('((unsigned*)$esp)[1]'))
        name=gdb.execute('info symbol '+str(callback),to_string=True).strip().split(' ')[0]
        if name in ('free_buffer12','free_buffer8','free_buffer4','close_timing'):
            AtExitReturned(gdb.newest_frame(),name)
        return False
AtExitEntry('*atexit',internal=True)
class ConsoleReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,callback,add):
        super().__init__(frame,internal=True);self.callback=callback;self.add=add
    def stop(self):
        state['console_calls'].append(dict(callback=self.callback,add=self.add,result=int(self.return_value)))
        return False
class ConsoleEntry(gdb.Breakpoint):
    def stop(self):
        callback=int(gdb.parse_and_eval('((unsigned*)$esp)[1]'))
        add=int(gdb.parse_and_eval('((unsigned*)$esp)[2]'))
        name=gdb.execute('info symbol '+str(callback),to_string=True).strip().split(' ')[0]
        ConsoleReturned(gdb.newest_frame(),name,add)
        return False
ConsoleEntry('*recon_SetConsoleCtrlHandler',internal=True)
class ModuleReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,kind,requested,module):
        super().__init__(frame,internal=True);self.kind=kind;self.requested=requested;self.module=module
    def stop(self):
        state['module_probes'].append(dict(kind=self.kind,requested=self.requested,module=self.module,result=int(self.return_value)))
        return False
class ModuleEntry(gdb.Breakpoint):
    def __init__(self,symbol,kind):super().__init__(symbol,internal=True);self.kind=kind
    def stop(self):
        args=gdb.parse_and_eval('(unsigned*)$esp')
        module=int(args[1]) if self.kind=='proc' else 0
        address=int(args[2 if self.kind=='proc' else 1])
        requested=gdb.Value(address).cast(gdb.lookup_type('char').pointer()).string()
        ModuleReturned(gdb.newest_frame(),self.kind,requested,module)
        return False
ModuleEntry('*recon_GetModuleHandleA','module')
ModuleEntry('*recon_GetProcAddress','proc')
class SystemReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,kind,address,size,before):
        super().__init__(frame,internal=True);self.kind=kind;self.address=address;self.size=size;self.before=before
    def stop(self):
        after=gdb.selected_inferior().read_memory(self.address,self.size).tobytes().hex()
        state['system_calls'].append(dict(kind=self.kind,address=self.address,before=self.before,after=after))
        return False
class SystemEntry(gdb.Breakpoint):
    def __init__(self,symbol,kind,size):super().__init__(symbol,internal=True);self.kind=kind;self.size=size
    def stop(self):
        address=int(gdb.parse_and_eval('((unsigned*)$esp)[1]'))
        before=gdb.selected_inferior().read_memory(address,self.size).tobytes().hex()
        SystemReturned(gdb.newest_frame(),self.kind,address,self.size,before)
        return False
SystemEntry('*recon_GetSystemInfo','system',36)
SystemEntry('*recon_GlobalMemoryStatus','memory',32)
class VirtualReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,arguments):super().__init__(frame,internal=True);self.arguments=arguments
    def stop(self):
        address=int(self.return_value)
        state['virtual_alloc_calls'].append(dict(arguments=self.arguments,result=address,initial=gdb.selected_inferior().read_memory(address,self.arguments[1]).tobytes().hex() if address else None))
        return False
class VirtualEntry(gdb.Breakpoint):
    def stop(self):
        arguments=[int(gdb.parse_and_eval('((unsigned*)$esp)[%d]'%i)) for i in range(1,5)]
        VirtualReturned(gdb.newest_frame(),arguments)
        return False
VirtualEntry('*recon_VirtualAlloc',internal=True)
class ParserAllocationReturned(gdb.FinishBreakpoint):
    def __init__(self,frame,size):super().__init__(frame,internal=True);self.size=size
    def stop(self):
        state['parser_allocations'].append(dict(bytes=self.size,result=int(self.return_value)))
        return False
class ParserAllocationEntry(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame()
        if frame.older() and frame.older().name()=='parse_switch':
            ParserAllocationReturned(frame,int(gdb.parse_and_eval('((unsigned*)$esp)[1]')))
        return False
ParserAllocationEntry('*allocate_memory',internal=True)

def callback_names(begin,count):
    result=[]
    for index in range(count):
        pointer=int((begin+index).dereference())
        result.append(gdb.execute('info symbol '+str(pointer),to_string=True).strip().split(' ')[0] if pointer else None)
    return result
class InitermEntry(gdb.Breakpoint):
    def stop(self):
        address=int(gdb.parse_and_eval('((unsigned*)$esp)[1]'))
        end=int(gdb.parse_and_eval('((unsigned*)$esp)[2]'))
        begin=gdb.Value(address).cast(gdb.lookup_type('ReconInitializer').pointer())
        state['initerm_calls'].append(callback_names(begin,(end-address)//4))
        return False
InitermEntry('*recon_initterm',internal=True)
class FirstEntry(gdb.Breakpoint):
    def __init__(self,logical,symbol):
        super().__init__(symbol,internal=True)
        self.logical=logical
    def stop(self):
        state['entries'].append(self.logical)
        self.enabled=False
        return False
for logical,symbol in [('_main', 'link_main'), ('_initialize_buffer12', 'initialize_buffer12'), ('_initialize_buffer8', 'initialize_buffer8'), ('_initialize_buffer4', 'initialize_buffer4'), ('_initialize_timing', 'initialize_timing'), ('_allocate_buffer12', 'allocate_buffer12'), ('_register_buffer12_cleanup', 'register_buffer12_cleanup'), ('_allocate_buffer8', 'allocate_buffer8'), ('_register_buffer8_cleanup', 'register_buffer8_cleanup'), ('_allocate_buffer4', 'allocate_buffer4'), ('_register_buffer4_cleanup', 'register_buffer4_cleanup'), ('_setup_timing', 'setup_timing'), ('_register_timing_cleanup', 'register_timing_cleanup'), ('_initialize_debug@8', 'initialize_debug'), ('_initialize_file_cache@16', 'initialize_file_cache'), ('_initialize_memory', 'initialize_memory'), ('_run_linker@8', 'run_linker'), ('_get_debug_flag@4', 'get_debug_flag'), ('_new_file_record', 'new_file_record'), ('_zero_allocate@4', 'zero_allocate'), ('_initialize_repro', 'initialize_repro'), ('_collect_file_arguments@16', 'collect_file_arguments'), ('_parse_command_line@16', 'parse_command_line'), ('_has_switch@4', 'has_switch'), ('_allocate_link_memory@8', 'allocate_link_memory'), ('_configure_link_options@12', 'configure_link_options'), ('LinkContext::Initialize', 'LinkContext::Initialize'), ('_parse_switch@8', 'parse_switch'), ('_parse_argument@8', 'parse_argument'), ('_is_libpath@4', 'is_libpath'), ('_append_argument@8', 'append_argument'), ('_append_argument_record@12', 'append_argument_record'), ('_create_string_table@12', 'create_string_table'), ('_allocate_memory@4', 'allocate_memory'), ('_initialize_hash_table@20', 'initialize_hash_table'), ('_hash_allocate@16', 'hash_allocate'), ('_reserve_memory@8', 'reserve_memory'), ('_create_memory_arena@4', 'create_memory_arena')]:
    FirstEntry(logical,symbol)
def stopped(event):
    state['signal']=getattr(event,'stop_signal',None)
    state['signal_number']=int(gdb.parse_and_eval('$_siginfo.si_signo'))
    frame=gdb.newest_frame()
    state['function']=frame.name()
    state['pc']=int(gdb.parse_and_eval('$pc'))
    state['previous_byte']=int.from_bytes(gdb.selected_inferior().read_memory(state['pc']-1,1).tobytes(),'little')
    state['x87_control_word']=int(gdb.parse_and_eval('$fctrl'))
    state['crt_fmode']=int(gdb.parse_and_eval('recon_fmode'))
    state['crt_commode']=int(gdb.parse_and_eval('recon_commode'))
    state['crt_adjust_fdiv']=int(gdb.parse_and_eval('recon_adjust_fdiv'))
    stdout_address=int(gdb.parse_and_eval('&recon_iob[1]'))
    state['crt_stdout_address']=stdout_address
    state['crt_stdout_bytes']=gdb.selected_inferior().read_memory(stdout_address,32).tobytes().hex()
    state['initializers']=[gdb.execute('info symbol '+str(int(gdb.parse_and_eval('application_initializers[%d]'%i))),to_string=True).strip().split(' ')[0] if i else None for i in range(5)]
    state['diagnostic_output']=int(gdb.parse_and_eval('diagnostic_output'))
    if state['function']=='initialize_debug':
        state['frontier_arguments']=dict(options=int(frame.read_var('options')),environment=frame.read_var('environment').string())
    if state['function']=='get_debug_flag':state['frontier_arguments']=dict(index=int(frame.read_var('index')))
    driver=frame
    while driver and driver.name()!='run_linker':driver=driver.older()
    if driver:
        count=int(driver.read_var('argc'));argv=driver.read_var('argv')
        state['frontier_arguments']=dict(argc=count,argv=[argv[i].string() for i in range(count)],argv_address=int(argv))
        state['selected_tool_name']=gdb.parse_and_eval('selected_tool_name').string()
        state['selected_tool_name_address']=int(gdb.parse_and_eval('selected_tool_name'))
        state['first_tool_name_address']=int(gdb.parse_and_eval('tool_modes[0].name'))
        state['selected_tool_kind']=int(gdb.parse_and_eval('selected_tool_kind'))
    if state['function'] in ('configure_link_options','parse_switch','duplicate_argument'):
        configuration=frame
        while configuration and configuration.name()!='configure_link_options':configuration=configuration.older()
        assert configuration
        receiver=configuration.read_var('context')
        state['context_this']=int(receiver);state['context_vxd']=int(receiver.dereference()['is_vxd'])
        state['caller_context']=int(driver.read_var('context'))
        state['link_context']=gdb.selected_inferior().read_memory(state['context_this'],664).tobytes().hex()
        state['context_extension']=receiver.dereference()['options']['default_extension'].string()
        state['configuration_arguments']=dict(context=int(receiver),first=int(configuration.read_var('module')),second=int(configuration.read_var('file')))
        state['context_auxiliary_address']=int(receiver.dereference()['auxiliary'])
        state['context_auxiliary']=gdb.selected_inferior().read_memory(state['context_auxiliary_address'],44).tobytes().hex()
        state['context_heap']=[]
        for member in ('symbols','strings'):
            table=receiver.dereference()[member];hash_state=table.dereference()['hash']['state']
            record=dict(table=int(table),table_raw=gdb.selected_inferior().read_memory(int(table),40).tobytes().hex(),hash=int(hash_state),hash_raw=gdb.selected_inferior().read_memory(int(hash_state),56).tobytes().hex(),arenas=[])
            for field in ('directory_arena','bucket_arena'):
                arena=int(hash_state.dereference()[field]);raw=gdb.selected_inferior().read_memory(arena,16).tobytes();words=struct.unpack('<4I',raw)
                record['arenas'].append(dict(address=arena,raw=raw.hex(),data=gdb.selected_inferior().read_memory(words[3],words[1]+words[2]).tobytes().hex()))
            state['context_heap'].append(record)
    elif state['function'] in ('LinkContext::Initialize','create_string_table','initialize_hash_table','hash_allocate','reserve_memory','create_memory_arena','allocate_memory'):
        owner=frame
        while owner and owner.name()!='LinkContext::Initialize':owner=owner.older()
        assert owner
        state['context_this']=int(owner.read_var('this'));state['context_vxd']=int(owner.read_var('vxd'))
        state['caller_context']=int(driver.read_var('context'))
        state['link_context']=gdb.selected_inferior().read_memory(state['context_this'],664).tobytes().hex()
        if state['function']!='LinkContext::Initialize':state['context_extension']=owner.read_var('this').dereference()['options']['default_extension'].string()
    if state['function']=='create_string_table':
        state['table_arguments']=dict(table=int(frame.read_var('table')),buckets=int(frame.read_var('buckets')),capacity=int(frame.read_var('capacity')))
    if state['function'] in ('initialize_hash_table','hash_allocate','reserve_memory','create_memory_arena','allocate_memory'):
        if state['function']=='initialize_hash_table':state['hash_arguments']=dict(hash=int(frame.read_var('hash')),buckets=int(frame.read_var('buckets')),capacity=int(frame.read_var('capacity')),key_name=int(frame.read_var('key_name')),flags=int(frame.read_var('flags')))
        state['string_table_address']=int(owner.read_var('this').dereference()['symbols'])
        state['string_table']=gdb.selected_inferior().read_memory(state['string_table_address'],40).tobytes().hex()
        state['string_key_name_address']=int(gdb.lookup_global_symbol('string_key_name').value().address)
    if state['function'] in ('hash_allocate','reserve_memory','create_memory_arena','allocate_memory'):
        allocator=frame
        while allocator and allocator.name()!='hash_allocate':allocator=allocator.older()
        assert allocator
        state['hash_state_address']=int(allocator.read_var('state'))
        state['hash_storage_arguments']=dict(state=state['hash_state_address'],arena=int(allocator.read_var('arena')),count=int(allocator.read_var('count')),size=int(allocator.read_var('size')))
        if state['function'] in ('reserve_memory','create_memory_arena','allocate_memory'):
            reserver=frame
            while reserver and reserver.name()!='reserve_memory':reserver=reserver.older()
            assert reserver
            state['arena_arguments']=dict(arena=int(reserver.read_var('arena')),bytes=int(reserver.read_var('bytes')),owner=int(reserver.read_var('arena').dereference()))
        if state['function'] in ('create_memory_arena','allocate_memory'):
            creator=frame
            while creator and creator.name()!='create_memory_arena':creator=creator.older()
            assert creator
            state['arena_creation_bytes']=int(creator.read_var('bytes'))
        if state['function']=='allocate_memory':state['allocation_bytes']=int(frame.read_var('bytes'))
        state['hash_state']=gdb.selected_inferior().read_memory(state['hash_state_address'],56).tobytes().hex()
    state['string_key_name_address']=int(gdb.lookup_global_symbol('string_key_name').value().address)
    state['member_pointers']={}
    for member in ('image_header_size','image_write_section','image_write_headers'):
        value=gdb.parse_and_eval(member);address=int(gdb.parse_and_eval('&'+member))
        state['member_pointers'][member]=dict(bytes=value.type.sizeof,raw=gdb.selected_inferior().read_memory(address,value.type.sizeof).tobytes().hex())
    state['member_methods']={};state['typed_member_values']={}
    for method in ('PeHeaderSize','VxdHeaderSize','PeWriteSection','VxdWriteSection','PeWriteHeaders','VxdWriteHeaders'):
        name='LinkContext::'+method
        symbol=gdb.lookup_global_symbol(name)
        assert symbol
        state['member_methods'][name]=int(symbol.value().address)
        state['typed_member_values'][name]=gdb.parse_and_eval('&'+name).bytes.hex()
    state['default_dos_image_address']=int(gdb.parse_and_eval('&default_dos_image'))
    state['default_dos_image']=gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('&default_dos_image')),128).tobytes().hex()
    state['option_arguments_address']=int(gdb.parse_and_eval('&option_arguments'))
    state['option_arguments']=[int(gdb.parse_and_eval('option_arguments.'+name)) for name in ('first','last','count')]
    if state['option_arguments'][0]:state['option_node']=gdb.selected_inferior().read_memory(state['option_arguments'][0],24).tobytes().hex()
    if state['function']=='has_switch':state['argument_detail']=dict(text=frame.read_var('name').string(),address=int(frame.read_var('name')))
    if state['function']=='parse_switch':state['argument_detail']=dict(file=int(frame.read_var('file')),text=frame.read_var('text').string(),address=int(frame.read_var('text')))
    if state['function']=='duplicate_argument':
        state['argument_detail']=dict(text=frame.read_var('text').string(),address=int(frame.read_var('text')))
        info=configuration.read_var('info')
        state['parsed_switch_address']=int(info)
        state['parsed_switch']=gdb.selected_inferior().read_memory(int(info),10).tobytes().hex()

    if state['function']=='allocate_link_memory':state['allocation_arguments']=dict(count=int(frame.read_var('count')),size=int(frame.read_var('size')))
    state['logo_pending']=int(gdb.parse_and_eval('logo_pending'))
    state['incremental_enabled']=int(gdb.parse_and_eval('incremental_enabled'))
    state['argument_tail']=int(gdb.parse_and_eval('argument_tail'))
    if state['function'] in ('parse_argument','is_libpath','append_argument','append_argument_record'):
        state['argument_detail']=dict(text=frame.read_var('text').string(),address=int(frame.read_var('text')))
        if state['function']=='parse_argument':state['argument_detail']['source']=int(frame.read_var('source'))
        if state['function'] in ('append_argument','append_argument_record'):state['argument_detail']['list']=int(frame.read_var('list'))
        if state['function']=='append_argument_record':state['argument_detail']['original_address']=int(frame.read_var('original_name'))
    state['input_prepass']=int(gdb.parse_and_eval('input_prepass'))
    state['input_files']=[int(gdb.parse_and_eval('input_files.'+name)) for name in ('first','last','count')]
    state['repro_directory']=int(gdb.parse_and_eval('repro_directory'))
    state['repro_response']=int(gdb.parse_and_eval('repro_response'))
    if state['function'] in ('collect_file_arguments','parse_command_line'):
        state['environment_arguments']=[frame.read_var('before').string(),frame.read_var('after').string()]
    if state['function']=='zero_allocate':state['frontier_arguments']=dict(bytes=int(frame.read_var('bytes')))
    if int(gdb.parse_and_eval('program_name')):
        state['program_name_address']=int(gdb.parse_and_eval('program_name'))
        state['program_name']=gdb.parse_and_eval('program_name').string()
        owner=frame
        while owner and owner.name()!='link_main':owner=owner.older()
        assert owner
        state['basename_address']=int(owner.read_var('basename').address)
    state['allocation_page_size']=int(gdb.parse_and_eval('allocation_page_size'))
    state['maximum_physical_memory']=int(gdb.parse_and_eval('maximum_physical_memory'))
    state['file_globals']={name:int(gdb.parse_and_eval(name)) for name in ('file_slots','file_newest','file_record_count','file_window_limit','file_mapping_enabled')}
    if state['file_globals']['file_slots']:
        raw_slots=gdb.selected_inferior().read_memory(state['file_globals']['file_slots'],64).tobytes()
        state['file_slots_bytes']=raw_slots.hex()
        slots=[int.from_bytes(raw_slots[i:i+4],'little') for i in range(0,64,4)]
        state['file_records']=[gdb.selected_inferior().read_memory(pointer,40).tobytes().hex() if pointer else None for pointer in slots]
    state['file_statistics']=[int(gdb.parse_and_eval('file_statistics[%d]'%i)) for i in range(15)]
    state['debug_flags']=gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('&debug_flags')),256).tobytes().hex()
    state['app_globals']={}
    for symbol,size in [('buffer12',12),('buffer8',12),('buffer4',12),('timing',20)]:
        address=int(gdb.parse_and_eval('&'+symbol))
        state['app_globals'][symbol]=gdb.selected_inferior().read_memory(address,size).tobytes().hex()
    frame=gdb.newest_frame()
    while frame and frame.name()!='recon_initterm':frame=frame.older()
    if frame:
        frame.select()
        current=int(gdb.parse_and_eval('begin'))
        end=int(gdb.parse_and_eval('end'))
        start=int(gdb.parse_and_eval('&application_initializers[0]'))
        state['initerm_current_index']=(current-start)//4
        state['initerm_end_index']=(end-start)//4
        gdb.newest_frame().select()
    frame=gdb.newest_frame()
    while frame and frame.name()!='target_startup':frame=frame.older()
    if frame:
        frame.select()
        count=int(gdb.parse_and_eval('argc'))
        state['crt_argv_address']=int(gdb.parse_and_eval('argv'))
        state['crt_first_argument_address']=int(gdb.parse_and_eval('argv[1]'))
        state['crt_argv']=[gdb.parse_and_eval('argv[%d]'%i).string() for i in range(count)]
        gdb.newest_frame().select()
    state['stack']=[int(gdb.parse_and_eval('((unsigned*)$esp)[%d]'%i)) for i in range(32)]
def exited(event):
    state['exit_status']=getattr(event,'exit_code',None)
gdb.events.stop.connect(stopped)
gdb.events.exited.connect(exited)
gdb.execute('set pagination off')
gdb.execute('set confirm off')
gdb.execute('run /? > /root/.ai-tmp/msvc2/vc5.10/link/native-vpkxmaj5/stdout.bin 2> /root/.ai-tmp/msvc2/vc5.10/link/native-vpkxmaj5/stderr.bin')
open('/root/.ai-tmp/msvc2/vc5.10/link/native-vpkxmaj5/state.json','w').write(json.dumps(state,indent=2)+'\n')
