set pagination off
set confirm off
set startup-with-shell on
set disable-randomization off
set args /nologo /unknown /cvnew /verbose /mapio dos-mapped.exe
break main
run /nologo /unknown /cvnew /verbose /mapio dos-mapped.exe > /root/.ai-tmp/vc1.0-cvpack/0185c-native/dos-mapped-growth-feedback/debug-stdout 2> /root/.ai-tmp/vc1.0-cvpack/0185c-native/dos-mapped-growth-feedback/debug-stderr
set argv[0] = recon_pgmptr
python
import runpy
runpy.run_path('/root/msvc2/decomp/vc1.0/cvpack/scripts/native-debug.py')["install_sopen_observer"]('/root/.ai-tmp/vc1.0-cvpack/0185c-native/dos-mapped-growth-feedback/crt-returns.json','/root/.ai-tmp/vc1.0-cvpack/0185c-native/dos-mapped-growth-feedback/debug-work',[])
end
continue
python
import json
json.dump(dict(signal=0,exit=int(gdb.parse_and_eval("$_exitcode")),frames=[],parameters={},globals={},walks={},later_heap_equality=False),open('/root/.ai-tmp/vc1.0-cvpack/0185c-native/dos-mapped-growth-feedback/gdb.json',"w"))
end
quit
