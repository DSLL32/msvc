from pathlib import Path
import sys,json,hashlib,shutil,subprocess,time,struct,importlib.util
B=Path('/root/msvc2/decomp/vc1.0/c1');R=B.parents[2];S=Path('/root/.ai-tmp/msvc2/vc1.0/c1')
sys.path.insert(0,str(B/'scripts'));from case_paths import windows
spec=importlib.util.spec_from_file_location('wt',B/'scripts/wine-tool.py');wt=importlib.util.module_from_spec(spec);spec.loader.exec_module(wt)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def request(p):
 data=p.read_bytes();mode,explicit=struct.unpack_from('<II',data);f=data[8:].split(b'\0');program=f.pop(0).decode('latin1')
 def vector():
  v=[]
  while f[0]:v.append(f.pop(0).decode('latin1'))
  f.pop(0);return v
 argv=vector();env=vector();assert f==[b''];return dict(mode=mode,file=program,argv=argv,environment=env if explicit else None)
functions={r['name']:r['start']-0x10000 for r in json.loads((B.parent/'cl/functions.json').read_text())['driver']}
for case,folder,source,flags in [('dispatch-type-diagnostics','dispatch-type-diagnostics','audit.c','/FRaudit.sbr /Z7 /Od /G5 /W4')]:
 work=S/case;capture=S/'transport'/(case+'-'+sys.argv[1]);work.mkdir(exist_ok=True);capture.mkdir(exist_ok=False)
 for old in work.iterdir():
  assert old.is_file() and not old.is_symlink();old.unlink()
 for p in(B/'cases'/folder/'inputs').iterdir():shutil.copyfile(p,work/p.name)
 config=capture/'config.txt';config.write_text('%x %x %x %x %x %x\n2026 1 15 0\n0\n'%(functions['_control_handler@4'],0x140d4,functions['_link_pass'],0x16018,functions['_ide_message'],0x140d0))
 env=wt.environment();env['PATH']=windows(wt.TC/'bin')
 for n in ['CL','_CL_','LINK','_LINK_','_MSC_IDE_FLAGS','MSC_CMD_FLAGS']:env.pop(n,None)
 command=['/usr/bin/wine',windows(S/'tools/capture-cl.exe'),windows(wt.TC/'bin/CL.EXE'),'/nologo /c '+flags+' '+source,*[windows(capture/p)for p in ['stdout','stderr','events']],windows(work),windows(config),'ordinary',windows(capture)]
 t=time.perf_counter()
 with (capture/'host.stdout').open('wb')as out,(capture/'host.stderr').open('wb')as err:r=subprocess.run(command,env=env,stdin=subprocess.DEVNULL,stdout=out,stderr=err,timeout=90)
 elapsed=time.perf_counter()-t
 entries=[request(p)for p in sorted(capture.glob('*-request.bin'))]
 receipt=dict(command=command,CL_arguments='/nologo /c '+flags+' '+source,helper_source='decomp/vc1.0/cl/src/capture-run.c',helper_source_sha256=sha(R/'decomp/vc1.0/cl/src/capture-run.c'),helper_sha256=sha(S/'tools/capture-cl.exe'),target_sha256=sha(wt.TC/'bin/CL.EXE'),cwd=windows(work),environment={k:env[k]for k in ['PATH','INCLUDE','LIB']},host_exit=r.returncode,execution_seconds=elapsed,children=entries)
 (capture/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(case,r.returncode,(capture/'events').read_text().splitlines()[-1],entries[0],flush=True)
 assert not r.returncode and 'exit 00000002' in (capture/'events').read_text(),case
