set pagination off
set confirm off
set startup-with-shell on
set disable-randomization off
set args /nologo /cvnew /verbose /mapio embedded-cpp.dll
break main
run /nologo /cvnew /verbose /mapio embedded-cpp.dll > /root/.ai-tmp/vc1.0-cvpack/0166-native/embedded-cpp-mapped/debug-stdout 2> /root/.ai-tmp/vc1.0-cvpack/0166-native/embedded-cpp-mapped/debug-stderr
set argv[0] = recon_pgmptr
python
import runpy
runpy.run_path('/root/msvc2/decomp/vc1.0/cvpack/scripts/native-debug.py')["install_mapped_open_observer"]('/root/.ai-tmp/vc1.0-cvpack/0166-native/embedded-cpp-mapped/mapped-open.json')
end
python
runpy.run_path('/root/msvc2/decomp/vc1.0/cvpack/scripts/native-debug.py')["install_mapping_observer"]('/root/.ai-tmp/vc1.0-cvpack/0166-native/embedded-cpp-mapped/mapping-returns.json')
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
json.dump(dict(host_controls_absent=True,unbound_CRT_uses_host_vector=True),open('/root/.ai-tmp/vc1.0-cvpack/0166-native/embedded-cpp-mapped/host-environment.json',"w"))
end
python
import runpy
runpy.run_path('/root/msvc2/decomp/vc1.0/cvpack/scripts/native-debug.py')["install_sopen_observer"]('/root/.ai-tmp/vc1.0-cvpack/0166-native/embedded-cpp-mapped/crt-returns.json','/root/.ai-tmp/vc1.0-cvpack/0166-native/embedded-cpp-mapped/debug-work',[])
end
continue
python
import json
json.dump(dict(signal=0,exit=int(gdb.parse_and_eval("$_exitcode")),frames=[],parameters={},globals={},walks={},later_heap_equality=False),open('/root/.ai-tmp/vc1.0-cvpack/0166-native/embedded-cpp-mapped/gdb.json',"w"))
end
quit
