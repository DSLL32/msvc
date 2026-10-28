set pagination off
set confirm off
start
break _setjmp
continue
python
import gdb
caller=gdb.newest_frame().older().name()
assert caller=="initial_save",caller
print("LIVE CALLER PROOF:",caller)
end
quit
