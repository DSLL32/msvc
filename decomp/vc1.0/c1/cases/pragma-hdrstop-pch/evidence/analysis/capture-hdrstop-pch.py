from pathlib import Path
import sys,json,hashlib,shutil,subprocess,time,struct,importlib.util,os,datetime
B=Path('/root/msvc2/decomp/vc1.0/c1');R=B.parents[2];S=Path('/root/.ai-tmp/msvc2/vc1.0/c1');sys.path.insert(0,str(B/'scripts'));from case_paths import windows
spec=importlib.util.spec_from_file_location('wt',B/'scripts/wine-tool.py');wt=importlib.util.module_from_spec(spec);spec.loader.exec_module(wt)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def request(p):
 data=p.read_bytes();mode,explicit=struct.unpack_from('<II',data);fields=data[8:].split(b'\0');program=fields.pop(0).decode('latin1')
 def vector():
  v=[]
  while fields[0]:v.append(fields.pop(0).decode('latin1'))
  fields.pop(0);return v
 argv=vector();env=vector();assert fields==[b''];return dict(mode=mode,file=program,argv=argv,environment=env if explicit else None)
functions={r['name']:r['start']-0x10000 for r in json.loads((B.parent/'cl/functions.json').read_text())['driver']}
work=S/'pragma-hdrstop-pch';work.mkdir(exist_ok=True)
for old in work.iterdir():
 assert old.is_file()and not old.is_symlink();old.unlink()
mtime_ns=1768480496000000000
for p in (B/'cases/pragma-hdrstop-pch/inputs').iterdir():shutil.copy2(p,work/p.name);os.utime(work/p.name,ns=(mtime_ns,mtime_ns))
run=sys.argv[1]
for phase,source,mode in [('create','producer.c','/Yc /FRproducer.sbr'),('use','consumer.c','/Yu /FRconsumer.sbr')]:
 capture=S/'transport'/('pragma-hdrstop-pch-'+run+'-'+phase);capture.mkdir(exist_ok=False)
 config=capture/'config.txt';config.write_text('%x %x %x %x %x %x\n2026 1 15 0\n0\n'%(functions['_control_handler@4'],0x140d4,functions['_link_pass'],0x16018,functions['_ide_message'],0x140d0))
 env=wt.environment();env['PATH']=windows(wt.TC/'bin')
 for n in ['CL','_CL_','LINK','_LINK_','_MSC_IDE_FLAGS','MSC_CMD_FLAGS']:env.pop(n,None)
 args='/nologo /c /Z7 /Od /G5 /W3 '+mode+' /Fphdrstop.pch '+source
 command=['/usr/bin/wine',windows(S/'tools/capture-cl.exe'),windows(wt.TC/'bin/CL.EXE'),args,*[windows(capture/p)for p in ['stdout','stderr','events']],windows(work),windows(config),'ordinary',windows(capture)]
 t=time.perf_counter()
 with(capture/'host.stdout').open('wb')as out,(capture/'host.stderr').open('wb')as err:r=subprocess.run(command,env=env,stdin=subprocess.DEVNULL,stdout=out,stderr=err,timeout=90)
 entries=[request(p)for p in sorted(capture.glob('*-request.bin'))]
 receipt=dict(command=command,CL_arguments=args,helper_source='decomp/vc1.0/cl/src/capture-run.c',helper_source_sha256=sha(R/'decomp/vc1.0/cl/src/capture-run.c'),helper_sha256=sha(S/'tools/capture-cl.exe'),target_sha256=sha(wt.TC/'bin/CL.EXE'),cwd=windows(work),environment={k:env[k]for k in ['PATH','INCLUDE','LIB']},host_exit=r.returncode,execution_seconds=time.perf_counter()-t,children=entries,input_mtime_ns=mtime_ns,C1_boundary_file_sets={stage:[dict(name=p.name,bytes=p.stat().st_size,sha256=sha(p),mtime_ns=p.stat().st_mtime_ns)for p in sorted((capture/stage).iterdir())]for stage in ['001-before','001-after']})
 (capture/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(run,phase,r.returncode,(capture/'stdout').read_text(),entries[0]if entries else None,flush=True)
 assert not r.returncode and 'exit 00000000'in(capture/'events').read_text()
