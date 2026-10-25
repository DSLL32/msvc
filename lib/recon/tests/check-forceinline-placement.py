"""Compiler-only helper shape plus placement alias; no original execution."""
import json
import re
from pathlib import Path
import subprocess
import sys
import time
obj,exe=map(Path,sys.argv[1:])
start=time.monotonic()
symbols=subprocess.check_output(['nm','-C',str(obj)],text=True)
assert not any(re.search(r'(\w+)::\1\(|operator new',line) for line in symbols.splitlines()),symbols
for name in ['construct','allocate','InlineBase::Value() const','InlineItem::Value() const']:
 assert any(' T '+name==line[8:] for line in symbols.splitlines()),(name,symbols)
assert ' U recon_cpp_new' in symbols and '_Znwj' not in symbols,symbols
assert ' U __cyg_profile_func_enter' not in symbols # Typed hook is defined by probe.
disassembly=subprocess.check_output(['objdump','-dr',str(obj)],text=True)
assert 'recon_cpp_new' in disassembly and '__cyg_profile_func_enter' in disassembly
p=subprocess.run([str(exe.resolve())],capture_output=True)
assert p.returncode==0 and not p.stdout and not p.stderr,(p.returncode,p.stdout,p.stderr)
print(json.dumps(dict(status='passed',seconds=time.monotonic()-start,scope='No inline constructor/placement helper vertices; real methods instrumented, allocation binds existing recon_cpp_new; placement retains caller pointer.')))
