#!/usr/bin/env python3
"""Fresh original-only masked capture alongside native GDB; never a fixture."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
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
    env={key:value for key,value in os.environ.items() if not key.startswith('RECON_')}
    env.update(drives,RECON_PROGRAM_PATH=PROGRAM,LIB=LIBRARY,TZ='PST8PDT',TMP=str(work),TEMP=str(work))
    for key in ('CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB'):env.pop(key,None)
    for key,value in spec.get('environment',{}).items():env[key]=value.replace('{lib}',LIBRARY)
    script=output/'entry.gdb'
    script.write_text('set pagination off\nset confirm off\nbreak *'+args.function+'\nrun '+shlex.join(spec['arguments'])+' >'+str(output/'native.stdout')+' 2>'+str(output/'native.stderr')+'\nprintf "NATIVE_MATCH '+args.function+'\\n"\nx/6wx $esp\nbt 5\nquit\n')
    def original():
        return subprocess.run(['python3',str(BINARY/'scripts/capture-case.py'),'--spec',str(args.spec.resolve()),'--mode','masked'],cwd=BINARY,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=55)
    def native():
        return subprocess.run(['gdb','-q','-batch','-x',str(script),'--args',str(BINARY/'build/native/LINK.EXE'),*spec['arguments']],cwd=work,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=35)
    start=time.perf_counter()
    with ThreadPoolExecutor(max_workers=2) as pool:
        left=pool.submit(original);right=pool.submit(native)
        original_result,native_result=left.result(),right.result()
    (output/'original-driver.log').write_bytes(original_result.stdout)
    (output/'native.gdb.log').write_bytes(native_result.stdout)
    assert original_result.returncode==native_result.returncode==0,(original_result.stdout,native_result.stdout)
    original_directory=Path(original_result.stdout.decode().splitlines()[0])
    events=[json.loads(line) for line in (original_directory/'events.jsonl').read_text().splitlines()]
    context=next(event for event in events if event['event']=='context')
    assert context['eip']-1==args.address,context
    assert b'Breakpoint 1,' in native_result.stdout and ('in '+args.function+' (').encode() in native_result.stdout or ('Breakpoint 1, '+args.function+' (').encode() in native_result.stdout, native_result.stdout
    assert b'NATIVE_MATCH '+args.function.encode() in native_result.stdout
    for key in ('stdout','stderr'):
        assert (output/('native.'+key)).read_bytes()==(original_directory/(key+'.bin')).read_bytes(),key+' partial stream differs'
    receipt=dict(status='pass',logical=args.function,original_address=hex(args.address),original_directory=str(original_directory),native_directory=str(output),native_context=context,seconds=time.perf_counter()-start,parallel_sides=True,partial_streams_exact=True)
    (output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(output/'receipt.json')
if __name__=='__main__':main()
