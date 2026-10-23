"""Reject missing or fabricated ownership of the reached recursive hash slab."""
import copy,json
from pathlib import Path
from unittest.mock import patch
from state import load
from objects import original_walks
from buffer_storage import compare as compare_buffer
from recursive_arena import compare,parse,validate,compiled_provenance

def check(directory):
    directory=Path(directory);config,pe=load();events=(directory/'original-events').read_text();state=json.loads((directory/'native-gdb.json').read_text());native=json.loads((directory/'native-recursive-arena.json').read_text());old=original_walks(events,json.loads((directory/'walks.json').read_text()));new=state['walks'];original=parse(events)
    buffer=compare_buffer(config,pe,events,json.loads((directory/'native-buffer-storage.json').read_text()),old,new,state,'hello-debug-dll')
    result=compare(config,pe,events,native,old,new,state,'hello-debug-dll',buffer);assert result['defined_ranges']==[[0,6],[8,12]]and len(result['nodes'])==1
    def run(change):
        a,b,c,d=copy.deepcopy((original,native,old,new));change(a,b,c,d)
        with patch('recursive_arena.parse',return_value=a):return compare(config,pe,events,b,c,d,state,'hello-debug-dll',buffer)
    def reject(label,action):
        try:action()
        except(AssertionError,KeyError,IndexError):print('rejected:',label)
        else:raise AssertionError('accepted '+label)
    def mutate_bytes(row,key,offset,raw):
        data=bytearray.fromhex(row[key]);data[offset:offset+len(raw)]=raw;row[key]=data.hex()
    reject('both recursive allocation observations missing',lambda:run(lambda a,b,c,d:[r['allocations'].clear()for r in(a,b)]))
    reject('both recursive frontier observations missing',lambda:run(lambda a,b,c,d:[r.update(frontier=None)for r in(a,b)]))
    reject('duplicate actual recursive allocation lifetime',lambda:run(lambda a,b,c,d:[r['allocations'].append(copy.deepcopy(r['allocations'][0]))for r in(a,b)]))
    reject('recursive slab requested size not12288',lambda:run(lambda a,b,c,d:b['allocations'][0].update(size=12289)))
    reject('recursive slab allocation parent not root4209',lambda:run(lambda a,b,c,d:b['allocations'][0].update(owner=b['allocations'][0]['owner']-28)))
    reject('recursive slab cursor not one actual used node',lambda:run(lambda a,b,c,d:b['frontier'].update(cursor=b['frontier']['cursor']+12)))
    reject('recursive slab remaining slot count differs',lambda:run(lambda a,b,c,d:b['frontier'].update(remaining=1022)))
    reject('recursive slab frontier belongs to another allocation',lambda:run(lambda a,b,c,d:b['frontier'].update(base=b['frontier']['base']+12)))
    reject('recursive node reserved bytes acquire invented writes',lambda:run(lambda a,b,c,d:mutate_bytes(b['frontier'],'bytes',6,b'\xff\xff')))
    reject('recursive unused suffix acquires invented writes',lambda:run(lambda a,b,c,d:mutate_bytes(b['frontier'],'bytes',12,b'\xff')))
    reject('recursive node global index differs on BOTH sides',lambda:run(lambda a,b,c,d:[mutate_bytes(r['frontier'],'bytes',4,b'\x46\x10')for r in(a,b)]))
    reject('recursive node record pointer loses actual copied owner',lambda:run(lambda a,b,c,d:mutate_bytes(b['frontier'],'bytes',0,b'\0'*4)))
    reject('recursive node next points outside owned used prefix',lambda:run(lambda a,b,c,d:mutate_bytes(b['frontier'],'bytes',8,(b['frontier']['base']+12).to_bytes(4,'little'))))
    reject('both recursive bucket arrays omit required node',lambda:run(lambda a,b,c,d:[r['name_hash_buckets'][0].update(bytes='00'*16384)for r in(c,d)]))
    reject('both recursive bucket counts omit required node',lambda:run(lambda a,b,c,d:[r['name_hash_counts'][0].update(bytes='00'*8192)for r in(c,d)]))
    def aliased(a,b,c,d):
        d['injected_allocation']=[dict(address=b['frontier']['base']+12,bytes='00'*12)]
    reject('recursive slab aliases another allocation owner',lambda:run(aliased))
    reject('missing original recursive allocation probe rearm',lambda:compare(config,pe,events.replace('recursive-hash-rearm 00019cb6 00019cb9\n',''),native,old,new,state,'hello-debug-dll',buffer))
    reject('missing recursive ownership evidence',lambda:validate({k:v for k,v in config.items()if k!='recursive_arena_observations'},pe))
    bad=copy.deepcopy(config);bad['recursive_arena_observations']['evidence_sha256']='0'*64
    reject('stale recursive ownership evidence',lambda:validate(bad,pe))
    from binary import PE
    from audit import publics
    compiled=PE(Path(__file__).resolve().parent.parent/'build/CVPACK.EXE');linked=publics();proof=compiled_provenance(compiled,linked)
    bad=copy.deepcopy(compiled);raw=bytearray(bad.data);at=bad.offset(proof['call']-5);assert raw[at]==0x68;raw[at+1]^=1;bad.data=bytes(raw)
    reject('compiled recursive allocator request changed',lambda:compiled_provenance(bad,linked))
    # Independently different but unchanged padding is valid, never equality credit.
    def independent_padding(a,b,c,d):
        mutate_bytes(b['allocations'][0],'before',6,b'\xa5\x5a');mutate_bytes(b['frontier'],'bytes',6,b'\xa5\x5a')
        mutate_bytes(b['allocations'][0],'before',12,b'\x96');mutate_bytes(b['frontier'],'bytes',12,b'\x96')
    run(independent_padding)
    rows=json.loads((directory/'native-symbol-output.json').read_text());rows[0]['partial_hash']^=1
    from symbol_output import compare as compare_symbol
    reject('referenced hash caller partial value differs',lambda:compare_symbol(config,pe,events,rows,old,new,True,state))
