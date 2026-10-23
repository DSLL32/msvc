#!/usr/bin/env python3
"""Capture a native INT3 frontier using Linux GDB; no original execution."""
import argparse
import hashlib
import re
import json
from pathlib import Path
import subprocess
import time
from provenance import freeze, native_sources, reuse_native_sources, freeze_native_binary, compiled_native_sources
from pch_state import MEMBERS as PCH_MEMBERS
from main_fields import SYSTEM_MEMBERS

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
    parser.add_argument("--prospective-tail-chain", action="store_true", help="bounded raw-only input observation, never acceptance")
    parser.add_argument("--negative-value", action="store_true", help="labeled negative signed-value auxiliary input")
    parser.add_argument("--compiled-build", type=Path, help="verified prebuild source-content manifest for this exact ELF")
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
    assert not args.prospective_tail_chain or not args.reuse_build_from
    source_reuse = None
    compiled_build = None
    assert not (args.compiled_build and args.reuse_build_from)
    if args.compiled_build:
        compiled_build=compiled_native_sources(args.compiled_build.resolve(),output/'source-snapshot.json',binary)
        (output/'compiled-build.json').write_text(json.dumps(compiled_build,indent=2)+'\n')
    elif args.reuse_build_from:
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
block=frame.block()
while block and not block.function:block=block.superblock
assert block and block.function
formals=[{'name':symbol.name,'type':str(symbol.type)} for symbol in block if symbol.is_argument]
value={'symbol':frame.name(),'pc':int(gdb.parse_and_eval('$pc')),'args':args,'formal_inventory':formals}
assert {row['name'] for row in formals}==set(args), 'omitted or invented native formal'

if frame.name() in ('InvokeCompilerPass','c1_main'):
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
value['caller']={'symbol':caller.name(),'args':{},'pc':int(caller.pc())}
if caller.name()=='InvokeCompilerPass':
    value['caller']['args']={name:int(caller.read_var(name).cast(gdb.lookup_type('unsigned int'))) for name in ('argc','argv','mode')}
    value['caller']['argv']=[caller.read_var('argv')[i].string() for i in range(value['caller']['args']['argc'])]
    value['caller']['argv_terminator']=int(caller.read_var('argv')[value['caller']['args']['argc']].cast(gdb.lookup_type('unsigned int')))
if caller.name()=='c1_main':
    value['caller']['pc']=int(caller.pc())
    value['caller']['args']={name:int(caller.read_var(name).cast(gdb.lookup_type('unsigned int'))) for name in ('argc','argv')}
if frame.name()=='recon_initterm':
    value['caller']['args']={name:int(caller.read_var(name).cast(gdb.lookup_type('unsigned int'))) for name in ('module','reason','reserved')}
