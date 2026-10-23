set pagination off
break emit_debug_method_list
run > /root/.ai-tmp/msvc2/vc1.0/c1xx/debug-method-list-null-native-policy-jskfyskj/stdout.bin 2> /root/.ai-tmp/msvc2/vc1.0/c1xx/debug-method-list-null-native-policy-jskfyskj/stderr.bin
python
import gdb,json
assert gdb.newest_frame().name()=="emit_debug_method_list"
assert int(gdb.parse_and_eval("overloads"))!=0
end
set var overloads = 0
disable 1
continue
python
frame=gdb.newest_frame()
assert frame.name()=="emit_debug_method_list", "NULL input escaped the guarded function"
assert int(gdb.parse_and_eval("$_siginfo.si_signo"))==5
assert int(gdb.parse_and_eval("overloads"))==0
pc=frame.pc();entry=int(gdb.parse_and_eval("(unsigned int)emit_debug_method_list"))
assert pc>entry and bytes(gdb.selected_inferior().read_memory(pc-1,1))==b"\xcc"
result={"status":"NULL overload cursor rejected before entryESP-8 read; public reachability unknown","function":frame.name(),"signal":5,"pc_offset":pc-entry}
open('/root/.ai-tmp/msvc2/vc1.0/c1xx/debug-method-list-null-native-policy-jskfyskj/observation.json',"w").write(json.dumps(result,indent=2)+"\n")
end
