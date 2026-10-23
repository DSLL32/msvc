"""Three recursive nodes and the first actual post-copy pointer fixup."""
import copy,json
from pathlib import Path
from unittest.mock import patch
from state import load
from objects import original_walks
from buffer_storage import compare as compare_buffer,parse as parse_buffer
from recursive_arena import compare as compare_arena,parse as parse_arena
from symbol_output import compare as compare_symbol

def check(directory):
    directory=Path(directory);config,pe=load();events=(directory/'original-events').read_text();state=json.loads((directory/'native-gdb.json').read_text());old=original_walks(events,json.loads((directory/'walks.json').read_text()));new=state['walks'];native=json.loads((directory/'native-buffer-storage.json').read_text());original=parse_buffer(events)
    context=compare_buffer(config,pe,events,native,old,new,state,'hello-debug-dll');assert len(context['comparison']['copies'])==74 and context['comparison']['defined']==2032 and context['comparison']['post_copy_writes']==[dict(type_index=4199,field=6,before=4209,after=0x1047,offset=2030,source=0x1ecaf)]
    slab=json.loads((directory/'native-recursive-arena.json').read_text());birth=parse_arena(events);result=compare_arena(config,pe,events,slab,old,new,state,'hello-debug-dll',context);assert len(result['nodes'])==3 and result['defined_ranges']==[[0,6],[8,12],[12,18],[20,24],[24,30],[32,36]]
    def reject(label,action):
        try:action()
        except(AssertionError,KeyError,IndexError):print('rejected:',label)
        else:raise AssertionError('accepted '+label)
    def write(row,key,at,value):
        raw=bytearray.fromhex(row[key]);raw[at:at+len(value)]=value;row[key]=raw.hex()
    def buffer(change):
        a,b,c,d=copy.deepcopy((original,native,old,new));change(a,b,c,d)
        with patch('buffer_storage.parse',return_value=a):return compare_buffer(config,pe,events,b,c,d,state,'hello-debug-dll')
    def arena(change):
        a,b,c,d=copy.deepcopy((birth,slab,old,new));change(a,b,c,d)
        with patch('recursive_arena.parse',return_value=a):return compare_arena(config,pe,events,b,c,d,state,'hello-debug-dll',context)
    reject('both post-copy pointer fields remain unconverted',lambda:buffer(lambda a,b,c,d:[write(r['frontier'],'payload',2030,(4209).to_bytes(2,'little'))for r in(a,b)]))
    reject('post-copy pointer resolves to wrong root on BOTH sides',lambda:buffer(lambda a,b,c,d:[write(r['frontier'],'payload',2030,(0x1048).to_bytes(2,'little'))for r in(a,b)]))
    reject('post-copy write changes neighboring defined byte',lambda:buffer(lambda a,b,c,d:write(b['frontier'],'payload',2029,b'\xff')))
    reject('post-copy write changes unallocated buffer tail',lambda:buffer(lambda a,b,c,d:write(b['frontier'],'payload',2032,b'\xff')))
    def mark_missing(a,b,c,d):
        for walks in(c,d):write(walks['local_type_records'][0],'bytes',(4199-4096)*28+20,b'\0')
    reject('post-copy recursive mark absent on BOTH sides',lambda:buffer(mark_missing))
    reject('both last pointer copies and publications omitted',lambda:buffer(lambda a,b,c,d:[r[k].pop()for r in(a,b)for k in('entries','returns')]))
    reject('post-copy pointer TypeRecord still points to provider',lambda:buffer(lambda a,b,c,d:write(d['local_type_records'][0],'bytes',(4199-4096)*28,b'\0'*4)))
    reject('third recursive node index aliases first global type',lambda:arena(lambda a,b,c,d:write(b['frontier'],'bytes',28,(0x1047).to_bytes(2,'little'))))
    reject('second recursive node points at wrong copied record',lambda:arena(lambda a,b,c,d:write(b['frontier'],'bytes',12,b'\0'*4)))
    reject('third recursive node reserved bytes mutated',lambda:arena(lambda a,b,c,d:write(b['frontier'],'bytes',30,b'\xff\xff')))
    reject('recursive prefix36 tail has unproved writes',lambda:arena(lambda a,b,c,d:write(b['frontier'],'bytes',36,b'\xff')))
    reject('recursive cursor fails to include three nodes',lambda:arena(lambda a,b,c,d:b['frontier'].update(cursor=b['frontier']['cursor']-12)))
    reject('recursive slot counter fails to include three nodes',lambda:arena(lambda a,b,c,d:b['frontier'].update(remaining=1023)))
    rows=json.loads((directory/'native-symbol-output.json').read_text());rows[0]['global_type']=0x1048
    reject('next packed-type DX argument differs',lambda:compare_symbol(config,pe,events,rows,old,new,True,state))
    rows=json.loads((directory/'native-symbol-output.json').read_text());rows[0]['fixed_field']=4209
    reject('next packed-type observer falsely leaves local reference',lambda:compare_symbol(config,pe,events,rows,old,new,True,state))