identity=gdb.parse_and_eval('*c1_host_identity')
value['module_identity']={key:identity[key].string() for key in ('name','path','sha256')}
value['module_identity']['flags']=int(identity['flags'])
value['module_token']=int(gdb.parse_and_eval('(unsigned int)c1_host_module'))
value['compiler_state']={
 'pdb_context':{key:int(gdb.parse_and_eval(('(int)' if key=='read_mode' else '(unsigned int)')+'c1_pdb_context.'+key)) for key in ('database','names','types','minimal_rebuild','rebuild_state','read_mode')},
 'pdb_context_layout':{'address':int(gdb.parse_and_eval('(unsigned int)&c1_pdb_context')),'size':int(gdb.parse_and_eval('sizeof(c1_pdb_context)')),'members':{key:int(gdb.parse_and_eval('(char*)&c1_pdb_context.'+key+'-(char*)&c1_pdb_context')) for key in ('database','names','types','minimal_rebuild','rebuild_state','read_mode')}},
 'output_pdb_context':{key:int(gdb.parse_and_eval(('(int)' if key=='read_mode' else '(unsigned int)')+'c1_output_pdb_context.'+key)) for key in ('database','names','types','minimal_rebuild','rebuild_state','read_mode')},
 'output_pdb_context_layout':{'address':int(gdb.parse_and_eval('(unsigned int)&c1_output_pdb_context')),'size':int(gdb.parse_and_eval('sizeof(c1_output_pdb_context)')),'members':{key:int(gdb.parse_and_eval('(char*)&c1_output_pdb_context.'+key+'-(char*)&c1_output_pdb_context')) for key in ('database','names','types','minimal_rebuild','rebuild_state','read_mode')}},
 'live_addresses':{'initialized':int(gdb.parse_and_eval('(unsigned int)&c1_data')),'bss':int(gdb.parse_and_eval('(unsigned int)&c1_bss'))},
 'initialized':{'first_call':int(gdb.parse_and_eval('c1_data.first_call')),'compiler_phase':int(gdb.parse_and_eval('c1_data.compiler_phase')),'c9_il':int(gdb.parse_and_eval('c1_data.c9_il')),'compiler_flags':int(gdb.parse_and_eval('(unsigned int)c1_data.compiler_flags')),'system_info':{key:int(gdb.parse_and_eval('(unsigned int)c1_data.system_info.'+key)) for key in SYS_KEYS}},
 'bss':{'work_buffers':[{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.work_buffers[%%d].'%%i+key)) for key in ('data','limit')} for i in range(2)],'debug_break':int(gdb.parse_and_eval('c1_bss.debug_break')),'command_argc':int(gdb.parse_and_eval('c1_bss.command_argc')),'command_argv':int(gdb.parse_and_eval('(unsigned int)c1_bss.command_argv')),'pch_state':{key:([int(gdb.parse_and_eval('(unsigned int)c1_bss.pch_state.mapping_handles[%%d]'%%i)) for i in range(6)] if key=='mapping_handles' else int(gdb.parse_and_eval(('(int)' if key in ('image_count','loaded_image_count') else '(unsigned int)')+'c1_bss.pch_state.'+key))) for key in PCH_KEYS},'empty_record_prototype':[int(gdb.parse_and_eval('c1_bss.empty_record_prototype[%%d]'%%i)) for i in range(20)],'pools':[{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.pools[%%d].'%%i+key)) for key in ('cursor','remaining','minimum_pages','arena')} for i in range(POOL_COUNT)],'pdb_signature':int(gdb.parse_and_eval('c1_bss.pdb_signature')),'literal':int(gdb.parse_and_eval('c1_bss.literal.integer')),'saved_literal':int(gdb.parse_and_eval('c1_bss.saved_literal.integer')),'saved_literal_available':int(gdb.parse_and_eval('c1_bss.saved_literal_available')),'saved_data':int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data')),
        'cleanup_count':int(gdb.parse_and_eval('c1_bss.cleanup_count')),
        'cleanup_callbacks':[int(gdb.parse_and_eval('(unsigned int)c1_bss.cleanup_callbacks[%%d]'%%i)) for i in range(32)],
        'context':{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.context.'+key)) for key in ('object','value','saved_frame')},
        'current_context':{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.current_context.'+key)) for key in ('object','value','saved_frame')},
        'contexts':[{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.contexts[%%d].'%%i+key)) for key in ('object','value','saved_frame')} for i in range(6)],
        'output_buffers':[{key:int(gdb.parse_and_eval(('(int)' if key=='file_descriptor' else '(unsigned int)')+'c1_bss.output_buffers[%%d].'%%i+key)) for key in ('data','cursor','capacity','relocated','relocation_context','stream','good','mode','file_descriptor')} for i in range(BUFFER_COUNT)]},
 'snapshot':({'first_call':int(gdb.parse_and_eval('c1_bss.saved_data->first_call')),'compiler_phase':int(gdb.parse_and_eval('c1_bss.saved_data->compiler_phase')),'c9_il':int(gdb.parse_and_eval('c1_bss.saved_data->c9_il')),'compiler_flags':int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data->compiler_flags')),'system_info':{key:int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data->system_info.'+key)) for key in SYS_KEYS}} if int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data')) else None),
 'reuse_state':int(gdb.parse_and_eval('c1_reuse_state')),
 'pass_mode':int(gdb.parse_and_eval('c1_pass_mode')),
 'initialization_callbacks':[int(gdb.parse_and_eval('(unsigned int)c1_initialization_callbacks[%%d]'%%i)) for i in range(34)],
 'native_layout':{'work_buffers':int(gdb.parse_and_eval('(char*)&c1_bss.work_buffers-(char*)&c1_bss')),'work_buffer_count':2,'work_buffer_size':int(gdb.parse_and_eval('sizeof(c1_bss.work_buffers[0])')),'work_buffer_members':{key:int(gdb.parse_and_eval('(char*)&c1_bss.work_buffers[0].'+key+'-(char*)&c1_bss.work_buffers[0]')) for key in ('data','limit')},'initialized_members':{key:int(gdb.parse_and_eval('(char*)&c1_data.'+key+'-(char*)&c1_data')) for key in ('first_call','compiler_phase','c9_il','compiler_flags','system_info')},'system_info_size':int(gdb.parse_and_eval('sizeof(c1_data.system_info)')),'system_info_members':{key:int(gdb.parse_and_eval('(char*)&c1_data.system_info.'+key+'-(char*)&c1_data.system_info')) for key in SYS_KEYS},'command_members':{key:int(gdb.parse_and_eval('(char*)&c1_bss.'+key+'-(char*)&c1_bss')) for key in ('debug_break','command_argc','command_argv')},'pch_state':int(gdb.parse_and_eval('(char*)&c1_bss.pch_state-(char*)&c1_bss')),'pch_state_size':int(gdb.parse_and_eval('sizeof(c1_bss.pch_state)')),'mapping_handle_count':int(gdb.parse_and_eval('sizeof(c1_bss.pch_state.mapping_handles)/sizeof(c1_bss.pch_state.mapping_handles[0])')),'pch_members':{key:int(gdb.parse_and_eval('(char*)&c1_bss.pch_state.'+key+'-(char*)&c1_bss.pch_state')) for key in PCH_KEYS},'empty_record_prototype':int(gdb.parse_and_eval('(char*)&c1_bss.empty_record_prototype-(char*)&c1_bss')),'empty_record_prototype_size':int(gdb.parse_and_eval('sizeof(c1_bss.empty_record_prototype)')),'initialized_size':int(gdb.parse_and_eval('sizeof(c1_data)')),'bss_size':int(gdb.parse_and_eval('sizeof(c1_bss)')),
 'pools':int(gdb.parse_and_eval('(char*)&c1_bss.pools-(char*)&c1_bss')),'pool_count':int(gdb.parse_and_eval('sizeof(c1_bss.pools)/sizeof(c1_bss.pools[0])')),'pool_size':int(gdb.parse_and_eval('sizeof(c1_bss.pools[0])')),'pool_members':{key:int(gdb.parse_and_eval('(char*)&c1_bss.pools[0].'+key+'-(char*)&c1_bss.pools[0]')) for key in ('cursor','remaining','minimum_pages','arena')},
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
        modeled_expression = commands.split("value['compiler_state']=", 1)[1].split("\nvalue['pass_mode']", 1)[0]
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
        allocation_probe=allocation_probe.replace('allocations=[]', 'def capture_modeled():\n    return '+modeled_expression+'\nallocations=[]')
        if field_map.get("pools"):
            symbol = field_map["pools"][0]["native_linkage_symbol"]
            table = subprocess.check_output(["nm", "-S", "--defined-only", str(binary)], text=True)
            address = int(re.search(r"^([0-9a-f]+) [0-9a-f]+ [Tt] " + re.escape(symbol) + "$", table, re.M)[1], 16)
            abi_probe = """import struct
pool_entries=[]
pool_returns=[]
class PoolReturn(gdb.Breakpoint):
    def stop(self):
        entry=pool_entries[-1]
        words=struct.unpack("<4I",gdb.selected_inferior().read_memory(entry["ecx"],16).tobytes())
        pool_returns.append({"pc":int(gdb.parse_and_eval("$pc")),"eax":int(gdb.parse_and_eval("$eax")),"esp":int(gdb.parse_and_eval("$esp")),"values":list(words)})
        self.enabled = False
        return False
class PoolEntry(gdb.Breakpoint):
    def stop(self):
        esp=int(gdb.parse_and_eval('$esp'))
        words=struct.unpack('<2I',gdb.selected_inferior().read_memory(esp,8).tobytes())
        receiver=int(gdb.parse_and_eval('$ecx'))
        pool=(receiver-int(gdb.parse_and_eval('(unsigned int)&c1_bss.pools[0]')))//16
        assert 0<=pool<POOL_COUNT
        if POOL_MEMBER:
            if POOL_SEED:
                saved=int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data'))
                gdb.selected_inferior().write_memory(int(gdb.parse_and_eval('$ecx')),struct.pack('<4I',saved+128+pool*16,91+pool,7+pool,saved+512+pool*16))
            PoolReturn('*0x%%x'%%words[0])
        block=gdb.block_for_pc(int(gdb.parse_and_eval('$pc')))
        while block and not block.function:block=block.superblock
        assert block and block.function
        formals=[{'name':symbol.name,'type':str(symbol.type)} for symbol in block if symbol.is_argument]
        assert {row['name'] for row in formals}=={'this','minimum_pages'}
        values=struct.unpack('<4I',gdb.selected_inferior().read_memory(int(gdb.parse_and_eval('$ecx')),16).tobytes())
        pool_entries.append({'formal_inventory':formals,'index':pool,'values':list(values),'pc':int(gdb.parse_and_eval('$pc')),'ecx':int(gdb.parse_and_eval('$ecx')),'esp':esp,'return':words[0],'argument':words[1]})
        return False
PoolEntry('*0x%x')
""" % address
            abi_probe=abi_probe.replace("POOL_MEMBER",str(any(row["name"]=="CompilerPool::Initialize" for row in candidate["functions"]))).replace("POOL_SEED",str(bool(args.restore_probe and candidate.get("restore_probe",{}).get("pool_member"))))
            allocation_probe = allocation_probe.replace("\nend\n", "\n" + abi_probe + "end\n")
            commands = commands.replace("open(%r,'w').write(json.dumps(value,indent=2)+'\\n')" % str(output/'state.json'),
                                        "open(%r,'w').write(json.dumps(pool_entries,indent=2)+'\\n')\n" % str(output/'pool-entry-observations.json') +
                                        "open(%r,'w').write(json.dumps(pool_returns,indent=2)+'\\n')\n" % str(output/'pool-return-observations.json') +
                                        "open(%r,'w').write(json.dumps(value,indent=2)+'\\n')" % str(output/'state.json'))
        if field_map.get("pch_state"):
            symbol=field_map["pch_state"]["native_linkage_symbol"]
            table=subprocess.check_output(["nm","-S","--defined-only",str(binary)],text=True)
            address=int(re.search(r"^([0-9a-f]+) [0-9a-f]+ [Tt] "+re.escape(symbol)+"$",table,re.M)[1],16)
            pch_probe="""import struct
pch_entries=[]
pch_returns=[]
class PchReturn(gdb.Breakpoint):
    def stop(self):
        receiver=pch_entries[-1]['ecx']
        bss=int(gdb.parse_and_eval('(unsigned int)&c1_bss'))
        pch_returns.append({'pc':int(gdb.parse_and_eval('$pc')),'eax':int(gdb.parse_and_eval('$eax')),'esp':int(gdb.parse_and_eval('$esp')),'edi':int(gdb.parse_and_eval('$edi')),
          'values':list(struct.unpack('<17I',gdb.selected_inferior().read_memory(receiver,68).tobytes())),
          'bss_bytes':list(gdb.selected_inferior().read_memory(bss,int(gdb.parse_and_eval('sizeof(c1_bss)'))).tobytes())})
        self.enabled=False
        return False
class PchEntry(gdb.Breakpoint):
    def stop(self):
        esp=int(gdb.parse_and_eval('$esp'));receiver=int(gdb.parse_and_eval('$ecx'))
        returned=struct.unpack('<I',gdb.selected_inferior().read_memory(esp,4).tobytes())[0]
        if PCH_SEED:
            saved=int(gdb.parse_and_eval('(unsigned int)c1_bss.saved_data'))
            words=[0]*17
            for offset,value in [(0,37),(52,19),(12,0x11223344),(20,0x55667788)]:words[offset//4]=value
            for i,offset in enumerate([4,8,16,24,28,32,36,40,44,48,56,60,64]):words[offset//4]=saved+1024+i*8
            gdb.selected_inferior().write_memory(receiver,struct.pack('<17I',*words))
        if PCH_MEMBER:PchReturn('*0x%%x'%%returned)
        block=gdb.block_for_pc(int(gdb.parse_and_eval('$pc')))
        while block and not block.function:block=block.superblock
        assert block and block.function
        formals=[{'name':symbol.name,'type':str(symbol.type)} for symbol in block if symbol.is_argument]
        assert {row['name'] for row in formals}=={'this'}
        bss=int(gdb.parse_and_eval('(unsigned int)&c1_bss'))
        pch_entries.append({'pc':int(gdb.parse_and_eval('$pc')),'ecx':receiver,'esp':esp,'return':returned,'formal_inventory':formals,'edi':int(gdb.parse_and_eval('$edi')),
          'values':list(struct.unpack('<17I',gdb.selected_inferior().read_memory(receiver,68).tobytes())),
          'bss_bytes':list(gdb.selected_inferior().read_memory(bss,int(gdb.parse_and_eval('sizeof(c1_bss)'))).tobytes())})
        return False
PchEntry('*0x%x')
"""%address
            pch_probe=pch_probe.replace('PCH_MEMBER',str(any(row['name']=='CompilerPchState::Initialize' for row in candidate['functions']))).replace('PCH_SEED',str(bool(args.restore_probe and candidate.get('restore_probe',{}).get('pch_member'))))
            allocation_probe=allocation_probe.replace("\nend\n","\n"+pch_probe+"end\n")
            commands=commands.replace("open(%r,'w').write(json.dumps(value,indent=2)+'\\n')"%str(output/'state.json'),"open(%r,'w').write(json.dumps(pch_entries,indent=2)+'\\n')\n"%str(output/'pch-entry-observations.json')+"open(%r,'w').write(json.dumps(pch_returns,indent=2)+'\\n')\n"%str(output/'pch-return-observations.json')+"open(%r,'w').write(json.dumps(value,indent=2)+'\\n')"%str(output/'state.json'))
        if field_map.get('main_fields'):
            table=subprocess.check_output(['nm','-S','--defined-only',str(binary)],text=True)
            main_address=int(re.search(r'^([0-9a-f]+) [0-9a-f]+ [Tt] c1_main$',table,re.M)[1],16)
            main_probe="""main_entries=[]
class ActualMainEntry(gdb.Breakpoint):
    def stop(self):
        esp=int(gdb.parse_and_eval('$esp'))
        words=list(struct.unpack('<3I',gdb.selected_inferior().read_memory(esp,12).tobytes()))
        block=gdb.block_for_pc(int(gdb.parse_and_eval('$pc')))
        while block and not block.function:block=block.superblock
        assert block and block.function
        formals=[{'name':symbol.name,'type':str(symbol.type)} for symbol in block if symbol.is_argument]
        assert {row['name'] for row in formals}=={'argc','argv'}
        assert words[1]<=256
        vector=gdb.Value(words[2]).cast(gdb.lookup_type('char').pointer().pointer())
        actual_argv=[vector[i].string() for i in range(words[1])]
        terminator=int(vector[words[1]].cast(gdb.lookup_type('unsigned int')))
        crt_state={'win32_last_error':int(gdb.parse_and_eval('last_error')),'x87_control':int(gdb.parse_and_eval('$fctrl')),
          'crt_attached':int(gdb.parse_and_eval('c1_crt_attached')),'raw_dllmain':int(gdb.parse_and_eval('(unsigned int)c1_raw_dllmain')),
          'onexit_begin':int(gdb.parse_and_eval('(unsigned int)c1_onexit_begin')),'onexit_end':int(gdb.parse_and_eval('(unsigned int)c1_onexit_end')),
          'onexit_first':int(gdb.parse_and_eval('(unsigned int)*c1_onexit_begin')),'crt_adjust_fdiv':int(gdb.parse_and_eval('c1_crt_adjust_fdiv')),
          'dll_instance':int(gdb.parse_and_eval('(unsigned int)c1_dll_instance')),'module_token':int(gdb.parse_and_eval('(unsigned int)c1_host_module'))}
        main_entries.append({'crt_state':crt_state,'argv':actual_argv,'argv_terminator':terminator,'boundary_rva':0x127a7,'symbol':block.function.name,'pc':int(gdb.parse_and_eval('$pc')),'esp':esp,'return':words[0],
          'args':dict(argc=words[1],argv=words[2]),'formal_inventory':formals,'compiler_state':capture_modeled()})
        return False
ActualMainEntry('*0x%x')
"""%main_address
            work_address=int(re.search(r'^([0-9a-f]+) [0-9a-f]+ [Tt] initialize_work_buffers$',table,re.M)[1],16)
            work_probe="""work_entries=[]
class ActualWorkEntry(gdb.Breakpoint):
    def stop(self):
        esp=int(gdb.parse_and_eval('$esp'))
        words=list(struct.unpack('<I',gdb.selected_inferior().read_memory(esp,4).tobytes()))
        block=gdb.block_for_pc(int(gdb.parse_and_eval('$pc')))
        while block and not block.function:block=block.superblock
        assert block and block.function
        formals=[{'name':symbol.name,'type':str(symbol.type)} for symbol in block if symbol.is_argument]
        assert not formals
        crt_state={'win32_last_error':int(gdb.parse_and_eval('last_error')),'x87_control':int(gdb.parse_and_eval('$fctrl')),
          'crt_attached':int(gdb.parse_and_eval('c1_crt_attached')),'raw_dllmain':int(gdb.parse_and_eval('(unsigned int)c1_raw_dllmain')),
          'onexit_begin':int(gdb.parse_and_eval('(unsigned int)c1_onexit_begin')),'onexit_end':int(gdb.parse_and_eval('(unsigned int)c1_onexit_end')),
          'onexit_first':int(gdb.parse_and_eval('(unsigned int)*c1_onexit_begin')),'crt_adjust_fdiv':int(gdb.parse_and_eval('c1_crt_adjust_fdiv')),
          'dll_instance':int(gdb.parse_and_eval('(unsigned int)c1_dll_instance')),'module_token':int(gdb.parse_and_eval('(unsigned int)c1_host_module'))}
        frame=gdb.newest_frame();caller=frame.older()
        work_entries.append({'crt_state':crt_state,'boundary_rva':0x12830,'symbol':block.function.name,'pc':int(gdb.parse_and_eval('$pc')),'esp':esp,'return':words[0],
          'args':{},'formal_inventory':formals,'compiler_state':capture_modeled(),'caller':{'symbol':caller.name(),'pc':int(caller.pc()),'args':{key:int(caller.read_var(key).cast(gdb.lookup_type('unsigned int'))) for key in ('argc','argv')}}})
        return False
ActualWorkEntry('*0x%x')
"""%work_address
            main_probe+=work_probe
            allocation_probe=allocation_probe.replace('\nend\n','\n'+main_probe+'end\n')
            commands=commands.replace("open(%r,'w').write(json.dumps(value,indent=2)+'\\n')"%str(output/'state.json'),"open(%r,'w').write(json.dumps(main_entries,indent=2)+'\\n')\n"%str(output/'main-entry-observations.json')+"open(%r,'w').write(json.dumps(value,indent=2)+'\\n')"%str(output/'state.json'))
        commands=commands.replace("open(%r,'w').write(json.dumps(value,indent=2)+'\\n')"%str(output/'state.json'),"open(%r,'w').write(json.dumps(work_entries,indent=2)+'\\n')\n"%str(output/'work-entry-observations.json')+"open(%r,'w').write(json.dumps(value,indent=2)+'\\n')"%str(output/'state.json'))
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
def seed_main_fields(prefix,which):
    plan=MAIN_FIELDS_PLAN[which]
    for key in ('compiler_phase','c9_il','compiler_flags'):gdb.execute('set var '+prefix+key+' = '+str(plan[key]))
    for key in SYS_KEYS:
        if key in ('lpMinimumApplicationAddress','lpMaximumApplicationAddress'):
            gdb.execute('set var '+prefix+'system_info.'+key+' = (void *)((char *)c1_bss.saved_data + '+str(plan[key])+')')
        else:gdb.execute('set var '+prefix+'system_info.'+key+' = '+str(plan[key]))
seed_main_fields('c1_bss.saved_data->','snapshot')
seed_main_fields('c1_data.','live')
gdb.execute('set var c1_bss.debug_break = 17')
gdb.execute('set var c1_bss.command_argc = 23')
gdb.execute('set var c1_bss.command_argv = (char **)((char *)c1_bss.saved_data + 4000)')
for i in range(2):
    for k,key in enumerate(('data','limit')):gdb.execute('set var c1_bss.work_buffers[%%d].%%s = (void *)((char *)c1_bss.saved_data + %%d)'%%(i,key,4200+(i*2+k)*100))
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
for i in range(20):gdb.execute('set var c1_bss.empty_record_prototype[%%d] = %%d'%%(i,1+i))
for key,value in [('image_count',37),('loaded_image_count',19),('stored_file_position',0x11223344),('file_offset',0x55667788)]:gdb.execute('set var c1_bss.pch_state.'+key+' = '+str(value))
pch_pointers=['root_image','images','file_name','file']+['mapping_handles[%%d]'%%i for i in range(6)]+['scratch_arena','original_pages','mapped_pages']
for i,key in enumerate(pch_pointers):gdb.execute('set var c1_bss.pch_state.'+key+' = (void *)((char *)c1_bss.saved_data + '+str(1024+i*8)+')')
VALUE_BEFORE_CLEAR
gdb.execute('set var c1_bss.pdb_signature = 0x13572468')
for i,key in enumerate(('database','names','types','minimal_rebuild','rebuild_state')):
    gdb.execute('set var c1_pdb_context.'+key+' = (void *)((char *)c1_bss.saved_data + '+str(i*8)+')')
gdb.execute('set var c1_pdb_context.read_mode = 37')
for i,key in enumerate(('database','names','types','minimal_rebuild','rebuild_state')):
    gdb.execute('set var c1_output_pdb_context.'+key+' = (void *)((char *)c1_bss.saved_data + '+str(64+i*8)+')')
gdb.execute('set var c1_output_pdb_context.read_mode = 37')
for i in range(POOL_COUNT):
    prefix='c1_bss.pools['+str(i)+'].'
    gdb.execute('set var '+prefix+'cursor = (unsigned char *)c1_bss.saved_data + '+str(128+i*16))
    gdb.execute('set var '+prefix+'remaining = '+str(91+i))
    gdb.execute('set var '+prefix+'minimum_pages = '+str(7+i))
    gdb.execute('set var '+prefix+'arena = (void *)((char *)c1_bss.saved_data + '+str(512+i*16)+')')
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
break initialize_state_126af
continue
python
restore_observations['after_pdb_context']=capture_modeled()
restore_observations['after_pdb_context_pc']=int(gdb.parse_and_eval('$pc'))
end
break initialize_pdb_context_126b6
continue
python
restore_observations['before_output_pdb_context']=capture_modeled()
restore_observations['output_pdb_context_pc']=int(gdb.parse_and_eval('$pc'))
end
break initialize_state_126ef
continue
python
restore_observations['after_output_pdb_context']=capture_modeled()
restore_observations['after_output_pdb_context_pc']=int(gdb.parse_and_eval('$pc'))
end
break initialize_state_1277b
continue
python
for i in range(20):gdb.execute('set var c1_bss.empty_record_prototype[%%d] = %%d'%%(i,0xa0+i))
restore_observations['before_prototype']=capture_modeled()
restore_observations['prototype_pc']=int(gdb.parse_and_eval('$pc'))
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
    commands=commands.replace('MAIN_FIELDS_PLAN',repr(candidate.get('restore_probe',{}).get('main_fields',{}))).replace('SYS_KEYS',repr(tuple(SYSTEM_MEMBERS)))
    commands=commands.replace("PCH_KEYS",repr(tuple(field_map.get("pch_state",{}).get("members",PCH_MEMBERS))))
    commands = commands.replace("BUFFER_COUNT", str(len(buffer_models))).replace("POOL_COUNT", str(len(field_map.get("pools", []))))
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
    if args.prospective_tail_chain:
        assert args.case == "minimal"
        from prospective import freeze as freeze_prospective
        freeze_prospective(output, candidate)
    elif candidate.get("functions"):
        binding = json.loads((PROJECT / "build/trial-audit-binding.json").read_text())
        assert all(sha(ROOT / path) == value for path, value in binding["inputs"].items())
        for source, target in (("trial-audit.json", "audit.json"), ("trial-audit-binding.json", "audit-binding.json")):
            (output / target).write_bytes((PROJECT / "build" / source).read_bytes())
    receipt = {"schema": 1, "kind": "native-prospective-tail-chain" if args.prospective_tail_chain else "native-gdb-frontier",
               "candidate_path": "cases/" + args.case + "/candidate.json",
               "probe": "restore" if args.restore_probe else "normal",
               "value_copy_sign": ("negative" if args.negative_value else "positive") if args.restore_probe else None,
               "pdb_reuse": args.pdb_reuse if args.restore_probe else None, "seconds": elapsed,
               "native_sha256": binary_sha, "build_source_reuse": source_reuse,
               "compiled_build_sha256": sha(output / "compiled-build.json") if compiled_build else None,
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
