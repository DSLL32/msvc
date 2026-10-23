#!/usr/bin/env python3
"""Compile the actual pending slot types and reject every direct invocation."""
import json
from pathlib import Path
import subprocess
import tempfile
import time
from state import BINARY, ROOT, sha


def main():
    import argparse
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
    output=p.parse_args().output.resolve();begin=time.monotonic()
    boundary=(BINARY/'cases/analysis/output-callback-boundary.json').read_bytes()
    slots=json.loads(boundary)['slots']
    flags=['clang','-m32','-std=gnu89','-fms-extensions','-fasm-blocks','-Wno-ignored-attributes',
           '-DRECON_NATIVE_TEST','-I'+str(ROOT/'lib/recon/include'),'-I'+str(ROOT/'lib/recon/native'),
           '-I'+str(BINARY/'src'),'-fsyntax-only']
    rejects=[]
    with tempfile.TemporaryDirectory(prefix='output-callback-guard-',dir=ROOT.parent/'.ai-tmp/msvc2/vc5.10/c2')as folder:
        source=Path(folder)/'guard.c'
        source.write_text('#include "output-callbacks.h"\n#include <recon.h>\n')
        control=subprocess.run(flags+[str(source)],capture_output=True);assert control.returncode==0,control.stderr.decode()
        for slot in slots:
            source.write_text('#include "output-callbacks.h"\n#include <recon.h>\nvoid attempted_callback(void) { '+slot['native']+'(); }\n')
            result=subprocess.run(flags+[str(source)],capture_output=True)
            assert result.returncode!=0 and b'not a function' in result.stderr,(slot['native'],result.stderr.decode())
            rejects.append({'slot':slot['native'],'status':result.returncode,'diagnostic_sha256':sha(result.stderr)})
    value={'status':'PASS','boundary_sha256':sha(boundary),'header_sha256':sha((BINARY/'src/output-callbacks.h').read_bytes()),
           'script_sha256':sha(Path(__file__).read_bytes()),'command_prefix':flags,'positive_layout_compile':True,'rejected':rejects,
           'seconds':time.monotonic()-begin,'scope':'Actual sizeof4/offset0/alignment4 assertions compile; all20 direct slot invocations fail. .target invocation/escape remains an explicit review hold and finite source guard, no callable ABI assertion.'}
    output.write_text(json.dumps(value,indent=2)+'\n');print(json.dumps(value,indent=2))

if __name__=='__main__':main()
