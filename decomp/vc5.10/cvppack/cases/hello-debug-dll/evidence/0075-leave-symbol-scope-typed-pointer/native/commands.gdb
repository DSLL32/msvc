set pagination off
set confirm off
set disable-randomization off
handle SIGTRAP stop print nopass
python
import importlib.util
spec=importlib.util.spec_from_file_location("frozen_gdb_state",'/root/msvc2/decomp/vc5.10/cvppack/cases/hello-debug-dll/evidence/0075-leave-symbol-scope-typed-pointer/native/gdb-state.py')
helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
helper.install('/root/msvc2/decomp/vc5.10/cvppack/cases/hello-debug-dll/evidence/0075-leave-symbol-scope-typed-pointer/native/configuration.json')
end
run /nologo hello.dll >/root/msvc2/decomp/vc5.10/cvppack/cases/hello-debug-dll/evidence/0075-leave-symbol-scope-typed-pointer/native/stdout 2>/root/msvc2/decomp/vc5.10/cvppack/cases/hello-debug-dll/evidence/0075-leave-symbol-scope-typed-pointer/native/stderr
python
helper.capture('/root/msvc2/decomp/vc5.10/cvppack/cases/hello-debug-dll/evidence/0075-leave-symbol-scope-typed-pointer/native/configuration.json','/root/msvc2/decomp/vc5.10/cvppack/cases/hello-debug-dll/evidence/0075-leave-symbol-scope-typed-pointer/native/data.json','/root/msvc2/decomp/vc5.10/cvppack/cases/hello-debug-dll/evidence/0075-leave-symbol-scope-typed-pointer/native/state.json')
end
quit
