set pagination off
set confirm off
set startup-with-shell on
set disable-randomization off
set args /nologo /cvnew hello-debug.dll
break main
run /nologo /cvnew hello-debug.dll > /root/.ai-tmp/vc1.0-cvpack/0098-final-scalar-native/hello-debug-dll/debug-stdout 2> /root/.ai-tmp/vc1.0-cvpack/0098-final-scalar-native/hello-debug-dll/debug-stderr
set argv[0] = recon_pgmptr
python
import runpy
runpy.run_path('/root/msvc2/decomp/vc1.0/cvpack/scripts/native-debug.py')["install_sopen_observer"]('/root/.ai-tmp/vc1.0-cvpack/0098-final-scalar-native/hello-debug-dll/crt-returns.json','/root/.ai-tmp/vc1.0-cvpack/0098-final-scalar-native/hello-debug-dll/debug-work',[{'name': 'hello-debug.dll', 'mode': 32770, 'share': 64, 'input': '/root/.ai-tmp/vc1.0-cvpack/0098-final-scalar-native/hello-debug-dll/debug-work/hello-debug.dll', 'master': '/root/msvc2/decomp/vc1.0/cvpack/cases/hello-debug-dll/input/hello-debug.dll'}, {'name': 'Z:\\root\\.ai-tmp\\vc1.0-link\\coverage-capture\\work\\msvc.pdb', 'mode': 32768, 'share': 64, 'input': '/root/.ai-tmp/vc1.0-cvpack/0098-final-scalar-native/hello-debug-dll/debug-work-drive-z/root/.ai-tmp/vc1.0-link/coverage-capture/work/msvc.pdb', 'master': '/root/msvc2/decomp/vc1.0/cvpack/cases/hello-debug-dll/input/msvc.pdb'}])
runpy.run_path('/root/msvc2/decomp/vc1.0/cvpack/scripts/scalar-block-debug.py')["install"]('/root/.ai-tmp/vc1.0-cvpack/0098-final-scalar-native/hello-debug-dll/scalar-block.json')
end
continue
python
import json
json.dump(dict(signal=0,frames=[gdb.newest_frame().name()],scalar_completion=True),open('/root/.ai-tmp/vc1.0-cvpack/0098-final-scalar-native/hello-debug-dll/gdb.json',"w"))
end
quit
