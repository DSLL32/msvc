import pathlib,hashlib,json,time,subprocess
root=pathlib.Path('/root/msvc2');cwd=root/'decomp/vc1.0/c1';out=pathlib.Path(__file__).parent

def catalog():return {str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest()for d in ['include','native','src']for p in sorted((root/'lib/recon'/d).rglob('*'))if p.is_file()and p.suffix in ['.c','.h']}
before=catalog();(out/'shared-before.json').write_text(json.dumps(before,indent=2)+'\n');results=[]
commands=[['make','-j4','audit-win32','native','native-trace','native-expression-tests','native-pdb-tests','native-pdb-trace-tests','native-resource-tests'],['make','-j4','native-trace-check'],['make','-j4','native-adoption-check'],['python3','scripts/check-graph-audit.py']]
try:
 for i,command in enumerate(commands):
  began=time.perf_counter()
  with(out/f'check-{i}.log').open('w')as f:result=subprocess.run(command,cwd=cwd,stdout=f,stderr=subprocess.STDOUT)
  row=dict(command=command,seconds=time.perf_counter()-began,exit=result.returncode,log=f'check-{i}.log');results.append(row);print(json.dumps(row),flush=True)
  if result.returncode:raise RuntimeError('check failed')
finally:
 after=catalog();(out/'shared-after.json').write_text(json.dumps(after,indent=2)+'\n');(out/'timings.json').write_text(json.dumps(dict(checks=results,shared_unchanged=before==after),indent=2)+'\n')
assert before==after
