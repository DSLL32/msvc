set pagination off
set confirm off
set startup-with-shell on
set disable-randomization off
set args /nologo /cvnew /verbose cfig386.dbg
break main
run /nologo /cvnew /verbose cfig386.dbg > /root/.ai-tmp/vc1.0-cvpack/0179-native/legacy-nb02-rejected/debug-stdout 2> /root/.ai-tmp/vc1.0-cvpack/0179-native/legacy-nb02-rejected/debug-stderr
set argv[0] = recon_pgmptr
python
import runpy
runpy.run_path('/root/msvc2/decomp/vc1.0/cvpack/scripts/native-debug.py')["install_sopen_observer"]('/root/.ai-tmp/vc1.0-cvpack/0179-native/legacy-nb02-rejected/crt-returns.json','/root/.ai-tmp/vc1.0-cvpack/0179-native/legacy-nb02-rejected/debug-work',[{'phase': 1, 'name': 'cfig386.dbg', 'mode': 32770, 'share': 64, 'input': '/root/.ai-tmp/vc1.0-cvpack/0179-native/legacy-nb02-rejected/debug-work/cfig386.dbg', 'master': '/root/msvc2/decomp/vc1.0/cvpack/cases/legacy-nb02-rejected/input/cfig386.dbg'}])
end
continue
python
import json
json.dump(dict(signal=0,exit=int(gdb.parse_and_eval("$_exitcode")),frames=[],parameters={},globals={},walks={},later_heap_equality=False),open('/root/.ai-tmp/vc1.0-cvpack/0179-native/legacy-nb02-rejected/gdb.json',"w"))
end
quit
