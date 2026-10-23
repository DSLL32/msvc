set pagination off
break fold_floating_binary
run > /root/.ai-tmp/msvc2/vc1.0/c1xx/floating-kind-policy-s0bb9j1b/0x100/stdout.bin 2> /root/.ai-tmp/msvc2/vc1.0/c1xx/floating-kind-policy-s0bb9j1b/0x100/stderr.bin
python
import gdb,json
assert gdb.newest_frame().name()=="fold_floating_binary"
assert int(gdb.parse_and_eval("expression->kind"))==0x53
assert int(gdb.parse_and_eval("expression->type->flags"))&0xefff==0x108
end
disable 1
break multiply_extended
continue
python
assert gdb.newest_frame().name()=="multiply_extended"
output=gdb.parse_and_eval("result")
output["low"].assign(0)
output["high"].assign(0xc0000000)
output["exponent"].assign(0x4000)
output["format"].assign(8)
end
return (int)0
python
assert gdb.newest_frame().name()=="fold_floating_binary"
gdb.parse_and_eval("expression")["type"]["flags"].assign(256)
end
disable 2
continue
python
frame=gdb.newest_frame()
assert frame.name()=="fold_floating_binary"
assert int(gdb.parse_and_eval("$_siginfo.si_signo"))==5
assert bytes(gdb.selected_inferior().read_memory(frame.pc()-1,1))==b"\xcc"
assert int(gdb.parse_and_eval("type"))==0x100
open('/root/.ai-tmp/msvc2/vc1.0/c1xx/floating-kind-policy-s0bb9j1b/0x100/observation.json',"w").write(json.dumps(dict(injected_type=256,stopped_function=gdb.newest_frame().name(),status="pass"),indent=2)+"\n")
end
