
import gdb,json,struct
from pathlib import Path
out=Path('/root/.ai-tmp/msvc2/vc1.0/link/emit_ide_message-codi1b5k/state.json');events=[];active=[];terminal=[]
def val(x):return int(gdb.parse_and_eval(x))&0xffffffff
def mem(p,n):return bytes(gdb.selected_inferior().read_memory(p,n))
def words(p,n):return list(struct.unpack('<'+'I'*n,mem(p,n*4)))
def snap(event,name,count,phase):
    esp=val('$esp');probe=val('ide_test_probe');stream=words(val('&recon_iob[1]'),8);used=stream[0]-stream[2]
    assert 0<=used<=4096
    globals=None
    if gdb.lookup_global_symbol('ide_api'):globals=[val('ide_api'),val('ide_channel')]
    return dict(event=event,name=name,phase=phase,eip=val('$eip'),esp=esp,eax=val('$eax'),
        ebx=val('$ebx'),esi=val('$esi'),edi=val('$edi'),ebp=val('$ebp'),arguments=words(esp,count+1),
        stdout_file=stream,pending=mem(stream[2],used).hex() if used else '',
        errors=[val('*ide_test_errno'),val('recon_doserrno'),val("'win32.c'::last_error")],globals=globals,
        mb_cur_max=val('recon_mb_cur_max'),ctype=mem(val('recon_pctype')-2,514).hex(),
        probe_address=probe,probe=mem(probe,1968).hex(),message_inputs=mem(val('&ide_test_inputs[0]'),512).hex(),
        out_slot_address=val('&ide_test_out[32]'),out_slot=mem(val('&ide_test_out[0]'),68).hex(),
        message_values=words(val('&ide_test_values[0]'),7),message_base=val('&ide_test_inputs[0]'),
        expected_environment=val('ide_test_environment'),callback_addresses=[val('&IdeProbeChannel'),val('&IdeProbeWrite'),val('&IdeProbeAssertion')])
def caller_name(address):
    block=gdb.block_for_pc(address-1)
    while block and not block.function:block=block.superblock
    return str(block.function.name) if block else None
class Return(gdb.Breakpoint):
    def __init__(self,before,name,callback,phase):
        self.before=before;self.name=name;self.callback=callback;self.phase=phase
        super().__init__('*'+str(before['arguments'][0]),internal=True,temporary=True)
    def stop(self):
        event=('channel_return' if self.name=='IdeProbeChannel' else 'write_return') if self.callback else 'target_return'
        after=snap(event,self.name,0,self.phase);before=self.before
        assert after['eip']==before['arguments'][0] and after['esp']==before['esp']+4,'cdecl return stack'
        for reg in ['ebx','esi','edi','ebp']:assert before[reg]==after[reg],('callee saved',reg,self.name)
        if self.callback:
            assert after['eax']==(7 if self.name=='IdeProbeChannel' else before['arguments'][3]),'callback defined result'
            assert before['errors']==after['errors'],'callback error effect'
        else:assert active.pop()==self.phase
        events.append(after);return False
class Entry(gdb.Breakpoint):
    def __init__(self,name,count,callback=False):
        self.name=name;self.count=count;self.callback=callback
        super().__init__('*'+name,internal=True)
    def stop(self):
        if self.callback:
            assert active,'callback without actual target caller'
            phase=active[-1]
        elif self.name=='initialize_ide_api':phase=2
        elif self.name=='emit_ide_message':phase=3+words(val('$esp'),3)[1]
        else:phase=10
        event=('channel_entry' if self.name=='IdeProbeChannel' else 'write_entry') if self.callback else 'target_entry'
        before=snap(event,self.name,self.count,phase);events.append(before)
        if self.callback:
            expected='initialize_ide_api' if self.name=='IdeProbeChannel' else 'emit_ide_message'
            before['actual_target_caller']=caller_name(before['arguments'][0]);assert before['actual_target_caller']==expected
            if self.name=='IdeProbeChannel':assert before['arguments'][1:]==[2]
            else:assert before['arguments'][1]==7 and before['arguments'][2]==before['probe_address']+56 and before['arguments'][3]<1024
        elif self.name not in ['initialize_ide_api', 'emit_ide_message']:
            events.append(dict(event='frontier',name=self.name,phase=phase));return True
        else:active.append(phase)
        Return(before,self.name,self.callback,phase);return False
class Assertion(gdb.Breakpoint):
    def stop(self):raise AssertionError('unexpected external assertion callback')
def exited(event):terminal.append(dict(event='exit',code=event.exit_code))
gdb.events.exited.connect(exited)
gdb.execute('set pagination off');gdb.execute('set confirm off')
for name,count in [('initialize_ide_api',1),('emit_ide_message',2),('read_ide_flags',1)]:Entry(name,count)
Entry('IdeProbeChannel',1,True);Entry('IdeProbeWrite',3,True);Assertion('*IdeProbeAssertion',internal=True)
gdb.execute('run '+'/root/.ai-tmp/msvc2/vc1.0/link/emit_ide_message-codi1b5k/protocol.bin'+' >'+'/root/.ai-tmp/msvc2/vc1.0/link/emit_ide_message-codi1b5k/stdout.bin'+' 2>'+'/root/.ai-tmp/msvc2/vc1.0/link/emit_ide_message-codi1b5k/stderr.bin')
assert events and (terminal or events[-1]['event']=='frontier'),'missing terminal or frontier'
if terminal:assert terminal==[dict(event='exit',code=0)] and not active
out.write_text(json.dumps(dict(events=events,terminal=terminal),indent=2)+'\n')
