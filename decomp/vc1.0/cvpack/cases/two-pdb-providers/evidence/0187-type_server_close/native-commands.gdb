set pagination off
set confirm off
set startup-with-shell on
set disable-randomization off
set args /nologo /cvnew /verbose providers.dll
break main
run /nologo /cvnew /verbose providers.dll > /root/.ai-tmp/vc1.0-cvpack/0187-native/two-pdb-providers/debug-stdout 2> /root/.ai-tmp/vc1.0-cvpack/0187-native/two-pdb-providers/debug-stderr
set argv[0] = recon_pgmptr
python
import runpy
runpy.run_path('/root/msvc2/decomp/vc1.0/cvpack/scripts/native-debug.py')["install_sopen_observer"]('/root/.ai-tmp/vc1.0-cvpack/0187-native/two-pdb-providers/crt-returns.json','/root/.ai-tmp/vc1.0-cvpack/0187-native/two-pdb-providers/debug-work',[{'phase': 1, 'name': 'providers.dll', 'mode': 32770, 'share': 64, 'input': '/root/.ai-tmp/vc1.0-cvpack/0187-native/two-pdb-providers/debug-work/providers.dll', 'master': '/root/msvc2/decomp/vc1.0/cvpack/cases/two-pdb-providers/input/providers.dll'}, {'phase': 2, 'name': 'Z:\\root\\.ai-tmp\\vc1.0-cvpack\\two-pdb-build\\pdb-two.pdb', 'mode': 32768, 'share': 64, 'input': '/root/.ai-tmp/vc1.0-cvpack/0187-native/two-pdb-providers/debug-work-drive-z/root/.ai-tmp/vc1.0-cvpack/two-pdb-build/pdb-two.pdb', 'master': '/root/msvc2/decomp/vc1.0/cvpack/cases/two-pdb-providers/input/pdb-two.pdb'}, {'phase': 3, 'name': 'Z:\\root\\.ai-tmp\\vc1.0-cvpack\\two-pdb-build\\pdb-one.pdb', 'mode': 32768, 'share': 64, 'input': '/root/.ai-tmp/vc1.0-cvpack/0187-native/two-pdb-providers/debug-work-drive-z/root/.ai-tmp/vc1.0-cvpack/two-pdb-build/pdb-one.pdb', 'master': '/root/msvc2/decomp/vc1.0/cvpack/cases/two-pdb-providers/input/pdb-one.pdb'}])
end
continue
python
import json
json.dump(dict(signal=0,exit=int(gdb.parse_and_eval("$_exitcode")),frames=[],parameters={},globals={},walks={},later_heap_equality=False),open('/root/.ai-tmp/vc1.0-cvpack/0187-native/two-pdb-providers/gdb.json',"w"))
end
quit
