#!/usr/bin/env python3
"""Explicit native GDB public-vfprintf output proof; never a fixture dependency."""
import hashlib,importlib.util,json,os,shlex,shutil,subprocess,tempfile,time
from pathlib import Path
from case_paths import SCRATCH,PROGRAM,LIBRARY,native_work
BINARY=Path(__file__).resolve().parents[1];BASE=BINARY/'cases/omf-continuation'

def main():
    spec=json.loads((BASE/'use16-controlled-spec.json').read_text());root=Path(tempfile.mkdtemp(prefix='omf-warning-complete-',dir=SCRATCH/'vc1.0/link/frontiers'));work,drives=native_work(root/'native','S:\\vc1.0\\link\\'+spec['case'])
    for item in spec['inputs']:
        source=BINARY/item['path'];assert hashlib.sha256(source.read_bytes()).hexdigest()==item['sha256'];shutil.copyfile(source,work/source.name)
    env={k:v for k,v in os.environ.items() if not k.startswith('RECON_')}
    for key in ['CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB']:env.pop(key,None)
    env.update(drives,RECON_PROGRAM_PATH=PROGRAM,LIB=LIBRARY,TZ='PST8PDT',TMP=str(work),TEMP=str(work))
    for k,v in spec['environment'].items():env[k]=v.replace('{lib}',LIBRARY)
    from native_tempnam import install
    install(spec,spec['case'],env)
    state=root/'state.json';script='''set pagination off
set confirm off
python
import gdb,json,struct
state={'results':[],'entries':[],'bypassed':[]};path=%r
def save():open(path,'w').write(json.dumps(state,indent=2)+'\\n')
class Return(gdb.FinishBreakpoint):
    def __init__(self,frame,stream):super().__init__(frame,internal=True);self.stream=stream
    def stop(self):
        out=struct.unpack('<8I',bytes(gdb.selected_inferior().read_memory(self.stream,32)));used=out[0]-out[2];assert 0<=used<=256
        state['results'].append(dict(result=int(self.return_value),errno=int(gdb.parse_and_eval('(int)errno')),doserrno=int(gdb.parse_and_eval('recon_doserrno')),last_error=int(gdb.parse_and_eval("'win32.c'::last_error")),count=out[1],flags=out[3],file=out[4],buffer_size=out[6],buffered=list(bytes(gdb.selected_inferior().read_memory(out[2],used)))))
        save();return False
class Enter(gdb.Breakpoint):
    def stop(self):
        caller=gdb.newest_frame().older();assert caller
        if caller.name() in ('recon_printf','recon_fprintf'):state['bypassed'].append(caller.name());return False
        assert caller.name()=='omf_warning'
        esp=int(gdb.parse_and_eval('$esp'));words=struct.unpack('<4I',bytes(gdb.selected_inferior().read_memory(esp,16)))
        assert words[1]==int(gdb.parse_and_eval('&recon_iob[1]'))
        out=struct.unpack('<8I',bytes(gdb.selected_inferior().read_memory(words[1],32)));used=out[0]-out[2];assert 0<=used<=256
        state['entries'].append(dict(count=out[1],flags=out[3],file=out[4],buffer_size=out[6],buffered=list(bytes(gdb.selected_inferior().read_memory(out[2],used)))))
        current=gdb.newest_frame();caller.select();owner=words[3]==int(gdb.parse_and_eval('&format'))+4;current.select()
        state['boundary_entry']=dict(logical='recon_vfprintf',caller_name=caller.name(),**{'pass':int(gdb.parse_and_eval('omf_pass'))},formals=dict(stream_is_stdout=True,format=gdb.Value(words[2]).cast(gdb.lookup_type('char').pointer()).string(),variadic_is_warning_area=owner,consumed_variadic_slots=0),stream=state['entries'][-1],errors=dict(errno=int(gdb.parse_and_eval('(int)errno')),doserrno=int(gdb.parse_and_eval('recon_doserrno')),last_error=int(gdb.parse_and_eval("'win32.c'::last_error"))))
        Return(current,words[1]);return False
Enter('*recon_vfprintf')
def exited(event):state['exit']=event.exit_code;save()
gdb.events.exited.connect(exited)
end
run %s >%s 2>%s
quit
'''%(str(state),shlex.join(spec['arguments']),str(root/'stdout.bin'),str(root/'stderr.bin'))
    (root/'entry.gdb').write_text(script);t=time.perf_counter();r=subprocess.run(['gdb','-q','-batch','-x',str(root/'entry.gdb'),'--args',str(BINARY/'build/native/LINK.EXE')],cwd=work,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);elapsed=time.perf_counter()-t;(root/'gdb.log').write_bytes(r.stdout);assert r.returncode==0 and state.exists(),r.stdout.decode()
    actual=json.loads(state.read_text());original=BASE/'warning-result/complete-original';events=[json.loads(s) for s in (original/'events.jsonl').read_text().splitlines()];rows=[e for e in events if e['event']=='omf_vfprintf_result'];assert len(rows)==1
    context=[e for e in events if e['event']=='context' and e['eip'] in (0x4817c,0x4817d)];assert len(context)==1
    from omf_continuation_abi import compare_result
    expected=compare_result(events,actual['results'])
    entries=[e for e in events if e['event']=='omf_continuation_abi' and e['logical']=='recon_vfprintf'];assert len(entries)==1 and actual['entries']==[entries[0]['stream']]
    assert actual['exit']==0
    for key in ['stdout','stderr']:assert (root/(key+'.bin')).read_bytes()==(BASE/'preflight/use16/1'/(key+'.bin')).read_bytes()
    s=importlib.util.spec_from_file_location('art',BINARY/'scripts/artifacts-omf-continuation.py');m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
    assert m.normalize_artifact((work/'use16.lib').read_bytes(),'archive-generated')==m.normalize_artifact((BASE/'preflight/use16/1/use16.lib').read_bytes(),'archive-generated')
    assert sorted(p.name for p in work.iterdir() if p.is_file())==['use16.lib','use16.obj']
    from omf_continuation_abi import compare
    abi=compare(events,actual['boundary_entry'])
    receipt=dict(incoming_abi=abi,status='pass',native=actual,original=expected,seconds=elapsed,directory=str(root),scope='Named public boundary only. Original/canonical shipped-vfprintf graph retained separately; no private native CRT graph/ABI claim. Full stdout buffer/result/errors and complete cached streams/archive/inventory exact.')
    (root/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(root/'receipt.json')
if __name__=='__main__':main()
