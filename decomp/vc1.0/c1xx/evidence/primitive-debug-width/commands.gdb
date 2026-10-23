set pagination off
break primitive_debug_type
run > /root/.ai-tmp/msvc2/vc1.0/c1xx/debug-width-native-policy-jowk1fpx/stdout.bin 2> /root/.ai-tmp/msvc2/vc1.0/c1xx/debug-width-native-policy-jowk1fpx/stderr.bin
python
import gdb,json
assert gdb.newest_frame().name()=="primitive_debug_type"
assert int(gdb.parse_and_eval("flags"))==0x64
end
set var flags = 0x69
disable 1
continue
python
frame=gdb.newest_frame()
assert frame.name()=="primitive_debug_type", "out-of-domain integral width escaped the guarded function"
assert int(gdb.parse_and_eval("$_siginfo.si_signo"))==5
assert int(gdb.parse_and_eval("flags"))==0x69
pc=frame.pc();entry=int(gdb.parse_and_eval("(unsigned int)primitive_debug_type"))
assert pc>entry and bytes(gdb.selected_inferior().read_memory(pc-1,1))==b"\xcc"
result={"status":"out-of-domain integral width9 rejected; public reachability/invalid-width equivalence unknown","function":frame.name(),"signal":5,"pc_offset":pc-entry}
open('/root/.ai-tmp/msvc2/vc1.0/c1xx/debug-width-native-policy-jowk1fpx/observation.json',"w").write(json.dumps(result,indent=2)+"\n")
end
