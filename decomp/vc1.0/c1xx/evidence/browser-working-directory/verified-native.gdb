break recon_getcwd
run > /root/.ai-tmp/msvc2/vc1.0/c1xx/cwd-boundary-y6pd4lnl/native-stdout.bin 2> /root/.ai-tmp/msvc2/vc1.0/c1xx/cwd-boundary-y6pd4lnl/native-stderr.bin
python
import gdb,json
memory=gdb.selected_inferior()
buffer=int(gdb.parse_and_eval("out")); capacity=int(gdb.parse_and_eval("capacity"))
caller=gdb.newest_frame().older().name()
def state():
    return dict(errno=int(gdb.parse_and_eval("(int)errno")),doserrno=int(gdb.parse_and_eval("recon_doserrno")),last_error=int(gdb.parse_and_eval("'win32.c'::last_error")),control=int(gdb.parse_and_eval("$fctrl"))&65535,status=int(gdb.parse_and_eval("$fstat"))&65535,tag=int(gdb.parse_and_eval("$ftag"))&65535,next=int(gdb.parse_and_eval("compiler_arena.pools[2].next")),remaining=int(gdb.parse_and_eval("compiler_arena.pools[2].remaining")),bytes=bytes(memory.read_memory(buffer,256)).hex())
before=state()
end
finish
python
observed=dict(buffer=buffer,capacity=capacity,caller=caller,returned_buffer=int(int(gdb.parse_and_eval("(unsigned)$eax"))==buffer),before=before,after=state())
open('/root/.ai-tmp/msvc2/vc1.0/c1xx/cwd-boundary-y6pd4lnl/native-observation.json',"w").write(json.dumps(observed,indent=2)+"\n")
end
