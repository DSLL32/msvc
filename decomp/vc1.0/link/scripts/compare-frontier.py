#!/usr/bin/env python3
"""Fresh original-only masked capture alongside native GDB; never a fixture."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
import re
from pathlib import Path
import shutil
import shlex
import subprocess
import tempfile
import time
from case_paths import ROOT,SCRATCH,PROGRAM,LIBRARY,native_work
BINARY=Path(__file__).resolve().parents[1]

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--spec',type=Path,required=True)
    ap.add_argument('--function',required=True)
    ap.add_argument('--address',type=lambda s:int(s,0),required=True)
    ap.add_argument('--caller',help='Require this accepted direct caller when native API aliases have earlier GPA calls')
    ap.add_argument('--earlier-caller',action='append',default=[],help='Explicit reviewed accepted GPA caller that may run before the direct thunk')
    args=ap.parse_args()
    spec=json.loads(args.spec.read_text())
    parent=SCRATCH/'vc1.0/link/frontiers'
    parent.mkdir(parents=True,exist_ok=True)
    output=Path(tempfile.mkdtemp(prefix=spec['case']+'-'+args.function+'-',dir=parent))
    canonical='S:\\vc1.0\\link\\'+spec['case']
    work,drives=native_work(output/'native',canonical)
    for item in spec['inputs']:
        source=(BINARY/item['path']).resolve()
        assert hashlib.sha256(source.read_bytes()).hexdigest()==item['sha256']
        shutil.copyfile(source,work/source.name)
    for directory in spec.get('directories',[]):
        name=Path(directory)
        assert not name.is_absolute() and '..' not in name.parts
        (work/name).mkdir(parents=True,exist_ok=True)
    env={key:value for key,value in os.environ.items() if not key.startswith('RECON_')}
    env.update(drives,RECON_PROGRAM_PATH=PROGRAM,LIB=LIBRARY,TZ='PST8PDT',TMP=str(work),TEMP=str(work))
    for key in ('CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB'):env.pop(key,None)
    for key,value in spec.get('environment',{}).items():env[key]=value.replace('{lib}',LIBRARY)
    process_fixture=None
    if spec.get('native_process_boundary'):
        import importlib.util
        module_spec=importlib.util.spec_from_file_location('native_frontier_repro',BINARY/'scripts/native-repro.py')
        module=importlib.util.module_from_spec(module_spec);module_spec.loader.exec_module(module)
        process_fixture=module.ProcessFixture(work,env)
    script=output/'entry.gdb'
    script.write_text('set pagination off\nset confirm off\nbreak *'+args.function+'\nrun '+shlex.join(spec['arguments'])+' >'+str(output/'native.stdout')+' 2>'+str(output/'native.stderr')+'\nprintf "NATIVE_MATCH '+args.function+'\\n"\nx/6wx $esp\nbt 5\nquit\n')
    if args.function=='emit_rom_relocations':
        state_path=output/'native-rom-fixups.json'
        commands="""python
import gdb,json
first=gdb.parse_and_eval('base_fixups')
count=int(gdb.parse_and_eval('base_fixup_next-base_fixups'))
assert 0<=count<=256
records=[dict(type=int(first[i]['type']),rva=int(first[i]['rva']),value=int(first[i]['value']['symbol_index'])) for i in range(count)]
state=dict(event='rom_fixups',stride=int(gdb.parse_and_eval('sizeof(BaseFixup)')),count=count,records=records)
open(%r,'w').write(json.dumps(state,indent=2)+'\\n')
end
""" % str(state_path)
        script.write_text(script.read_text().replace('quit\n',commands+'quit\n'))
    caller_record=None
    assert not args.earlier_caller or args.caller, 'Earlier caller requires a candidate caller'
    if args.caller:
        config=json.loads((BINARY/'functions.json').read_text())
        candidates=[r for r in config['driver'] if r['name'][1:].split('@')[0]==args.caller]
        assert len(candidates)==1 and candidates[0]['name'] in config['accepted'], 'Caller must be a known accepted application'
        caller_record=candidates[0]
        for name in args.earlier_caller:
            previous=[r for r in config['driver'] if r['name'][1:].split('@')[0]==name]
            assert name!=args.caller and len(previous)==1 and previous[0]['name'] in config['accepted'] and previous[0]['implementation']=='C', 'Earlier GPA caller must be reviewed and accepted'
        setup="""python
