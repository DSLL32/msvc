break open_forced_include
run > /root/.ai-tmp/msvc2/vc1.0/c1xx/string-boundary-cn84nq1h/native-stdout.bin 2> /root/.ai-tmp/msvc2/vc1.0/c1xx/string-boundary-cn84nq1h/native-stderr.bin
disable 1
break strchr
continue
python
import gdb,json
source=int(gdb.parse_and_eval("*(unsigned*)($esp+4)"))
character=int(gdb.parse_and_eval("*(unsigned*)($esp+8)"))
entry=gdb.newest_frame().name()
entry_pc=int(gdb.parse_and_eval("$pc")); entry_library=gdb.solib_name(entry_pc)
memory=gdb.selected_inferior()
def contents(): return (gdb.Value(source).cast(gdb.lookup_type("char").pointer()).string(encoding="latin1").encode("latin1")+bytes([0])).hex()
before=contents()
end
finish
python
result=int(gdb.parse_and_eval("(unsigned)$eax"))
observed=dict(character=character,result_offset=result-source if result else -1,before=before,after=contents(),entry=entry,entry_pc=entry_pc,entry_library=entry_library,entry_requested="strchr",caller=gdb.newest_frame().name())
open('/root/.ai-tmp/msvc2/vc1.0/c1xx/string-boundary-cn84nq1h/native-observation.json',"w").write(json.dumps(observed,indent=2)+"\n")
end
