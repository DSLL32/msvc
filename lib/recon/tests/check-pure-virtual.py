"""Only linkability and explicit pure-virtual denial; no CRT success claim."""
import json
import re
from pathlib import Path
import subprocess
import sys
import time
obj,exe=map(Path,sys.argv[1:])
start=time.monotonic()
symbols=subprocess.check_output(['nm','-C',str(obj)],text=True)
assert any(re.search(r' [Uw] __cxa_pure_virtual$', line) for line in symbols.splitlines()), symbols
assert not any(re.search(r'(\w+)::\1\(|operator new',line) for line in symbols.splitlines()),symbols
assert ' T Concrete::Value() const' in symbols and ' U __cyg_profile_func_enter' in symbols
exports=subprocess.check_output(['nm','-g',str(exe)],text=True)
assert ' T __cxa_pure_virtual' in exports
needed=subprocess.check_output(['readelf','-d',str(exe)],text=True)
assert 'libstdc++' not in needed
for args,code in [([],0),(['invoke'],125)]:
 p=subprocess.run([str(exe.resolve()),*args],capture_output=True)
 assert p.returncode==code and not p.stdout,(args,p.returncode,p.stdout,p.stderr)
 assert p.stderr==(b'recon: unsupported native operation: pure virtual invocation\n' if args else b''),p.stderr
print(json.dumps(dict(status='passed',seconds=time.monotonic()-start,scope='Abstract vtable links without libstdc++; genuine pure-virtual invocation terminates125, no helper vertices or App instrumentation suppression.')))
