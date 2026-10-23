set pagination off
set confirm off
run
python
import gdb
frame=gdb.newest_frame()
assert frame.name()=="pack_new_types", frame.name()
assert int(frame.read_var("index"))==32
assert int(frame.read_var("kind"))==65535
assert int(frame.read_var("conversion"))==int(gdb.parse_and_eval("&symbol_conversions[32]"))
pc=int(gdb.parse_and_eval("$pc"))
assert bytes(gdb.selected_inferior().read_memory(pc-1,1))==b"\xcc"
assert gdb.parse_and_eval("$_siginfo.si_signo")==5
print("Unknown symbol kind stopped before callback read: index32, kindFFFF, owning pack_new_types frame")
end
quit
