break grow_virtual_entries
run > /root/.ai-tmp/vc1.0-c1xx/realloc-boundary-661hgv1d/native-stdout.bin 2> /root/.ai-tmp/vc1.0-c1xx/realloc-boundary-661hgv1d/native-stderr.bin
disable 1
break realloc
continue
python
import gdb,json
pointer=int(gdb.parse_and_eval("*(unsigned*)($esp+4)"))
size=int(gdb.parse_and_eval("*(unsigned*)($esp+8)"))
table=gdb.newest_frame().older().read_var("table")
count=int(table["entry_count"]); capacity=int(table["entry_capacity"])
memory=gdb.selected_inferior()
before=bytes(memory.read_memory(pointer,count*16)).hex()
def symbol_name(pointer): return gdb.Value(pointer).cast(gdb.lookup_type("CppSymbol").pointer())["name"].string()
before_name=symbol_name(int.from_bytes(bytes.fromhex(before)[:4],"little"))
end
finish
python
result=int(gdb.parse_and_eval("(unsigned)$eax"))
after=bytes(memory.read_memory(result,count*16)).hex() if result else ""
after_name=symbol_name(int.from_bytes(bytes.fromhex(after)[:4],"little")) if result else ""
observed=dict(size=size,count=count,capacity=capacity,result_nonnull=int(bool(result)),before=before,after=after,before_name=before_name,after_name=after_name,caller=gdb.newest_frame().name())
open('/root/.ai-tmp/vc1.0-c1xx/realloc-boundary-661hgv1d/native-observation.json',"w").write(json.dumps(observed,indent=2)+"\n")
end
