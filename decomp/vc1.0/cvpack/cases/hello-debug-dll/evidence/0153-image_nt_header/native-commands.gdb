set pagination off
set confirm off
set startup-with-shell on
set disable-randomization off
set args /nologo /cvnew hello-debug.dll
break main
run /nologo /cvnew hello-debug.dll > /root/.ai-tmp/vc1.0-cvpack/0153-guard-native/hello-debug-dll/debug-stdout 2> /root/.ai-tmp/vc1.0-cvpack/0153-guard-native/hello-debug-dll/debug-stderr
set argv[0] = recon_pgmptr
python
import runpy
runpy.run_path('/root/msvc2/decomp/vc1.0/cvpack/scripts/native-debug.py')["install_freebuf_observer"]('/root/.ai-tmp/vc1.0-cvpack/0153-guard-native/hello-debug-dll/freebuf-returns.json')
end
python
import json
vector=gdb.parse_and_eval("(char **)environ");entries=[]
for i in range(16384):
 p=vector[i]
 if not int(p): break
 value=p.string(encoding="latin1");assert len(value)<=4096;entries.append(value)
else: raise RuntimeError("unterminated target environment")
assert not any(v.startswith(("GLIBC_TUNABLES=","MALLOC_"))for v in entries),"host allocator control leaked to target environment"
assert int(gdb.parse_and_eval("recon_crt_arguments_bound"))==0,"unexpected target CRT namespace"
json.dump(dict(host_controls_absent=True,unbound_CRT_uses_host_vector=True),open('/root/.ai-tmp/vc1.0-cvpack/0153-guard-native/hello-debug-dll/host-environment.json',"w"))
end
python
import runpy
runpy.run_path('/root/msvc2/decomp/vc1.0/cvpack/scripts/native-debug.py')["install_sopen_observer"]('/root/.ai-tmp/vc1.0-cvpack/0153-guard-native/hello-debug-dll/crt-returns.json','/root/.ai-tmp/vc1.0-cvpack/0153-guard-native/hello-debug-dll/debug-work',[{'phase': 1, 'name': 'hello-debug.dll', 'mode': 32770, 'share': 64, 'input': '/root/.ai-tmp/vc1.0-cvpack/0153-guard-native/hello-debug-dll/debug-work/hello-debug.dll', 'master': '/root/msvc2/decomp/vc1.0/cvpack/cases/hello-debug-dll/input/hello-debug.dll'}, {'phase': 2, 'name': 'Z:\\root\\.ai-tmp\\vc1.0-link\\coverage-capture\\work\\msvc.pdb', 'mode': 32768, 'share': 64, 'input': '/root/.ai-tmp/vc1.0-cvpack/0153-guard-native/hello-debug-dll/debug-work-drive-z/root/.ai-tmp/vc1.0-link/coverage-capture/work/msvc.pdb', 'master': '/root/msvc2/decomp/vc1.0/cvpack/cases/hello-debug-dll/input/msvc.pdb'}, {'phase': 3, 'name': 'hello-debug.dll', 'mode': 32770, 'share': 64, 'input': '/root/.ai-tmp/vc1.0-cvpack/0153-guard-native/hello-debug-dll/debug-work/hello-debug.dll', 'master': '/root/msvc2/decomp/vc1.0/cvpack/cases/runtime-variants/checksum-input.dll'}])
end
continue
python
import json
json.dump(dict(signal=0,exit=int(gdb.parse_and_eval("$_exitcode")),frames=[],parameters={},globals={},walks={},later_heap_equality=False),open('/root/.ai-tmp/vc1.0-cvpack/0153-guard-native/hello-debug-dll/gdb.json',"w"))
end
quit
