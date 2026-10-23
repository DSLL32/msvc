from pathlib import Path
import json,subprocess,hashlib,time
B=Path('/root/msvc2/decomp/vc1.0/c1');R=B.parents[2];S=Path('/root/.ai-tmp/msvc2/vc1.0/c1')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def snapshot():return {str(p.relative_to(R)):sha(p)for d in ['src','include','native']for p in sorted((R/'lib/recon'/d).rglob('*'))if p.suffix in ['.c','.h']}
paths=[B/'build/native/C13232.EXE',B/'build/native-trace/C13232.EXE'];old={str(p.relative_to(B)):sha(p)for p in paths};before=snapshot();cmd=['make','-j4','-WMakefile','-W../../../lib/recon/src/crt.c','native','native-trace'];start=time.perf_counter()
with(S/'browser-limit-final-live-relink.log').open('w')as out:r=subprocess.run(cmd,cwd=B,stdout=out,stderr=subprocess.STDOUT)
row=dict(command=cmd,seconds=time.perf_counter()-start,exit=r.returncode,shared_inputs=before,shared_inputs_unchanged=before==snapshot(),prior_ELFs=old,ELFs={str(p.relative_to(B)):sha(p)for p in paths},scope='Explicit final link compiles sharedCdirectly with frozen current sources/headers; source snapshots taken immediately before/after. Existing object files retain the unchanged accepted target sources. Earlier cached finalnative make receipt is retained separately; this receipt establishes actual shared compilation at final link, not merely a present-tree fingerprint.')
row['ELFs_unchanged']=row['ELFs']==old;(S/'browser-limit-final-live-relink.json').write_text(json.dumps(row,indent=2)+'\n');print(row['exit'],row['seconds'],'identicalELFs',row['ELFs_unchanged']);assert not r.returncode and row['shared_inputs_unchanged']
