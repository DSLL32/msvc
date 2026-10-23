set pagination off
set confirm off
set startup-with-shell on
set disable-randomization off
set args /nologo /cvnew hello-debug.dll
break main
run /nologo /cvnew hello-debug.dll > /root/.ai-tmp/vc1.0-cvpack/0012-buffered-native-finalguard/hello-debug-dll/debug-stdout 2> /root/.ai-tmp/vc1.0-cvpack/0012-buffered-native-finalguard/hello-debug-dll/debug-stderr
set argv[0] = recon_pgmptr
python
import runpy
runpy.run_path('/root/msvc2/decomp/vc1.0/cvpack/scripts/native-debug.py')["install_sopen_observer"]('/root/.ai-tmp/vc1.0-cvpack/0012-buffered-native-finalguard/hello-debug-dll/crt-returns.json','/root/.ai-tmp/vc1.0-cvpack/0012-buffered-native-finalguard/hello-debug-dll/debug-work','/root/msvc2/decomp/vc1.0/link/cases/native-boundaries/hello-debug-dll/before.dll')
end
continue
python
import gdb,json
current=gdb.newest_frame()
parameters={}
for symbol in current.block():
 if symbol.is_argument:
  value=current.read_var(symbol); kind=value.type.strip_typedefs().code
  if kind==gdb.TYPE_CODE_PTR:
   parameters[symbol.name]=dict(null=int(value)==0,address=int(value))
   if int(value) and value.type.target().strip_typedefs().code==gdb.TYPE_CODE_INT and value.type.target().sizeof==1: parameters[symbol.name]["string"]=value.string(encoding="latin1").encode("latin1").hex()
   for item in [{'name': 'show_logo', 'symbol': '_show_logo', 'original': 225284, 'size': 4, 'source': 'src/main.c'}, {'name': 'banner_pending', 'symbol': '_banner_pending', 'original': 225304, 'size': 4, 'source': 'src/main.c'}, {'name': 'cvnew', 'symbol': '_cvnew', 'original': 225316, 'size': 4, 'source': 'src/main.c'}, {'name': 'feedback', 'symbol': '_feedback', 'original': 225308, 'size': 4, 'source': 'src/main.c'}, {'name': 'pcode', 'symbol': '_pcode', 'original': 225300, 'size': 4, 'source': 'src/main.c'}, {'name': 'input_file', 'symbol': '_input_file', 'original': 209572, 'size': 4, 'source': 'src/main.c'}, {'name': 'already_packed', 'symbol': '_already_packed', 'original': 227420, 'size': 4, 'source': 'src/main.c'}, {'name': 'checksum_required', 'symbol': '_checksum_required', 'original': 227488, 'size': 4, 'source': 'src/main.c'}, {'name': 'mapped_io', 'symbol': '_mapped_io', 'original': 225320, 'size': 1, 'source': 'src/main.c'}, {'name': 'debug_present', 'symbol': '_debug_present', 'original': 227416, 'size': 1, 'source': 'src/main.c'}, {'name': 'module_count', 'symbol': '_module_count', 'original': 227448, 'size': 2, 'source': 'src/main.c'}, {'name': 'statistics_enabled', 'symbol': '_statistics_enabled', 'original': 225540, 'size': 2, 'source': 'src/main.c'}, {'name': 'executable_name', 'symbol': '_executable_name', 'original': 210504, 'size': 260, 'source': 'src/main.c'}, {'name': 'directory_data', 'symbol': '_directory_data', 'original': 208980, 'size': 4, 'source': 'src/main.c'}, {'name': 'module_data', 'symbol': '_module_data', 'original': 208984, 'size': 4, 'source': 'src/main.c'}, {'name': 'type_data', 'symbol': '_type_data', 'original': 208988, 'size': 4, 'source': 'src/main.c'}, {'name': 'symbol_data', 'symbol': '_symbol_data', 'original': 208992, 'size': 4, 'source': 'src/main.c'}, {'name': 'line_data', 'symbol': '_line_data', 'original': 208996, 'size': 4, 'source': 'src/main.c'}, {'name': 'verbose', 'symbol': '_verbose', 'original': 225288, 'size': 4, 'source': 'src/options.c'}, {'name': 'strip_symbols', 'symbol': '_strip_symbols', 'original': 225292, 'size': 4, 'source': 'src/options.c'}, {'name': 'minimum_types', 'symbol': '_minimum_types', 'original': 225296, 'size': 4, 'source': 'src/options.c'}, {'name': 'debug_filename', 'symbol': '_debug_filename', 'original': 225324, 'size': 4, 'source': 'src/options.c'}, {'name': 'diagnostics', 'symbol': '_diagnostics', 'original': 232432, 'size': 160, 'source': 'src/diagnostics.c', 'kind': 'string-pointer-array', 'provenance': 'Actual bounded indexed40-entry table shared by original warning/fatal handlers. Logical string contents matter; no pointer equality test is performed by these handlers.'}, {'name': 'free_pages', 'symbol': '_free_pages', 'original': 232696, 'size': 4, 'source': 'src/files.c'}, {'name': 'free_files', 'symbol': '_free_files', 'original': 232724, 'size': 4, 'source': 'src/files.c'}, {'name': 'file_slots', 'symbol': '_file_slots', 'original': 208896, 'size': 4, 'source': 'src/files.c'}, {'name': 'file_count', 'symbol': '_file_count', 'original': 232708, 'size': 4, 'source': 'src/files.c'}, {'name': 'page_count', 'symbol': '_page_count', 'original': 232720, 'size': 4, 'source': 'src/files.c'}, {'name': 'file_limit', 'symbol': '_file_limit', 'original': 217792, 'size': 4, 'source': 'src/files.c'}, {'name': 'open_limit', 'symbol': '_open_limit', 'original': 217796, 'size': 4, 'source': 'src/files.c'}, {'name': 'page_limit', 'symbol': '_page_limit', 'original': 232716, 'size': 4, 'source': 'src/files.c'}, {'name': 'mapped_files', 'symbol': '_mapped_files', 'original': 232744, 'size': 1, 'source': 'src/files.c'}, {'name': 'file_failure', 'symbol': '_file_failure', 'original': 232740, 'size': 4, 'source': 'src/files.c'}, {'name': 'mapping_create', 'symbol': '_mapping_create', 'original': 232676, 'size': 4, 'source': 'src/files.c'}, {'name': 'mapping_map', 'symbol': '_mapping_map', 'original': 232680, 'size': 4, 'source': 'src/files.c'}, {'name': 'mapping_unmap', 'symbol': '_mapping_unmap', 'original': 232684, 'size': 4, 'source': 'src/files.c'}, {'name': 'active_files', 'symbol': '_active_files', 'original': 232728, 'size': 4, 'source': 'src/files.c'}, {'name': 'closed_files', 'symbol': '_closed_files', 'original': 232732, 'size': 4, 'source': 'src/files.c'}, {'name': 'closed_files_tail', 'symbol': '_closed_files_tail', 'original': 232736, 'size': 4, 'source': 'src/files.c'}, {'name': 'closed_file_count', 'symbol': '_closed_file_count', 'original': 232712, 'size': 4, 'source': 'src/files.c'}]:
    address=int(gdb.parse_and_eval("&"+item["name"]))
    if address<=int(value)<address+item["size"]: parameters[symbol.name]["global"]=dict(name=item["name"],offset=int(value)-address)
   if int(value) and value.type.target().strip_typedefs().code==gdb.TYPE_CODE_FUNC: parameters[symbol.name]["function"]=gdb.block_for_pc(int(value)).function.name
  else: parameters[symbol.name]=int(value)
