set pagination off
break search_include_directories
run > /root/.ai-tmp/msvc2/vc1.0/c1xx/include-native-policy-jzyejggl/stdout.bin 2> /root/.ai-tmp/msvc2/vc1.0/c1xx/include-native-policy-jzyejggl/stderr.bin
python
import gdb,json
assert gdb.newest_frame().name()=="search_include_directories"
assert int(gdb.parse_and_eval("options.include_directories->count"))>=2
assert int(gdb.parse_and_eval("*(char*)options.include_directories->items[0]"))==0
end
disable 1
continue
python
frame=gdb.newest_frame()
assert frame.name()=="search_include_directories", "empty directory escaped the guarded function"
assert int(gdb.parse_and_eval("$_siginfo.si_signo"))==5
assert int(gdb.parse_and_eval("directory"))!=0 and int(gdb.parse_and_eval("*directory"))==0
pc=frame.pc();entry=int(gdb.parse_and_eval("(unsigned int)search_include_directories"))
assert pc>entry and bytes(gdb.selected_inferior().read_memory(pc-1,1))==b"\xcc"
result={"status":"valid public empty-I native path rejected","function":frame.name(),"signal":5,"pc_offset":pc-entry}
open('/root/.ai-tmp/msvc2/vc1.0/c1xx/include-native-policy-jzyejggl/observation.json',"w").write(json.dumps(result,indent=2)+"\n")
end
