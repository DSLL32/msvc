"""Cached normal-case counterexamples for the typed append/storage boundary."""
import copy,json
from pathlib import Path
from unittest.mock import patch
from state import BINARY,load
from objects import original_walks
from buffer_storage import compare,parse,compiled_provenance

def changed(hexadecimal,offset,value):
    raw=bytearray.fromhex(hexadecimal);raw[offset]=value;return raw.hex()

def check(directory):
    directory=Path(directory);config,pe=load()
    events=(directory/'original-events').read_text();native=json.loads((directory/'native-buffer-storage.json').read_text());state=json.loads((directory/'native-gdb.json').read_text())
    old=parse(events);spec=json.loads((directory/'walks.json').read_text());walks=original_walks(events,spec)
    def run(a,b,c,d,s,text=events):
        with patch('buffer_storage.parse',return_value=a):return compare(config,pe,text,b,c,d,s,'hello-debug-dll')
    run(old,native,walks,state['walks'],state)
    def reject(label,mutate,text=events):
        a,b,c,d,s=copy.deepcopy((old,native,walks,state['walks'],state));mutate(a,b,c,d,s)
        try:run(a,b,c,d,s,text)
        except(AssertionError,KeyError,IndexError,ValueError,TypeError):print('rejected:',label)
        else:raise AssertionError('accepted '+label)
    def entries(index,key,value):return lambda a,b,c,d,s:b['entries'][index].__setitem__(key,value)
    reject('missing all buffer observations',lambda a,b,c,d,s:b.clear())
    reject('missing original copy inventory',lambda a,b,c,d,s:a['entries'].clear())
    reject('missing native copy return',lambda a,b,c,d,s:b['returns'].pop())
    reject('extra buffer allocation',lambda a,b,c,d,s:b['allocations'].append(copy.deepcopy(b['allocations'][0])))
    reject('unknown buffer block size',lambda a,b,c,d,s:b['allocations'][0].__setitem__('size',32516))
    reject('unknown copy size',entries(1,'size',16))
    reject('wrong copy sequence',entries(1,'sequence',3))
    reject('wrong parent TypeRecord',entries(0,'owner',native['entries'][0]['owner']+28))
    reject('source outside defined PDB payload',entries(0,'source',native['entries'][0]['source']+12288))
    reject('incomplete source record',entries(0,'source_bytes',native['entries'][0]['source_bytes'][:-2]))
    reject('wrong append return owner',lambda a,b,c,d,s:b['returns'][0].__setitem__('result',b['returns'][0]['result']+4))
    reject('nonzero calloc node birth',lambda a,b,c,d,s:b['allocations'][0].__setitem__('node_before','01'+b['allocations'][0]['node_before'][2:]))
    reject('premature buffer publication',lambda a,b,c,d,s:b['allocations'][0].__setitem__('root_before',changed(b['allocations'][0]['root_before'],4,1)))
    reject('changed initialized copy',lambda a,b,c,d,s:b['returns'][0].__setitem__('payload',changed(b['returns'][0]['payload'],0,255)))
    reject('unproved tail write',lambda a,b,c,d,s:b['returns'][0].__setitem__('payload',changed(b['returns'][0]['payload'],1000,255)))
    reject('bad buffer available count',lambda a,b,c,d,s:b['returns'][1].__setitem__('root',changed(b['returns'][1]['root'],16,0)))
    reject('bad block used count',lambda a,b,c,d,s:b['returns'][2].__setitem__('node',changed(b['returns'][2]['node'],0,27)))
    reject('unobserved frontier write',lambda a,b,c,d,s:b['frontier'].__setitem__('payload',changed(b['frontier']['payload'],1000,255)))
    reject('wrong frontier identity',lambda a,b,c,d,s:b['frontier'].__setitem__('name','append_buffer'))
    reject('missing original per-call entry rearm',lambda a,b,c,d,s:None,events.replace('buffer-rearm 00022f40 00022f41\n','',1))
    reject('missing second return rearm',lambda a,b,c,d,s:None,events.replace('buffer-rearm 00019a65 00019a67\n','',1))
    reject('wrong caller return address',lambda a,b,c,d,s:a['entries'][1].__setitem__('return_address',0x19a67))
    reject('wrong callee cleanup',lambda a,b,c,d,s:a['returns'][1].__setitem__('esp',a['returns'][1]['esp']+4))
    reject('missing type hash nodes',lambda a,b,c,d,s:b['hash_nodes'].clear())
    reject('aliased type hash nodes',lambda a,b,c,d,s:list(b['hash_nodes'].values())[1].__setitem__('address',list(b['hash_nodes'].values())[0]['address']))
    reject('wrong type hash index',lambda a,b,c,d,s:list(b['hash_nodes'].values())[0].__setitem__('bytes',changed(list(b['hash_nodes'].values())[0]['bytes'],0,4)))
    reject('wrong type hash data owner',lambda a,b,c,d,s:list(b['hash_nodes'].values())[0].__setitem__('bytes',changed(list(b['hash_nodes'].values())[0]['bytes'],4,255)))
    reject('wrong type hash chain count',lambda a,b,c,d,s:d['type_hash_counts'][0].__setitem__('bytes',changed(d['type_hash_counts'][0]['bytes'],355*2,2)))
    reject('wrong TypeRecord publication',lambda a,b,c,d,s:d['local_type_records'][0].__setitem__('bytes',changed(d['local_type_records'][0]['bytes'],19*28+4,4)))
    def pdb_write(a,b,c,d,s):
        for w in(c,d):
            block=w['current_type_server'][0]['objects']['type-storage-1'];raw=bytes.fromhex(block['bytes']);block['bytes']=changed(block['bytes'],100,raw[100]^1)
    reject('both sides unexplained PDB modification',pdb_write)
    # Independently different uninitialized payload tails and reserved node
    # bytes must remain accepted, while every own tail is demonstrably stable.
    a,b,c,d,s=copy.deepcopy((old,native,walks,state['walks'],state))
    for row,value in((a,0x55),(b,0xaa)):
        row['allocations'][0]['before']=changed(row['allocations'][0]['before'],1000,value)
        for r in row['returns']:r['payload']=changed(r['payload'],1000,value)
        row['frontier']['payload']=changed(row['frontier']['payload'],1000,value)
        for r in row['hash_nodes'].values():r['bytes']=changed(r['bytes'],2,value)
    run(a,b,c,d,s);print('accepted: distinct own-stable undefined payload/reserved bytes')
    from audit import publics
    from binary import PE
    rebuilt=PE(BINARY/'build/CVPACK.EXE');linked=publics();proof=compiled_provenance(rebuilt,linked)
    tests=[('changed type-node malloc size',proof['call']-1,16),('changed type-node defined field store',proof['call']+5+bytes.fromhex(proof['publication']).index(b'\x89\x41\x04')+2,8)]
    for label,address,value in tests:
        modified=copy.copy(rebuilt);modified.data=bytearray(rebuilt.data);modified.data[modified.offset(address)]=value
        try:compiled_provenance(modified,linked)
        except AssertionError:print('rejected:',label)
        else:raise AssertionError('accepted '+label)
    from type_pages import compare as compare_pages
    pages=json.loads((directory/'native-type-pages.json').read_text())
    good=run(old,native,walks,state['walks'],state)
    def page_run(text,rows,proof):
        a=dict(walks,**good['original_namespace']);b=dict(state['walks'],**good['native_namespace'])
        return compare_pages(config,pe,text,rows,a,b,state,'hello-debug-dll',proof)
    page_run(events,pages,good)
    for label,text,rows,proof in[
        ('type page slots without buffer write proof',events,pages,None),
        ('type page wrong first pointer slot',events,[dict(pages[0],postbytes=changed(pages[0]['postbytes'],0,255))],good),
        ('type page unwritten tail mutation',events,[dict(pages[0],postbytes=changed(pages[0]['postbytes'],100,255))],good),
        ('missing type page return observation',events,[],good)]:
        try:page_run(text,rows,proof)
        except(AssertionError,KeyError,IndexError,ValueError,TypeError):print('rejected:',label)
        else:raise AssertionError('accepted '+label)
    import tempfile,hashlib
    from buffer_storage import validate
    proof=json.loads((BINARY/config['buffer_storage_observations']['evidence']).read_text())
    with tempfile.TemporaryDirectory(prefix='buffer-proof-',dir=Path.home()/'.ai-tmp/vc1.0-cvpack')as folder:
        path=Path(folder)/'proof.json'
        for label,change in[
            ('unbound buffer source',lambda p:p['sources'].__setitem__('src/types.c','0'*64)),
            ('unknown buffer block count',lambda p:p.__setitem__('maximum_blocks',2)),
            ('unproved PDB write field',lambda p:p['pdb_defined_writes'][0].__setitem__('field',8)),
            ('unproved type hash padding credit',lambda p:p.__setitem__('defined_hash_node_ranges',[[0,12]])),
            ('wrong original allocation site',lambda p:p['original_sites'].__setitem__('allocated',0x22f70)),
            ('wrong native payload observation line',lambda p:p.__setitem__('native_allocation_line',p['native_allocation_line']+1)),
            ('wrong original instruction proof',lambda p:p['instructions'][0].__setitem__('sha256','0'*64))]:
            candidate=copy.deepcopy(proof);change(candidate);path.write_text(json.dumps(candidate));current=copy.deepcopy(config)
            current['buffer_storage_observations']=dict(evidence=str(path),evidence_sha256=hashlib.sha256(path.read_bytes()).hexdigest())
            try:validate(current,pe)
            except AssertionError:print('rejected:',label)
            else:raise AssertionError('accepted '+label)
