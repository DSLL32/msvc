#!/usr/bin/env python3
"""Native-only fixtures against explicitly frozen original boundaries/results."""
import argparse
import json
import os
import resource
import shlex
from pathlib import Path
import subprocess
import time
from state import BINARY, ROOT, load, case, prepare, sha
from processes import Processes

def run_case(name, directory, debug=False):
    start = time.perf_counter()
    config, original = load()
    snapshot=directory/'configuration.json'
    snapshot.write_bytes((BINARY/'functions.json').read_bytes())
    assert json.loads(snapshot.read_text())==config,'configuration changed during native setup'
    from seams import snapshot_policy,check_policy_snapshot
    policy_hash=snapshot_policy(directory)
    record = case(name)
    work = directory / 'work'
    prepare(record, work)
    stdout, stderr = directory/'stdout', directory/'stderr'
    report = directory/'gdb.json'
    script = directory/'commands.gdb'
    executable = BINARY / 'build/native/CVPACK.EXE'
    executable_hash=sha(executable)
    expected = record['current']
    debug=debug and expected['terminal']=='trap'
    data=json.loads((BINARY/'data.json').read_text())
    walks=json.loads((BINARY/'walks.json').read_text()) if (BINARY/'walks.json').exists() else []
    from reads import validate as validate_reads
    read_sites=validate_reads(config,original)if name=='hello-debug-dll'else []
    from symbol_output import validate as validate_symbol_output
    symbol_proof=validate_symbol_output(config,original)if name=='hello-debug-dll'else None
    from type_pages import validate as validate_type_pages
    page_proof=validate_type_pages(config,original)if name=='hello-debug-dll'else None
    from buffer_storage import validate as validate_buffer_storage
    buffer_proof=validate_buffer_storage(config,original)if name=='hello-debug-dll'else None
    from recursive_arena import validate as validate_recursive_arena
    from audit import publics
    from binary import PE
    recursive_proof=validate_recursive_arena(config,original,PE(BINARY/'build/CVPACK.EXE'),publics())if name=='hello-debug-dll'else None
    from packed_arena import validate as validate_packed_arena
    packed_proof=validate_packed_arena(config,original,PE(BINARY/'build/CVPACK.EXE'),publics())if name=='hello-debug-dll'else None
    from symbol_blocks import validate as validate_symbol_blocks
    blocks_proof=validate_symbol_blocks(config,original,PE(BINARY/'build/CVPACK.EXE'),publics())if name=='hello-debug-dll'else None
    from inputs import namespace as input_namespace,relative as input_relative
    from seams import approved as approved_seams
    policies=[]
    for seam in approved_seams(config,original,json.loads((BINARY/'runtime.json').read_text()),name):
        if seam['guard']['mode']==0x8002:prepared=directory/'debug-work'/seam['guard']['name']
        else:prepared=input_namespace(directory/'debug-work')/input_relative(dict(windows=seam['guard']['name']))
        policies.append(dict(name=seam['guard']['name'],mode=seam['guard']['mode'],share=seam['guard']['share'],input=str(prepared),master=str(ROOT/seam['input']['path'])))
    canonical='Z:'+str(ROOT/config['target']['path']).replace('/','\\')
    script.write_text('set pagination off\nset confirm off\nset startup-with-shell on\n'
        'set disable-randomization off\nset args '+' '.join(map(shlex.quote,record['argv']))+'\n'
        'break main\nrun '+' '.join(map(shlex.quote,record['argv']))+' > '+shlex.quote(str(directory/'debug-stdout'))+' 2> '+shlex.quote(str(directory/'debug-stderr'))+'\n'
        'set argv[0] = recon_pgmptr\n'
        +('python\nimport runpy\nrunpy.run_path('+repr(str(BINARY/'scripts/native-debug.py'))+')["install_sopen_observer"]('+repr(str(directory/'crt-returns.json'))+','+repr(str(directory/'debug-work'))+','+repr(policies)+')\nend\n'if name=='hello-debug-dll'else '')+
        ('python\nimport runpy\nrunpy.run_path('+repr(str(BINARY/'scripts/native-debug.py'))+')["install_read_observer"]('+repr(str(directory/'read-returns.json'))+','+repr(read_sites)+')\nend\n'if read_sites else '')+
        ('python\nimport runpy\nrunpy.run_path('+repr(str(BINARY/'scripts/native-debug.py'))+')["install_pdb_header_observer"]('+repr(str(directory/'pdb-read-returns.json'))+','+repr(str(BINARY/'cases/hello-debug-dll/input/msvc.pdb'))+','+repr(str(next((p['input']for p in policies if p['mode']==0x8000),'')))+')\nend\n'if name=='hello-debug-dll'else '')+
        ('python\nimport runpy\nrunpy.run_path('+repr(str(BINARY/'scripts/native-debug.py'))+')["install_pdb_consumed_observer"]('+repr(str(directory/'pdb-consumed.json'))+','+repr(str(BINARY/'src/pdb.c'))+','+str(next(i for i,line in enumerate((BINARY/'src/pdb.c').read_text().splitlines(),1)if line.strip()=='if (server->incremental) {'))+')\nend\n'if name=='hello-debug-dll'else '')+
        ('python\nrunpy.run_path('+repr(str(BINARY/'scripts/native-debug.py'))+')["install_symbol_allocation_observer"]('+repr(str(BINARY/'src/types.c'))+','+str(symbol_proof['native_allocation_line'])+')\nend\n'if symbol_proof else '')+
        ('python\nrunpy.run_path('+repr(str(BINARY/'scripts/native-debug.py'))+')["install_type_page_observer"]('+repr(str(BINARY/'src/types.c'))+','+str(page_proof['native_allocation_line'])+')\nend\n'if page_proof else '')+
        ('python\nrunpy.run_path('+repr(str(BINARY/'scripts/buffer-debug.py'))+')["install"]('+repr(str(BINARY/'src/types.c'))+','+str(buffer_proof['native_allocation_line'])+')\nend\n'if buffer_proof else '')+
        ('python\nrunpy.run_path('+repr(str(BINARY/'scripts/module-address-debug.py'))+')["install"]()\nend\n'if name=='hello-debug-dll'and any(r['name']=='address_in_module'and r['status']!='stub'for r in config['functions'])else '')+
        ('python\nrunpy.run_path('+repr(str(BINARY/'scripts/symbol-block-debug.py'))+')["install"]('+repr(str(BINARY/'src/modules.c'))+','+str(blocks_proof['native_allocation_line'])+')\nend\n'if blocks_proof else '')+
        ('python\nrunpy.run_path('+repr(str(BINARY/'scripts/packed-arena-debug.py'))+')["install"]('+repr(str(BINARY/'src/types.c'))+','+str(packed_proof['native_allocation_line'])+')\nend\n'if packed_proof else '')+
        ('python\nrunpy.run_path('+repr(str(BINARY/'scripts/recursive-arena-debug.py'))+')["install"]('+repr(str(BINARY/'src/types.c'))+','+str(recursive_proof['native_allocation_line'])+')\nend\n'if recursive_proof else '')+
        ('python\nrunpy.run_path('+repr(str(BINARY/'scripts/resolver-debug.py'))+')["install"]()\nend\n'if any(r['name']=='get_type_record'and r['status']!='stub'for r in config['functions'])else '')+
        'continue\npython\n'
        'import gdb,json\n'
        'current=gdb.newest_frame()\nparameters={}\n'
        'import runpy\n'
        'progress=runpy.run_path('+repr(str(BINARY/'scripts/native-debug.py'))+')["capture_read_progress"]('+repr(str(BINARY/'src/files.c'))+','+str(next(i for i,line in enumerate((BINARY/'src/files.c').read_text().splitlines(),1)if line.strip()=='advance_file_page(slot);'))+')\n'
        'json.dump(progress,open('+repr(str(directory/'read-progress.json'))+',"w"))\n'
        'symbol_output=runpy.run_path('+repr(str(BINARY/'scripts/native-debug.py'))+')["capture_symbol_output"]()\n'
        'json.dump(symbol_output,open('+repr(str(directory/'symbol-output.json'))+',"w"))\n'
        'pages=runpy.run_path('+repr(str(BINARY/'scripts/native-debug.py'))+')["capture_type_page"]()\n'
        'json.dump(pages,open('+repr(str(directory/'type-pages.json'))+',"w"))\n'
        'buffer_storage=runpy.run_path('+repr(str(BINARY/'scripts/buffer-debug.py'))+')["capture"]()\n'
        'json.dump(buffer_storage,open('+repr(str(directory/'buffer-storage.json'))+',"w"))\n'
        'module_addresses=runpy.run_path('+repr(str(BINARY/'scripts/module-address-debug.py'))+')["capture"]()\n'
        'json.dump(module_addresses,open('+repr(str(directory/'module-address.json'))+',"w"))\n'
        'symbol_blocks=runpy.run_path('+repr(str(BINARY/'scripts/symbol-block-debug.py'))+')["capture"]()\n'
        'json.dump(symbol_blocks,open('+repr(str(directory/'symbol-blocks.json'))+',"w"))\n'
        'packed_arena=runpy.run_path('+repr(str(BINARY/'scripts/packed-arena-debug.py'))+')["capture"]()\n'
        'json.dump(packed_arena,open('+repr(str(directory/'packed-arena.json'))+',"w"))\n'
        'recursive_arena=runpy.run_path('+repr(str(BINARY/'scripts/recursive-arena-debug.py'))+')["capture"]()\n'
        'json.dump(recursive_arena,open('+repr(str(directory/'recursive-arena.json'))+',"w"))\n'
        'resolver=runpy.run_path('+repr(str(BINARY/'scripts/resolver-debug.py'))+')["capture"]()\n'
        'json.dump(resolver,open('+repr(str(directory/'resolver-returns.json'))+',"w"))\n'
        'for symbol in current.block():\n'
        ' if symbol.is_argument:\n'
        '  value=current.read_var(symbol); kind=value.type.strip_typedefs().code\n'
        '  if kind==gdb.TYPE_CODE_PTR:\n'
        '   parameters[symbol.name]=dict(null=int(value)==0,address=int(value))\n'
        '   if int(value) and value.type.strip_typedefs().target().strip_typedefs().code==gdb.TYPE_CODE_INT and value.type.strip_typedefs().target().strip_typedefs().sizeof==1: parameters[symbol.name]["string"]=value.string(encoding="latin1").encode("latin1").hex()\n'
        '   for item in '+repr(data)+':\n'
        '    address=int(gdb.parse_and_eval("&"+item["name"]))\n'
        '    if address<=int(value)<address+item["size"]: parameters[symbol.name]["global"]=dict(name=item["name"],offset=int(value)-address)\n'
        '   if int(value) and value.type.strip_typedefs().target().strip_typedefs().code==gdb.TYPE_CODE_FUNC: parameters[symbol.name]["function"]=gdb.block_for_pc(int(value)).function.name\n'
        '  else: parameters[symbol.name]=int(value)\n'
        'frontier=next(r for r in '+repr(config['functions'])+' if r["native"]==current.name())\n'
        'for description in frontier.get("parameters",[]):\n'
        ' if description["kind"]not in ("stack-output","stack-input"): continue\n'
        ' proof=json.load(open('+repr(str(BINARY))+'+"/"+description["evidence"]))\n'
        ' owner=current.older()\n'
        ' while owner and owner.name()!=proof["owner"]: owner=owner.older()\n'
        ' if not owner: raise RuntimeError("missing output owner")\n'
        ' local=owner.read_var(proof["local"]); address=int(local.address); argument=parameters[description["name"]]\n'
        ' if argument["address"]!=address+proof["offset"] or local.type.sizeof!=proof["size"]: raise RuntimeError("wrong output local")\n'
        ' argument["local_address"]=address\n'
        ' argument["local"]=dict(owner=owner.name(),name=proof["local"],size=int(local.type.sizeof),offset=proof["offset"])\n'
        ' argument["prebytes"]=bytes(gdb.selected_inferior().read_memory(argument["address"],proof["size"])).hex()\n'
        'for description in frontier.get("parameters",[]):\n'
        ' if description["kind"]!="transient-pointer": continue\n'
        ' proof=json.load(open('+repr(str(BINARY))+'+"/"+description["evidence"]))\n'
        ' owner=current.older()\n'
        ' while owner and owner.name()!="type_server_open": owner=owner.older()\n'
        ' if not owner: raise RuntimeError("missing transient allocation owner")\n'
        ' local=owner.read_var("server"); argument=parameters[description["name"]]\n'
        ' if int(local)!=argument["address"] or local.type.target().sizeof!=proof["size"]: raise RuntimeError("wrong transient object identity")\n'
        ' argument["owner_pointer"]=int(local);argument["owner"]=dict(function=owner.name(),local="server",size=int(local.type.target().sizeof))\n'
        ' if proof.get("phase")=="header-read": argument["read_return"]=json.load(open('+repr(str(directory/'pdb-read-returns.json'))+'))[-1]\n'
        ' raw=bytes(gdb.selected_inferior().read_memory(argument["address"],proof["size"]));argument["prebytes"]=raw.hex();argument["children"]={}\n'
        ' for offset in proof["child_fields"]:\n'
        '  address=int.from_bytes(raw[offset:offset+4],"little");argument["children"][str(offset)]=dict(address=address,bytes=bytes(gdb.selected_inferior().read_memory(address,12)).hex())\n'
        'frames=[]\nf=gdb.newest_frame()\n'
        'while f:\n frames.append(f.name()); f=f.older()\n'
        'f=gdb.newest_frame()\n'
        'while f and f.name()!= "main": f=f.older()\n'
        'if f: f.select()\n'
        'signal=gdb.parse_and_eval("$_siginfo.si_signo")\n'
        'argc=int(gdb.parse_and_eval("argc"))\n'
        'argv=[gdb.parse_and_eval("argv[%d]"%i).string() for i in range(argc)]\n'
        'fields=["_ptr","_cnt","_base","_flag","_file","_charbuf","_bufsiz","_tmpfname"]\n'
        'stream={n:int(gdb.parse_and_eval("recon_iob[1]."+n)) for n in fields}\n'
        'state={"feedback":int(gdb.parse_and_eval("feedback")),"stdout_osfile":int(gdb.parse_and_eval("recon_osfile[1]"))}\n'
        'state["crt_errno"]=int(gdb.parse_and_eval("(int)errno"))\n'
        'globals={}\n'
        'global_addresses={item["name"]:int(gdb.parse_and_eval("&"+item["name"])) for item in '+repr(data)+'}\n'
        'for item in '+repr(data)+':\n'
        ' name=item["name"]\n'
        ' value=gdb.parse_and_eval(name)\n'
        ' if item.get("kind")=="callback-table":\n'
        '  raw=bytes(gdb.selected_inferior().read_memory(value.address,item["size"]));records=[]\n'
        '  stride=item["stride"];pointer=item["pointer_offset"]\n'
        '  for offset in range(0,len(raw),stride):\n'
        '   address=int.from_bytes(raw[offset+pointer:offset+pointer+4],"little");function=gdb.block_for_pc(address).function.name if address else None\n'
        '   records.append(dict(fields=raw[offset:offset+pointer].hex(),function=function))\n'
        '  globals[name]=records\n'
        ' elif item.get("kind")=="pointer-array":\n'
        '  assert value.type.sizeof==item["size"],"global pointer array declaration extent differs"\n'
        '  globals[name]=[dict(null=int(value[i])==0,address=int(value[i]))for i in range(item["size"]//4)]\n'
        ' elif item.get("kind")=="string-pointer-array": globals[name]=[value[i].string(encoding="latin1").encode("latin1").hex() for i in range(item["size"]//4)]\n'
        ' elif value.type.strip_typedefs().code==gdb.TYPE_CODE_PTR:\n'
        '  globals[name]=dict(null=int(value)==0,address=int(value))\n'
        '  if int(value) and value.type.strip_typedefs().target().strip_typedefs().code==gdb.TYPE_CODE_FUNC: globals[name]["function"]=gdb.block_for_pc(int(value)).function.name\n'
        ' else: globals[name]=bytes(gdb.selected_inferior().read_memory(value.address,value.type.sizeof)).hex()\n'
        'walks={}\n'
        'for item in '+repr(walks)+':\n'
        ' name=item["name"];pointer=int(gdb.parse_and_eval(name));nodes=[]\n'
        ' count=int(gdb.parse_and_eval(item["count_native"])) if item.get("count_native") else 1\n'
        ' if count<0 or (item.get("count_native") and count>item.get("count_limit",item["limit"])): raise RuntimeError("unbounded object count")\n'
        ' size=item["size"]*count\n'
        ' if size>524288 or (pointer and not size and not item.get("zero_allocation")): raise RuntimeError("invalid object extent")\n'
        ' if item.get("arena"):\n'
        '  available=int(gdb.parse_and_eval(item["arena"]["available_native"])); capacity=item["arena"]["capacity"]\n'
        '  if not 0<=available<=capacity or (not pointer and available): raise RuntimeError("invalid public arena cursor")\n'
        '  if pointer: pointer-=capacity-available\n'
        ' while pointer and len(nodes)<item["limit"]:\n'
        '  raw=bytes(gdb.selected_inferior().read_memory(pointer,size)) if size else b"";nodes.append(dict(address=pointer,bytes=raw.hex()))\n'
        '  if item.get("pdb_graph"):\n'
        '   import runpy\n'
        '   nodes[-1]["objects"]=runpy.run_path('+repr(str(BINARY/'scripts/pdb_objects.py'))+')["capture"](pointer,lambda address,size:bytes(gdb.selected_inferior().read_memory(address,size)))\n'
        '  strings={}\n'
        '  for offset in item.get("string_fields",[]):\n'
        '   address=int.from_bytes(raw[offset:offset+4],"little");text=b""\n'
        '   if address:\n'
        '    text=gdb.Value(address).cast(gdb.lookup_type("char").pointer()).string(encoding="latin1").encode("latin1")+b"\\0"\n'
        '    if len(text)>1024: raise RuntimeError("unbounded object string")\n'
        '   strings[str(offset)]=dict(address=address,bytes=text.hex())\n'
        '  if strings: nodes[-1]["strings"]=strings\n'
        '  arrays={}\n'
        '  for array in item.get("arrays",[]):\n'
        '   when=array["when"];flags=int.from_bytes(raw[when["offset"]:when["offset"]+4],"little")\n'
        '   if flags&when["mask"]!=when["value"]: continue\n'
        '   offset=array["offset"];address=int.from_bytes(raw[offset:offset+4],"little")\n'
        '   count=array["fixed_count"] if "fixed_count" in array else int.from_bytes(raw[array["count_offset"]:array["count_offset"]+4],"little")>>array["count_shift"]\n'
        '   if count>array["max_count"]: raise RuntimeError("unbounded object array")\n'
        '   payload=bytes(gdb.selected_inferior().read_memory(address,count*array.get("element_size",4))) if address and count else b""\n'
        '   arrays[str(offset)]=dict(address=address,bytes=payload.hex())\n'
        '  if arrays: nodes[-1]["arrays"]=arrays\n'
        '  if item.get("file_descriptor"):\n'
        '   fd=int.from_bytes(raw[16:20],"little");flags=int.from_bytes(raw[4:8],"little")\n'
        '   if fd>=3 and fd!=0xffffffff and not(flags&8):\n'
        '    if fd>=int(gdb.parse_and_eval("recon_nhandle")): raise RuntimeError("invalid object descriptor")\n'
        '    osfile=int(gdb.parse_and_eval("recon_osfile[%d]"%fd));descriptor=dict(descriptor=fd,osfile=osfile,type=0,position=0,owned=False)\n'
        '    if osfile&1:\n'
        '     import os,stat,pathlib\n'
        '     host=int(gdb.parse_and_eval("recon_osfhnd[%d]"%fd))-1;pid=gdb.selected_inferior().pid\n'
        '     actual=os.stat("/proc/%d/fd/%d"%(pid,host));expected_file=os.stat('+repr(str(directory/'debug-work'/'hello-debug.dll'))+')\n'
        '     info=pathlib.Path("/proc/%d/fdinfo/%d"%(pid,host)).read_text()\n'
        '     descriptor.update(type=1 if stat.S_ISREG(actual.st_mode)else -1,position=int(next(line.split()[1]for line in info.splitlines()if line.startswith("pos:"))),owned=(actual.st_dev,actual.st_ino)==(expected_file.st_dev,expected_file.st_ino))\n'
        '    nodes[-1]["descriptors"]={"16":descriptor}\n'
        '  pointer=0 if item["next"]==0xffffffff else int.from_bytes(raw[item["next"]:item["next"]+4],"little")\n'
        ' if pointer: raise RuntimeError("unterminated object walk: "+name)\n'
        ' if (item.get("type_cache") or item.get("global_type_cache")) and nodes: runpy.run_path('+repr(str(BINARY/'scripts/native-debug.py'))+')["capture_type_cache"](nodes[0])\n'
        ' walks[name]=nodes\n'
        'json.dump(dict(signal=int(signal),frames=frames,argc=argc,argv=argv,stdout=stream,state=state,parameters=parameters,globals=globals,global_addresses=global_addresses,walks=walks,pending_read_entries=[r for r in getattr(gdb,"cvpack_read_events",[])if "returned"not in r]),open('+repr(str(report))+',"w"))\n'
        'end\nquit\n')
    env = os.environ.copy()
    for variable in list(env):
        if variable.startswith(('WINE','RECON_')) or variable in ('CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','_MSC_IDE_FLAGS','LINK32_DB'):
            env.pop(variable)
    env.update(LC_ALL='C',TZ='PST8PDT',RECON_PROGRAM_PATH=canonical,PATH='',
               LIB='Z:'+str(ROOT/'toolchains/1.0-nt/lib').replace('/','\\'))
    from inputs import environment as input_environment,verify as verify_inputs
    input_environment(record,work,env)
    symbols=directory/'trace-filter'
    symbols.write_text(''.join(r['native']+'\n' for r in config['functions']))
    seen=directory/'trace-seen'
    seen.write_bytes(b'')
    trace=directory/'trace'
    setup = time.perf_counter()-start
    run = time.perf_counter()
    def no_core():
        resource.setrlimit(resource.RLIMIT_CORE,(0,0))
    processes=Processes(name,work,env,expected.get('spawn_requests',0))
    with trace.open('wb') as log,stdout.open('wb') as out,stderr.open('wb') as err:
        env.update(RECON_TRACE_FD=str(log.fileno()),RECON_TRACE_FILTER=str(symbols),
                   RECON_TRACE_SEEN=str(seen))
        child=subprocess.run([canonical,*record['argv']],executable=str(executable),cwd=work,env=env,
            stdout=out,stderr=err,stdin=subprocess.DEVNULL,timeout=10,
            pass_fds=(log.fileno(),processes.fileno()),preexec_fn=no_core)
    verify_inputs(record,work)
    processes.close()
    processes.check()
    execution = time.perf_counter()-run
    debug_start=time.perf_counter()
    if debug:
        for variable in list(env):
            if variable.startswith('RECON_TRACE_'): env.pop(variable)
        # A debugger rerun starts from an independent copy of every input.
        debug_work=directory/'debug-work'
        prepare(record,debug_work)
        input_environment(record,debug_work,env)
        debug_processes=Processes(name,debug_work,env,expected.get('spawn_requests',0))
        # GDB records frames/state; direct execution above owns fixture checks.
        debugger=subprocess.run(['/usr/bin/gdb','-q','-batch','-x',str(script),str(executable)],
            cwd=debug_work,env=env,stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=30,pass_fds=(debug_processes.fileno(),))
        verify_inputs(record,debug_work)
        debug_processes.close()
        debug_processes.check()
        (directory/'gdb.stdout').write_bytes(debugger.stdout)
        (directory/'gdb.stderr').write_bytes(debugger.stderr)
        assert debugger.returncode==0,debugger.stdout.decode('latin1')+debugger.stderr.decode('latin1')
    debugging=time.perf_counter()-debug_start if debug else 0
    check = time.perf_counter()
    assert expected['terminal'] in ('trap','exit')
    assert child.returncode==(-5 if expected['terminal']=='trap' else expected['exit']),child.returncode
    entries=trace.read_text().splitlines()
    if expected['terminal']=='trap':assert entries and entries[-1]==expected['native_function'],entries
    application={r['native'] for r in config['functions'] if r['kind']=='application'}
    assert [n for n in entries if n in application]==expected.get('application_trace',[]),entries
    if debug:
        actual=json.loads(report.read_text())
        assert actual['signal']==5,actual
        assert expected['native_function'] in actual['frames'],actual
    for stream in ('stdout','stderr'):
        reference = BINARY / 'cases' / name / expected[stream]
        assert (directory/stream).read_bytes()==reference.read_bytes(), stream+' mismatch'
    outputs={item['name']:item for item in expected.get('artifacts',[])}
    for item in record.get('inputs',[]):
        artifact=outputs.get(item['name'],item)
        assert sha(work/item['name'])==artifact['sha256'], 'artifact mismatch: '+item['name']
    assert sha(snapshot)==sha(BINARY/'functions.json'),'configuration changed during native run'
    assert sha(executable)==executable_hash,'native executable changed during run'
    check_policy_snapshot(directory,policy_hash)
    return dict(case=name,terminal=expected['terminal'],status=child.returncode,function=expected.get('native_function'),
        executable_sha256=executable_hash,configuration_sha256=sha(snapshot),original_sha256=config['target']['sha256'],
        conditional_support_sha256=policy_hash,
        process_boundary=processes.receipt(),
        seconds=dict(setup=setup,execution=execution,debugging=debugging,
                     checks=time.perf_counter()-check,total=time.perf_counter()-start))

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case')
    parser.add_argument('--output',type=Path)
    parser.add_argument('--debug',action='store_true')
    args=parser.parse_args()
    directory=args.output or Path.home()/'.ai-tmp/vc1.0-cvpack'/('native-'+str(time.time_ns()))
    directory.mkdir(parents=True,exist_ok=False)
    names=[args.case] if args.case else [r['name'] for r in json.loads((BINARY/'cases/queue.json').read_text())['cases'] if r['status']!='queued']
    receipts=[]
    for name in names:
        case_directory=directory/name
        case_directory.mkdir()
        receipts.append(run_case(name,case_directory,args.debug))
    (directory/'receipt.json').write_text(json.dumps(receipts,indent=2)+'\n')
    for receipt in receipts:print(receipt['case'],receipt['terminal'],receipt['seconds'])
    print(directory/'receipt.json')

if __name__=='__main__':
    main()
