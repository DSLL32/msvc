#!/usr/bin/env python3
"""Semantic rejection controls for ECX arguments and caller-owned callback inputs."""
import copy
import importlib.util
import json
import time
from state import BINARY
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py');decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
start=time.perf_counter();rejected=[]
def fixture(name):
    p=BINARY/'cases/hello-debug-dll/evidence'/name
    c=json.loads((p/'original/configuration.json').read_text());d=json.loads((p/'original/data.json').read_text());e=(p/'original/events').read_text();n=json.loads((p/'native/state.json').read_text())
    a={'hello.dll':(p/'original/artifact-hello.dll').read_bytes()};v={side:{f.name:f.read_bytes() for f in (p/side).glob(pattern)} for side,pattern in [('original','events-view-*.bin'),('native','native-view-*.bin')]}
    decoder.compare(c,d,e,n,a,v)
    return c,d,e,n,a,v
register=fixture('0042-map-legacy-index');callback=fixture('0046-insert-public-symbol')
def reject(name,case,mutate):
    c,d,e,n,a,v=copy.deepcopy(case);e=mutate(n,e)
    try:decoder.compare(c,d,e,n,a,v)
    except (AssertionError,KeyError):rejected.append(name)
    else:raise AssertionError('accepted '+name)
def state_mutation(function):
    def mutate(n,e):function(n);return e
    return mutate
reject('wrong-native-ECX-argument',register,state_mutation(lambda n:n['parameters'].__setitem__('index',1)))
def wrong_ecx(n,e):
    lines=e.splitlines();i=max(i for i,line in enumerate(lines) if line.startswith('registers '));fields=lines[i].split();fields[3]='00000001';lines[i]=' '.join(fields);return '\n'.join(lines)+'\n'
reject('wrong-raw-original-ECX-argument',register,wrong_ecx)
for label,field,value in [('record-content','bytes','00'*28),('record-capacity','capacity',28),('record-owner','owner','other')]:
    reject(label,callback,state_mutation(lambda n,f=field,v=value:n['parameters']['record'].__setitem__(f,v)))
for name in ('hash','name','segment','offset'):
    reject('wrong-output-width-'+name,callback,state_mutation(lambda n,k=name:n['parameters'][k].__setitem__('width',1)))
reject('wrong-defined-segment-input',callback,state_mutation(lambda n:n['parameters']['segment'].__setitem__('bytes','0100')))
reject('wrong-defined-offset-input',callback,state_mutation(lambda n:n['parameters']['offset'].__setitem__('bytes','01000000')))
reject('wrong-callback-divisor',callback,state_mutation(lambda n:n['parameters'].__setitem__('flags',1)))
report=dict(schema=1,native_only=True,positive_pairs=2,rejected=rejected,seconds=time.perf_counter()-start)
(BINARY/'cases/hello-debug-dll/abi-state-checks.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
