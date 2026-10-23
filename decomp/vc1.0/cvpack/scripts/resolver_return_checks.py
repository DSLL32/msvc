"""Defined resolver output controls on the normal four-call case."""
import copy,json
from pathlib import Path
from unittest.mock import patch
from state import load
from objects import original_walks
from resolver_returns import compare,parse
from buffer_storage import compare as compare_buffer,parse as parse_buffer

def check(directory):
    directory=Path(directory);config,pe=load();events=(directory/'original-events').read_text();state=json.loads((directory/'native-gdb.json').read_text());old=original_walks(events,json.loads((directory/'walks.json').read_text()));new=state['walks']
    native=json.loads((directory/'native-resolver-returns.json').read_text());original=parse(events)
    def run(a,b,text=events):
        with patch('resolver_returns.parse',return_value=a):return compare(config,pe,text,b,old,new,'hello-debug-dll')
    run(original,native)
    def reject(label,change,text=events):
        a,b=copy.deepcopy((original,native));change(a,b)
        try:run(a,b,text)
        except(AssertionError,KeyError,IndexError):print('rejected:',label)
        else:raise AssertionError('accepted '+label)
    reject('both resolver call inventories missing',lambda a,b:[r.__setitem__(k,[])for r in(a,b)for k in('entries','returns')])
    reject('both resolver last invocations missing',lambda a,b:[r[k].pop()for r in(a,b)for k in('entries','returns')])
    reject('original resolver flag points to different caller local',lambda a,b:a['entries'][0].__setitem__('flag',a['entries'][0]['flag']+2))
    reject('native resolver flag has different typed local',lambda a,b:b['entries'][0].__setitem__('local','named'))
    reject('both resolver defined flag words changed',lambda a,b:[r['returns'][0].__setitem__('value','0100')for r in(a,b)])
    reject('both resolver relative result identities changed',lambda a,b:[r['returns'][0].__setitem__('relative',127)for r in(a,b)])
    reject('resolver result pointer shifted to next TypeRecord',lambda a,b:b['returns'][0].__setitem__('result',b['returns'][0]['result']+28))
    reject('both resolver returned record byte observations missing',lambda a,b:[r['returns'][0].__setitem__('record','')for r in(a,b)])
    reject('resolver native parent TypeRecord ownership changed',lambda a,b:b['entries'][0].__setitem__('parent_record',b['entries'][0]['parent_record']+28))
    reject('resolver per-entry trap not rearmed',lambda a,b:None,events.replace('resolver-rearm 0001e700 0001e703','omitted-rearm',1))
    a,b=copy.deepcopy((original,native));b['entries'][0]['before']='aabb';run(a,b)
    print('accepted: resolver undefined entry bytes retained independently')
    buffer=json.loads((directory/'native-buffer-storage.json').read_text());original_buffer=parse_buffer(events)
    def write(raw,offset,value,size=4):
        data=bytearray.fromhex(raw);data[offset:offset+size]=value.to_bytes(size,'little');return data.hex()
    def buffer_reject(label,change):
        a,b,c,d=copy.deepcopy((original_buffer,buffer,old,new));change(a,b,c,d)
        try:
            with patch('buffer_storage.parse',return_value=a):compare_buffer(config,pe,events,b,c,d,state,'hello-debug-dll')
        except(AssertionError,KeyError,IndexError):print('rejected:',label)
        else:raise AssertionError('accepted '+label)
    def missing_write(a,b,c,d):
        for walks in(c,d):
            table=walks['local_type_records'][0];block=walks['current_type_server'][0]['objects']['type-storage-1'];at=(4199-4096)*28;pointer=int.from_bytes(bytes.fromhex(table['bytes'])[at:at+4],'little');offset=pointer-block['address']+6
            block['bytes']=write(block['bytes'],offset,4198,2)
    buffer_reject('both pending recursive pointer writes missing',missing_write)
    def wrong_index(a,b,c,d):
        table=d['local_type_records'][0];at=(4199-4096)*28+4;table['bytes']=write(table['bytes'],at,4208,2)
    buffer_reject('pending recursive pointer result has wrong active target',wrong_index)
    def inactive(a,b,c,d):
        table=d['local_type_records'][0];at=(4209-4096)*28+24;flags=int.from_bytes(bytes.fromhex(table['bytes'])[at:at+2],'little');table['bytes']=write(table['bytes'],at,flags&~0x40,2)
    buffer_reject('pending recursive target is no longer active',inactive)
    buffer_reject('both recursive-frontier last type copies omitted',lambda a,b,c,d:[r[k].pop()for r in(a,b)for k in('entries','returns')])
