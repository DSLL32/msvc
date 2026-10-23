#!/usr/bin/env python3
"""Exercise exact ELF/owned-source/content checks without modifying live inputs."""
import json,shutil,tempfile,time
from pathlib import Path
import provenance as p

def main():
 start=time.perf_counter();record=p.PROJECT/'build/native-build.json';binary=p.PROJECT/'build/native/c1-host'
 tests=['different-ELF','wrong-kind','unstable-build','wrong-ELF-hash','missing-owned-source','changed-owned-source','missing-content','corrupt-content']
 with tempfile.TemporaryDirectory(prefix='compiled-build-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c1') as temp:
  d=Path(temp);p.compiled_native_sources(record,d/'positive.json',binary)
  for test in tests:
   current=json.loads(record.read_text());trial=d/'trial.json';elf=binary
   if test=='different-ELF':elf=d/'other-ELF';elf.write_bytes(b'wrong')
   elif test=='wrong-kind':current['kind']='unchecked'
   elif test=='unstable-build':current['stable_before_after']=False
   elif test=='wrong-ELF-hash':current['native_sha256']='0'*64
   elif test=='missing-owned-source':current['sources'].pop('decomp/vc5.10/c1/Makefile')
   elif test=='changed-owned-source':current['sources']['decomp/vc5.10/c1/Makefile']['sha256']='0'*64
   else:
    item=next(iter(current['sources'].values()));item['content']=str(d/'missing' if test=='missing-content' else d/'corrupt')
    if test=='corrupt-content':Path(item['content']).write_bytes(b'wrong frozen source')
   trial.write_text(json.dumps(current))
   try:p.compiled_native_sources(trial,d/'result.json',elf)
   except (AssertionError,KeyError,FileNotFoundError):continue
   raise AssertionError('accepted '+test)
 print(json.dumps(dict(fresh_count=len(tests),seconds=time.perf_counter()-start,rejected=tests)))
if __name__=='__main__':main()
