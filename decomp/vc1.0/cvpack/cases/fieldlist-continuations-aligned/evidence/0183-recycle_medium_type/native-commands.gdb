set pagination off
set confirm off
set startup-with-shell on
set disable-randomization off
set args /nologo /cvnew /verbose /mapio fields.dll
break main
run /nologo /cvnew /verbose /mapio fields.dll > /root/.ai-tmp/vc1.0-cvpack/0183-aligned-native/fieldlist-continuations-aligned/debug-stdout 2> /root/.ai-tmp/vc1.0-cvpack/0183-aligned-native/fieldlist-continuations-aligned/debug-stderr
set argv[0] = recon_pgmptr
python
import runpy
runpy.run_path('/root/msvc2/decomp/vc1.0/cvpack/scripts/native-debug.py')["install_sopen_observer"]('/root/.ai-tmp/vc1.0-cvpack/0183-aligned-native/fieldlist-continuations-aligned/crt-returns.json','/root/.ai-tmp/vc1.0-cvpack/0183-aligned-native/fieldlist-continuations-aligned/debug-work',[])
end
continue
python
import json
json.dump(dict(signal=0,exit=int(gdb.parse_and_eval("$_exitcode")),frames=[],parameters={},globals={},walks={},later_heap_equality=False),open('/root/.ai-tmp/vc1.0-cvpack/0183-aligned-native/fieldlist-continuations-aligned/gdb.json',"w"))
end
quit
