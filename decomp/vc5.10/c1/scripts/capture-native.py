#!/usr/bin/env python3
"""Capture a native INT3 frontier using Linux GDB; no original execution."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import time
from provenance import freeze, native_sources, reuse_native_sources, freeze_native_binary

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
    parser.add_argument("--negative-value", action="store_true", help="labeled negative signed-value auxiliary input")
    parser.add_argument("--reuse-build-from", type=Path, help="prior verified capture of the same ELF and owned sources")
    parser.add_argument("--pdb-reuse", type=int, choices=(0,1), default=0, help="labeled persistent-context helper reuse branch")
    args = parser.parse_args()
    assert not args.restore_probe or args.case == "minimal"
    assert not args.negative_value or args.restore_probe
    assert not args.pdb_reuse or args.restore_probe
    assert args.name.replace("-", "").replace("_", "").isalnum()
    output = SCRATCH / args.name
    output.mkdir(parents=True, exist_ok=False)
    binary = PROJECT / "build/native/c1-host"
    binary_sha = sha(binary)
    candidate_path = PROJECT / ("cases/" + args.case + "/candidate.json")
    candidate = json.loads(candidate_path.read_text())
    source_reuse = None
    if args.reuse_build_from:
        source_reuse = reuse_native_sources(args.reuse_build_from.resolve(), output / "source-snapshot.json",
                                            binary, candidate_path, PROJECT / "build/c1.dll",
                                            PROJECT / candidate.get("audit_config", "functions.json"))
    else:
        assert subprocess.run(["make", "-q", "build/native/c1-host"], cwd=PROJECT,
                              stdout=subprocess.DEVNULL).returncode == 0, "rebuild the stale native binary first"
        freeze(native_sources(), output / "source-snapshot.json")
    field_map = json.loads((PROJECT / candidate["field_map"]).read_text()) if candidate.get("field_map") else {}
    buffer_models = field_map.get("output_buffers", [])
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
 'pdb_context':{key:int(gdb.parse_and_eval(('(int)' if key=='read_mode' else '(unsigned int)')+'c1_pdb_context.'+key)) for key in ('database','names','types','minimal_rebuild','rebuild_state','read_mode')},
 'pdb_context_layout':{'address':int(gdb.parse_and_eval('(unsigned int)&c1_pdb_context')),'size':int(gdb.parse_and_eval('sizeof(c1_pdb_context)')),'members':{key:int(gdb.parse_and_eval('(char*)&c1_pdb_context.'+key+'-(char*)&c1_pdb_context')) for key in ('database','names','types','minimal_rebuild','rebuild_state','read_mode')}},
 'live_addresses':{'initialized':int(gdb.parse_and_eval('(unsigned int)&c1_data')),'bss':int(gdb.parse_and_eval('(unsigned int)&c1_bss'))},
 'initialized':{'first_call':int(gdb.parse_and_eval('c1_data.first_call'))},
 'bss':{'pdb_signature':int(gdb.parse_and_eval('c1_bss.pdb_signature')),'literal':int(gdb.parse_and_eval('c1_bss.literal.integer')),'saved_literal':int(gdb.parse_and_eval('c1_bss.saved_literal.integer')),'saved_literal_available':int(gdb.parse_and_eval('c1_bss.saved_literal_available')),'saved_data':int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data')),
        'cleanup_count':int(gdb.parse_and_eval('c1_bss.cleanup_count')),
        'cleanup_callbacks':[int(gdb.parse_and_eval('(unsigned int)c1_bss.cleanup_callbacks[%%d]'%%i)) for i in range(32)],
        'context':{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.context.'+key)) for key in ('object','value','saved_frame')},
        'current_context':{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.current_context.'+key)) for key in ('object','value','saved_frame')},
        'contexts':[{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.contexts[%%d].'%%i+key)) for key in ('object','value','saved_frame')} for i in range(6)],
        'output_buffers':[{key:int(gdb.parse_and_eval(('(int)' if key=='file_descriptor' else '(unsigned int)')+'c1_bss.output_buffers[%%d].'%%i+key)) for key in ('data','cursor','capacity','relocated','relocation_context','stream','good','mode','file_descriptor')} for i in range(BUFFER_COUNT)]},
 'snapshot':({'first_call':int(gdb.parse_and_eval('c1_bss.saved_data->first_call'))} if int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data')) else None),
 'reuse_state':int(gdb.parse_and_eval('c1_reuse_state')),
 'pass_mode':int(gdb.parse_and_eval('c1_pass_mode')),
 'initialization_callbacks':[int(gdb.parse_and_eval('(unsigned int)c1_initialization_callbacks[%%d]'%%i)) for i in range(34)],
 'native_layout':{'initialized_size':int(gdb.parse_and_eval('sizeof(c1_data)')),'bss_size':int(gdb.parse_and_eval('sizeof(c1_bss)')),
 'pdb_signature':int(gdb.parse_and_eval('(char*)&c1_bss.pdb_signature-(char*)&c1_bss')),
 'literal':int(gdb.parse_and_eval('(char*)&c1_bss.literal-(char*)&c1_bss')),'saved_literal':int(gdb.parse_and_eval('(char*)&c1_bss.saved_literal-(char*)&c1_bss')),'saved_literal_available':int(gdb.parse_and_eval('(char*)&c1_bss.saved_literal_available-(char*)&c1_bss')),'value_size':int(gdb.parse_and_eval('sizeof(c1_bss.literal)')),
 'first_call':int(gdb.parse_and_eval('(char*)&c1_data.first_call-(char*)&c1_data')),
 'saved_data':int(gdb.parse_and_eval('(char*)&c1_bss.saved_data-(char*)&c1_bss')),
 'cleanup_count':int(gdb.parse_and_eval('(char*)&c1_bss.cleanup_count-(char*)&c1_bss')),
 'cleanup_callbacks':int(gdb.parse_and_eval('(char*)&c1_bss.cleanup_callbacks-(char*)&c1_bss')),
 'context':int(gdb.parse_and_eval('(char*)&c1_bss.context-(char*)&c1_bss')),
 'current_context':int(gdb.parse_and_eval('(char*)&c1_bss.current_context-(char*)&c1_bss')),
 'contexts':int(gdb.parse_and_eval('(char*)&c1_bss.contexts-(char*)&c1_bss')),
 'contexts_stride':int(gdb.parse_and_eval('sizeof(c1_bss.contexts[0])')),
 'output_buffers':int(gdb.parse_and_eval('(char*)&c1_bss.output_buffers-(char*)&c1_bss')),
 'output_buffer_count':int(gdb.parse_and_eval('sizeof(c1_bss.output_buffers)/sizeof(c1_bss.output_buffers[0])')),
 'output_buffer_size':int(gdb.parse_and_eval('sizeof(c1_bss.output_buffers[0])')),
 'buffer_members':{key:int(gdb.parse_and_eval('(char*)&c1_bss.output_buffers[0].'+key+'-(char*)&c1_bss.output_buffers[0]')) for key in ('data','cursor','capacity','relocated','relocation_context','stream','good','mode','file_descriptor')},
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
def seed_output_buffer(index):
    prefix='c1_bss.output_buffers['+str(index)+'].'
    start='((unsigned char *)c1_bss.saved_data + '+str(index*16)+')'
    gdb.execute('set var '+prefix+'data = '+start)
    gdb.execute('set var '+prefix+'cursor = '+start+' + 4')
    gdb.execute('set var '+prefix+'capacity = '+str(47+index))
    gdb.execute('set var '+prefix+'relocated = (C1BufferRelocator)initialize_state_12365')
    gdb.execute('set var '+prefix+'relocation_context = (void *)'+start)
    gdb.execute('set var '+prefix+'stream = (void *)('+start+' + 8)')
    gdb.execute('set var '+prefix+'good = 9')
    gdb.execute('set var '+prefix+'mode = '+str(165+index))
    gdb.execute('set var '+prefix+'file_descriptor = '+str(57+index))
restore_observations={'pre_seed':capture_modeled(),'buffer_constructors':{}}
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
for i in range(BUFFER_COUNT): seed_output_buffer(i)
VALUE_BEFORE_CLEAR
gdb.execute('set var c1_bss.pdb_signature = 0x13572468')
for i,key in enumerate(('database','names','types','minimal_rebuild','rebuild_state')):
    gdb.execute('set var c1_pdb_context.'+key+' = (void *)((char *)c1_bss.saved_data + '+str(i*8)+')')
gdb.execute('set var c1_pdb_context.read_mode = 37')
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
BUFFER_CONSTRUCTOR_PROBES
VALUE_CALLBACK_PROBE
break initialize_state_12663
continue
python
restore_observations['after_value_copy']=capture_modeled()
restore_observations['after_value_copy_pc']=int(gdb.parse_and_eval('$pc'))
end
break initialize_state_12664
continue
python
gdb.execute('set var c1_bss.pdb_signature = 0x24681357')
restore_observations['before_signature']=capture_modeled()
restore_observations['signature_pc']=int(gdb.parse_and_eval('$pc'))
end
break initialize_state_1266f
continue
python
restore_observations['after_signature']=capture_modeled()
end
break initialize_pdb_context_12676
continue
python
gdb.execute('set var c1_reuse_state = PDB_REUSE')
restore_observations['before_pdb_context']=capture_modeled()
restore_observations['pdb_context_pc']=int(gdb.parse_and_eval('$pc'))
end
python
open(%r,'w').write(json.dumps(restore_observations,indent=2)+'\\n')
end
continue""" % (expression, str(PROJECT / 'src/pass.c'), restore_line, str(output / 'restore-observations.json'))
        buffer_probes = []
        for model in buffer_models:
            index, rva = model["index"], model["constructor_rva"]
            buffer_probes.append("break %s\ncontinue\npython\nseed_output_buffer(%d)\nrestore_observations['buffer_constructors'][%r]={'before':capture_modeled(),'pc':int(gdb.parse_and_eval('$pc'))}\nend" %
                                 (model["constructor_native_symbol"], index, str(rva)))
        setup = setup.replace("BUFFER_CONSTRUCTOR_PROBES", "\n".join(buffer_probes))
        value_plan = candidate["restore_probe"]["value_copy"]
        def seed_value(source, cached):
            return "\n".join("gdb.execute(%r)" % ("set var c1_bss." + field + " = " + str(value))
                             for field, value in [("literal.integer", source), ("saved_literal.integer", cached),
                                                  ("saved_literal_available", value_plan["flag_before"])])
        setup = setup.replace("VALUE_BEFORE_CLEAR", seed_value(value_plan["before_clear_source"], value_plan["before_clear_cached"]))
        value_probe = "break initialize_state_12646\ncontinue\npython\n" + seed_value(value_plan["negative_source" if args.negative_value else "positive_source"], value_plan["cached_before_copy"]) + "\nrestore_observations['before_value_copy']=capture_modeled()\nrestore_observations['value_copy_pc']=int(gdb.parse_and_eval('$pc'))\nend"
        setup = setup.replace("VALUE_CALLBACK_PROBE", value_probe).replace("PDB_REUSE", str(args.pdb_reuse))
        commands = commands.replace("run\nbt\npython", setup + "\nbt\npython", 1)
    commands = commands.replace("BUFFER_COUNT", str(len(buffer_models)))
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
    freeze_native_binary(binary, binary_sha)
    (output / "candidate.json").write_bytes(candidate_path.read_bytes())
    (output / "functions.json").write_bytes((PROJECT / candidate.get("audit_config", "functions.json")).read_bytes())
    if candidate.get("functions"):
        binding = json.loads((PROJECT / "build/trial-audit-binding.json").read_text())
        assert all(sha(ROOT / path) == value for path, value in binding["inputs"].items())
        for source, target in (("trial-audit.json", "audit.json"), ("trial-audit-binding.json", "audit-binding.json")):
            (output / target).write_bytes((PROJECT / "build" / source).read_bytes())
    receipt = {"schema": 1, "kind": "native-gdb-frontier",
               "candidate_path": "cases/" + args.case + "/candidate.json",
               "probe": "restore" if args.restore_probe else "normal",
               "value_copy_sign": ("negative" if args.negative_value else "positive") if args.restore_probe else None,
               "pdb_reuse": args.pdb_reuse if args.restore_probe else None, "seconds": elapsed,
               "native_sha256": binary_sha, "build_source_reuse": source_reuse,
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
