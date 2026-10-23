#!/usr/bin/env python3
"""Host-only output structure, narrow clocks and actual ABI/FILE-state controls."""
import copy,hashlib,importlib.util,json,struct,time
from pathlib import Path
from binary import archive,COFF
from omf_continuation_abi import compare,compare_result
BASE=Path('cases/omf-continuation')
s=importlib.util.spec_from_file_location('omf_clock','scripts/artifacts-omf-continuation.py');clock=importlib.util.module_from_spec(s);s.loader.exec_module(clock)

def structure(path):
    members=list(archive(path));assert len(members)==1
    name,data=members[0];obj=COFF(data);assert len(obj.sections)==3
    code,ordinary,continued=obj.sections
    assert code['data']==bytes.fromhex('a1000000008b0d000000008b1500000000c3')
    assert ordinary['data']==bytes.fromhex('112233445566778899aabbccddeeff00')
    assert continued['data']==bytes.fromhex('01020304050607081112131415161718')
    assert [(o,s['name'],s['value'],s['section'],k) for o,s,k in code['relocs']]==[(13,'$$R0002',8,2,6),(7,'$$R0001',4,2,6),(1,'$$R0001',4,2,6)]
    symoff=struct.unpack_from('<I',data,8)[0]
    assert struct.unpack_from('<I',data,symoff+18*8)[0]==8,'Preserve original stale COMDAT auxiliary length8 versus actual16Brawspan'
    assert all(struct.unpack_from('<H',data,20+40*i+34)[0]==0 for i in range(3)),'LINSYM is consumed without emitted COFFline records'
    return dict(member=name,coff_bytes=len(data),continued_bytes=16,stale_aux_length=8,relocations=3,displaced_symbols=2,line_records=0)


def main():
    t=time.perf_counter();rejected=[];structures={}
    for variant in ['continued','use16']:
        first=BASE/'controlled-reference'/variant/'1'/(variant+'.lib');second=BASE/'controlled-reference'/variant/'2'/(variant+'.lib')
        structures[variant]=structure(first);a=first.read_bytes();b=second.read_bytes();assert clock.normalize_artifact(a,'archive-generated')==clock.normalize_artifact(b,'archive-generated')
        fields=clock.timestamp_fields(a,'archive-generated');bad=bytearray(a);bad[fields[0]['offset']+10]^=1
        try:clock.timestamp_fields(bad,'archive-generated')
        except AssertionError:rejected.append(variant+':date-suffix')
        else:raise AssertionError('Date suffix incorrectly normalized')
        for field in fields:
            assert field['bytes'] in (10,4)
    rows=[]
    for variant in ['insert','extend','lines','lookup','warning']:
        r=json.loads((BASE/'acceptance'/variant/'receipt.json').read_text())['omf_continuation_abi_state'];events=[r['original']];actual=r['native'];rows.append((variant,events,actual))
    complete=json.loads((BASE/'warning-result/native-complete-stream/receipt.json').read_text())
    events=[json.loads(l) for l in (BASE/'warning-result/complete-original/events.jsonl').read_text().splitlines()];rows.append(('vfprintf',events,complete['native']['boundary_entry']))
    for variant,events,actual in rows:
        compare(events,actual)
        for label in ['missing-original','wrong-caller','missing-formal','changed-input']+(['missing-stream','wrong-stream-count','wrong-entry-errno'] if variant=='vfprintf' else []):
            left=copy.deepcopy(events);right=copy.deepcopy(actual)
            if label=='missing-original':left=[]
            elif label=='wrong-caller':right['caller_name']='unrelated_caller'
            elif label=='missing-formal':right.pop('formals')
            elif label=='missing-stream':right.pop('stream')
            elif label=='wrong-stream-count':right['stream']['count']+=1
            elif label=='wrong-entry-errno':right['errors']['errno']=0
            elif variant=='lines':right['unread_record'][0]^=1
            else:
                field=next(iter(right['formals']));value=right['formals'][field];right['formals'][field]=value+1 if isinstance(value,int) else 'changed'
            try:compare(left,right)
            except (AssertionError,KeyError):rejected.append(variant+':'+label)
            else:raise AssertionError('Invalid actual OMF boundary accepted')
    original=complete['original'];native=complete['native']['results'];assert compare_result(events,native)==original;assert native==[original] and original['result']==14 and original['errno']==original['doserrno']==2 and original['last_error']==0
    assert bytes(original['buffered'])==b'cvtomf() : warning : 16-bit segment' and original['count']==2013 and original['buffer_size']==2048
    for label in ['result','errno','flags','count','buffer_size','buffered']:
        bad=copy.deepcopy(native);value=bad[0][label];bad[0][label]=value+1 if isinstance(value,int) else []
        try:compare_result(events,bad)
        except AssertionError:rejected.append('vfprintf-result:'+label)
        else:raise AssertionError('Invalid vfprintf result accepted')
    try:compare_result([],[])
    except AssertionError:rejected.append('vfprintf-result:both-omitted')
    else:raise AssertionError('Empty vfprintf result inventory accepted')
    result=dict(status='pass',structures=structures,negative_controls=rejected,seconds=time.perf_counter()-t,processes_launched=0)
    (BASE/'checks.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
if __name__=='__main__':main()