import gdb
class QualifiedBreakpoint(gdb.Breakpoint):
    def stop(self):
        frame=gdb.newest_frame().older()
        if frame is None or frame.name() is None:
            raise gdb.GdbError('Missing native API caller')
        if frame.name()==%r:return True
        if frame.name() in %r:
            gdb.write('NATIVE_REVIEWED_EARLIER '+frame.name()+'\\n')
            return False
        raise gdb.GdbError('Unexpected native API caller: '+frame.name())
QualifiedBreakpoint(%r)
end
""" % (args.caller,args.earlier_caller,'*'+args.function)
        script.write_text(script.read_text().replace('break *'+args.function+'\n',setup))
    def original():
        return subprocess.run(['python3',str(BINARY/'scripts/capture-case.py'),'--spec',str(args.spec.resolve()),'--mode','masked'],cwd=BINARY,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=55)
    def native():
        return subprocess.run(['gdb','-q','-batch','-x',str(script),'--args',str(BINARY/'build/native/LINK.EXE'),*spec['arguments']],cwd=work,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=35,pass_fds=(process_fixture.fileno(),) if process_fixture else ())
    start=time.perf_counter()
    with ThreadPoolExecutor(max_workers=2) as pool:
        left=pool.submit(original);right=pool.submit(native)
        original_result,native_result=left.result(),right.result()
    if process_fixture:
        process_fixture.close();process_fixture.check_partial()
        (output/'process.json').write_text(json.dumps(process_fixture.receipt(),indent=2)+'\n')
    (output/'original-driver.log').write_bytes(original_result.stdout)
    (output/'native.gdb.log').write_bytes(native_result.stdout)
    assert original_result.returncode==native_result.returncode==0,(original_result.stdout,native_result.stdout)
    original_directory=Path(original_result.stdout.decode().splitlines()[0])
    events=[json.loads(line) for line in (original_directory/'events.jsonl').read_text().splitlines()]
    context=next(event for event in events if event['event']=='context')
    assert context['eip']-1==args.address,context
    assert b'Breakpoint 1,' in native_result.stdout and (('in '+args.function+' (').encode() in native_result.stdout or ('Breakpoint 1, '+args.function+' (').encode() in native_result.stdout), native_result.stdout
    assert b'NATIVE_MATCH '+args.function.encode() in native_result.stdout
    for key in ('stdout','stderr'):
        assert (output/('native.'+key)).read_bytes()==(original_directory/(key+'.bin')).read_bytes(),key+' partial stream differs'
    caller_proof=None
    if caller_record:
        observed=re.findall(rb'^#1\s+(?:0x[0-9a-f]+\s+in\s+)?([A-Za-z_][A-Za-z_0-9]*)\s*\(',native_result.stdout,re.M)
        assert len(observed)==1 and observed[0].decode()==args.caller, 'Missing/unexpected actual native caller'
        actual_caller=observed[0].decode()
        import importlib.util
        module_spec=importlib.util.spec_from_file_location('frontier_call_edge',BINARY/'scripts/call-edge.py')
        module=importlib.util.module_from_spec(module_spec);module_spec.loader.exec_module(module)
        from binary import PE
        caller_proof=module.validate(context,args.address,caller_record,PE(ROOT/'toolchains/1.0-nt/bin/LINK.EXE'),actual_caller)
        caller_proof['reviewed_earlier_callers']=args.earlier_caller
        caller_proof['observed_earlier_callers']=re.findall(r'^NATIVE_REVIEWED_EARLIER (\w+)$',native_result.stdout.decode(),re.M)
    typed_state=None
    if args.function=='emit_rom_relocations':
        original_state=next(event for event in events if event['event']=='rom_fixups')
        native_state=json.loads(state_path.read_text())
        assert original_state==native_state,(original_state,native_state)
        typed_state=dict(status='pass',live_fields=['type@0:2','rva@8:4','value@12:4'],stride=16,
                         original=original_state,native=native_state,
                         unconsumed='Record+4 has no proved reader/writer semantics; never compared or serialized. Type padding+2 likewise not a live field.')
        (output/'rom-fixups-comparison.json').write_text(json.dumps(typed_state,indent=2)+'\n')
    receipt=dict(status='pass',logical=args.function,original_address=hex(args.address),original_directory=str(original_directory),native_directory=str(output),native_context=context,seconds=time.perf_counter()-start,parallel_sides=True,partial_streams_exact=True,typed_rom_fixups=typed_state,caller_edge=caller_proof)
    (output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(output/'receipt.json')
if __name__=='__main__':main()
