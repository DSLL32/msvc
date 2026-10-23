#!/usr/bin/env python3
"""Native-only cached status, streams, artifacts and first-entry regressions."""
import json,os,shutil,subprocess,tempfile,time,hashlib
from pathlib import Path
BINARY=Path(__file__).resolve().parents[1]
def sha(data):return hashlib.sha256(data).hexdigest()
start=time.perf_counter();config=json.loads((BINARY/'functions.json').read_text())
environment=os.environ.copy();environment.pop('MAKEFLAGS',None);environment.pop('MFLAGS',None)
build=time.perf_counter();subprocess.run(['make','--no-print-directory','-j4','native'],cwd=BINARY,env=environment,check=True,stdout=subprocess.DEVNULL);build=time.perf_counter()-build
cases=[]
for case in ('help','resources-rich','resources-rich-ce'):
 reference=BINARY/'cases'/case/'reference';master=json.loads((reference/'fixture.json').read_text())
 assert config['target']['sha256']==master['target_sha256']
 assert all(any(f['name']==name and f['status']=='accepted' for f in config['functions']) for name in master['required_accepted'])
 with tempfile.TemporaryDirectory(prefix=case+'-fixture-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/cvtres') as temporary:
  out=Path(temporary);setup=time.perf_counter()
  subprocess.run(['python3',str(BINARY/'scripts/prepare-resources.py'),'--output',str(out/'help.resources')],cwd=BINARY,check=True,stdout=subprocess.DEVNULL)
  for item in master.get('inputs',[]):
   source=BINARY/item['path'];assert sha(source.read_bytes())==item['sha256']
   shutil.copyfile(source,out/item['name'])
  (out/'filter').write_text(''.join(f['native']+'\n' for f in config['functions']));(out/'seen').write_text('')
  env=environment.copy();env.pop('CVTRES_INITIAL_TRAP',None)
  env.update(RECON_PROGRAM_PATH=master['argv0'],CVTRES_RESOURCE_PROVIDER=str(out/'help.resources'),RECON_TRACE_FILTER=str(out/'filter'),RECON_TRACE_SEEN=str(out/'seen'))
  with (out/'trace').open('wb') as trace:
   env['RECON_TRACE_FD']=str(trace.fileno());setup=time.perf_counter()-setup;execution=time.perf_counter()
   result=subprocess.run([master['argv0']]+master['arguments'],executable=str(BINARY/'build/native/CVTRES'),cwd=out,env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=10,pass_fds=(trace.fileno(),))
  execution=time.perf_counter()-execution;checks=time.perf_counter()
  assert result.returncode==master['reference']['status'],case+' status differs'
  for label,data in [('stdout',result.stdout),('stderr',result.stderr)]:
   expected=(reference/(label+'.bin')).read_bytes()
   assert sha(expected)==master['reference'][label+'_sha256'] and len(expected)==master['reference'][label+'_size']
   assert data==expected,case+' '+label+' differs from canonical master'
  for name,record in master['reference'].get('artifacts',{}).items():
   expected=(reference/name).read_bytes()
   assert sha(expected)==record['sha256'] and len(expected)==record['bytes']
   assert (out/name).read_bytes()==expected,case+' artifact differs: '+name
  for item in master.get('inputs',[]):assert sha((out/item['name']).read_bytes())==item['sha256'],'fixture input changed'
  expected=(reference/'trace.txt').read_bytes();assert sha(expected)==master['trace_sha256']
  assert sha((reference/'accepted-events.txt').read_bytes())==master['trace_source_events_sha256']
  assert (out/'trace').read_bytes()==expected,case+' application first-entry trace differs'
  checks=time.perf_counter()-checks
  cases.append(dict(case=case,passed=True,status=result.returncode,stdout_bytes=len(result.stdout),stderr_bytes=len(result.stderr),artifacts=master['reference'].get('artifacts',{}),application_entries=expected.decode().splitlines(),seconds=dict(setup=setup,execution=execution,checks=checks)))
report=dict(schema=2,passed=True,cases=cases,native_sha256=sha((BINARY/'build/native/CVTRES').read_bytes()),seconds=dict(build=build,total=time.perf_counter()-start))
(BINARY/'build/fixture-times.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
