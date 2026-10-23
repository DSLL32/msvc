set pagination off
set confirm off
set startup-with-shell on
set disable-randomization off
set args /help
break main
run /help > /root/.ai-tmp/vc1.0-cvpack/startup-native-entry/debug-stdout 2> /root/.ai-tmp/vc1.0-cvpack/startup-native-entry/debug-stderr
set argv[0] = recon_pgmptr
python
import gdb,json
frames=[]
f=gdb.newest_frame()
while f:
 frames.append(f.name()); f=f.older()
signal=gdb.parse_and_eval("$_siginfo.si_signo")
argc=int(gdb.parse_and_eval("argc"))
argv=[gdb.parse_and_eval("argv[%d]"%i).string() for i in range(argc)]
fields=["_ptr","_cnt","_base","_flag","_file","_charbuf","_bufsiz","_tmpfname"]
stream={n:int(gdb.parse_and_eval("recon_iob[1]."+n)) for n in fields}
json.dump(dict(signal=int(signal),frames=frames,argc=argc,argv=argv,stdout=stream),open('/root/.ai-tmp/vc1.0-cvpack/startup-native-entry/gdb.json',"w"))
end
quit
