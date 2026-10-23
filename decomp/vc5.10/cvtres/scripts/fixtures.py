#!/usr/bin/env python3
"""Native-only /help regression: cached outputs, first entries, no Windows child."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time
import hashlib
BINARY=Path(__file__).resolve().parents[1]
def sha(data):return hashlib.sha256(data).hexdigest()
start=time.perf_counter();config=json.loads((BINARY/'functions.json').read_text())
reference=BINARY/'cases/help/reference';master=json.loads((reference/'fixture.json').read_text())
assert config['target']['sha256']==master['target_sha256']
assert all(any(f['name']==name and f['status']=='accepted' for f in config['functions']) for name in master['required_accepted'])
build_environment=os.environ.copy();build_environment.pop('MAKEFLAGS',None);build_environment.pop('MFLAGS',None)
build=time.perf_counter();subprocess.run(['make','--no-print-directory','-j4','native'],cwd=BINARY,env=build_environment,check=True,stdout=subprocess.DEVNULL);build=time.perf_counter()-build
with tempfile.TemporaryDirectory(prefix='fixture-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/cvtres') as temporary:
 out=Path(temporary);setup=time.perf_counter()
 subprocess.run(['python3',str(BINARY/'scripts/prepare-resources.py'),'--output',str(out/'help.resources')],cwd=BINARY,check=True,stdout=subprocess.DEVNULL)
 (out/'filter').write_text(''.join(f['native']+'\n' for f in config['functions']));(out/'seen').write_text('')
 environment=os.environ.copy();environment.pop('CVTRES_INITIAL_TRAP',None)
 environment.update(RECON_PROGRAM_PATH=master['argv0'],CVTRES_RESOURCE_PROVIDER=str(out/'help.resources'),RECON_TRACE_FILTER=str(out/'filter'),RECON_TRACE_SEEN=str(out/'seen'))
 with (out/'trace').open('wb') as trace:
  environment['RECON_TRACE_FD']=str(trace.fileno());setup=time.perf_counter()-setup;execution=time.perf_counter()
  result=subprocess.run([master['argv0']]+master['arguments'],executable=str(BINARY/'build/native/CVTRES'),cwd=out,env=environment,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=10,pass_fds=(trace.fileno(),))
 execution=time.perf_counter()-execution;checks=time.perf_counter()
 assert result.returncode==master['reference']['status']==1,'help status differs'
 for label,data in [('stdout',result.stdout),('stderr',result.stderr)]:
  expected=(reference/(label+'.bin')).read_bytes()
  assert sha(expected)==master['reference'][label+'_sha256'] and len(expected)==master['reference'][label+'_size']
  assert data==expected,label+' differs from canonical master'
 expected=(reference/'trace.txt').read_bytes();assert sha(expected)==master['trace_sha256']
 assert sha((reference/'accepted-events.txt').read_bytes())==master['trace_source_events_sha256']
 assert (out/'trace').read_bytes()==expected,'application first-entry trace differs'
 checks=time.perf_counter()-checks
report=dict(schema=1,case='help',passed=True,status=result.returncode,stdout_bytes=len(result.stdout),stderr_bytes=len(result.stderr),application_entries=expected.decode().splitlines(),native_sha256=sha((BINARY/'build/native/CVTRES').read_bytes()),seconds=dict(build=build,setup=setup,execution=execution,checks=checks,total=time.perf_counter()-start))
(BINARY/'build/fixture-times.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
