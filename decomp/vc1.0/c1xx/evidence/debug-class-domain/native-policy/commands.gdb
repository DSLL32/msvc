set pagination off
break emit_debug_class
run > /root/.ai-tmp/msvc2/vc1.0/c1xx/debug-class-domain-native-policy-g7d8z1nb/stdout.bin 2> /root/.ai-tmp/msvc2/vc1.0/c1xx/debug-class-domain-native-policy-g7d8z1nb/stderr.bin
python
import gdb,json
assert gdb.newest_frame().name()=="emit_debug_class"
assert int(gdb.parse_and_eval("tag->type->flags"))==0x408
end
set var tag->type->flags = 0
disable 1
continue
python
frame=gdb.newest_frame()
assert frame.name()=="emit_debug_class", "unsupported descriptor escaped the guarded function"
assert int(gdb.parse_and_eval("$_siginfo.si_signo"))==5
assert int(gdb.parse_and_eval("tag->type->flags"))==0
pc=frame.pc();entry=int(gdb.parse_and_eval("(unsigned int)emit_debug_class"))
assert pc>entry and bytes(gdb.selected_inferior().read_memory(pc-1,1))==b"\xcc"
result={"status":"unsupported descriptor rejected before entryESP-24 length read; public reachability unknown","function":frame.name(),"signal":5,"pc_offset":pc-entry}
open('/root/.ai-tmp/msvc2/vc1.0/c1xx/debug-class-domain-native-policy-g7d8z1nb/observation.json',"w").write(json.dumps(result,indent=2)+"\n")
end
