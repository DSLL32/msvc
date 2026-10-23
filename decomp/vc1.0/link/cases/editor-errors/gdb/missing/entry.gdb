set pagination off
set confirm off
python
import gdb,json,struct
state={'formatted':[]}
path='/root/.ai-tmp/msvc2/vc1.0/link/frontiers/editor-errors-missing-gdb-ohvnd6v4/state.json'
def save():open(path,'w').write(json.dumps(state,indent=2)+'\n')
class Return(gdb.FinishBreakpoint):
    def __init__(self,frame,output,capacity):
        super().__init__(frame,internal=True);self.output=output;self.capacity=capacity
    def stop(self):
        value=int(self.return_value);assert 0<=value<self.capacity
        raw=bytes(gdb.selected_inferior().read_memory(self.output,value+1));assert raw[-1]==0
        state['formatted'].append(dict(result=value,capacity=self.capacity,text=raw[:-1].decode('latin1'),errno=int(gdb.parse_and_eval('(int)errno'))));save();return False
class Enter(gdb.Breakpoint):
    def stop(self):
        esp=int(gdb.parse_and_eval('$esp'));words=struct.unpack('<5I',bytes(gdb.selected_inferior().read_memory(esp,20)))
        caller=gdb.newest_frame().older();assert caller.name() in ('editor_rebase_error','editor_bind_error')
        Return(gdb.newest_frame(),words[1],words[2]);return False
Enter('*recon_snprintf')
def exited(event):state['exit']=event.exit_code;save()
gdb.events.exited.connect(exited)
end
run -edit /section:.text,R missing.exe >/root/.ai-tmp/msvc2/vc1.0/link/frontiers/editor-errors-missing-gdb-ohvnd6v4/stdout.bin 2>/root/.ai-tmp/msvc2/vc1.0/link/frontiers/editor-errors-missing-gdb-ohvnd6v4/stderr.bin
quit
