set pagination off
set confirm off
set startup-with-shell on
set disable-randomization off
set args /nologo /cvnew hello-debug.dll
break main
run /nologo /cvnew hello-debug.dll > /root/.ai-tmp/vc1.0-cvpack/0099-native/hello-debug-dll/debug-stdout 2> /root/.ai-tmp/vc1.0-cvpack/0099-native/hello-debug-dll/debug-stderr
set argv[0] = recon_pgmptr
python
import runpy
runpy.run_path('/root/msvc2/decomp/vc1.0/cvpack/scripts/native-debug.py')["install_sopen_observer"]('/root/.ai-tmp/vc1.0-cvpack/0099-native/hello-debug-dll/crt-returns.json','/root/.ai-tmp/vc1.0-cvpack/0099-native/hello-debug-dll/debug-work',[{'name': 'hello-debug.dll', 'mode': 32770, 'share': 64, 'input': '/root/.ai-tmp/vc1.0-cvpack/0099-native/hello-debug-dll/debug-work/hello-debug.dll', 'master': '/root/msvc2/decomp/vc1.0/cvpack/cases/hello-debug-dll/input/hello-debug.dll'}, {'name': 'Z:\\root\\.ai-tmp\\vc1.0-link\\coverage-capture\\work\\msvc.pdb', 'mode': 32768, 'share': 64, 'input': '/root/.ai-tmp/vc1.0-cvpack/0099-native/hello-debug-dll/debug-work-drive-z/root/.ai-tmp/vc1.0-link/coverage-capture/work/msvc.pdb', 'master': '/root/msvc2/decomp/vc1.0/cvpack/cases/hello-debug-dll/input/msvc.pdb'}])
end
continue
python
import json
f=gdb.newest_frame();frames=[];frame=f
while frame:frames.append(frame.name());frame=frame.older()
parameters={r["name"]:int(f.read_var(r["name"]))for r in []}
globals={r["name"]:bytes(gdb.selected_inferior().read_memory(int(gdb.parse_and_eval("&"+r["name"])),r["size"])).hex()for r in [{'name': 'statistics_enabled', 'symbol': '_statistics_enabled', 'original': 225540, 'size': 2, 'source': 'src/main.c'}, {'name': 'strip_handle', 'symbol': '_strip_handle', 'original': 210364, 'size': 4, 'source': 'src/strip.c'}, {'name': 'strip_file_header_offset', 'symbol': '_strip_file_header_offset', 'original': 210116, 'size': 4, 'source': 'src/strip.c'}, {'name': 'strip_section_header_offset', 'symbol': '_strip_section_header_offset', 'original': 210112, 'size': 4, 'source': 'src/strip.c'}, {'name': 'strip_read_offset', 'symbol': '_strip_read_offset', 'original': 210100, 'size': 4, 'source': 'src/strip.c'}, {'name': 'strip_symbol_offset', 'symbol': '_strip_symbol_offset', 'original': 210108, 'size': 4, 'source': 'src/strip.c'}, {'name': 'strip_file_header', 'symbol': '_strip_file_header', 'original': 210344, 'size': 20, 'source': 'src/strip.c'}, {'name': 'strip_optional_header', 'symbol': '_strip_optional_header', 'original': 210120, 'size': 224, 'source': 'src/strip.c'}, {'name': 'strip_records', 'symbol': '_strip_records', 'original': 230184, 'size': 4, 'source': 'src/strip.c'}]}
json.dump(dict(signal=int(gdb.parse_and_eval("$_siginfo.si_signo")),frames=frames,parameters=parameters,globals=globals,later_heap_equality=False),open('/root/.ai-tmp/vc1.0-cvpack/0099-native/hello-debug-dll/gdb.json',"w"))
end
quit
