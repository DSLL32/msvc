set pagination off
set confirm off
set startup-with-shell on
set disable-randomization off
set args /help
break main
run /help > /root/.ai-tmp/vc1.0-cvpack/fatal-help-native/help/debug-stdout 2> /root/.ai-tmp/vc1.0-cvpack/fatal-help-native/help/debug-stderr
set argv[0] = recon_pgmptr
continue
python
import gdb,json
current=gdb.newest_frame()
parameters={}
for symbol in current.block():
 if symbol.is_argument:
  value=current.read_var(symbol); kind=value.type.strip_typedefs().code
  parameters[symbol.name]=dict(null=int(value)==0) if kind==gdb.TYPE_CODE_PTR else int(value)
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
globals={}
for item in [{'name': 'show_logo', 'symbol': '_show_logo', 'original': 225284, 'size': 4, 'source': 'src/main.c'}, {'name': 'banner_pending', 'symbol': '_banner_pending', 'original': 225304, 'size': 4, 'source': 'src/main.c'}, {'name': 'cvnew', 'symbol': '_cvnew', 'original': 225316, 'size': 4, 'source': 'src/main.c'}, {'name': 'feedback', 'symbol': '_feedback', 'original': 225308, 'size': 4, 'source': 'src/main.c'}, {'name': 'pcode', 'symbol': '_pcode', 'original': 225300, 'size': 4, 'source': 'src/main.c'}, {'name': 'input_file', 'symbol': '_input_file', 'original': 209572, 'size': 4, 'source': 'src/main.c'}, {'name': 'already_packed', 'symbol': '_already_packed', 'original': 227420, 'size': 4, 'source': 'src/main.c'}, {'name': 'checksum_required', 'symbol': '_checksum_required', 'original': 227488, 'size': 4, 'source': 'src/main.c'}, {'name': 'mapped_io', 'symbol': '_mapped_io', 'original': 225320, 'size': 1, 'source': 'src/main.c'}, {'name': 'debug_present', 'symbol': '_debug_present', 'original': 227416, 'size': 1, 'source': 'src/main.c'}, {'name': 'module_count', 'symbol': '_module_count', 'original': 227448, 'size': 2, 'source': 'src/main.c'}, {'name': 'statistics_enabled', 'symbol': '_statistics_enabled', 'original': 225540, 'size': 2, 'source': 'src/main.c'}, {'name': 'executable_name', 'symbol': '_executable_name', 'original': 210504, 'size': 260, 'source': 'src/main.c'}, {'name': 'directory_data', 'symbol': '_directory_data', 'original': 208980, 'size': 4, 'source': 'src/main.c'}, {'name': 'module_data', 'symbol': '_module_data', 'original': 208984, 'size': 4, 'source': 'src/main.c'}, {'name': 'type_data', 'symbol': '_type_data', 'original': 208988, 'size': 4, 'source': 'src/main.c'}, {'name': 'symbol_data', 'symbol': '_symbol_data', 'original': 208992, 'size': 4, 'source': 'src/main.c'}, {'name': 'line_data', 'symbol': '_line_data', 'original': 208996, 'size': 4, 'source': 'src/main.c'}, {'name': 'verbose', 'symbol': '_verbose', 'original': 225288, 'size': 4, 'source': 'src/options.c'}, {'name': 'strip_symbols', 'symbol': '_strip_symbols', 'original': 225292, 'size': 4, 'source': 'src/options.c'}, {'name': 'minimum_types', 'symbol': '_minimum_types', 'original': 225296, 'size': 4, 'source': 'src/options.c'}, {'name': 'debug_filename', 'symbol': '_debug_filename', 'original': 225324, 'size': 4, 'source': 'src/options.c'}, {'name': 'diagnostics', 'symbol': '_diagnostics', 'original': 232432, 'size': 160, 'source': 'src/diagnostics.c', 'kind': 'string-pointer-array', 'provenance': 'Actual bounded indexed40-entry table shared by original warning/fatal handlers. Logical string contents matter; no pointer equality test is performed by these handlers.'}]:
 name=item["name"]
 value=gdb.parse_and_eval(name)
 if item.get("kind")=="string-pointer-array": globals[name]=[value[i].string(encoding="latin1").encode("latin1").hex() for i in range(item["size"]//4)]
 else: globals[name]=dict(null=int(value)==0) if value.type.strip_typedefs().code==gdb.TYPE_CODE_PTR else bytes(gdb.selected_inferior().read_memory(value.address,value.type.sizeof)).hex()
json.dump(dict(signal=int(signal),frames=frames,argc=argc,argv=argv,stdout=stream,state=state,parameters=parameters,globals=globals),open('/root/.ai-tmp/vc1.0-cvpack/fatal-help-native/help/gdb.json',"w"))
end
quit
