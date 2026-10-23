#!/usr/bin/env python3
"""Explicit original CL-to-C1 argument/return/IR contract capture."""
import argparse,json,shutil,subprocess,time,re
from wine_tool_support import environment,windows,PROJECT,TC,SCRATCH
from provenance import freeze,sha

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--name',required=True);a=p.parse_args();assert a.name.replace('-','').isalnum()
 d=SCRATCH/'vc5.10/c1'/a.name;d.mkdir(parents=True,exist_ok=False)
 for src,name in [(PROJECT/'build/minimal-cl-probe.exe','minimal-cl-probe.exe'),(TC/'bin/MSVCRT.DLL','MSVCRT.DLL'),(PROJECT/'cases/minimal/inputs/test.c','test.c')]:shutil.copy2(src,d/name)
 freeze([PROJECT/'scripts/minimal-cl-probe.c',PROJECT/'scripts/capture-minimal-cl.py',PROJECT/'scripts/wine_tool_support.py'],d/'source-snapshot.json')
 command=['wine',windows(d/'minimal-cl-probe.exe'),windows(TC/'bin/CL.EXE'),'/nologo /c test.c','events.txt'];start=time.perf_counter()
 with (d/'stdout.bin').open('wb') as out,(d/'stderr.bin').open('wb') as err:result=subprocess.run(command,cwd=d,env=environment(),stdout=out,stderr=err,timeout=90)
 elapsed=time.perf_counter()-start
 assert sha(d/'loaded-c1.bin')==sha(TC/'bin/C1.DLL')
 assert sha(d/'loaded-msvcrt.bin')==sha(TC/'bin/MSVCRT.DLL')
 raw=(d/'events.txt').read_text();inv=re.search(r'^invoke argc=(\d+) flag=(\d+)$',raw,re.M);assert inv
 start=raw.index(inv.group(0));end=raw.index('invoke-return ',start)
 vector=re.findall(r'^argv (\d+) [0-9a-f]+ (.*)$',raw[start:end],re.M)
 assert [int(i) for i,_ in vector]==list(range(int(inv[1])))
 ret=re.search(r'^invoke-return status=(\d+) esp=([0-9a-f]+) expected=([0-9a-f]+)$',raw,re.M);assert ret and ret[2]==ret[3]
 receipt={'schema':1,'kind':'actual-original-CL-to-C1-contract','not_acceptance':True,'exit':result.returncode,'seconds':elapsed,'command':command,'cwd':windows(d),'target_sha256':sha(TC/'bin/C1.DLL'),'driver_sha256':sha(TC/'bin/CL.EXE'),'provider_sha256':sha(TC/'bin/MSVCRT.DLL'),'argc':int(inv[1]),'argv':[arg for _,arg in vector],'BOOL':int(inv[2]),'return':int(ret[1]),'stdcall_cleanup':12,'artifacts':{str(f.relative_to(d)):{'sha256':sha(f),'size':f.stat().st_size} for f in d.rglob('*') if f.is_file()}}
 (d/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:receipt[k] for k in ['cwd','exit','seconds','argc','argv','BOOL','return','stdcall_cleanup']}));return result.returncode
if __name__=='__main__':raise SystemExit(main())
