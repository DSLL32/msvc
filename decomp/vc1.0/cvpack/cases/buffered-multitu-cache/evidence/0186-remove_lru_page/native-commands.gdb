set pagination off
set confirm off
set startup-with-shell on
set disable-randomization off
set args /nologo /cvnew /verbose cache.dll
break main
run /nologo /cvnew /verbose cache.dll > /root/.ai-tmp/vc1.0-cvpack/0186-native/buffered-multitu-cache/debug-stdout 2> /root/.ai-tmp/vc1.0-cvpack/0186-native/buffered-multitu-cache/debug-stderr
set argv[0] = recon_pgmptr
python
import runpy
runpy.run_path('/root/msvc2/decomp/vc1.0/cvpack/scripts/native-debug.py')["install_sopen_observer"]('/root/.ai-tmp/vc1.0-cvpack/0186-native/buffered-multitu-cache/crt-returns.json','/root/.ai-tmp/vc1.0-cvpack/0186-native/buffered-multitu-cache/debug-work',[{'phase': 1, 'name': 'cache.dll', 'mode': 32770, 'share': 64, 'input': '/root/.ai-tmp/vc1.0-cvpack/0186-native/buffered-multitu-cache/debug-work/cache.dll', 'master': '/root/msvc2/decomp/vc1.0/cvpack/cases/buffered-multitu-cache/input/cache.dll'}])
end
continue
python
import json
json.dump(dict(signal=0,exit=int(gdb.parse_and_eval("$_exitcode")),frames=[],parameters={},globals={},walks={},later_heap_equality=False),open('/root/.ai-tmp/vc1.0-cvpack/0186-native/buffered-multitu-cache/gdb.json',"w"))
end
quit
