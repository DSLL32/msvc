break add_inline_pointer
run > /root/.ai-tmp/vc1.0-c1xx/copy-boundary-ruqaepb9/native-stdout.bin 2> /root/.ai-tmp/vc1.0-c1xx/copy-boundary-ruqaepb9/native-stderr.bin
disable 1
break memmove
continue
python
import gdb,json
destination=int(gdb.parse_and_eval("*(unsigned*)($esp+4)"))
source=int(gdb.parse_and_eval("*(unsigned*)($esp+8)"))
size=int(gdb.parse_and_eval("*(unsigned*)($esp+12)"))
entry=gdb.newest_frame().name()
memory=gdb.selected_inferior()
before=bytes(memory.read_memory(source,size)).hex()
def names(data): return [gdb.Value(int.from_bytes(bytes.fromhex(data)[i:i+4],"little")).cast(gdb.lookup_type("CppSymbol").pointer())["name"].string() for i in range(0,len(bytes.fromhex(data)),4)]
before_names=names(before)
end
finish
python
result=int(gdb.parse_and_eval("(unsigned)$eax"))
after=bytes(memory.read_memory(destination,size)).hex()
observed=dict(size=size,delta=destination-source,returned_destination=int(result==destination),before=before,after=after,before_names=before_names,after_names=names(after),entry=entry,caller=gdb.newest_frame().name())
open('/root/.ai-tmp/vc1.0-c1xx/copy-boundary-ruqaepb9/native-observation.json',"w").write(json.dumps(observed,indent=2)+"\n")
end
