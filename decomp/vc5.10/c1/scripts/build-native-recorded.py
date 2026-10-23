#!/usr/bin/env python3
"""Freeze actual native build inputs, reject concurrent source changes, retain ELF."""
import json,subprocess,time
from pathlib import Path
from provenance import PROJECT,ROOT,STORE,freeze,native_sources,sha,freeze_native_binary

def main():
 manifest=PROJECT/'build/native-build-sources.json';manifest.parent.mkdir(exist_ok=True)
 before=freeze(native_sources(),manifest)
 start=time.perf_counter()
 result=subprocess.run(['make','-j4','native'],cwd=PROJECT,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
 elapsed=time.perf_counter()-start
 (PROJECT/'build/native-recorded.log').write_bytes(result.stdout)
 result.check_returncode()
 after={str(p.relative_to(ROOT)):sha(p) for p in native_sources()}
 if after!={name:item['sha256'] for name,item in before.items()}:
  # Its mtime could be newer than an edit made while the compiler was reading
  # shared sources. A later Make no-op cannot certify that rejected executable.
  (PROJECT/'build/native/c1-host').unlink(missing_ok=True)
  raise AssertionError('source changed during native compilation; rejected ELF removed; rebuild with a stable input snapshot')
 binary=PROJECT/'build/native/c1-host';digest=sha(binary);freeze_native_binary(binary,digest)
 record=dict(schema=1,kind='native-build-source-content',native_sha256=digest,
             command=['make','-j4','native'],seconds=elapsed,stable_before_after=True,sources=before)
 output=PROJECT/'build/native-build.json';output.write_text(json.dumps(record,indent=2)+'\n')
 print(json.dumps(dict(native_sha256=digest,seconds=elapsed,sources=len(before))))
if __name__=='__main__':main()
