import os,subprocess,sys
for binary in sys.argv[1:]:
 for present in (False,True):
  env={**os.environ,'RECON_TEST_OTHER':'retained'}
  env.pop('GLIBC_TUNABLES',None)
  if present:env['GLIBC_TUNABLES']='glibc.malloc.tcache_count=0:glibc.malloc.mmap_threshold=0:glibc.malloc.trim_threshold=0'
  p=subprocess.run([binary],env=env,capture_output=True)
  assert p.returncode==0 and p.stdout==b'host-control namespace and full FP/errors pass\n' and not p.stderr,(binary,present,p.returncode,p.stdout,p.stderr)
print('configured/absent host-control startup, target envp/environ/getenv, unrelated env and full108FP/errors pass')
