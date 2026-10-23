set pagination off
set confirm off
set startup-with-shell on
set disable-randomization off
set args /help
break main
run /help > /root/.ai-tmp/vc1.0-cvpack/options-help-native-state/help/debug-stdout 2> /root/.ai-tmp/vc1.0-cvpack/options-help-native-state/help/debug-stderr
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
for name in ['show_logo', 'banner_pending', 'cvnew', 'feedback', 'pcode', 'input_file', 'already_packed', 'checksum_required', 'mapped_io', 'debug_present', 'module_count', 'statistics_enabled', 'executable_name', 'directory_data', 'module_data', 'type_data', 'symbol_data', 'line_data', 'verbose', 'strip_symbols', 'minimum_types', 'debug_filename']:
 value=gdb.parse_and_eval(name)
 globals[name]=dict(null=int(value)==0) if value.type.strip_typedefs().code==gdb.TYPE_CODE_PTR else bytes(gdb.selected_inferior().read_memory(value.address,value.type.sizeof)).hex()
json.dump(dict(signal=int(signal),frames=frames,argc=argc,argv=argv,stdout=stream,state=state,parameters=parameters,globals=globals),open('/root/.ai-tmp/vc1.0-cvpack/options-help-native-state/help/gdb.json',"w"))
end
quit
