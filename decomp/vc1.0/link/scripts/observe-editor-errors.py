#!/usr/bin/env python3
"""Explicit native GDB bounded-format/output proof; never a fixture dependency."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import time
from case_paths import SCRATCH,PROGRAM,LIBRARY,native_work

BINARY=Path(__file__).resolve().parents[1]
BASE=BINARY/'cases/editor-errors'


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--phase',choices=['rebase','bind','missing'],default='rebase');args=ap.parse_args()
    spec=json.loads((BASE/(args.phase+'-spec.json')).read_text());root=Path(tempfile.mkdtemp(prefix=spec['case']+'-gdb-',dir=SCRATCH/'vc1.0/link/frontiers'));work,drives=native_work(root/'native','S:\\vc1.0\\link\\'+spec['case'])
    for item in spec['inputs']:
        source=BINARY/item['path'];assert hashlib.sha256(source.read_bytes()).hexdigest()==item['sha256'];shutil.copyfile(source,work/source.name)
    env={k:v for k,v in os.environ.items() if not k.startswith('RECON_')}
    for key in ['CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB']:env.pop(key,None)
    env.update(drives,RECON_PROGRAM_PATH=PROGRAM,LIB=LIBRARY,TZ='PST8PDT',TMP=str(work),TEMP=str(work))
    for key,value in spec.get('environment',{}).items():env[key]=value.replace('{lib}',LIBRARY)
    state=root/'state.json'
    script='''set pagination off
set confirm off
python
import gdb,json,struct
state={'formatted':[]}
path=%r
def save():open(path,'w').write(json.dumps(state,indent=2)+'\\n')
class Return(gdb.FinishBreakpoint):
    def __init__(self,frame,output,capacity):
        super().__init__(frame,internal=True);self.output=output;self.capacity=capacity
    def stop(self):
        value=int(self.return_value);assert 0<=value<self.capacity
        raw=bytes(gdb.selected_inferior().read_memory(self.output,value+1));assert raw[-1]==0
        state['formatted'].append(dict(result=value,capacity=self.capacity,text=raw[:-1].decode('latin1'),errno=int(gdb.parse_and_eval('(int)errno'))));save();return False
class Enter(gdb.Breakpoint):
    def stop(self):
        esp=int(gdb.parse_and_eval('$esp'));words=struct.unpack('<5I',bytes(gdb.selected_inferior().read_memory(esp,20)))
        caller=gdb.newest_frame().older();assert caller.name() in ('editor_rebase_error','editor_bind_error')
        Return(gdb.newest_frame(),words[1],words[2]);return False
Enter('*recon_snprintf')
def exited(event):state['exit']=event.exit_code;save()
gdb.events.exited.connect(exited)
end
run %s >%s 2>%s
quit
'''%(str(state),shlex.join(spec['arguments']),str(root/'stdout.bin'),str(root/'stderr.bin'))
    (root/'entry.gdb').write_text(script);start=time.perf_counter();r=subprocess.run(['gdb','-q','-batch','-x',str(root/'entry.gdb'),'--args',str(BINARY/'build/native/LINK.EXE')],cwd=work,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);elapsed=time.perf_counter()-start;(root/'gdb.log').write_bytes(r.stdout)
    assert r.returncode==0 and state.exists(),r.stdout.decode();actual=json.loads(state.read_text());expected=BASE/'reference'/args.phase/'1';original=json.loads((expected/'receipt.json').read_text());assert actual['exit']==original['summary']['root_exit']
    for key in ['stdout','stderr']:assert (root/(key+'.bin')).read_bytes()==(expected/(key+'.bin')).read_bytes(),key
    files={p.name:dict(bytes=p.stat().st_size,sha256=hashlib.sha256(p.read_bytes()).hexdigest()) for p in work.iterdir() if p.is_file()};assert files==original['working_files']
    if args.phase=='rebase':
        events=[json.loads(s) for s in (BASE/'snprintf/original/events.jsonl').read_text().splitlines()];results=[e for e in events if e['event']=='editor_snprintf_result'];assert len(results)==1 and actual['formatted']==[{k:v for k,v in results[0].items() if k!='event'}]
    receipt=dict(status='pass',phase=args.phase,native=actual,seconds=elapsed,directory=str(root),working_files=files,scope='Native ELF/GDB only; exact cached original streams/status/allfiles, bounded snprintf return/defined text/errno where observed. No native internal CRT ABI or uninitialized output-tail equivalence.')
    (root/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(root/'receipt.json')
if __name__=='__main__':main()
