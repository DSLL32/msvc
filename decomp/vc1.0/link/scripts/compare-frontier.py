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
    from native_tempnam import install as install_tempnam
    install_tempnam(spec,spec['case'],env)
    process_fixture=None
    if spec.get('native_process_boundary'):
        import importlib.util
        module_spec=importlib.util.spec_from_file_location('native_frontier_repro',BINARY/'scripts/native-repro.py')
        module=importlib.util.module_from_spec(module_spec);module_spec.loader.exec_module(module)
        process_fixture=module.ProcessFixture(work,env)
    script=output/'entry.gdb'
    script.write_text('set pagination off\nset confirm off\nbreak *'+args.function+'\nrun '+shlex.join(spec['arguments'])+' >'+str(output/'native.stdout')+' 2>'+str(output/'native.stderr')+'\nprintf "NATIVE_MATCH '+args.function+'\\n"\nx/6wx $esp\nbt 5\nquit\n')
    from resource_abi import LOGICAL as RESOURCE_ABI, gdb_commands as resource_abi_commands
    resource_path=output/'native-resource-abi.json'
    if args.function in RESOURCE_ABI:
        script.write_text(script.read_text().replace('quit\n',resource_abi_commands(args.function,resource_path)+'quit\n'))
    from editor_error_abi import LOGICAL as EDITOR_ERROR_ABI, gdb_commands as editor_error_commands
    editor_error_path=output/'native-editor-error-abi.json'
    if args.function in EDITOR_ERROR_ABI:
        script.write_text(script.read_text().replace('quit\n',editor_error_commands(args.function,editor_error_path)+'quit\n'))
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
    if (args.function.startswith('decode_op_') or args.function=='decode_descriptor_memory'):
        decoder_path=output/'native-decoder-state.json'
        commands="""python
import gdb,json,struct
esp=int(gdb.parse_and_eval('$esp'))
read=lambda address,size: bytes(gdb.selected_inferior().read_memory(address,size))
stack=struct.unpack('<4I',read(esp,16))
words=list(struct.unpack('<56I',read(stack[3],224)))
base=words[19]
words[18]=(words[18]-base)&0xffffffff if words[18] else 0xffffffff
words[19]=int(bool(base))
state=dict(event='decoder_state',words=words,operand=stack[2],cursor=(stack[1]-base)&0xffffffff)
open(%r,'w').write(json.dumps(state,indent=2)+'\\n')
end
""" % str(decoder_path)
        script.write_text(script.read_text().replace('quit\n',commands+'quit\n'))
    if args.function=='write_helper_thunk_member':
        machine_path=output/'native-import-machine.json'
        commands="""python
import gdb,json,struct
esp=int(gdb.parse_and_eval('$esp'))
stack=struct.unpack('<3I',bytes(gdb.selected_inferior().read_memory(esp,12)))
words=list(struct.unpack('<9I',bytes(gdb.selected_inferior().read_memory(stack[1],36))))
references={n:int(gdb.parse_and_eval('&'+n)) for n in ['import_jump_i860','import_relocations_i860','import_helper_i860']}
state=dict(bytes=int(gdb.parse_and_eval('sizeof(ImportMachine)')),machine_is_i860=stack[1]==int(gdb.parse_and_eval('&import_i860')),image_is_export_image=stack[2]==int(gdb.parse_and_eval('export_image')),words=words,references=references)
open(%r,'w').write(json.dumps(state,indent=2)+'\\n')
end
""" % str(machine_path)
        script.write_text(script.read_text().replace('quit\n',commands+'quit\n'))
    if args.function=='recon_longjmp':
        jump_path=output/'native-longjmp-state.json'
        commands="""python
import gdb,json,struct
esp=int(gdb.parse_and_eval('$esp'))
args=struct.unpack('<3I',bytes(gdb.selected_inferior().read_memory(esp,12)))
legacy=int(gdb.parse_and_eval('&disassembler_jump'))
context=gdb.parse_and_eval('jump_contexts')
found=[]
while int(context):
    if int(context['legacy'])==legacy:found.append(int(context['opaque_setjmp3']))
    context=context['next']
state=dict(buffer_is_disassembler=args[1]==legacy,value=args[2],registered_contexts=found)
open(%r,'w').write(json.dumps(state,indent=2)+'\\n')
end
""" % str(jump_path)
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
    decoder_state=None
    if (args.function.startswith('decode_op_') or args.function=='decode_descriptor_memory'):
        states=[event for event in events if event['event']=='decoder_state']
        from decoder_state import compare as compare_decoder_state
        decoder_state=json.loads(decoder_path.read_text())
        compare_decoder_state(states,decoder_state)
    resource_abi_state=None
    if args.function in RESOURCE_ABI:
        from resource_abi import compare as compare_resource_abi
        resource_abi_state=compare_resource_abi(events,json.loads(resource_path.read_text()))
    editor_error_abi_state=None
    if args.function in EDITOR_ERROR_ABI:
        from editor_error_abi import compare as compare_editor_error_abi
        editor_error_abi_state=compare_editor_error_abi(events,json.loads(editor_error_path.read_text()))
    import_machine_state=None
    if args.function=='write_helper_thunk_member':
        observations=[e for e in events if e['event']=='import_machine_state']
        assert len(observations)==1, 'Missing/nonunique original descriptor state'
        expected=observations[0];actual=json.loads(machine_path.read_text())
        assert expected['bytes']==actual['bytes']==36
        aliases={r['name']:r['address'] for r in json.loads((BINARY/'data-symbols.json').read_text())}
        assert expected['argument']==aliases['import_i860']
        assert actual['machine_is_i860'] and actual['image_is_export_image']
        left=expected['words'][:];right=actual['words'][:]
        for index,name in [(3,'import_jump_i860'),(5,'import_relocations_i860'),(7,'import_helper_i860')]:
            assert left[index]==aliases[name] and right[index]==actual['references'][name]
            left[index]=right[index]=name
        assert left==right,(left,right)
        import_machine_state=dict(status='pass',bytes=36,normalized_words=left,original=expected,native=actual)
    longjmp_state=None
    if args.function=='recon_longjmp':
        states=[event for event in events if event['event']=='longjmp_boundary']
        assert len(states)==1, 'Missing/nonunique original longjmp observation'
        actual=json.loads(jump_path.read_text());expected=states[0]
        assert expected['buffer_is_disassembler']==actual['buffer_is_disassembler']==True
        assert expected['value']==actual['value']==1
        assert expected['registration_is_current']==True, 'Global unwind must be bypassed'
        assert actual['registered_contexts']==[0], 'Native context must be plain eight-word setjmp'
        assert expected['saved_scope']==expected['registration_scope']==0, 'Original local-unwind loop must already be at its saved scope'
        assert 0x4ae20<=expected['saved_eip']<0x4b192, 'Original continuation must belong to disassemble_section'
        longjmp_state=dict(original=expected,native=actual,status='pass')
    typed_state=None
    if args.function=='emit_rom_relocations':
        original_state=next(event for event in events if event['event']=='rom_fixups')
        native_state=json.loads(state_path.read_text())
        assert original_state==native_state,(original_state,native_state)
        typed_state=dict(status='pass',live_fields=['type@0:2','rva@8:4','value@12:4'],stride=16,
                         original=original_state,native=native_state,
                         unconsumed='Record+4 has no proved reader/writer semantics; never compared or serialized. Type padding+2 likewise not a live field.')
        (output/'rom-fixups-comparison.json').write_text(json.dumps(typed_state,indent=2)+'\n')
    receipt=dict(status='pass',logical=args.function,original_address=hex(args.address),original_directory=str(original_directory),native_directory=str(output),native_context=context,seconds=time.perf_counter()-start,parallel_sides=True,partial_streams_exact=True,typed_rom_fixups=typed_state,resource_abi_state=resource_abi_state,editor_error_abi_state=editor_error_abi_state,import_machine_state=import_machine_state,decoder_state=decoder_state,longjmp_state=longjmp_state,caller_edge=caller_proof)
    (output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(output/'receipt.json')
if __name__=='__main__':main()
