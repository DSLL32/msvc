#!/usr/bin/env python3
"""Capture a native INT3 frontier using Linux GDB; no original execution."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import time
from provenance import freeze, native_sources

PROJECT = Path(__file__).resolve().parents[1]
ROOT = PROJECT.parents[2]
SCRATCH = Path.home() / ".ai-tmp/msvc2/vc5.10/c1"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--name", required=True)
    parser.add_argument("--case", choices=("lifecycle", "minimal"), default="lifecycle")
    parser.add_argument("--restore-probe", action="store_true")
    args = parser.parse_args()
    assert not args.restore_probe or args.case == "minimal"
    assert args.name.replace("-", "").replace("_", "").isalnum()
    output = SCRATCH / args.name
    output.mkdir(parents=True, exist_ok=False)
    binary = PROJECT / "build/native/c1-host"
    assert subprocess.run(["make", "-q", "build/native/c1-host"], cwd=PROJECT,
                          stdout=subprocess.DEVNULL).returncode == 0, "rebuild the stale native binary first"
    freeze(native_sources(), output / "source-snapshot.json")
    binary_sha = sha(binary)
    candidate_path = PROJECT / ("cases/" + args.case + "/candidate.json")
    candidate = json.loads(candidate_path.read_text())
    import_boundary = candidate.get("expected_frontier", {}).get("kind") == "import-boundary"
    breakpoint_command = "break recon_initterm\n" if import_boundary else ""
    pass_boundary = candidate.get("expected_frontier", {}).get("kind") == "compiler-pass-entry"
    arguments = (("argc", "argv", "mode") if pass_boundary else
                 ("begin", "end") if import_boundary else ("module", "reason", "reserved"))
    arguments = tuple(candidate.get("expected_frontier", {}).get("native_args", arguments))
    commands = """set pagination off
set confirm off
%s\
run
bt
python
import gdb,json
frame=gdb.newest_frame()
args={name:int(gdb.parse_and_eval('(unsigned int)'+name)) for name in %r}
value={'symbol':frame.name(),'pc':int(gdb.parse_and_eval('$pc')),'args':args}
if frame.name()=='InvokeCompilerPass':
    value['argv']=[gdb.parse_and_eval('argv[%%d]'%%i).string() for i in range(args['argc'])]
    value['argv_terminator']=int(gdb.parse_and_eval('(unsigned int)argv[%%d]'%%args['argc']))
ancestor=frame
while ancestor and ancestor.name()!='InvokeCompilerPass': ancestor=ancestor.older()
value['invocation']=None
if ancestor:
    callargs={name:int(ancestor.read_var(name).cast(gdb.lookup_type('unsigned int'))) for name in ('argc','argv','mode')}
    value['invocation']={'symbol':ancestor.name(),'args':callargs,
      'argv':[ancestor.read_var('argv')[i].string() for i in range(callargs['argc'])],
      'argv_terminator':int(ancestor.read_var('argv')[callargs['argc']].cast(gdb.lookup_type('unsigned int')))}
caller=frame.older()
value['caller']={'symbol':caller.name(),'args':{}}
if caller.name()=='InvokeCompilerPass':
    value['caller']['args']={name:int(caller.read_var(name).cast(gdb.lookup_type('unsigned int'))) for name in ('argc','argv','mode')}
    value['caller']['argv']=[caller.read_var('argv')[i].string() for i in range(value['caller']['args']['argc'])]
    value['caller']['argv_terminator']=int(caller.read_var('argv')[value['caller']['args']['argc']].cast(gdb.lookup_type('unsigned int')))
if frame.name()=='recon_initterm':
    value['caller']['args']={name:int(caller.read_var(name).cast(gdb.lookup_type('unsigned int'))) for name in ('module','reason','reserved')}
