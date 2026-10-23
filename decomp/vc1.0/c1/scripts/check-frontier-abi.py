#!/usr/bin/env python3
"""Native GDB controls for proved narrow inputs and typed descriptor observations."""
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import time
BINARY=Path(__file__).resolve().parents[1]
def main():
    start=time.perf_counter();root=Path.home()/'.ai-tmp/msvc2/vc1.0/c1/validation';directory=Path(tempfile.mkdtemp(prefix='frontier-abi-controls-',dir=root));case=BINARY/'cases/dispatch-type-diagnostics';results=[]
    def test(name,source,change,expected,case=case):
        reference=directory/(name+'-reference');shutil.copytree(case/'evidence/ABI'/source,reference);events=(reference/'events').read_text();modified=change(events);assert modified!=events;(reference/'events').write_text(modified)
        output=directory/name;cmd=[sys.executable,str(BINARY/'scripts/debug-frontier.py'),'--case',str(case),'--directory',str(output),'--reference',str(reference)];began=time.perf_counter();r=subprocess.run(cmd,cwd=BINARY,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30);(directory/(name+'.log')).write_bytes(r.stdout)
        if expected:
            assert r.returncode and expected in r.stdout.decode('latin1'),(name,r.stdout.decode('latin1'));status='rejected'
        else:assert not r.returncode,r.stdout.decode('latin1');status='accepted proven unspecified slot padding'
        results.append(dict(name=name,status=status,seconds=time.perf_counter()-began))
    def operation(events,mask):
        match=re.search(r'^stack 1 ([0-9a-f]+)$',events,re.M);assert match
        return events[:match.start(1)]+('%08x'%(int(match[1],16)^mask))+events[match.end(1):]
    test('wrong-operation-low-byte','conversion',lambda e:operation(e,1),'original/native ABI input differs')
    test('unspecified-operation-upper-bytes','conversion',lambda e:operation(e,0xa5a50000),None)
    test('wrong-tag-flag-byte','type',lambda e:re.sub(r'(^complete-type [0-9a-f]+ [0-9a-f]+ )2$',r'\g<1>3',e,flags=re.M),'AssertionError')
    test('wrong-array-type-extent','conversion',lambda e:e.replace('type-shape target 1 2 1 4','type-shape target 1 2 1 5'),'type pair differs')
    registry=BINARY/'cases/prototype-scope-registry'
    test('wrong-enclosing-scope-kind','scope',lambda e:re.sub(r'(^scope-link 0 [0-9a-f]+ [0-9a-f]+ )1$',r'\g<1>2',e,flags=re.M),'declaration scope inputs differ',registry)
    def wrong_owner(events):
        match=re.search(r'(^scope-context )([0-9a-f]+)( [0-9a-f]+ \d+ )([0-9a-f]+)$',events,re.M);assert match
        return events[:match.start(4)]+match[2]+events[match.end(4):]
    test('wrong-directory-owner','scope',wrong_owner,'declaration scope inputs differ',registry)
    receipt=dict(status='pass',tests=results,elapsed_seconds=time.perf_counter()-start,directory=str(directory),policy='Private copies of frozen original observations only. Fresh native GDB; no Wine/original execution, fixture writes or new reconstruction case. Original receiver byte loads independently justify upper24-bit unspecified argument control.');(directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt,indent=2))
if __name__=='__main__':main()
