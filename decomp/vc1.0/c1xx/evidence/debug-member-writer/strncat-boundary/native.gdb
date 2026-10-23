break write_debug_member_name
run > /root/.ai-tmp/msvc2/vc1.0/c1xx/concat-boundary-f3averk1/native-stdout.bin 2> /root/.ai-tmp/msvc2/vc1.0/c1xx/concat-boundary-f3averk1/native-stderr.bin
disable 1
break *'strncat@plt'
continue
python
import gdb,json
destination=int(gdb.parse_and_eval("*(unsigned*)($esp+4)"))
source=int(gdb.parse_and_eval("*(unsigned*)($esp+8)"))
count=int(gdb.parse_and_eval("*(unsigned*)($esp+12)"))
entry=gdb.newest_frame().name()
entry_pc=int(gdb.parse_and_eval("$pc")); entry_library=gdb.solib_name(entry_pc)
memory=gdb.selected_inferior()
def contents(address):
    result=bytearray()
    for offset in range(512):
        value=bytes(memory.read_memory(address+offset,1))[0]; result.append(value)
        if not value: return result.hex()
    raise AssertionError("bounded C string exceeds512bytes")
before_destination=contents(destination); before_source=contents(source)
gdb.execute("set $concat_return = *(unsigned*)$esp")
end
stepi
python
implementation_pc=int(gdb.parse_and_eval("$pc")); implementation_library=gdb.solib_name(implementation_pc)
end
disable 2
tbreak *$concat_return
continue
python
result=int(gdb.parse_and_eval("(unsigned)$eax"))
observed=dict(count=count,returned_destination=int(result==destination),before_destination=before_destination,before_source=before_source,after_destination=contents(destination),after_source=contents(source),entry=entry,entry_pc=entry_pc,entry_library=entry_library,entry_requested="strncat@plt",implementation_pc=implementation_pc,implementation_library=implementation_library,caller=gdb.newest_frame().name())
open('/root/.ai-tmp/msvc2/vc1.0/c1xx/concat-boundary-f3averk1/native-observation.json',"w").write(json.dumps(observed,indent=2)+"\n")
end
