set pagination off
set confirm off
python
import gdb,json,struct
state={'results':[],'entries':[],'bypassed':[]};path='/root/.ai-tmp/msvc2/vc1.0/link/frontiers/omf-warning-complete-9drro91_/state.json'
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
        out=struct.unpack('<8I',bytes(gdb.selected_inferior().read_memory(words[1],32)));used=out[0]-out[2];assert 0<=used<=256
        state['entries'].append(dict(count=out[1],flags=out[3],file=out[4],buffer_size=out[6],buffered=list(bytes(gdb.selected_inferior().read_memory(out[2],used)))))
        current=gdb.newest_frame();caller.select();owner=words[3]==int(gdb.parse_and_eval('&format'))+4;current.select()
        state['boundary_entry']=dict(logical='recon_vfprintf',caller_name=caller.name(),**{'pass':int(gdb.parse_and_eval('omf_pass'))},formals=dict(stream_is_stdout=True,format=gdb.Value(words[2]).cast(gdb.lookup_type('char').pointer()).string(),variadic_is_warning_area=owner,consumed_variadic_slots=0),stream=state['entries'][-1],errors=dict(errno=int(gdb.parse_and_eval('(int)errno')),doserrno=int(gdb.parse_and_eval('recon_doserrno')),last_error=int(gdb.parse_and_eval("'win32.c'::last_error"))))
        Return(current,words[1]);return False
Enter('*recon_vfprintf')
def exited(event):state['exit']=event.exit_code;save()
gdb.events.exited.connect(exited)
end
run -lib /out:use16.lib use16.obj >/root/.ai-tmp/msvc2/vc1.0/link/frontiers/omf-warning-complete-9drro91_/stdout.bin 2>/root/.ai-tmp/msvc2/vc1.0/link/frontiers/omf-warning-complete-9drro91_/stderr.bin
quit
