#!/usr/bin/env python3
"""Offline tiny historical-phase decode reuse proof; never executes target code."""
import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import time
from state import BINARY,sha

def module(name,file):
    spec=importlib.util.spec_from_file_location(name,BINARY/'scripts'/file)
    value=importlib.util.module_from_spec(spec);spec.loader.exec_module(value);return value

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('phase',choices=['before','after'])
args=parser.parse_args()
pair=BINARY/'cases/hello-debug-dll/evidence/0112-first-hash-block';directory=pair/'decode-reuse';directory.mkdir(exist_ok=True)
raw=module('phase_reuse_raw','raw-state.py');controller=module('phase_reuse_controller','address-hash-complete-state.py')
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text());events=(pair/'original/events').read_text();native=json.loads((pair/'native-generation/state.json').read_text())
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()};views={side:{p.name:p.read_bytes()for p in (pair/folder).glob(pattern)}for side,folder,pattern in [('original','original','events-view-*.bin'),('native','native-generation','native-view-*.bin')]}
for file in ['raw-state.py','address-hash-complete-state.py']:(directory/(args.phase+'-'+file)).write_bytes((BINARY/'scripts'/file).read_bytes())
calls=[];decode=raw.decode

def measured_decode(value):
    calls.append(dict(sha256=hashlib.sha256(value.encode()).hexdigest(),lines=len(value.splitlines())))
    return decode(value)
raw.decode=measured_decode
start=time.perf_counter();result=raw.compare(config,data,events,native,artifacts,views);seconds=time.perf_counter()-start
payload=(json.dumps(result,sort_keys=True,separators=(',',':'))+'\n').encode();(directory/(args.phase+'-result.json')).write_bytes(payload)
bindings={str(p.relative_to(BINARY)):sha(p)for p in [pair/'original/events',pair/'original/receipt.json',pair/'original/configuration.json',pair/'original/data.json',pair/'original/graph-metadata.json',pair/'native-generation/state.json',pair/'native-generation/receipt.json',BINARY/'build/native/CVPACK',BINARY/'build/CVPACK.EXE']}
receipt=dict(schema=1,offline_only=True,seconds=seconds,public_result_sha256=hashlib.sha256(payload).hexdigest(),decode_calls=copy.deepcopy(calls),inputs=bindings,source={file:sha(BINARY/'scripts'/file)for file in ['raw-state.py','address-hash-complete-state.py']})
if args.phase=='after':
    before=json.loads((directory/'before.json').read_text());assert bindings==before['inputs'],'changed reuse experiment execution inputs';assert payload==(directory/'before-result.json').read_bytes(),'changed public comparison result'
    assert len(before['decode_calls'])==3 and len(calls)==2 and before['decode_calls'][0]==before['decode_calls'][2] and calls==before['decode_calls'][:2],'historical decode is not the exact removed duplicate'
    phase_events,previous,_=controller.first_phase(events,native);phase=dict(config,observe_address_hash_complete=False);negatives=[]
    def reject(name,call,reason):
        try:call()
        except (AssertionError,TypeError,KeyError)as error:
            assert reason in str(error),(name,str(error));negatives.append(dict(name=name,diagnostic=str(error)))
        else:raise AssertionError('accepted '+name)
    reject('private-flag-complete-recursion',lambda:raw.compare(config,data,events,native,artifacts,views,_return_original=True),'complete mode')
    reject('private-flag-nonboolean',lambda:raw.compare(phase,data,phase_events,previous,artifacts,views,_return_original=1),'boolean')
    reject('unchecked-decoded-state-argument',lambda:raw.compare(phase,data,phase_events,previous,artifacts,views,_original={}),"unexpected keyword")
    wrong=copy.deepcopy(previous);wrong['function']='first_hash_output_block'
    reject('wrong-phase-native-identity',lambda:raw.compare(phase,data,phase_events,wrong,artifacts,views,_return_original=True),'')
    reject('current-raw-as-historical-phase',lambda:raw.compare(phase,data,events,previous,artifacts,views,_return_original=True),'')
    first,state=raw.compare(phase,data,phase_events,previous,artifacts,views,_return_original=True)
    assert first==result['prefix'] and state['address']==0x40edc0
    assert state==decode(phase_events),'comparison mutated its parsed historical phase'
    # Mutating a returned state cannot poison a later comparison: every private
    # request independently decodes and validates its actual immutable events.
    state['address']=0;state['memory'].clear()
    second,fresh=raw.compare(phase,data,phase_events,previous,artifacts,views,_return_original=True)
    assert second==first and fresh['address']==0x40edc0 and fresh['memory']
    phase_compare,phase_decode=raw._phase_comparison()
    count=len(calls);premature=phase_decode(phase_events);assert len(calls)==count+1 and premature['address']==0x40edc0
    assert phase_compare(phase,data,phase_events,previous,artifacts,views)==first
    count=len(calls);cached=phase_decode(phase_events);assert len(calls)==count and cached['address']==0x40edc0
    equal_but_distinct=phase_events.encode().decode();assert equal_but_distinct==phase_events and equal_but_distinct is not phase_events
    count=len(calls);assert phase_decode(equal_but_distinct)==cached and len(calls)==count+1
    lines=events.splitlines();begin=lines.index('addresshash-phase-entry 0040edc0');end=lines.index('addresshash-phase-end 0040edc0')
    current_events='\n'.join(lines[:begin]+[line for line in lines[begin:end+1]if line.startswith('mapped-view ')]+lines[end+1:])+'\n'
    count=len(calls);assert phase_decode(current_events)['address']==0x4100d0 and len(calls)==count+1
    # A failed comparison cannot install a new trusted phase in the closure.
    reject('failed-phase-cache-install',lambda:phase_compare(phase,data,equal_but_distinct,wrong,artifacts,views),'')
    count=len(calls);assert phase_decode(equal_but_distinct)['address']==0x40edc0 and len(calls)==count+1
    receipt.update(negative_count=len(negatives),negatives=negatives,returned_state_is_not_cached=True,premature_and_distinct_phase_always_decode=True,saved_seconds=before['seconds']-seconds,speedup=before['seconds']/seconds)
(directory/(args.phase+'.json')).write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps(dict(phase=args.phase,seconds=seconds,decode_calls=len(receipt['decode_calls']),result_sha256=receipt['public_result_sha256'])))