identity=gdb.parse_and_eval('*c1_host_identity')
value['module_identity']={key:identity[key].string() for key in ('name','path','sha256')}
value['module_identity']['flags']=int(identity['flags'])
value['module_token']=int(gdb.parse_and_eval('(unsigned int)c1_host_module'))
value['compiler_state']={
 'live_addresses':{'initialized':int(gdb.parse_and_eval('(unsigned int)&c1_data')),'bss':int(gdb.parse_and_eval('(unsigned int)&c1_bss'))},
 'initialized':{'first_call':int(gdb.parse_and_eval('c1_data.first_call'))},
 'bss':{'saved_data':int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data')),
        'cleanup_count':int(gdb.parse_and_eval('c1_bss.cleanup_count')),
        'cleanup_callbacks':[int(gdb.parse_and_eval('(unsigned int)c1_bss.cleanup_callbacks[%%d]'%%i)) for i in range(32)],
        'context':{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.context.'+key)) for key in ('object','value','saved_frame')},
        'current_context':{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.current_context.'+key)) for key in ('object','value','saved_frame')},
        'contexts':[{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.contexts[%%d].'%%i+key)) for key in ('object','value','saved_frame')} for i in range(6)],
        'output_buffer':{key:int(gdb.parse_and_eval(('(int)' if key=='file_descriptor' else '(unsigned int)')+'c1_bss.output_buffer.'+key)) for key in ('data','cursor','capacity','relocated','relocation_context','stream','good','mode','file_descriptor')}},
 'snapshot':({'first_call':int(gdb.parse_and_eval('c1_bss.saved_data->first_call'))} if int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data')) else None),
 'reuse_state':int(gdb.parse_and_eval('c1_reuse_state')),
 'pass_mode':int(gdb.parse_and_eval('c1_pass_mode')),
 'initialization_callbacks':[int(gdb.parse_and_eval('(unsigned int)c1_initialization_callbacks[%%d]'%%i)) for i in range(34)],
 'native_layout':{'initialized_size':int(gdb.parse_and_eval('sizeof(c1_data)')),'bss_size':int(gdb.parse_and_eval('sizeof(c1_bss)')),
 'first_call':int(gdb.parse_and_eval('(char*)&c1_data.first_call-(char*)&c1_data')),
 'saved_data':int(gdb.parse_and_eval('(char*)&c1_bss.saved_data-(char*)&c1_bss')),
 'cleanup_count':int(gdb.parse_and_eval('(char*)&c1_bss.cleanup_count-(char*)&c1_bss')),
 'cleanup_callbacks':int(gdb.parse_and_eval('(char*)&c1_bss.cleanup_callbacks-(char*)&c1_bss')),
 'context':int(gdb.parse_and_eval('(char*)&c1_bss.context-(char*)&c1_bss')),
 'current_context':int(gdb.parse_and_eval('(char*)&c1_bss.current_context-(char*)&c1_bss')),
 'contexts':int(gdb.parse_and_eval('(char*)&c1_bss.contexts-(char*)&c1_bss')),
 'contexts_stride':int(gdb.parse_and_eval('sizeof(c1_bss.contexts[0])')),
 'output_buffer':int(gdb.parse_and_eval('(char*)&c1_bss.output_buffer-(char*)&c1_bss')),
 'output_buffer_size':int(gdb.parse_and_eval('sizeof(c1_bss.output_buffer)')),
 'buffer_members':{key:int(gdb.parse_and_eval('(char*)&c1_bss.output_buffer.'+key+'-(char*)&c1_bss.output_buffer')) for key in ('data','cursor','capacity','relocated','relocation_context','stream','good','mode','file_descriptor')},
 'context_members':{key:int(gdb.parse_and_eval('(char*)&c1_bss.context.'+key+'-(char*)&c1_bss.context')) for key in ('object','value','saved_frame')}}}
value['pass_mode']=int(gdb.parse_and_eval('c1_pass_mode'))
value['crt_attached']=int(gdb.parse_and_eval('c1_crt_attached'))
value['raw_dllmain']=int(gdb.parse_and_eval('(unsigned int)c1_raw_dllmain'))
value['win32_last_error']=int(gdb.parse_and_eval('last_error'))
value['x87_control']=int(gdb.parse_and_eval('$fctrl'))
value['onexit_begin']=int(gdb.parse_and_eval('(unsigned int)c1_onexit_begin'))
value['onexit_end']=int(gdb.parse_and_eval('(unsigned int)c1_onexit_end'))
value['onexit_first']=int(gdb.parse_and_eval('(unsigned int)*c1_onexit_begin')) if value['onexit_begin'] else None
value['crt_adjust_fdiv']=int(gdb.parse_and_eval('c1_crt_adjust_fdiv'))
value['dll_instance']=int(gdb.parse_and_eval('(unsigned int)c1_dll_instance'))
value['initializer_begin']=int(gdb.parse_and_eval('(unsigned int)&c1_initializers[0]'))
value['initializer_end']=int(gdb.parse_and_eval('(unsigned int)&c1_initializers[1]'))
value['initializer_slots']=[int(gdb.parse_and_eval('(unsigned int)c1_initializers[0]'))]
open(%r,'w').write(json.dumps(value,indent=2)+'\\n')
end
quit
""" % (breakpoint_command, arguments, str(output / "state.json"))
    if candidate.get("field_map"):
        allocation_probe = """python
import gdb,json
allocations=[]
class CompilerAllocation(gdb.Breakpoint):
    def stop(self):
        allocations.append({'size':int(gdb.parse_and_eval('size')),'pc':int(gdb.parse_and_eval('$pc'))})
        return False
CompilerAllocation('allocate_compiler_memory')
end
"""
        commands = commands.replace("run\nbt\npython", allocation_probe + "run\nbt\npython", 1)
        commands = commands.replace("open(%r,'w').write(json.dumps(value,indent=2)+'\\n')" % str(output/'state.json'),
                                    "open(%r,'w').write(json.dumps(allocations,indent=2)+'\\n')\n" % str(output/'allocation-observations.json') +
                                    "open(%r,'w').write(json.dumps(value,indent=2)+'\\n')" % str(output/'state.json'))
    if args.restore_probe:
        expression = commands.split("value['compiler_state']=", 1)[1].split("\nvalue['pass_mode']", 1)[0]
        restore_line = next(i for i, line in enumerate((PROJECT / "src/pass.c").read_text().splitlines(), 1)
                            if "c1_reuse_state = !c1_pass_mode;" in line)
        setup = """break initialize_compiler_pass
run
python
import gdb,json
def capture_modeled():
    return %s
def seed_output_buffer():
    gdb.execute('set var c1_bss.output_buffer.data = (unsigned char *)c1_bss.saved_data')
    gdb.execute('set var c1_bss.output_buffer.cursor = (unsigned char *)c1_bss.saved_data + 4')
    gdb.execute('set var c1_bss.output_buffer.capacity = 47')
    gdb.execute('set var c1_bss.output_buffer.relocated = (C1BufferRelocator)initialize_state_12365')
    gdb.execute('set var c1_bss.output_buffer.relocation_context = (void *)c1_bss.saved_data')
    gdb.execute('set var c1_bss.output_buffer.stream = (void *)((unsigned char *)c1_bss.saved_data + 8)')
    gdb.execute('set var c1_bss.output_buffer.good = 9')
    gdb.execute('set var c1_bss.output_buffer.mode = 165')
    gdb.execute('set var c1_bss.output_buffer.file_descriptor = 57')
restore_observations={'pre_seed':capture_modeled()}
gdb.execute('set var c1_bss.saved_data = (CompilerInitializedState *)malloc(6548)')
gdb.execute('set var c1_bss.saved_data->first_call = 0x13579bdf')
gdb.execute('set var c1_data.first_call = 0')
gdb.execute('set var c1_bss.cleanup_count = 7')
gdb.execute('set var c1_bss.cleanup_callbacks[0] = initialize_state_12365')
gdb.execute('set var c1_bss.cleanup_callbacks[1] = initialize_state_12377')
gdb.execute('set var c1_bss.context.object = &c1_data.first_call')
gdb.execute('set var c1_bss.context.value = 23')
gdb.execute('set var c1_bss.context.saved_frame = (void *)c1_bss.saved_data')
gdb.execute('set var c1_bss.current_context.object = &c1_data.first_call')
gdb.execute('set var c1_bss.current_context.value = 29')
gdb.execute('set var c1_bss.current_context.saved_frame = (void *)c1_bss.saved_data')
for i in range(6):
    gdb.execute('set var c1_bss.contexts[%%d].object = &c1_data.first_call'%%i)
    gdb.execute('set var c1_bss.contexts[%%d].value = %%d'%%(i,31+i))
    gdb.execute('set var c1_bss.contexts[%%d].saved_frame = (void *)c1_bss.saved_data'%%i)
seed_output_buffer()
gdb.execute('set var c1_pass_mode = 1')
gdb.execute('set var c1_reuse_state = 9')
restore_observations['before']=capture_modeled()
end
break %s:%d
continue
python
restore_observations['post_restore']=capture_modeled()
restore_observations['post_restore_pc']=int(gdb.parse_and_eval('$pc'))
restore_observations['post_restore_line']=gdb.newest_frame().find_sal().line
end
break initialize_state_12365
continue
python
gdb.execute('set var c1_bss.context.object = &c1_data.first_call')
gdb.execute('set var c1_bss.context.value = 23')
gdb.execute('set var c1_bss.context.saved_frame = (void *)c1_bss.saved_data')
restore_observations['before_callback']=capture_modeled()
restore_observations['callback_pc']=int(gdb.parse_and_eval('$pc'))
end
break initialize_state_12377
continue
python
gdb.execute('set var c1_bss.current_context.object = &c1_data.first_call')
gdb.execute('set var c1_bss.current_context.value = 29')
gdb.execute('set var c1_bss.current_context.saved_frame = (void *)c1_bss.saved_data')
restore_observations['before_current_callback']=capture_modeled()
restore_observations['current_callback_pc']=int(gdb.parse_and_eval('$pc'))
end
break initialize_state_12389
continue
python
for i in range(6):
    gdb.execute('set var c1_bss.contexts[%%d].object = &c1_data.first_call'%%i)
    gdb.execute('set var c1_bss.contexts[%%d].value = %%d'%%(i,31+i))
    gdb.execute('set var c1_bss.contexts[%%d].saved_frame = (void *)c1_bss.saved_data'%%i)
restore_observations['before_contexts_callback']=capture_modeled()
restore_observations['contexts_callback_pc']=int(gdb.parse_and_eval('$pc'))
end
break initialize_output_buffer_123ae
continue
python
seed_output_buffer()
restore_observations['before_buffer_constructor']=capture_modeled()
restore_observations['buffer_constructor_pc']=int(gdb.parse_and_eval('$pc'))
open(%r,'w').write(json.dumps(restore_observations,indent=2)+'\\n')
end
continue""" % (expression, str(PROJECT / 'src/pass.c'), restore_line, str(output / 'restore-observations.json'))
        commands = commands.replace("run\nbt\npython", setup + "\nbt\npython", 1)
    (output / "commands.gdb").write_text(commands)
    started = time.perf_counter()
    if args.case == "minimal":
        import shutil
        shutil.copy2(PROJECT / "cases/minimal/inputs/test.c", output / "test.c")
    result = subprocess.run(["gdb", "--batch", "-x", str(output / "commands.gdb"),
                             "--args", str(binary)] + (["minimal"] if args.case == "minimal" else []), cwd=output,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=15)
    elapsed = time.perf_counter() - started
    (output / "gdb.txt").write_bytes(result.stdout)
    assert result.returncode == 0
    if import_boundary:
        assert b"Breakpoint 1, recon_initterm (" in result.stdout
    else:
        assert b"Program received signal SIGTRAP" in result.stdout
    state = json.loads((output / "state.json").read_text())
    assert state["symbol"], "missing raw GDB function identity"
    assert sha(binary) == binary_sha
    (output / "candidate.json").write_bytes(candidate_path.read_bytes())
    (output / "functions.json").write_bytes((PROJECT / candidate.get("audit_config", "functions.json")).read_bytes())
    if candidate.get("functions"):
        binding = json.loads((PROJECT / "build/trial-audit-binding.json").read_text())
        assert all(sha(ROOT / path) == value for path, value in binding["inputs"].items())
        for source, target in (("trial-audit.json", "audit.json"), ("trial-audit-binding.json", "audit-binding.json")):
            (output / target).write_bytes((PROJECT / "build" / source).read_bytes())
    receipt = {"schema": 1, "kind": "native-gdb-frontier",
               "candidate_path": "cases/" + args.case + "/candidate.json",
               "probe": "restore" if args.restore_probe else "normal", "seconds": elapsed,
               "native_sha256": binary_sha,
               "rebuilt_sha256": sha(PROJECT / "build/c1.dll"),
               "candidate_sha256": sha(output / "candidate.json"),
               "functions_sha256": sha(output / "functions.json"),
               "source_hashes": {str(path.relative_to(PROJECT)): sha(path)
                                 for path in sorted((PROJECT / "src").glob("*")) if path.is_file()},
               "audit_binding_sha256": sha(output / "audit-binding.json") if (output / "audit-binding.json").exists() else None,
               "artifacts": {p.name: {"sha256": sha(p), "size": p.stat().st_size}
                             for p in sorted(output.iterdir()) if p.is_file()}}
    (output / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps({"output": str(output), "symbol": state["symbol"], "seconds": elapsed}))


if __name__ == "__main__":
    main()
