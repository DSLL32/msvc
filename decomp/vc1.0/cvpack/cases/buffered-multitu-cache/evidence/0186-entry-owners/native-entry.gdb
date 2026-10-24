set pagination off
set confirm off
set startup-with-shell off
set disable-randomization off
break main
run /nologo /cvnew /verbose cache.dll
set argv[0] = recon_pgmptr
break remove_lru_page
continue
python
import gdb,json
assert gdb.newest_frame().name()=="remove_lru_page"
roots={name:int(gdb.parse_and_eval(name))for name in("free_pages","used_pages","oldest_page","newest_page","page_count")}
nodes=[];p=roots["used_pages"]
while p:
 assert len(nodes)<16 and p not in [x["address"]for x in nodes]
 raw=bytes(gdb.selected_inferior().read_memory(p,4136));nodes.append(dict(address=p,bytes=raw.hex()));p=int.from_bytes(raw[4124:4128],"little")
assert len(nodes)==roots["page_count"]==16 and roots["free_pages"]==0
json.dump(dict(roots=roots,nodes=nodes,current_module=int(gdb.parse_and_eval("current_module->module"))),open('/root/.ai-tmp/vc1.0-cvpack/0186-entry-owners-native/entry.json',"w"),indent=2)
end
quit
