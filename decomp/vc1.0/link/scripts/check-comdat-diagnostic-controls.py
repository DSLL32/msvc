#!/usr/bin/env python3
"""Host-only mutations plus one ELF address-refusal control, never originals."""
import copy,hashlib,importlib.util,json,os,struct,subprocess,tempfile,time
from pathlib import Path
from comdat_diagnostic_abi import compare_history
from indexes import indexes
from case_paths import ROOT,SCRATCH,PROGRAM,native_work
BINARY=Path(__file__).resolve().parents[1];CASE=BINARY/'cases/comdat-graph-diagnostics'
spec=importlib.util.spec_from_file_location('comdat_fixture',BINARY/'scripts/check-comdat-diagnostics.py');fixture=importlib.util.module_from_spec(spec);spec.loader.exec_module(fixture)
def main():
    start=time.perf_counter();pe,*_=indexes();native=json.loads((CASE/'acceptance/native-final/state.json').read_text());original=[json.loads(x) for x in (CASE/'acceptance/print_comdat_edge/original-trial/events.jsonl').read_text().splitlines()];payload=(CASE/'inputs/graph.bin').read_bytes();assert compare_history(native,original,payload,pe,True)
    rejected=[]
    def reject(name,action):
        try:action()
        except (AssertionError,KeyError,IndexError,struct.error,ValueError):rejected.append(name);return
        raise AssertionError('negative accepted: '+name)
    def mutate(label,selector,field,value):
        data=copy.deepcopy(native);event=next(x for x in data['events'] if selector(x));event[field]=value;reject(label,lambda:compare_history(data,original,payload,pe,True))
    first=lambda x:x['event']=='entry' and x['name']=='dump_comdat_graph'
    printf=lambda x:x['event']=='entry' and x['name']=='debug_printf' and x['format']==b'|con=0x%p'.hex()
    ret=lambda x:x['event']=='return' and x['name']=='print_comdat_node'
    for label,field,value in [('missing-actual-formals','arguments',[]),('wrong-input-address','input_base',0x12001000),('wrong-owned-root','arguments',[1,0x12000080]),('changed-guard','region','00'*4096),('wrong-error','errors',[1,0,0]),('missing-ctype','ctype',''),('wrong-debug-mode','debug_flags','01'+'00'*99),('wrong-log-owner','debug_log',1),('unexpected-caller','actual_caller','main')]:mutate(label,first,field,value)
    mutate('wrong-defined-pointer',printf,'arguments',[1,1,0x12000300])
    mutate('wrong-consumed-word-count',printf,'arguments',[1,1])
    mutate('wrong-format',printf,'format',b'|con=0x%x'.hex())
    mutate('changed-pending',ret,'pending','00')
    mutate('wrong-result',ret,'eax',2)
    mutate('wrong-stack-return',ret,'esp',0)
    mutate('nonvolatile-clobber',ret,'ebx',0)
    mutate('wrong-label-text',lambda x:x['event']=='return' and x['name']=='comdat_usage_label','return_text',b'X'.hex())
    data=copy.deepcopy(native);data['events']=[];reject('empty-native-inventory',lambda:compare_history(data,original,payload,pe,True))
    reject('empty-original-inventory',lambda:compare_history(native,[],payload,pe,True))
    data=copy.deepcopy(native);data['events'].pop(4);reject('omitted-real-entry',lambda:compare_history(data,original,payload,pe,True))
    data=copy.deepcopy(native);data['terminal']=[];reject('missing-terminal',lambda:compare_history(data,original,payload,pe,True))
    stdout=(CASE/'baseline/1/stdout.bin').read_bytes();fixture.validate_stdout(stdout)
    reject('old-libc-prefix',lambda:fixture.validate_stdout(stdout.replace(b'0x12000100',b'0x0x12000100')))
    reject('changed-pointer-byte',lambda:fixture.validate_stdout(stdout.replace(b'12000140',b'12000144')))
    reject('LF-only',lambda:fixture.validate_stdout(stdout.replace(b'\r\n',b'\n')))
    expected=['dump_comdat_graph','debug_printf','print_comdat_node','comdat_usage_label','print_comdat_edge']
    reject('empty-trace',lambda:fixture.validate_trace([],expected));reject('both-empty-trace',lambda:fixture.validate_trace([],[]));reject('unexpected-stub',lambda:fixture.validate_trace(expected+['open_debug_log'],expected))
    directory=Path(tempfile.mkdtemp(prefix='comdat-refusal-',dir=SCRATCH/'vc1.0/link'));work,drives=native_work(directory,r'S:\vc1.0\link\comdat-graph-diagnostics');exe=BINARY/'build/native-trace/comdat-diagnostics';env={k:v for k,v in os.environ.items() if not k.startswith('RECON_')};env.update(drives,RECON_PROGRAM_PATH=PROGRAM,LC_ALL='C',TZ='PST8PDT')
    spec=importlib.util.spec_from_file_location('comdat_control_trace',BINARY/'scripts/native-traces.py');trace=importlib.util.module_from_spec(spec);spec.loader.exec_module(trace);cache=trace.load(BINARY/'build/native-trace');(directory/'filter').write_text(''.join(n+'\n' for n in sorted(cache['application_definitions'])));(directory/'seen').write_text('')
    with (directory/'trace').open('wb') as fd:
        env.update(RECON_TRACE_FD=str(fd.fileno()),RECON_TRACE_FILTER=str(directory/'filter'),RECON_TRACE_SEEN=str(directory/'seen'));t=time.perf_counter();result=subprocess.run([str(exe),str(CASE/'inputs/graph.bin'),'--occupied'],cwd=work,env=env,pass_fds=(fd.fileno(),),capture_output=True,timeout=5);seconds=time.perf_counter()-t
    assert result.returncode==122 and result.stdout==result.stderr==b'' and (directory/'trace').read_bytes()==b'' and not list(work.iterdir()),'occupied input address must deny without target entry/file effect'
    receipt=dict(status='pass',negative_count=len(rejected),rejected=rejected,total_seconds=time.perf_counter()-start,occupied_address_control=dict(driver_exit=122,app_entries=0,seconds=seconds,elf_sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),scope='External caller address refusal, not original target behavior; owned4096-byte5A region checked unchanged and released by host before122'),scope='Mutation checks are host-only; refusal runs only ELF, no original/Wine/debugger/helper')
    (CASE/'negative-controls.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':main()
