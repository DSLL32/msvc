#!/usr/bin/env python3
"""Native-only lifecycle regression against explicitly captured original evidence."""
import hashlib,json,os,subprocess,tempfile,time
from pathlib import Path
PROJECT=Path(__file__).resolve().parents[1]
SCRATCH=Path.home()/'.ai-tmp/msvc2/vc5.10/c1'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 started=time.perf_counter();master=json.loads((PROJECT/'cases/lifecycle/master.json').read_text())
 reference=PROJECT/master['reference'];receipt=reference/'receipt.json'
 assert sha(receipt)==master['receipt_sha256']
 for name in ('events.txt','stdout.bin','stderr.bin'):
  assert sha(reference/name)==master['artifacts'][name]
 raw=(reference/'events.txt').read_text();assert 'exit=0 target=1 crt=1 initialized=1' in raw
 assert raw.count('rva=0001218e ')==2
 with tempfile.TemporaryDirectory(prefix='fixture-',dir=SCRATCH) as temporary:
  d=Path(temporary);(d/'filter').write_text('DllMain\nInvokeCompilerPass\nAbortCompilerPass\n');(d/'seen').write_bytes(b'')
  env={key:value for key,value in os.environ.items() if not key.startswith('RECON_TRACE_')}
  with (d/'trace').open('wb') as trace:
   env.update(RECON_TRACE_FD=str(trace.fileno()),RECON_TRACE_FILTER=str(d/'filter'),RECON_TRACE_SEEN=str(d/'seen'))
   setup=time.perf_counter()-started;runstart=time.perf_counter()
   run=subprocess.run([str(PROJECT/'build/native/c1-host')],cwd=d,env=env,pass_fds=(trace.fileno(),),stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=5)
   execution=time.perf_counter()-runstart
  checkstart=time.perf_counter()
  assert run.returncode==master['exit']==0
  assert run.stdout==(reference/'stdout.bin').read_bytes()
  assert run.stderr==(reference/'stderr.bin').read_bytes()
  assert (d/'trace').read_bytes()==b'DllMain\n'
  checks=time.perf_counter()-checkstart
 print(json.dumps({'case':'dll-lifecycle-and-exports','status':'PASS','native_only':True,'seconds':{'setup':setup,'execution':execution,'checks':checks,'total':time.perf_counter()-started},'application_trace':['DllMain'],'original_target_sha256':master['target_sha256']}))
if __name__=='__main__':main()
