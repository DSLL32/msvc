#!/usr/bin/env python3
"""Independently replay the original C1 producer and its dependent consumer."""
from pathlib import Path
import hashlib,json,os,shutil,subprocess
F=Path(__file__).resolve().parent
R=F.parents[3]
T=R/'toolchains/1.0-nt'
S=Path.home()/'.ai-tmp/vc1.0-c1/pch/independent'
P=S/'wine'
def win(p):return 'Z:'+str(p.resolve()).replace('/','\\')
def sha(b):return hashlib.sha256(b).hexdigest()
def main():
 S.mkdir(parents=True,exist_ok=True)
 env=os.environ.copy();env.update(WINEPREFIX=str(P),WINEARCH='win32',WINEDEBUG='-all',WINEPATH=win(T/'bin'),INCLUDE=win(T/'include'),LIB=win(T/'lib'))
 for k in ('CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS'):env.pop(k,None)
 if not (P/'system.reg').exists():subprocess.run(['wineboot','-u'],env=env,check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
 root=S/'root';root.mkdir(exist_ok=True);link=P/'dosdevices/n:'
 if link.is_symlink():assert link.resolve()==root.resolve()
 else:link.symlink_to(root)
 helper=S/'run-phase.exe'
 subprocess.run(['wine',str(T/'bin/CL.EXE'),'/nologo','/O1','/Fe'+win(helper),'/Fo'+win(S/'run-phase.obj'),win(F/'run-phase.c')],cwd=S,env=env,check=True,stdout=subprocess.DEVNULL)
 m=json.loads((F/'manifest.json').read_text());receipts=[]
 for iteration in range(2):
  work=root/'pch'
  if work.exists():shutil.rmtree(work)
  work.mkdir()
  for key in ('c1-create','c1-use'):
   phase=m['phases'][key]
   if key=='c1-use':
    for p in work.iterdir():
     if p.name.startswith('ir'):p.unlink()
   for src in phase['inputs']:
    data=(F/src['path']).read_bytes();assert sha(data)==src['sha256']
    path=work/Path(src['path']).name
    if key=='c1-use' and path.name=='state.pch':assert path.read_bytes()==data
    else:path.write_bytes(data)
   flags=phase['environment']['MSC_CMD_FLAGS'].format(bin=win(T/'bin'),include=win(T/'include'))
   childenv=env.copy();childenv['MSC_CMD_FLAGS']=flags
   p=subprocess.run(['wine',str(helper),win(T/'bin/C13232.EXE'),phase['canonical_working_directory']],cwd=S,env=childenv,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=120)
   assert p.returncode==phase['expected_exit'],(key,p.returncode,p.stdout,p.stderr)
   assert p.stdout==phase['expected_stdout'].encode('latin1'),(key,p.stdout)
   assert p.stderr==phase['expected_stderr'].encode('latin1'),(key,p.stderr)
   expected={Path(x['path']).name for x in phase['inputs']}|{x['output_name'] for x in phase['outputs']}
   assert {x.name for x in work.iterdir()}==expected
   for output in phase['outputs']:
    data=(work/output['output_name']).read_bytes();assert len(data)==output['bytes'] and sha(data)==output['sha256'],(key,output['output_name'],len(data),sha(data))
   receipts.append(dict(run=iteration,phase=key,exit=p.returncode,complete_file_set_exact=True,all_outputs_raw_exact=True))
 (S/'receipt.json').write_text(json.dumps(receipts,indent=2)+'\n')
 print(json.dumps(receipts,indent=2))
if __name__=='__main__':main()
