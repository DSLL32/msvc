#!/usr/bin/env python3
"""Defined type-table bytes, segment ownership and native-owned tail controls."""
import argparse
import copy
import importlib.util
import json
from pathlib import Path
import re
import time
from state import BINARY, sha


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pair',type=Path,default=BINARY/'cases/hello-debug-dll/evidence/0051-initialize-module-type-storage')
    parser.add_argument('--output',type=Path,default=BINARY/'cases/hello-debug-dll/type-record-state-checks.json')
    args=parser.parse_args();pair=args.pair
    spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py')
    decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
    config=json.loads((pair/'original/configuration.json').read_text())
    data=json.loads((pair/'original/data.json').read_text())
    events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
    artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()}
    views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in
        [('original','events-view-*.bin'),('native','native-view-*.bin')]}
    def compare(raw,state):return decoder.compare(config,data,raw,state,artifacts,views)
    start=time.perf_counter();baseline=compare(events,native)
    original=decoder.decode(events)
    def address(state,name):return int.from_bytes(bytes.fromhex(state['globals'][name]),'little')
    table=address(native,'type_records');segment=address(native,'line_data')
    capacity=address(native,'type_record_capacity');count=address(native,'type_record_count')
    assert count==336 and capacity==12000
    original_table=next(key for key,value in original['memory'].items() if len(value)==capacity*32)
    original_segment=next(item for item in data if item['name']=='line_data')
    original_segment=int.from_bytes(original['memory'][original_segment['original']],'little')
    def memory(state,pointer,offset,value):
        raw=bytearray.fromhex(state['memory'][str(pointer)]);raw[offset:offset+len(value)]=value
        state['memory'][str(pointer)]=raw.hex()
    def original_memory(raw,pointer,change):
        pattern=r'^memory %08x ([0-9a-f]+)$'%pointer
        matches=list(re.finditer(pattern,raw,re.M));assert len(matches)==1
        content=bytearray.fromhex(matches[0][1]);change(content)
        return raw[:matches[0].start(1)]+content.hex()+raw[matches[0].end(1):]
    tests=[]
    def reject(name,mutation,reason,original_change=None):
        state=copy.deepcopy(native);mutation(state)
        raw=original_change(events) if original_change else events
        try:compare(raw,state)
        except (AssertionError,KeyError) as error:
            assert reason in str(error), ('unexpected guard',name,str(error))
            tests.append(dict(name=name,diagnostic=str(error)))
        else:raise AssertionError('accepted '+name)
    reject('record-payload-offset-alias',lambda s:memory(s,table,0,(segment+1).to_bytes(4,'little')),
        'type record segment/payload offset alias differs')
    reject('initialized-record-index',lambda s:memory(s,table,4,b'\x01'), 'initialized type record fields differ')
    reject('initialized-record-status',lambda s:memory(s,table,28,b'\x01'), 'initialized type record fields differ')
    reject('unused-cleared-table-tail',lambda s:memory(s,table,capacity*32-1,b'\x01'), 'unused type record capacity is not cleared')
    reject('matching-dirty-unused-capacity',lambda s:memory(s,table,capacity*32-1,b'\x01'),
        'unused type record capacity is not cleared',
        lambda raw:original_memory(raw,original_table,lambda content:content.__setitem__(-1,1)))
    reject('truncated-defined-table',lambda s:s['memory'].__setitem__(str(table),s['memory'][str(table)][:-64]),
        'incomplete initialized type record table')
    reject('changed-initialized-type-input',lambda s:memory(s,segment,100,bytes([bytes.fromhex(native['memory'][str(segment)])[100]^1])), 'initialized type segment bytes differ')
    reject('truncated-segment-capacity',lambda s:s['memory'].__setitem__(str(segment),s['memory'][str(segment)][:-2]),
        'incomplete type segment capacity observation')
    reject('wrong-cleared-allocation-request',lambda s:s['cleared_allocations'][-1].update(size=capacity*32+32),
        'cleared allocation request/result differs')
    reject('missing-cleared-allocation',lambda s:s['cleared_allocations'].pop(), 'cleared allocator inventory differs')
    reject('cleared-allocation-identity-reuse',lambda s:s['cleared_allocations'][-1].update(address=s['cleared_allocations'][0]['address']),
        'live cleared allocation identity reused')
    tail=address(native,'type_segment_bytes')-4
    reject('both-payload-pointers-into-uninitialized-tail',lambda s:memory(s,table,0,(segment+tail).to_bytes(4,'little')),
        'type record exceeds initialized segment input',
        lambda raw:original_memory(raw,original_table,lambda content:content.__setitem__(slice(0,4),(original_segment+tail).to_bytes(4,'little'))))
    reject('changed-type-read-segment-offset',lambda s:s['file_reads'][-1].update(segment_offset=1),
        'actual read caller/extent/position/result/initialized output differs')
    # The read initializes 12324 bytes of this 12332-byte allocation. Its eight
    # native-owned tail bytes are observed for bounds, never replayed/compared.
    state=copy.deepcopy(native)
    raw=bytearray.fromhex(state['memory'][str(segment)])
    defined=native['file_reads'][-1]['returned']
    assert defined==12324 and len(raw)==12332
    raw[defined:]=bytes(byte^0xff for byte in raw[defined:]);state['memory'][str(segment)]=raw.hex()
    compare(events,state)
    result=dict(schema=1,native_only=True,application_records=baseline['typed_record_entries_checked'],
        initialized_table_bytes=capacity*32,initialized_segment_bytes=defined,uninitialized_segment_tail_bytes=len(raw)-defined,
        positive_native_tail_change=True,rejected=tests,count=len(tests),
        checker_sha256=sha(Path(__file__)),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),
        inputs=[dict(path=str((pair/name).relative_to(BINARY)),sha256=sha(pair/name)) for name in
            ('original/receipt.json','original/configuration.json','original/data.json','original/events','native/receipt.json','native/state.json')],
        seconds=time.perf_counter()-start)
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(dict(rejected=len(tests),records=result['application_records'],
        positive_native_tail_change=True,seconds=result['seconds']),indent=2))


if __name__=='__main__':main()
