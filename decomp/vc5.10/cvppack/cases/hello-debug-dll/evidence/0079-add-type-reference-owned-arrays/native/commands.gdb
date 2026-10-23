set pagination off
set confirm off
set disable-randomization off
handle SIGTRAP stop print nopass
python
import importlib.util
spec=importlib.util.spec_from_file_location("frozen_gdb_state",'/root/msvc2/decomp/vc5.10/cvppack/cases/hello-debug-dll/evidence/0079-add-type-reference-owned-arrays/native/gdb-state.py')
helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
helper.install('/root/msvc2/decomp/vc5.10/cvppack/cases/hello-debug-dll/evidence/0079-add-type-reference-owned-arrays/native/configuration.json')
end
run /nologo hello.dll >/root/msvc2/decomp/vc5.10/cvppack/cases/hello-debug-dll/evidence/0079-add-type-reference-owned-arrays/native/stdout 2>/root/msvc2/decomp/vc5.10/cvppack/cases/hello-debug-dll/evidence/0079-add-type-reference-owned-arrays/native/stderr
python
helper.capture('/root/msvc2/decomp/vc5.10/cvppack/cases/hello-debug-dll/evidence/0079-add-type-reference-owned-arrays/native/configuration.json','/root/msvc2/decomp/vc5.10/cvppack/cases/hello-debug-dll/evidence/0079-add-type-reference-owned-arrays/native/data.json','/root/msvc2/decomp/vc5.10/cvppack/cases/hello-debug-dll/evidence/0079-add-type-reference-owned-arrays/native/state.json')
end
quit
