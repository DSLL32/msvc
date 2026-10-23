from pathlib import Path
import subprocess,time,json,sys,hashlib
B=Path('/root/msvc2/decomp/vc1.0/c1');S=Path('/root/.ai-tmp/msvc2/vc1.0/c1');R=B.parents[2]
PIN=Path('/root/.ai-tmp/msvc2/vc1.0/c1/shared-inputs/hdrstop')
PIN_FILES=json.loads((PIN/'manifest.json').read_text())['files']
def snapshot():
 actual={relative:hashlib.sha256((PIN/relative).read_bytes()).hexdigest()for relative in PIN_FILES}
 assert actual==PIN_FILES, 'Pinned shared input changed'
 return actual
case,phase,label=sys.argv[1:];cfg=json.loads((B/'functions.json').read_text());count=len(cfg['accepted']);tag=label+'-'+str(count);ops=[]
for n,args in [('native-build',['make','-j4','RECON='+str(PIN/'lib/recon'),'native','native-trace']),('original-capture',['python3','scripts/capture-frontier.py','--case','cases/'+case,'--phase',phase,'--directory',str(S/'captures'/tag),'--debugger',str(S/'tools/capture-original.exe')]),('native-frontier',['python3','scripts/debug-frontier.py','--case','cases/'+case,'--phase',phase,'--directory',str(S/'validation'/tag),'--reference',str(S/'captures'/tag)])]:
 before=snapshot()if n=='native-build'else None;t=time.perf_counter()
 with(S/(tag+'-'+n+'.log')).open('w')as out:r=subprocess.run(args,cwd=B,stdout=out,stderr=subprocess.STDOUT)
 row=dict(name=n,command=args,seconds=time.perf_counter()-t,exit=r.returncode)
 if before is not None:
  after=snapshot();p=S/(tag+'-compiled-shared-inputs.json');p.write_text(json.dumps(dict(before=before,after=after,stable=before==after,ELFs={str(q.relative_to(B)):hashlib.sha256(q.read_bytes()).hexdigest()for q in [B/'build/native/C13232.EXE',B/'build/native-trace/C13232.EXE']if q.exists()},pin_path=str(PIN),pin_manifest_sha256=hashlib.sha256((PIN/'manifest.json').read_bytes()).hexdigest(),scope='Root-approved immutable shared input snapshot; shared .c linked directly; refresh deliberately and final live-root build required. Shared C/H fingerprints observed immediately around this actual make. Build log identifies real compiler/link vs cached operations; no later tree substituted.'),indent=2)+'\n');row['compiled_source_snapshot']=str(p);row['compiled_source_snapshot_sha256']=hashlib.sha256(p.read_bytes()).hexdigest();row['compiled_inputs_stable']=before==after
 ops.append(row);(S/(tag+'-timings.json')).write_text(json.dumps(ops,indent=2)+'\n');assert not r.returncode,(n,str(S/(tag+'-'+n+'.log')))
 if before is not None:assert before==after,('Shared source changed during build; do not capture/accept',str(p))
print((S/'captures'/tag/'events').read_text().splitlines()[0]);print((S/'validation'/tag/'receipt.json').read_text()[:120])
