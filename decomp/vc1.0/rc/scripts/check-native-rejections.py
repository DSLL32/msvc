#!/usr/bin/env python3
"""Exercise actual native rejection paths without mutating frozen references."""
import copy
import importlib.util
import json
from pathlib import Path
import tempfile
import time

HERE=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('rc_native_checker',HERE/'check-native.py')
checker=importlib.util.module_from_spec(spec);spec.loader.exec_module(checker)
checker.NATIVE_RUNS.mkdir(parents=True,exist_ok=True)
root=Path(tempfile.mkdtemp(prefix='native-rejections-',dir=checker.NATIVE_RUNS))
cache=checker.traces.load(checker.BINARY/'build/native-trace')
filter_path=root/'filter';filter_path.write_text(''.join(n+'\n' for n in cache['application_definitions']))
tracing=dict(cache=cache,filter=filter_path)
executable=checker.BINARY/'build/native-trace/RC.EXE'
results=[]

def reject(name,case,expected,setup):
    start=time.perf_counter();directory=root/name;directory.mkdir()
    restore=setup()
    try:
        checker.run_case(case,executable,directory,tracing)
        raise RuntimeError('bad native case was accepted')
    except AssertionError as error:
        assert expected in str(error),(name,str(error))
        results.append(dict(test=name,status='expected rejection',reason=str(error),seconds=time.perf_counter()-start))
    finally:restore()

original_request=checker.processes.ProcessFixture.request

def corrupt_input():
    def request(self,value):
        name='RC'+('%06d'%value['pid'])[-6:]
        path=self.work/name;data=bytearray(path.read_bytes());data[0]^=1;path.write_bytes(data)
        return original_request(self,value)
    checker.processes.ProcessFixture.request=request
    return lambda:setattr(checker.processes.ProcessFixture,'request',original_request)
reject('prepared-input-corruption','scanf-pushback','complete prepared RCPP input differs',corrupt_input)

original_init=checker.processes.ProcessFixture.__init__
def unknown_child():
    def initialize(self,name,work,environment):
        original_init(self,name,work,environment)
        environment['RECON_PROGRAM_PATH']='R:\\uncaptured-child\\RC.EXE'
    checker.processes.ProcessFixture.__init__=initialize
    return lambda:setattr(checker.processes.ProcessFixture,'__init__',original_init)
reject('unknown-child-invocation','scanf-pushback','RCPP executable spelling differs',unknown_child)

def wrong_entry():
    original=cache['cases']['direct-basename']['expected']
    cache['cases']['direct-basename']['expected']=['is_space_byte']
    return lambda:cache['cases']['direct-basename'].__setitem__('expected',original)
reject('wrong-application-entry','direct-basename','application first-entry trace differs',wrong_entry)
(root/'receipt.json').write_text(json.dumps(dict(status='pass',Wine_invocations=0,tests=results),indent=2)+'\n')
print('Native rejections:',len(results),'passed; receipt:',root/'receipt.json')
