set pagination off
break has_single_function
run > /root/.ai-tmp/msvc2/vc1.0/c1xx/single-function-native-policy-t_nknnt9/stdout.bin 2> /root/.ai-tmp/msvc2/vc1.0/c1xx/single-function-native-policy-t_nknnt9/stderr.bin
python
import gdb,json
assert gdb.newest_frame().name()=="has_single_function"
assert int(gdb.parse_and_eval("symbol"))!=0
end
set var symbol = 0
disable 1
continue
python
frame=gdb.newest_frame()
assert frame.name()=="has_single_function", "NULL input escaped the guarded function"
assert int(gdb.parse_and_eval("$_siginfo.si_signo"))==5
assert int(gdb.parse_and_eval("symbol"))==0
pc=frame.pc();entry=int(gdb.parse_and_eval("(unsigned int)has_single_function"))
assert pc>entry and bytes(gdb.selected_inferior().read_memory(pc-1,1))==b"\xcc"
result={"status":"NULL-input native path rejected; public reachability unknown","function":frame.name(),"signal":5,"pc_offset":pc-entry}
open('/root/.ai-tmp/msvc2/vc1.0/c1xx/single-function-native-policy-t_nknnt9/observation.json',"w").write(json.dumps(result,indent=2)+"\n")
end
