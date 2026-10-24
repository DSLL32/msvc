#!/usr/bin/env python3
"""Native-only denial controls using temporary copies of captured C6 refs."""
import copy,hashlib,json,subprocess,sys,tempfile,time
from pathlib import Path
BINARY=Path(__file__).resolve().parents[1];CASE=BINARY/'cases/direct-cvtomf-codeview'
def main():
    metadata=json.loads((CASE/'variants/no-lines/manifest.json').read_text());results=[]
    with tempfile.TemporaryDirectory(prefix='.controls-',dir=CASE) as folder:
        folder=Path(folder)
        for kind in ['changed-input','missing-reference','changed-output','changed-entry-provenance']:
            m=copy.deepcopy(metadata)
            if kind=='missing-reference':m['input']['path']=str((folder/'absent.obj').relative_to(CASE))
            elif kind=='changed-entry-provenance':m['expected_application_entries'][0]='omf_debug_number'
            else:
                key='input' if kind=='changed-input' else 'output';raw=bytearray((CASE/m[key]['path']).read_bytes());raw[0 if key=='input' else 134]^=1;path=folder/(kind+'.obj');path.write_bytes(raw);m[key]['path']=str(path.relative_to(CASE))
                if key=='output':m[key]['sha256']=hashlib.sha256(raw).hexdigest()
            manifest=folder/(kind+'.json');manifest.write_text(json.dumps(m,indent=2)+'\n');command=[sys.executable,str(BINARY/'scripts/check-cvtomf.py'),'--manifest',str(manifest)]
            start=time.perf_counter();r=subprocess.run(command,cwd=BINARY,capture_output=True,timeout=5);elapsed=time.perf_counter()-start
            assert r.returncode!=0,(kind,r.stdout,r.stderr)
            expected={'changed-input':b'cached reference/input changed','missing-reference':b'FileNotFoundError','changed-output':b'COFF non-clock bytes differ','changed-entry-provenance':b'AssertionError'}[kind]
            assert expected in r.stderr,(kind,r.stderr)
            results.append(dict(control=kind,status='rejected',returncode=r.returncode,seconds=elapsed,expected_failure=expected.decode()))
    print(json.dumps(dict(status='pass',controls=results,scope='Temporary owned copies only; no capture/original/Wine execution or original-reference mutation')))
if __name__=='__main__':main()
