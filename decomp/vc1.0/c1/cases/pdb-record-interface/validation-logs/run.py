import pathlib,hashlib,json,time,subprocess
root=pathlib.Path('/root/msvc2'); cwd=root/'decomp/vc1.0/c1'; out=pathlib.Path(__file__).parent

def cat():
 return {str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for d in ['include','native','src'] for p in sorted((root/'lib/recon'/d).rglob('*')) if p.is_file() and p.suffix in ['.c','.h']}
before=cat(); (out/'shared-before.json').write_text(json.dumps(before,indent=2)+'\n'); results=[]
commands=[['make','-j4','audit-win32','native','native-trace','native-expression-tests','native-pdb-tests','native-pdb-trace-tests'],['make','-j4','native-trace-check'],['make','-j4','native-adoption-check'],['python3','scripts/check-graph-audit.py']]
try:
 for i,cmd in enumerate(commands):
  t=time.monotonic()
  with (out/f'check-{i}.log').open('w') as f: r=subprocess.run(cmd,cwd=cwd,stdout=f,stderr=subprocess.STDOUT)
  row={'command':cmd,'seconds':time.monotonic()-t,'exit':r.returncode,'log':f'check-{i}.log'}; results.append(row); print(json.dumps(row),flush=True)
  if r.returncode: raise RuntimeError(f'check {i} failed')
finally:
 after=cat(); (out/'shared-after.json').write_text(json.dumps(after,indent=2)+'\n'); (out/'timings.json').write_text(json.dumps({'checks':results,'shared_unchanged':before==after},indent=2)+'\n')
assert before==after
