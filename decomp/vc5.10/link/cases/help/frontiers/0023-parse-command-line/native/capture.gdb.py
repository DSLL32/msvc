
import gdb,json
state={'entries':[],'initerm_calls':[],'operator_new_calls':[],'atexit_calls':[],'console_calls':[],'module_probes':[],'system_calls':[]}
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
for logical,symbol in [('_main', 'link_main'), ('_initialize_buffer12', 'initialize_buffer12'), ('_initialize_buffer8', 'initialize_buffer8'), ('_initialize_buffer4', 'initialize_buffer4'), ('_initialize_timing', 'initialize_timing'), ('_allocate_buffer12', 'allocate_buffer12'), ('_register_buffer12_cleanup', 'register_buffer12_cleanup'), ('_allocate_buffer8', 'allocate_buffer8'), ('_register_buffer8_cleanup', 'register_buffer8_cleanup'), ('_allocate_buffer4', 'allocate_buffer4'), ('_register_buffer4_cleanup', 'register_buffer4_cleanup'), ('_setup_timing', 'setup_timing'), ('_register_timing_cleanup', 'register_timing_cleanup'), ('_initialize_debug@8', 'initialize_debug'), ('_initialize_file_cache@16', 'initialize_file_cache'), ('_initialize_memory', 'initialize_memory'), ('_run_linker@8', 'run_linker'), ('_get_debug_flag@4', 'get_debug_flag'), ('_new_file_record', 'new_file_record'), ('_zero_allocate@4', 'zero_allocate'), ('_initialize_repro', 'initialize_repro'), ('_collect_file_arguments@16', 'collect_file_arguments'), ('_parse_command_line@16', 'parse_command_line')]:
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
    if state['function'] in ('run_linker','initialize_repro','collect_file_arguments','parse_command_line','parse_argument'):
        driver=frame
        while driver and driver.name()!='run_linker':driver=driver.older()
        assert driver
        count=int(driver.read_var('argc'));argv=driver.read_var('argv')
        state['frontier_arguments']=dict(argc=count,argv=[argv[i].string() for i in range(count)],argv_address=int(argv))
        state['selected_tool_name']=gdb.parse_and_eval('selected_tool_name').string()
        state['selected_tool_name_address']=int(gdb.parse_and_eval('selected_tool_name'))
        state['first_tool_name_address']=int(gdb.parse_and_eval('tool_modes[0].name'))
        state['selected_tool_kind']=int(gdb.parse_and_eval('selected_tool_kind'))
    state['argument_tail']=int(gdb.parse_and_eval('argument_tail'))
    if state['function']=='parse_argument':
        state['argument_detail']=dict(text=frame.read_var('text').string(),source=int(frame.read_var('source')),address=int(frame.read_var('text')))
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
gdb.execute('run /? > /root/.ai-tmp/msvc2/vc5.10/link/native-d3fuh8nf/stdout.bin 2> /root/.ai-tmp/msvc2/vc5.10/link/native-d3fuh8nf/stderr.bin')
open('/root/.ai-tmp/msvc2/vc5.10/link/native-d3fuh8nf/state.json','w').write(json.dumps(state,indent=2)+'\n')
