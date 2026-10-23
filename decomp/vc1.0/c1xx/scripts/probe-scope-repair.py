"""Explicit original-only bounded parser-repair probe; never a fixture.

Observe the pointer-producing last writer, stale byte, branch comparison and
scope result. Original executable/fixture files remain read-only.
"""
import fcntl
import gzip
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import time
from binary import PE
from case_paths import BINARY, CAPTURE_ROOT, SCRATCH, windows


def main():
    spec=importlib.util.spec_from_file_location('scope_original_tool',BINARY/'scripts/wine-tool.py')
    tool=importlib.util.module_from_spec(spec);spec.loader.exec_module(tool)
    fixture=BINARY.parent/'fixtures/c1xx-copy-assignment'
    phase=json.loads((fixture/'manifest.json').read_text())['phases']['c1']
    original=tool.TC/'bin/C1XX3232.EXE';image=PE(original)
    sites=[(0x3f29a,'after_writer'),(0x26750,'scope_entry'),(0x26794,'token_comparison'),(0x29224,'scope_return')]
    destination=BINARY/'evidence/implicit-scope-lookahead/probes';destination.mkdir()
    command_line=phase['command_line_template'].format(bin=windows(tool.TC/'bin'))
    flags=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(tool.TC/'bin'),include=windows(tool.TC/'include'))
    with (CAPTURE_ROOT/'capture.lock').open('a') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX)
        for label in ['valid','missing-rparen']:
            saved=destination/label;saved.mkdir()
            scratch=Path(tempfile.mkdtemp(prefix='scope-probe-',dir=CAPTURE_ROOT))
            work=SCRATCH/'vc1.0/c1xx'/('scope-probe-'+label);work.mkdir()
            for item in phase['inputs']:
                data=(fixture/item['path']).read_bytes()
                assert hashlib.sha256(data).hexdigest()==item['sha256']
                if Path(item['path']).name=='copy.cpp' and label=='missing-rparen':
                    assert data.count(b'if (this != &other)')==1
                    data=data.replace(b'if (this != &other)',b'if (this != &other')
                (work/Path(item['path']).name).write_bytes(data)
            shutil.copytree(work,saved/'inputs')
            data=bytearray(image.data)
            for address,name in sites:data[image.offset(address)]=0xcc
            target=scratch/'observed-original.exe';target.write_bytes(data)
            plan=scratch/'sites.plan';plan.write_text(''.join('%x %02x %s\n'%(address,image.read(address,1)[0],name) for address,name in sites))
            environment=tool.environment();environment.update(C1XX_SCOPE_PLAN=windows(plan),C1XX_SCOPE_LOG=windows(scratch/'scope.log'))
            command=['wine',windows(BINARY/'build/reference-scope-observer.exe'),windows(target),'',
                windows(scratch/'stdout.bin'),windows(scratch/'stderr.bin'),windows(scratch/'events.txt'),windows(work),
                'MSC_CMD_FLAGS='+flags,command_line]
            start=time.monotonic()
            with (scratch/'launcher.log').open('wb') as log:
                result=subprocess.run(command,cwd=BINARY,env=environment,stdout=log,stderr=subprocess.STDOUT,timeout=90)
            elapsed=time.monotonic()-start
            for name in ['scope.log','sites.plan','events.txt','stdout.bin','stderr.bin','launcher.log']:
                if (scratch/name).exists():shutil.copyfile(scratch/name,saved/name)
            shutil.copytree(work,saved/'files')
            observed=re.findall(r'^site (\S+) ',(saved/'scope.log').read_text(),re.M)
            receipt=dict(scope='original-only parser repair characterization, no reconstruction acceptance',
                status=result.returncode,seconds=elapsed,original_sha256=hashlib.sha256(image.data).hexdigest(),
                observer_sha256=hashlib.sha256((BINARY/'build/reference-scope-observer.exe').read_bytes()).hexdigest(),
                observed=observed,expected_sites=[name for address,name in sites],canonical_cwd=windows(work),
                command_line=command_line,environment={'MSC_CMD_FLAGS':flags},
                mutation='remove exactly one closing parenthesis from if (this != &other)' if label=='missing-rparen' else 'none',
                files={p.name:dict(bytes=p.stat().st_size,sha256=hashlib.sha256(p.read_bytes()).hexdigest()) for p in work.iterdir() if p.is_file()})
            (saved/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
            print(json.dumps(dict(case=label,status=result.returncode,seconds=elapsed,observed=observed)),flush=True)
            assert result.returncode==0,'observer failed; raw evidence retained'
            assert observed==[name for address,name in sites], 'unexpected probe path; raw evidence retained'


if __name__=='__main__':main()
