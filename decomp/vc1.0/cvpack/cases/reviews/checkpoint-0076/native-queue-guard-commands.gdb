set pagination off
set confirm off
set startup-with-shell off
set disable-randomization off
break main
run
set first_type_match = (TypeMatch *)1
python
import gdb,json
try:gdb.execute("call finish_recursive_types()")
except gdb.error as error:
 assert "signaled"in str(error),str(error)
f=gdb.newest_frame()
assert int(gdb.parse_and_eval('$_siginfo.si_signo'))==5 and f.name()=='finish_recursive_types'
assert int(f.read_var('match'))==1
pc=int(gdb.parse_and_eval('$pc'));assert bytes(gdb.selected_inferior().read_memory(pc-1,1))==b'\xcc'
json.dump(dict(signal=5,function=f.name(),queue=1,scope='Native-only unsupported nonempty queue traps before dereference/free; not original-program evidence'),open('/root/.ai-tmp/vc1.0-cvpack/0076-nonempty-queue-guard/result.json','w'))
end
quit
