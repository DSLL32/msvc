set pagination off
set confirm off
python
import gdb,json,struct,re
state={}
path='/root/.ai-tmp/msvc2/vc1.0/link/frontiers/resource-jump-7m3qo6nq/state.json'
def save():open(path,'w').write(json.dumps(state,indent=2)+'\n')
def error_value():return int(gdb.parse_and_eval('(int)errno'))
def errors():return dict(errno=error_value(),doserrno=int(gdb.parse_and_eval('recon_doserrno')),last_error=int(gdb.parse_and_eval("'win32.c'::last_error")))
class TempnamReturn(gdb.FinishBreakpoint):
    def stop(self):
        pointer=int(self.return_value)
        state['tempnam_after']=dict(result=gdb.Value(pointer).cast(gdb.lookup_type('char').pointer()).string(),counter=int(gdb.parse_and_eval('recon_tempnam_counter')),prefix_length=int(gdb.parse_and_eval('recon_tempnam_prefix_length')),**errors())
        save();return False
class TempnamEnter(gdb.Breakpoint):
    def stop(self):
        esp=int(gdb.parse_and_eval('$esp'));args=struct.unpack('<3I',bytes(gdb.selected_inferior().read_memory(esp,12)))
        string=lambda p:gdb.Value(p).cast(gdb.lookup_type('char').pointer()).string()
        state['tempnam_before']=dict(directory=string(args[1]),prefix=string(args[2]),caller=gdb.newest_frame().older().name(),counter=int(gdb.parse_and_eval('recon_tempnam_counter')),prefix_length=int(gdb.parse_and_eval('recon_tempnam_prefix_length')),**errors())
        save();TempnamReturn(gdb.newest_frame(),internal=True);return False
TempnamEnter('*recon_tempnam')
class Enter(gdb.Breakpoint):
    def stop(self):
        esp=int(gdb.parse_and_eval('$esp'))
        args=struct.unpack('<3I',bytes(gdb.selected_inferior().read_memory(esp,12)))
        legacy=int(gdb.parse_and_eval('&resource_abort'))
        context=gdb.parse_and_eval('jump_contexts');found=[]
        while int(context):
            if int(context['legacy'])==legacy:found.append(int(context['opaque_setjmp3']))
            context=context['next']
        state['entry']=dict(buffer_is_resource=args[1]==legacy,buffer_bytes=int(gdb.parse_and_eval('sizeof(resource_abort)')),value=args[2],registered_contexts=found,caller=gdb.newest_frame().older().name(),errno=error_value())
        save();return False
class Resume(gdb.Breakpoint):
    def stop(self):
        value=int(gdb.parse_and_eval('$eax'))&0xffffffff
        state['resume']=dict(value=value,signed_value=struct.unpack('<i',struct.pack('<I',value))[0],function=gdb.newest_frame().name(),errno=error_value())
        save();return False
Enter('*recon_longjmp')
lines=gdb.execute('disassemble convert_resource_input',to_string=True).splitlines()
indexes=[i for i,line in enumerate(lines) if 'call' in line and '_setjmp@plt' in line]
assert len(indexes)==1,lines
match=re.search(r'<\+(\d+)>',lines[indexes[0]+1]);assert match
point=Resume('*convert_resource_input+'+match.group(1));point.condition='$eax != 0'
class Fatal(gdb.Breakpoint):
    def stop(self):
        esp=int(gdb.parse_and_eval('$esp'));args=struct.unpack('<4I',bytes(gdb.selected_inferior().read_memory(esp,16)))
        text=lambda p:gdb.Value(p).cast(gdb.lookup_type('char').pointer()).string()
        state['fatal']=dict(file=text(args[1]),message=args[2],first_variadic_string=text(args[3]),caller=gdb.newest_frame().older().name(),**errors());save();return False
class UnlinkReturn(gdb.FinishBreakpoint):
    def stop(self):
        state['unlink_after']=dict(result=int(self.return_value)&0xffffffff,**errors());save();return False
class Unlink(gdb.Breakpoint):
    def stop(self):
        esp=int(gdb.parse_and_eval('$esp'));args=struct.unpack('<2I',bytes(gdb.selected_inferior().read_memory(esp,8)))
        state['unlink_before']=dict(file=gdb.Value(args[1]).cast(gdb.lookup_type('char').pointer()).string(),caller=gdb.newest_frame().older().name(),**errors());save();UnlinkReturn(gdb.newest_frame(),internal=True);return False
Fatal('*fatal_message');Unlink('*recon_unlink')
def exited(event):state['exit']=event.exit_code;save()
gdb.events.exited.connect(exited)
end
run -link /nodefaultlib /entry:main /subsystem:console /debug:none /out:identity.exe test.obj identity.res >/root/.ai-tmp/msvc2/vc1.0/link/frontiers/resource-jump-7m3qo6nq/stdout.bin 2>/root/.ai-tmp/msvc2/vc1.0/link/frontiers/resource-jump-7m3qo6nq/stderr.bin
printf "RESOURCE_RECOVERED_THROUGH_FATAL\n"
quit
