"""Finite module-three type packing at its first global-data callback."""
import copy,json
from pathlib import Path
from unittest.mock import patch
from state import load
from objects import original_walks
from buffer_storage import compare as compare_buffer,parse
from type_cache import compare as compare_primary
from global_type_cache import compare as compare_global
from type_pages import compare as compare_pages

def check(directory):
    directory=Path(directory);config,pe=load();events=(directory/'original-events').read_text()
    state=json.loads((directory/'native-gdb.json').read_text());native=json.loads((directory/'native-buffer-storage.json').read_text())
    old=original_walks(events,json.loads((directory/'walks.json').read_text()));new=state['walks'];original=parse(events)
    context=compare_buffer(config,pe,events,native,old,new,state,'hello-debug-dll')
    assert len(context['comparison']['copies'])==26 and context['comparison']['defined']==500 and len(context['comparison']['pdb_writes'])==18
    def reject(label,call):
        try:call()
        except(AssertionError,KeyError,IndexError):print('rejected:',label)
        else:raise AssertionError('accepted '+label)
    def copies(change):
        a,b=copy.deepcopy((original,native));change(a,b)
        with patch('buffer_storage.parse',return_value=a):compare_buffer(config,pe,events,b,old,new,state,'hello-debug-dll')
    reject('both sides omit late module type copies',lambda:copies(lambda a,b:[r.__setitem__(k,r[k][:8])for r in(a,b)for k in('entries','returns')]))
    reject('late module type copy wrong source parent',lambda:copies(lambda a,b:b['entries'][25].__setitem__('owner',b['entries'][25]['owner']-28)))
    reject('late module type copy wrong complete extent',lambda:copies(lambda a,b:b['entries'][9].__setitem__('size',104)))
    def write(raw,offset,value,size=4):
        data=bytearray.fromhex(raw);data[offset:offset+size]=value.to_bytes(size,'little');return data.hex()
    def primary(change):
        a,b,c=copy.deepcopy((old,new,context));change(a,b,c)
        x,y=a['type_work_a'][0],b['type_work_a'][0]
        compare_primary(bytearray.fromhex(x['bytes']),bytearray.fromhex(y['bytes']),x,y,a,b,c)
    primary(lambda a,b,c:None)
    def changed_name(a,b,c):
        root=b['type_data'][0];root['bytes']=write(root['bytes'],384+6,0x41,1)
    reject('converted UDT current name differs from frozen symbol name',lambda:primary(changed_name))
    def wrong_copy(a,b,c):
        node=b['local_type_records'][0];at=(4185-4096)*28;node['bytes']=write(node['bytes'],at+4,0x1009,2)
    reject('later primary-cache packed index lacks observed return',lambda:primary(wrong_copy))
    def global_state(change):
        a,b,c=copy.deepcopy((old,new,context));change(a,b,c);x,y=a['type_work_b'][0],b['type_work_b'][0]
        return compare_global(bytearray.fromhex(x['bytes']),bytearray.fromhex(y['bytes']),x,y,a,b,c)
    result=global_state(lambda a,b,c:None);assert sorted(r['input_type']for r in result)==[4171,4185,4192]
    def missing(a,b,c):
        for w in(a,b):
            root=w['type_work_b'][0];key=next(k for k,r in root['objects'].items()if int.from_bytes(bytes.fromhex(r['bytes'])[8:10],'little')==0x1015)
            del root['objects'][key]
    reject('both sides omit later named-cache allocation',lambda:global_state(missing))
    def wrong_type(a,b,c):
        row=next(r for r in b['type_work_b'][0]['objects'].values()if int.from_bytes(bytes.fromhex(r['bytes'])[8:10],'little')==0x100a)
        row['bytes']=write(row['bytes'],4,int.from_bytes(bytes.fromhex(row['bytes'])[4:8],'little')+4)
    reject('later named-cache record pointer has wrong packed owner',lambda:global_state(wrong_type))
    pages=json.loads((directory/'native-type-pages.json').read_text())
    def page_change():
        rows=copy.deepcopy(pages);rows[0]['postbytes']=write(rows[0]['postbytes'],100,0)
        compare_pages(config,pe,events,rows,dict(old,**context['original_namespace']),dict(new,**context['native_namespace']),state,'hello-debug-dll',context)
    reject('last module type page slot omits observed return',page_change)
