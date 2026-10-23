#!/usr/bin/env python3
"""Host-only controls for EDIT input ownership, bounded formatting and file oracles."""
import copy
import hashlib
import json
from pathlib import Path
import struct
import time
from editor_error_abi import compare
BASE=Path('cases/editor-errors')

def bounded(events,actual):
    original=[e for e in events if e['event']=='editor_snprintf_result']
    assert len(original)==len(actual)==1,'Nonempty bounded-format inventory required'
    expected={k:v for k,v in original[0].items() if k!='event'}
    assert actual[0]==expected
    assert 0<=expected['result']<expected['capacity']==340
    assert len(expected['text'].encode('latin1'))==expected['result']

def main():
    start=time.perf_counter();rejected=[]
    for variant in ['rebase','snprintf','bind','file-failure']:
        row=json.loads((BASE/'acceptance'/variant/'receipt.json').read_text())['editor_error_abi_state']
        compare([row['original']],row['native'])
        for label in ['missing-original','missing-formal','wrong-caller','changed-argument']+(['wrong-output-owner','wrong-output-capacity'] if variant=='snprintf' else []):
            events=[copy.deepcopy(row['original'])];actual=copy.deepcopy(row['native'])
            if label=='missing-original':events=[]
            elif label=='missing-formal':actual['formals'].pop(next(iter(actual['formals'])))
            elif label=='wrong-caller':actual['caller_name']='unrelated_caller'
            elif label=='wrong-output-owner':actual['formals']['output_is_error_message']=False
            elif label=='wrong-output-capacity':actual['formals']['capacity']=339
            else:
                key=next(iter(actual['formals']));value=actual['formals'][key];actual['formals'][key]=value+1 if isinstance(value,int) else 'changed'
            try:compare(events,actual)
            except (AssertionError,KeyError):rejected.append(variant+':'+label)
            else:raise AssertionError('Invalid EDIT ABI observation accepted')
    events=[json.loads(line) for line in (BASE/'snprintf/original/events.jsonl').read_text().splitlines()]
    native=json.loads((BASE/'snprintf/native-complete/receipt.json').read_text())['native']['formatted'];bounded(events,native)
    for label in ['both-omitted','wrong-result','wrong-text','wrong-capacity','wrong-errno']:
        left=copy.deepcopy(events);right=copy.deepcopy(native)
        if label=='both-omitted':left=[];right=[]
        elif label=='wrong-text':right[0]['text']+='changed'
        else:right[0][{'wrong-result':'result','wrong-capacity':'capacity','wrong-errno':'errno'}[label]]+=1
        try:bounded(left,right)
        except AssertionError:rejected.append('format:'+label)
        else:raise AssertionError('Invalid bounded-format observation accepted')
    fixed=(BASE/'inputs/fixed.dll').read_bytes();provider=(BASE/'inputs/provider.dll').read_bytes()
    for data,is_fixed in [(fixed,True),(provider,False)]:
        pe=struct.unpack_from('<I',data,60)[0];optional=pe+24
        assert data[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',data,pe+4)[0]==0x14c
        assert struct.unpack_from('<II',data,optional+96)==(0,0)
        assert struct.unpack_from('<II',data,optional+104)==(0,0)
        assert bool(struct.unpack_from('<H',data,pe+22)[0]&1)==is_fixed
        if is_fixed:assert struct.unpack_from('<II',data,optional+136)==(0,0)
    originals=json.loads((BASE/'original-validation.json').read_text())
    for phase,code in [('rebase',54),('bind',53),('missing',7)]:
        row=originals[phase];assert len(row['runs'])==2 and row['raw_outputs_exact']
        for run in row['runs']:assert run['summary']['root_exit']==code and run['working_files']==row['actual_file_inventory']
        for name,value in row['actual_file_inventory'].items():
            data=(BASE/'inputs'/name).read_bytes();assert value==dict(bytes=len(data),sha256=hashlib.sha256(data).hexdigest())
    result=dict(status='pass',negative_controls=rejected,seconds=time.perf_counter()-start,processes_launched=0)
    (BASE/'checks.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
if __name__=='__main__':main()
