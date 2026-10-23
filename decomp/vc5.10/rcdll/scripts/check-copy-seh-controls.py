#!/usr/bin/env python3
"""Negative controls for the local SEH role contract, using scratch PE copies."""
import importlib.util
import json
from pathlib import Path
import struct
import tempfile
import time
from protocol import HERE,sha
spec=importlib.util.spec_from_file_location('copy_seh',HERE/'scripts/check-copy-seh.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
started=time.perf_counter();proof=module.verify();image=module.Image(HERE/'build/RCDLL.DLL');new=proof['rebuilt'];data=image.data;controls=[]
with tempfile.TemporaryDirectory(prefix='copy-seh-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/rcdll')as root:
    path=Path(root)/'RCDLL.DLL'
    def reject(name,patches):
        changed=bytearray(data)
        for address,value in patches:
            at=image.offset(address,len(value));changed[at:at+len(value)]=value
        path.write_bytes(changed)
        try:module.verify(rebuilt_path=path)
        except AssertionError:controls.append(name)
        else:raise AssertionError('bad SEH roles accepted: '+name)
    parent=new['parent_ret']['address'];filter=new['filter']['start'];handler=new['handler']['start']
    reject('wrong-parent-ret4',[(parent,b'\xc2\x04\0')])
    assert new['filter']['end']-filter==6
    reject('swapped-parent-ret0-filter-ret8-union-unchanged',[(parent,b'\xc2\0\0'),(filter,bytes.fromhex('6a0158c20800'))])
    reject('wrong-filter-eax0',[(filter,bytes.fromhex('b800000000c3'))])
    reject('wrong-scope-handler',[(new['scope']+8,struct.pack('<I',filter))])
    reject('wrong-scope-enclosing-level',[(new['scope'],struct.pack('<I',0))])
    reject('wrong-handler-parent-frame',[(handler+2,b'\xec')])
    thunk=new['exception_import']['thunk'];reject('wrong-exception-import-slot',[(thunk+2,struct.pack('<I',new['exception_import']['slot']+4))])
report=dict(negative_controls=controls,elapsed_seconds=time.perf_counter()-started,rebuilt_sha256=sha(HERE/'build/RCDLL.DLL'),map_sha256=sha(HERE/'build/rcdll.map'),checker_sha256=sha(HERE/'scripts/check-copy-seh.py'),control_sha256=sha(Path(__file__)),scope='SEH role/ABI residual only. Shared CFG and union RET checks remain enabled.')
(HERE/'build/copy-seh-controls.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
