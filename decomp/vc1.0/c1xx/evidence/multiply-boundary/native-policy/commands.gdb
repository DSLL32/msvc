set pagination off
break multiplication_overflows
run > /root/.ai-tmp/msvc2/vc1.0/c1xx/multiply-native-policy-izk8gghz/stdout.bin 2> /root/.ai-tmp/msvc2/vc1.0/c1xx/multiply-native-policy-izk8gghz/stderr.bin
python
import gdb,json
assert gdb.newest_frame().name()=="multiplication_overflows"
assert int(gdb.parse_and_eval("bytes"))==4
end
set var left = 1
set var right = 4294967295
set var unsigned_arithmetic = 0
set var bytes = 3
disable 1
continue
python
frame=gdb.newest_frame()
assert frame.name()=="multiplication_overflows", "unsupported width escaped the guarded function"
assert int(gdb.parse_and_eval("$_siginfo.si_signo"))==5
assert int(gdb.parse_and_eval("bytes"))==3
pc=frame.pc();entry=int(gdb.parse_and_eval("(unsigned int)multiplication_overflows"))
assert pc>entry and bytes(gdb.selected_inferior().read_memory(pc-1,1))==b"\xcc"
result={"status":"unsupported native path rejected","function":frame.name(),"signal":5,"injected_width":3,"pc_offset":pc-entry}
open('/root/.ai-tmp/msvc2/vc1.0/c1xx/multiply-native-policy-izk8gghz/observation.json',"w").write(json.dumps(result,indent=2)+"\n")
end
