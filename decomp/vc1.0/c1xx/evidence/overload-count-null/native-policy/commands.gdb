set pagination off
break count_emitted_overloads
run > /root/.ai-tmp/msvc2/vc1.0/c1xx/overload-count-null-native-policy-fl1q6ce6/stdout.bin 2> /root/.ai-tmp/msvc2/vc1.0/c1xx/overload-count-null-native-policy-fl1q6ce6/stderr.bin
python
import gdb,json
assert gdb.newest_frame().name()=="count_emitted_overloads"
assert int(gdb.parse_and_eval("overloads"))!=0
end
set var overloads = 0
disable 1
continue
python
frame=gdb.newest_frame()
assert frame.name()=="count_emitted_overloads", "NULL input escaped the guarded function"
assert int(gdb.parse_and_eval("$_siginfo.si_signo"))==5
assert int(gdb.parse_and_eval("overloads"))==0
pc=frame.pc();entry=int(gdb.parse_and_eval("(unsigned int)count_emitted_overloads"))
assert pc>entry and bytes(gdb.selected_inferior().read_memory(pc-1,1))==b"\xcc"
result={"status":"NULL overload cursor rejected before entryESP-8 read; public reachability unknown","function":frame.name(),"signal":5,"pc_offset":pc-entry}
open('/root/.ai-tmp/msvc2/vc1.0/c1xx/overload-count-null-native-policy-fl1q6ce6/observation.json',"w").write(json.dumps(result,indent=2)+"\n")
end
