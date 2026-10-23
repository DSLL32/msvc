python
import gdb,json
marker_rows=[]
class MarkerReturn(gdb.FinishBreakpoint):
    def __init__(self,frame,row):
        super().__init__(frame,internal=True)
        self.row=row
    def stop(self):
        self.row["value"]=int(self.return_value)
        marker_rows.append(self.row)
        open('/root/.ai-tmp/msvc2/vc1.0/c1xx/constructor-marker-w9h0mfqf/native/markers.json',"w").write(json.dumps(marker_rows,indent=2)+"\n")
        return False
class MarkerEntry(gdb.Breakpoint):
    def __init__(self,name,argument,kind):
        super().__init__(name,internal=True)
        self.argument=argument;self.kind=kind
    def stop(self):
        frame=gdb.newest_frame()
        symbol=frame.read_var(self.argument)
        raw=bytes(gdb.selected_inferior().read_memory(int(symbol)+48,4))
        row={"kind":self.kind,"low":raw[0],"member":raw[2]}
        assert int(symbol["cpp"]["parts"]["low_flags"])==raw[0]
        assert int(symbol["cpp"]["parts"]["member_flags"])==raw[2]
        MarkerReturn(frame,row)
        return False
MarkerEntry("default_constructor_candidate","candidate","callback")
MarkerEntry("is_void_parameter_constructor","function","checker")
end
run > /root/.ai-tmp/msvc2/vc1.0/c1xx/constructor-marker-w9h0mfqf/native/stdout.bin 2> /root/.ai-tmp/msvc2/vc1.0/c1xx/constructor-marker-w9h0mfqf/native/stderr.bin
bt
python
assert gdb.newest_frame().name()=="make_constructor_access_probe"
assert int(gdb.parse_and_eval("$_siginfo.si_signo"))==5
end
