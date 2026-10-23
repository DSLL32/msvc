"""Reached forward type resolver: exact pending member write and finite owners."""
import copy,json
from pathlib import Path
from unittest.mock import patch
from state import load
from objects import original_walks
from buffer_storage import compare as compare_buffer,parse
from global_type_cache import compare as compare_global
from type_pages import compare as compare_pages

def check(directory):
    directory=Path(directory);config,pe=load();events=(directory/'original-events').read_text();state=json.loads((directory/'native-gdb.json').read_text())
    native=json.loads((directory/'native-buffer-storage.json').read_text());old=original_walks(events,json.loads((directory/'walks.json').read_text()));new=state['walks'];original=parse(events)
    context=compare_buffer(config,pe,events,native,old,new,state,'hello-debug-dll');assert len(context['comparison']['copies'])==35 and context['comparison']['defined']==800 and len(context['comparison']['pdb_writes'])==24
    def reject(label,call):
        try:call()
        except(AssertionError,KeyError,IndexError):print('rejected:',label)
        else:raise AssertionError('accepted '+label)
    def write(raw,offset,value,size=4):
        data=bytearray.fromhex(raw);data[offset:offset+size]=value.to_bytes(size,'little');return data.hex()
    def run(change):
        a,b,c,d=copy.deepcopy((original,native,old,new));change(a,b,c,d)
        with patch('buffer_storage.parse',return_value=a):compare_buffer(config,pe,events,b,c,d,state,'hello-debug-dll')
    reject('both sides omit forward-resolver type copies',lambda:run(lambda a,b,c,d:[r.__setitem__(k,r[k][:26])for r in(a,b)for k in('entries','returns')]))
    reject('forward-resolver late copy wrong owner',lambda:run(lambda a,b,c,d:b['entries'][34].__setitem__('owner',b['entries'][34]['owner']-28)))
    def missing_write(a,b,c,d):
        for w in(c,d):
            block=w['current_type_server'][0]['objects']['type-storage-1'];record=w['local_type_records'][0];pointer=int.from_bytes(bytes.fromhex(record['bytes'])[(4219-4096)*28:(4219-4096)*28+4],'little');offset=pointer-block['address']+6
            block['bytes']=write(block['bytes'],offset,4217,2)
    reject('both sides omit actual pending member conversion',lambda:run(missing_write))
    def wrong_parent(a,b,c,d):
        record=d['local_type_records'][0];offset=(4219-4096)*28;pointer=int.from_bytes(bytes.fromhex(record['bytes'])[offset:offset+4],'little');record['bytes']=write(record['bytes'],offset,pointer+4)
    reject('pending member write belongs to different provider record',lambda:run(wrong_parent))
    def inactive_parent(a,b,c,d):
        record=d['local_type_records'][0];offset=(4219-4096)*28+24;flags=int.from_bytes(bytes.fromhex(record['bytes'])[offset:offset+2],'little');record['bytes']=write(record['bytes'],offset,flags&~0x40,2)
    reject('pending member owner is outside active compaction',lambda:run(inactive_parent))
    def missing_cache():
        a,b=copy.deepcopy((old,new))
        for w in(a,b):
            objects=w['type_work_b'][0]['objects'];key=next(k for k,r in objects.items()if int.from_bytes(bytes.fromhex(r['bytes'])[8:10],'little')==0x1022);del objects[key]
        x,y=a['type_work_b'][0],b['type_work_b'][0];compare_global(bytearray.fromhex(x['bytes']),bytearray.fromhex(y['bytes']),x,y,a,b,context)
    reject('both sides omit resolver target named-cache allocation',missing_cache)
    def wrong_page():
        rows=json.loads((directory/'native-type-pages.json').read_text());rows[0]['postbytes']=write(rows[0]['postbytes'],136,0)
        compare_pages(config,pe,events,rows,dict(old,**context['original_namespace']),dict(new,**context['native_namespace']),state,'hello-debug-dll',context)
    reject('forward-resolver last page slot lacks actual copy return',wrong_page)
