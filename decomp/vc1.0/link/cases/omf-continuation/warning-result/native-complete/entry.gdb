set pagination off
set confirm off
python
import gdb,json,struct
state={'results':[],'bypassed':[]};path='/root/.ai-tmp/msvc2/vc1.0/link/frontiers/omf-warning-complete-gmgghgr7/state.json'
def save():open(path,'w').write(json.dumps(state,indent=2)+'\n')
class Return(gdb.FinishBreakpoint):
    def __init__(self,frame,stream):super().__init__(frame,internal=True);self.stream=stream
    def stop(self):
        out=struct.unpack('<8I',bytes(gdb.selected_inferior().read_memory(self.stream,32)));used=out[0]-out[2];assert 0<=used<=256
        state['results'].append(dict(result=int(self.return_value),errno=int(gdb.parse_and_eval('(int)errno')),doserrno=int(gdb.parse_and_eval('recon_doserrno')),last_error=int(gdb.parse_and_eval("'win32.c'::last_error")),count=out[1],flags=out[3],file=out[4],buffer_size=out[6],buffered=list(bytes(gdb.selected_inferior().read_memory(out[2],used)))))
        save();return False
class Enter(gdb.Breakpoint):
    def stop(self):
        caller=gdb.newest_frame().older();assert caller
        if caller.name() in ('recon_printf','recon_fprintf'):state['bypassed'].append(caller.name());return False
        assert caller.name()=='omf_warning'
        esp=int(gdb.parse_and_eval('$esp'));words=struct.unpack('<4I',bytes(gdb.selected_inferior().read_memory(esp,16)))
        assert words[1]==int(gdb.parse_and_eval('&recon_iob[1]'))
        Return(gdb.newest_frame(),words[1]);return False
Enter('*recon_vfprintf')
def exited(event):state['exit']=event.exit_code;save()
gdb.events.exited.connect(exited)
end
run -lib /out:use16.lib use16.obj >/root/.ai-tmp/msvc2/vc1.0/link/frontiers/omf-warning-complete-gmgghgr7/stdout.bin 2>/root/.ai-tmp/msvc2/vc1.0/link/frontiers/omf-warning-complete-gmgghgr7/stderr.bin
quit