frames=[]
f=gdb.newest_frame()
while f:
 frames.append(f.name()); f=f.older()
f=gdb.newest_frame()
while f and f.name()!= "main": f=f.older()
if f: f.select()
signal=gdb.parse_and_eval("$_siginfo.si_signo")
argc=int(gdb.parse_and_eval("argc"))
argv=[gdb.parse_and_eval("argv[%d]"%i).string() for i in range(argc)]
fields=["_ptr","_cnt","_base","_flag","_file","_charbuf","_bufsiz","_tmpfname"]
stream={n:int(gdb.parse_and_eval("recon_iob[1]."+n)) for n in fields}
state={"feedback":int(gdb.parse_and_eval("feedback")),"stdout_osfile":int(gdb.parse_and_eval("recon_osfile[1]"))}
state["crt_errno"]=int(gdb.parse_and_eval("(int)errno"))
globals={}
for item in [{'name': 'show_logo', 'symbol': '_show_logo', 'original': 225284, 'size': 4, 'source': 'src/main.c'}, {'name': 'banner_pending', 'symbol': '_banner_pending', 'original': 225304, 'size': 4, 'source': 'src/main.c'}, {'name': 'cvnew', 'symbol': '_cvnew', 'original': 225316, 'size': 4, 'source': 'src/main.c'}, {'name': 'feedback', 'symbol': '_feedback', 'original': 225308, 'size': 4, 'source': 'src/main.c'}, {'name': 'pcode', 'symbol': '_pcode', 'original': 225300, 'size': 4, 'source': 'src/main.c'}, {'name': 'input_file', 'symbol': '_input_file', 'original': 209572, 'size': 4, 'source': 'src/main.c'}, {'name': 'already_packed', 'symbol': '_already_packed', 'original': 227420, 'size': 4, 'source': 'src/main.c'}, {'name': 'checksum_required', 'symbol': '_checksum_required', 'original': 227488, 'size': 4, 'source': 'src/main.c'}, {'name': 'mapped_io', 'symbol': '_mapped_io', 'original': 225320, 'size': 1, 'source': 'src/main.c'}, {'name': 'debug_present', 'symbol': '_debug_present', 'original': 227416, 'size': 1, 'source': 'src/main.c'}, {'name': 'module_count', 'symbol': '_module_count', 'original': 227448, 'size': 2, 'source': 'src/main.c'}, {'name': 'statistics_enabled', 'symbol': '_statistics_enabled', 'original': 225540, 'size': 2, 'source': 'src/main.c'}, {'name': 'executable_name', 'symbol': '_executable_name', 'original': 210504, 'size': 260, 'source': 'src/main.c'}, {'name': 'directory_data', 'symbol': '_directory_data', 'original': 208980, 'size': 4, 'source': 'src/main.c'}, {'name': 'module_data', 'symbol': '_module_data', 'original': 208984, 'size': 4, 'source': 'src/main.c'}, {'name': 'type_data', 'symbol': '_type_data', 'original': 208988, 'size': 4, 'source': 'src/main.c'}, {'name': 'symbol_data', 'symbol': '_symbol_data', 'original': 208992, 'size': 4, 'source': 'src/main.c'}, {'name': 'line_data', 'symbol': '_line_data', 'original': 208996, 'size': 4, 'source': 'src/main.c'}, {'name': 'verbose', 'symbol': '_verbose', 'original': 225288, 'size': 4, 'source': 'src/options.c'}, {'name': 'strip_symbols', 'symbol': '_strip_symbols', 'original': 225292, 'size': 4, 'source': 'src/options.c'}, {'name': 'minimum_types', 'symbol': '_minimum_types', 'original': 225296, 'size': 4, 'source': 'src/options.c'}, {'name': 'debug_filename', 'symbol': '_debug_filename', 'original': 225324, 'size': 4, 'source': 'src/options.c'}, {'name': 'diagnostics', 'symbol': '_diagnostics', 'original': 232432, 'size': 160, 'source': 'src/diagnostics.c', 'kind': 'string-pointer-array', 'provenance': 'Actual bounded indexed40-entry table shared by original warning/fatal handlers. Logical string contents matter; no pointer equality test is performed by these handlers.'}, {'name': 'free_pages', 'symbol': '_free_pages', 'original': 232696, 'size': 4, 'source': 'src/files.c'}, {'name': 'free_files', 'symbol': '_free_files', 'original': 232724, 'size': 4, 'source': 'src/files.c'}, {'name': 'file_slots', 'symbol': '_file_slots', 'original': 208896, 'size': 4, 'source': 'src/files.c'}, {'name': 'file_count', 'symbol': '_file_count', 'original': 232708, 'size': 4, 'source': 'src/files.c'}, {'name': 'page_count', 'symbol': '_page_count', 'original': 232720, 'size': 4, 'source': 'src/files.c'}, {'name': 'file_limit', 'symbol': '_file_limit', 'original': 217792, 'size': 4, 'source': 'src/files.c'}, {'name': 'open_limit', 'symbol': '_open_limit', 'original': 217796, 'size': 4, 'source': 'src/files.c'}, {'name': 'page_limit', 'symbol': '_page_limit', 'original': 232716, 'size': 4, 'source': 'src/files.c'}, {'name': 'mapped_files', 'symbol': '_mapped_files', 'original': 232744, 'size': 1, 'source': 'src/files.c'}, {'name': 'file_failure', 'symbol': '_file_failure', 'original': 232740, 'size': 4, 'source': 'src/files.c'}, {'name': 'mapping_create', 'symbol': '_mapping_create', 'original': 232676, 'size': 4, 'source': 'src/files.c'}, {'name': 'mapping_map', 'symbol': '_mapping_map', 'original': 232680, 'size': 4, 'source': 'src/files.c'}, {'name': 'mapping_unmap', 'symbol': '_mapping_unmap', 'original': 232684, 'size': 4, 'source': 'src/files.c'}, {'name': 'active_files', 'symbol': '_active_files', 'original': 232728, 'size': 4, 'source': 'src/files.c'}, {'name': 'closed_files', 'symbol': '_closed_files', 'original': 232732, 'size': 4, 'source': 'src/files.c'}, {'name': 'closed_files_tail', 'symbol': '_closed_files_tail', 'original': 232736, 'size': 4, 'source': 'src/files.c'}, {'name': 'closed_file_count', 'symbol': '_closed_file_count', 'original': 232712, 'size': 4, 'source': 'src/files.c'}]:
 name=item["name"]
 value=gdb.parse_and_eval(name)
 if item.get("kind")=="string-pointer-array": globals[name]=[value[i].string(encoding="latin1").encode("latin1").hex() for i in range(item["size"]//4)]
 else: globals[name]=dict(null=int(value)==0) if value.type.strip_typedefs().code==gdb.TYPE_CODE_PTR else bytes(gdb.selected_inferior().read_memory(value.address,value.type.sizeof)).hex()
walks={}
for item in [{'name': 'free_pages', 'original': 232696, 'size': 4136, 'next': 4124, 'limit': 64, 'pointer_fields': [4120, 4124, 4128, 4132]}, {'name': 'free_files', 'original': 232724, 'size': 40, 'next': 36, 'limit': 64, 'pointer_fields': [0, 20, 32, 36], 'string_fields': [0], 'mode_guard': {'offset': 4, 'mask': 8, 'value': 0, 'reason': 'Only buffered FileSlot semantics reached. Mapped handle/view normalization requires a separate typed observer before acceptance.'}, 'arrays': [{'offset': 20, 'count_offset': 28, 'count_shift': 12, 'max_count': 16384, 'when': {'offset': 4, 'mask': 8, 'value': 0}, 'sentinels': [1]}]}, {'name': 'file_slots', 'original': 208896, 'size': 4, 'next': 4294967295, 'limit': 64, 'count_original': 217792, 'count_native': 'file_limit', 'pointer_array': True}, {'name': 'active_files', 'original': 232728, 'size': 40, 'next': 36, 'limit': 64, 'pointer_fields': [0, 20, 32, 36], 'string_fields': [0], 'mode_guard': {'offset': 4, 'mask': 8, 'value': 0, 'reason': 'Only buffered FileSlot semantics reached. Mapped handle/view normalization requires a separate typed observer before acceptance.'}, 'arrays': [{'offset': 20, 'count_offset': 28, 'count_shift': 12, 'max_count': 16384, 'when': {'offset': 4, 'mask': 8, 'value': 0}, 'sentinels': [1]}]}, {'name': 'closed_files', 'original': 232732, 'size': 40, 'next': 36, 'limit': 64, 'pointer_fields': [0, 20, 32, 36], 'string_fields': [0], 'mode_guard': {'offset': 4, 'mask': 8, 'value': 0, 'reason': 'Only buffered FileSlot semantics reached. Mapped handle/view normalization requires a separate typed observer before acceptance.'}, 'arrays': [{'offset': 20, 'count_offset': 28, 'count_shift': 12, 'max_count': 16384, 'when': {'offset': 4, 'mask': 8, 'value': 0}, 'sentinels': [1]}]}]:
 name=item["name"];pointer=int(gdb.parse_and_eval(name));nodes=[]
 size=item["size"]*(int(gdb.parse_and_eval(item["count_native"])) if item.get("count_native") else 1)
 while pointer and len(nodes)<item["limit"]:
  raw=bytes(gdb.selected_inferior().read_memory(pointer,size));nodes.append(dict(address=pointer,bytes=raw.hex()))
  strings={}
  for offset in item.get("string_fields",[]):
   address=int.from_bytes(raw[offset:offset+4],"little");text=b""
   if address:
    text=gdb.Value(address).cast(gdb.lookup_type("char").pointer()).string(encoding="latin1").encode("latin1")+b"\0"
    if len(text)>1024: raise RuntimeError("unbounded object string")
   strings[str(offset)]=dict(address=address,bytes=text.hex())
  if strings: nodes[-1]["strings"]=strings
  arrays={}
  for array in item.get("arrays",[]):
   when=array["when"];flags=int.from_bytes(raw[when["offset"]:when["offset"]+4],"little")
   if flags&when["mask"]!=when["value"]: continue
   offset=array["offset"];address=int.from_bytes(raw[offset:offset+4],"little")
   count=int.from_bytes(raw[array["count_offset"]:array["count_offset"]+4],"little")>>array["count_shift"]
   if count>array["max_count"]: raise RuntimeError("unbounded object array")
   payload=bytes(gdb.selected_inferior().read_memory(address,count*4)) if address else b""
   arrays[str(offset)]=dict(address=address,bytes=payload.hex())
  if arrays: nodes[-1]["arrays"]=arrays
  pointer=0 if item["next"]==0xffffffff else int.from_bytes(raw[item["next"]:item["next"]+4],"little")
 if pointer: raise RuntimeError("unterminated object walk: "+name)
 walks[name]=nodes
json.dump(dict(signal=int(signal),frames=frames,argc=argc,argv=argv,stdout=stream,state=state,parameters=parameters,globals=globals,walks=walks),open('/root/.ai-tmp/vc1.0-cvpack/0012-buffered-native-finalguard/hello-debug-dll/gdb.json',"w"))
end
quit
