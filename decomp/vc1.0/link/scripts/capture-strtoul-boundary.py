#!/usr/bin/env python3
"""Explicit original/native public CRT evidence capture, outside fixture execution."""
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
import json,os,sys,subprocess,tempfile,shutil,shlex,time,hashlib,textwrap
def main():
    B=Path(__file__).resolve().parents[1];sys.path.insert(0,str(B/'scripts'));from case_paths import SCRATCH,PROGRAM,LIBRARY,native_work
    spec=json.loads((B/'cases/editor-rebase-bind/spec-rebase.json').read_text());out=Path(tempfile.mkdtemp(prefix='strtoul-boundary-',dir=SCRATCH/'vc1.0/link/frontiers'));work,drives=native_work(out/'native',r'S:\vc1.0\link\editor-rebase')
    for x in spec['inputs']:shutil.copyfile(B/x['path'],work/Path(x['path']).name)
    env={k:v for k,v in os.environ.items() if not k.startswith('RECON_')};env.update(drives,RECON_PROGRAM_PATH=PROGRAM,LIB=LIBRARY,TZ='PST8PDT',TMP=str(work),TEMP=str(work))
    for k in ('CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB','LINK_REPRO','COPYCMD'):env.pop(k,None)
    gdb=out/'entry.gdb';gdb.write_text(textwrap.dedent('''\
    set pagination off
    set confirm off
    break *strtoul
    run %s >%s 2>%s
    python
    import gdb,json
    inf=gdb.selected_inferior()
    esp=int(gdb.parse_and_eval('$esp'))
    a=[int.from_bytes(inf.read_memory(esp+i,4),'little') for i in (4,8,12)]
    text=bytes(inf.read_memory(a[0],9))
    assert text==b'0x500000\\x00'
    before=dict(input=text[:-1].decode(),base=a[2],end_pointer_present=a[1]!=0,errno=int(gdb.parse_and_eval('(int)errno')))
    end
    finish
    python
    result=int(gdb.parse_and_eval('$eax'))&0xffffffff
    endptr=int.from_bytes(inf.read_memory(a[1],4),'little')
    after=dict(result=result,end_offset=endptr-a[0],end_points_into_input=a[0]<=endptr<=a[0]+8,errno=int(gdb.parse_and_eval('(int)errno')))
    open(%r,'w').write(json.dumps(dict(before=before,after=after),indent=2)+'\\n')
    end
    quit
    ''')%(shlex.join(spec['arguments']),out/'stdout.bin',out/'stderr.bin',str(out/'native.json')))
    def left():return subprocess.run(['python3','scripts/capture-case.py','--spec','cases/editor-rebase-bind/spec-rebase.json','--mode','original','--sample','0x541d0:analysis_strtoul_entry','--sample','0x4ff96:analysis_strtoul_return'],cwd=B,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=40)
    def right():return subprocess.run(['gdb','-q','-batch','-x',str(gdb),'--args',str(B/'build/native/LINK.EXE')],cwd=work,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30)
    start=time.perf_counter()
    with ThreadPoolExecutor(max_workers=2) as pool:
     l=pool.submit(left);r=pool.submit(right);l,r=l.result(),r.result()
    (out/'original.log').write_bytes(l.stdout);(out/'native.log').write_bytes(r.stdout);assert l.returncode==r.returncode==0,(l.stdout,r.stdout)
    directory=Path(l.stdout.decode().splitlines()[0]);events=[json.loads(x) for x in (directory/'events.jsonl').read_text().splitlines()];before=[x for x in events if x['event']=='strtoul-before'];after=[x for x in events if x['event']=='strtoul-after'];entry=[x for x in events if x['event']=='entry'];assert len(before)==len(after)==1 and [x['name'] for x in entry]==['analysis_strtoul_entry','analysis_strtoul_return'],(before,after,entry)

    original=dict(before={k:v for k,v in before[0].items() if k!='event'},after={k:v for k,v in after[0].items() if k!='event'})
    original['before']['errno']=entry[0]['crt_errno'];original['after']['errno']=entry[1]['crt_errno']
    native=json.loads((out/'native.json').read_text());assert original==native,(original,native)
    expected=dict(before=dict(input='0x500000',base=16,end_pointer_present=True,errno=0),after=dict(result=0x500000,end_offset=8,end_points_into_input=True,errno=0))
    assert native==expected,(native,expected)
    receipt=dict(status='pass',expected_inventory=[dict(phase='editor-rebase',call=1,**expected)],original=[original],native=[native],seconds=time.perf_counter()-start,original_directory=str(directory),native_directory=str(out),policy='Public CRT boundary only; no native strtoxl helper ABI equivalence. Expected inventory is explicit/nonempty; omissions fail.',end_pointer_identity='End pointer is the same input allocation+8, at its NUL; no numerical process-pointer comparison.')
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(out/'receipt.json')

if __name__=='__main__':main()
