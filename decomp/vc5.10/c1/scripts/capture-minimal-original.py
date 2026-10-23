#!/usr/bin/env python3
"""Explicit original-only minimal compiler-pass reference; never a fixture helper."""
import argparse,hashlib,json,shutil,subprocess,time,importlib.util
from pathlib import Path
from wine_tool_support import environment,windows,ROOT,PROJECT,TC,SCRATCH
from provenance import freeze
s=importlib.util.spec_from_file_location('capture',Path(__file__).with_name('capture-original.py'));capture=importlib.util.module_from_spec(s);s.loader.exec_module(capture)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--name',required=True);a=p.parse_args();assert a.name.replace('-','').isalnum()
 d=SCRATCH/'vc5.10/c1'/a.name;d.mkdir(parents=True,exist_ok=False)
 preflight=capture.preflight();target=TC/'bin/C1.DLL';crt=TC/'bin/MSVCRT.DLL'
 for src,dst in [(PROJECT/'build/dll-capture.exe','dll-capture.exe'),(crt,'MSVCRT.DLL'),(PROJECT/'cases/minimal/inputs/test.c','test.c')]:shutil.copy2(src,d/dst)
 # Analysis entry breakpoints only; the entire original remains unmasked here.
 points=[0x1202a,0x120ce,0x1218e,0x12223,0x12229]
 (d/'ranges.txt').write_text(''.join('%x %x 1\n'%(at,at+1) for at in points))
 freeze([PROJECT/'scripts/dll-capture.c',Path(__file__),PROJECT/'scripts/wine_tool_support.py'],d/'source-snapshot.json')
 env=environment();command=['wine',windows(d/'dll-capture.exe'),'--debug',windows(target),windows(crt),'baseline','ranges.txt','minimal']
 started=time.perf_counter()
 with (d/'stdout.bin').open('wb') as out,(d/'stderr.bin').open('wb') as err:r=subprocess.run(command,cwd=d,env=env,stdout=out,stderr=err,timeout=90)
 seconds=time.perf_counter()-started
 assert sha(d/'loaded-target.bin')==sha(target);assert sha(d/'loaded-msvcrt.bin')==capture.CRT_HASH
 receipt={'schema':1,'kind':'explicit-original-minimal-reference','not_acceptance':True,'exit':r.returncode,'seconds':seconds,'command':command,'cwd':windows(d),'preflight':preflight,'target_sha256':sha(target),'provider_sha256':sha(crt),'helper_sha256':sha(d/'dll-capture.exe'),'environment_inputs':{key:env.get(key) for key in ['WINEPREFIX','WINEDLLOVERRIDES','WINEPATH','INCLUDE','LIB','CL','_CL_','TMP','TEMP','TZ','MSC_CMD_FLAGS']},'artifacts':{f.name:{'sha256':sha(f),'size':f.stat().st_size} for f in d.iterdir() if f.is_file()}}
 (d/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({'directory':str(d),'exit':r.returncode,'seconds':seconds,'artifacts':list(receipt['artifacts'])}))
 return 0 if r.returncode==0 else 1
if __name__=='__main__':raise SystemExit(main())
