"""Full packed pool and two retained symbol blocks at procedure conversion."""
import copy,json
from pathlib import Path
from unittest.mock import patch
from state import load
from objects import original_walks
from buffer_storage import compare as buffer_compare,parse as buffer_parse
from packed_arena import compare as pool_compare,parse as pool_parse,validate as pool_validate,compiled_provenance
from public_arena import context as arena_context
from symbol_blocks import context as block_context,parse as block_parse,check_disjoint
from resolver_returns import compare as resolver_compare,parse as resolver_parse

def check(directory,original_directory=None,native_directory=None):
    directory=Path(directory)
    def read(side,name):
        root=Path(original_directory if side=='original'else native_directory)if original_directory else directory
        return(root/(name if original_directory else side+'-'+name)).read_text()
    config,pe=load();events=read('original','events');state=json.loads(read('native','gdb.json'));old=original_walks(events,json.loads((directory/'walks.json').read_text())if not original_directory else json.loads(Path('walks.json').read_text()));new=state['walks']
    native=json.loads(read('native','buffer-storage.json'));original=buffer_parse(events);pool=json.loads(read('native','packed-arena.json'));birth=pool_parse(events)
    context=pool_compare(config,pe,events,pool,old,new,state,'hello-debug-dll',original,native);buffer=buffer_compare(config,pe,events,native,old,new,state,'hello-debug-dll',context)
    assert len(buffer['comparison']['copies'])==113 and len(buffer['comparison']['hash_nodes'])==113 and buffer['comparison']['defined']==3268
    assert len(buffer['comparison']['pdb_writes'])==88 and len(context['nodes'])==3
    blocks=json.loads(read('native','symbol-blocks.json'));block_original=block_parse(events)
    def block_run(change):
        a,b,c,d=copy.deepcopy((block_original,blocks,old,state));change(a,b,c,d);arena=arena_context(config,pe,events,d)
        with patch('symbol_blocks.parse',return_value=a):return block_context(config,pe,events,b,c,d,arena)
    good=block_run(lambda a,b,c,d:None);assert good['blocks']==2 and good['nodes']==242
    check_disjoint(good,{**buffer['original_namespace'],**good['original_namespace']},{**buffer['native_namespace'],**good['native_namespace']})
    def reject(label,action):
        try:action()
        except(AssertionError,KeyError,IndexError,StopIteration):print('rejected:',label)
        else:raise AssertionError('accepted '+label)
    def write(row,key,at,value):
        raw=bytearray.fromhex(row[key]);raw[at:at+len(value)]=value;row[key]=raw.hex()
    def pool_run(change):
        a,b,c,d=copy.deepcopy((birth,pool,old,new));change(a,b,c,d)
        with patch('packed_arena.parse',return_value=a):return pool_compare(config,pe,events,b,c,d,state,'hello-debug-dll',original,native)
    def buffer_run(change):
        a,b,c,d=copy.deepcopy((original,native,old,new));change(a,b,c,d)
        with patch('buffer_storage.parse',return_value=a):return buffer_compare(config,pe,events,b,c,d,state,'hello-debug-dll',context)
    reject('missing mandatory retained symbol block observations',lambda:block_context(config,pe,events,None,old,state,arena_context(config,pe,events,state)))
    reject('both retained block birth inventories absent',lambda:block_run(lambda a,b,c,d:[r['allocations'].clear()for r in(a,b)]))
    reject('first retained block falsely expired on BOTH sides',lambda:block_run(lambda a,b,c,d:[r['allocations'].pop(0)for r in(a,b)]))
    reject('second retained block wrong family on BOTH sides',lambda:block_run(lambda a,b,c,d:[r['allocations'][1].update(family='intern_public')for r in(a,b)]))
    reject('retained symbol allocation extent changed',lambda:block_run(lambda a,b,c,d:b['allocations'][0].update(size=8101)))
    reject('retained block birth predecessor cursor lost',lambda:block_run(lambda a,b,c,d:b['allocations'][1].update(previous_cursor=0)))
    reject('retained block exhaustion residual changed',lambda:block_run(lambda a,b,c,d:b['allocations'][1].update(previous_available=0)))
    reject('second block first request changed',lambda:block_run(lambda a,b,c,d:b['allocations'][1].update(request=44)))
    reject('old retained block unused tail mutated',lambda:block_run(lambda a,b,c,d:write(b['allocations'][0],'after',8090,b'\xff')))
    reject('retained node reserved bytes receive unproved writes',lambda:block_run(lambda a,b,c,d:write(b['allocations'][0],'after',6,b'\xff\xff')))
    reject('second retained node suffix receives unproved writes',lambda:block_run(lambda a,b,c,d:write(b['allocations'][1],'after',38,b'\xff\xff')))
    def independent_reserved(a,b,c,d):
        write(b['allocations'][0],'before',6,b'\xa5\x5a');write(b['allocations'][0],'after',6,b'\xa5\x5a')
    block_run(independent_reserved)
    reject('new typed allocation aliases old retained block',lambda:check_disjoint(good,{'fake':[dict(address=good['original']['allocations'][0]['base']+12,bytes='00'*12)]},buffer['native_namespace']))
    reject('new packed pool malloc observations both absent',lambda:pool_run(lambda a,b,c,d:[r['allocations'].clear()for r in(a,b)]))
    reject('packed pool owner not first actual pointer copy',lambda:pool_run(lambda a,b,c,d:b['allocations'][0].update(owner=b['allocations'][0]['owner']+4)))
    reject('packed pool first lowDX argument differs',lambda:pool_run(lambda a,b,c,d:b['allocations'][0].update(type=0x1048)))
    reject('packed pool is treated as recursive-node layout',lambda:pool_run(lambda a,b,c,d:write(b['frontier'],'bytes',0,b'\0'*4)))
    reject('packed pool reserved bytes altered',lambda:pool_run(lambda a,b,c,d:write(b['frontier'],'bytes',2,b'\xff\xff')))
    reject('packed pool unused tail altered',lambda:pool_run(lambda a,b,c,d:write(b['frontier'],'bytes',36,b'\xff')))
    reject('packed pool cursor does not cover three actual nodes',lambda:pool_run(lambda a,b,c,d:b['frontier'].update(cursor=b['frontier']['cursor']-12)))
    reject('packed node data no longer owns actual copy',lambda:pool_run(lambda a,b,c,d:write(b['frontier'],'bytes',4,b'\0'*4)))
    reject('missing typed packed parent proof',lambda:buffer_compare(config,pe,events,native,old,new,state,'hello-debug-dll'))
    reject('both recursive root fixups absent before next copy',lambda:buffer_run(lambda a,b,c,d:[write(r['returns'][74],'payload',1830,(4208).to_bytes(2,'little'))for r in(a,b)]))
    reject('both recursive fieldlist fixups absent before next copy',lambda:buffer_run(lambda a,b,c,d:[write(r['returns'][74],'payload',1914,(4199).to_bytes(2,'little'))for r in(a,b)]))
    old_resolver=resolver_parse(events);new_resolver=json.loads(read('native','resolver-returns.json'))
    for r in(old_resolver,new_resolver):r['entries']=r['entries'][:4];r['returns']=r['returns'][:4]
    def missing_resolvers():
        with patch('resolver_returns.parse',return_value=old_resolver):resolver_compare(config,pe,events,new_resolver,old,new,'hello-debug-dll')
    reject('both newly required four resolver calls omitted',missing_resolvers)
    from audit import publics
    from binary import PE
    compiled=PE(Path(__file__).resolve().parent.parent/'build/CVPACK.EXE');linked=publics();canonical=compiled_provenance(compiled,linked);bad=copy.deepcopy(compiled);raw=bytearray(bad.data);at=bad.offset(canonical['call']-5);assert raw[at]==0x68;raw[at+1]^=1;bad.data=bytes(raw)
    reject('packed pool canonical malloc request differs',lambda:compiled_provenance(bad,linked))
