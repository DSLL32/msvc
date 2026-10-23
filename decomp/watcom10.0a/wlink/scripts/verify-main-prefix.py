#!/usr/bin/env python3
"""Native-only regression for accepted main, stopping at pending Spawn."""
import json,os,resource,subprocess,tempfile,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]

def main():
 began=time.monotonic();case=ROOT/'cases/help/main-prefix-01'
 accepted=json.loads((ROOT/'functions.json').read_text())
 if accepted['accepted']!=['main']:raise SystemExit('Update the cached frontier before running this main-only prefix fixture')
 argv=json.loads((case/'acceptance.json').read_text())['paired_inputs']['argv']
 scratch=Path.home()/'.ai-tmp/msvc2/watcom10.0a/wlink/native-fixtures';scratch.mkdir(parents=True,exist_ok=True)
 with tempfile.TemporaryDirectory(prefix='main-',dir=scratch) as name:
  work=Path(name);(work/'case').mkdir()
  env=os.environ.copy()
  for key in tuple(env):
   if key.startswith('RECON_TRACE_'):env.pop(key)
  setup=time.monotonic()-began
  with (work/'trace.txt').open('wb') as trace,(work/'stdout.bin').open('wb') as out,(work/'stderr.bin').open('wb') as err:
   env.update(RECON_TRACE_FD=str(trace.fileno()),RECON_TRACE_FILTER=str(case/'native-trace/filter.txt'),RECON_TRACE_SEEN=str(case/'native-trace/seen.txt'))
   start=time.monotonic()
   child=subprocess.run(argv,executable=str(ROOT/'build/native/WLINK'),env=env,cwd=work/'case',stdin=subprocess.DEVNULL,stdout=out,stderr=err,pass_fds=(trace.fileno(),),timeout=5,preexec_fn=lambda:resource.setrlimit(resource.RLIMIT_CORE,(0,0)))
   elapsed=time.monotonic()-start
  check_start=time.monotonic()
  if child.returncode!=-5:raise SystemExit('Native main did not stop at the expected INT3 frontier: '+str(child.returncode))
  if (work/'trace.txt').read_bytes()!=(case/'original-entry-trace.derived.txt').read_bytes():raise SystemExit('Application entry trace diverged')
  for stream in ['stdout.bin','stderr.bin']:
   if (work/stream).read_bytes()!=(case/stream).read_bytes():raise SystemExit(stream+' differs')
  if list((work/'case').iterdir()):raise SystemExit('Unexpected case artifacts')
  checks=time.monotonic()-check_start
 print(json.dumps({'fixture':'main→Spawn accepted prefix','status':'PASS','case_completion':'help remains in progress','seconds':{'setup':setup,'execution':elapsed,'checks':checks,'total':time.monotonic()-began}}))
if __name__=='__main__':main()
