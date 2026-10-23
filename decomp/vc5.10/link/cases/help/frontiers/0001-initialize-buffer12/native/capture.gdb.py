
import gdb,json
state={'entries':[],'initerm_calls':[]}
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
for logical,symbol in [('_initialize_buffer12', 'initialize_buffer12')]:
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
    state['initializers']=[gdb.execute('info symbol '+str(int(gdb.parse_and_eval('application_initializers[%d]'%i))),to_string=True).strip().split(' ')[0] if i else None for i in range(5)]
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
        state['crt_argv']=[gdb.parse_and_eval('argv[%d]'%i).string() for i in range(count)]
        gdb.newest_frame().select()
    state['stack']=[int(gdb.parse_and_eval('((unsigned*)$esp)[%d]'%i)) for i in range(32)]
def exited(event):
    state['exit_status']=getattr(event,'exit_code',None)
gdb.events.stop.connect(stopped)
gdb.events.exited.connect(exited)
gdb.execute('set pagination off')
gdb.execute('set confirm off')
gdb.execute('run /? > /root/.ai-tmp/msvc2/vc5.10/link/native-yv55rbal/stdout.bin 2> /root/.ai-tmp/msvc2/vc5.10/link/native-yv55rbal/stderr.bin')
open('/root/.ai-tmp/msvc2/vc5.10/link/native-yv55rbal/state.json','w').write(json.dumps(state,indent=2)+'\n')
