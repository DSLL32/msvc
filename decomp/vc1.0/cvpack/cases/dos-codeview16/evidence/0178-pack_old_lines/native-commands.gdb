set pagination off
set confirm off
set startup-with-shell on
set disable-randomization off
set args /nologo /cvnew /verbose dos-cv.exe
break main
run /nologo /cvnew /verbose dos-cv.exe > /root/.ai-tmp/vc1.0-cvpack/0178-native/dos-codeview16/debug-stdout 2> /root/.ai-tmp/vc1.0-cvpack/0178-native/dos-codeview16/debug-stderr
set argv[0] = recon_pgmptr
python
import runpy
runpy.run_path('/root/msvc2/decomp/vc1.0/cvpack/scripts/native-debug.py')["install_sopen_observer"]('/root/.ai-tmp/vc1.0-cvpack/0178-native/dos-codeview16/crt-returns.json','/root/.ai-tmp/vc1.0-cvpack/0178-native/dos-codeview16/debug-work',[{'phase': 1, 'name': 'dos-cv.exe', 'mode': 32770, 'share': 64, 'input': '/root/.ai-tmp/vc1.0-cvpack/0178-native/dos-codeview16/debug-work/dos-cv.exe', 'master': '/root/msvc2/decomp/vc1.0/cvpack/cases/dos-codeview16/input/dos-cv.exe'}])
end
continue
python
import json
json.dump(dict(signal=0,exit=int(gdb.parse_and_eval("$_exitcode")),frames=[],parameters={},globals={},walks={},later_heap_equality=False),open('/root/.ai-tmp/vc1.0-cvpack/0178-native/dos-codeview16/gdb.json',"w"))
end
quit
