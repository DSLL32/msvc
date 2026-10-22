#!/usr/bin/env python3
"""Fail-closed regressions: wrong original frontier, changed child input/status."""
import copy
import importlib.machinery
import json
from pathlib import Path
import tempfile
import time

HERE=Path(__file__).resolve().parent
check=importlib.machinery.SourceFileLoader('cl_check_native',str(HERE/'check-native.py')).load_module()


def main():
    start=time.perf_counter();registry=check.cases.specifications();cache=check.traces.load(check.cases.BINARY/'build/native-trace')
    check.processes.verify_toolchain()
    root=Path(tempfile.mkdtemp(prefix='reject-',dir=check.SCRATCH));results=[]
    variants=[]
    wrong=copy.deepcopy(cache);wrong['cases']['compile']['expected']=['main']
    variants.append(('wrong-frontier',dict(registry['compile']),wrong,'application frontier sequence differs'))
    changed=copy.deepcopy(registry['compile']);changed['text_files']={'test.c':'int main() { return 1; }\n'}
    variants.append(('changed-initial-input',changed,cache,'initial case input differs'))
    variants.append(('changed-child-input',dict(registry['compile']),cache,'child complete input tree differs'))
    variants.append(('changed-output-artifact',dict(registry['compile']),cache,'artifact bytes differ'))
    status=dict(registry['compile']);status['expected_exit']=7
    variants.append(('wrong-status',status,cache,'status differs'))
    for name,case,expected,message in variants:
        folder=root/name;folder.mkdir()
        original_respond=check.processes.ProcessFixture.respond
        original_finish=check.processes.ProcessFixture.finish
        if name=='changed-child-input':
            def changed_input(self,request):
                (self.work/'test.c').write_bytes(b'int main() { return 1; }\n')
                return original_respond(self,request)
            check.processes.ProcessFixture.respond=changed_input
        if name=='changed-output-artifact':
            def changed_output(self):
                original_finish(self)
                path=self.work/'test.exe';data=bytearray(path.read_bytes());data[8192]^=1;path.write_bytes(data)
            check.processes.ProcessFixture.finish=changed_output
        try:check.run(case,expected,True,folder)
        except AssertionError as error:
            assert message in str(error),(name,str(error))
            results.append(dict(check=name,status='rejected',reason=str(error)))
        else:raise AssertionError('invalid fixture was accepted: '+name)
        finally:
            check.processes.ProcessFixture.respond=original_respond
            check.processes.ProcessFixture.finish=original_finish
    result=dict(status='passed',checks=results,total_seconds=time.perf_counter()-start)
    (root/'receipt.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));print(root/'receipt.json')

if __name__=='__main__':main()
