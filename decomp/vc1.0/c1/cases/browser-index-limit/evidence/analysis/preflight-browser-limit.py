from pathlib import Path
import subprocess,sys,importlib.util,shutil,json,time,os
B=Path('/root/msvc2/decomp/vc1.0/c1');S=Path('/root/.ai-tmp/msvc2/vc1.0/c1');D=S/'browser-limit-preflight/draft1';D.mkdir(parents=True);work=D/'work';work.mkdir()
for p in(B/'cases/browser-index-limit/inputs').iterdir():shutil.copy2(p,work/p.name)
sys.path.insert(0,str(B/'scripts'));from case_paths import windows
sp=importlib.util.spec_from_file_location('wt',B/'scripts/wine-tool.py');wt=importlib.util.module_from_spec(sp);sp.loader.exec_module(wt);env=wt.environment();rows=[]
for name,args in [('compiler',['/nologo','/Z7','/Od','/G5','/W3','/FRregistry.sbr','registry.c','/Feregistry.exe'])]:
 cmd=['wine',windows(wt.TC/'bin'/('LINK.EXE'if name=='link'else'CL.EXE')),*args];t=time.perf_counter();r=subprocess.run(cmd,env=env,cwd=work,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=60);(D/(name+'.stdout')).write_bytes(r.stdout);(D/(name+'.stderr')).write_bytes(r.stderr);rows.append(dict(phase=name,command=cmd,exit=r.returncode,seconds=time.perf_counter()-t));(D/'receipt.json').write_text(json.dumps(rows,indent=2)+'\n');print(name,r.returncode,r.stdout.decode('latin1'),r.stderr.decode('latin1'),flush=True)
 if r.returncode:break
else:
 t=time.perf_counter();r=subprocess.run(['wine',windows(work/'registry.exe')],env=env,cwd=work,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=30);(D/'runtime.stdout').write_bytes(r.stdout);(D/'runtime.stderr').write_bytes(r.stderr);rows.append(dict(phase='runtime',exit=r.returncode,seconds=time.perf_counter()-t));(D/'receipt.json').write_text(json.dumps(rows,indent=2)+'\n');print('runtime',r.returncode)
