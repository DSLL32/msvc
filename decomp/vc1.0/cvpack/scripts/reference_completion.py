"""Full retained module3 contracts at the actual97 caller-return checkpoint."""
import json,re,hashlib
from pathlib import Path
from state import BINARY
from objects import original_walks,compare_walks
from symbol_references import compare as compare_references,validate as validate_references

def compare(config,pe,original,native):
    original,native=Path(original),Path(native);events=(original/'events').read_text();state=json.loads((native/'gdb.json').read_text())
    assert state['signal']==0 and state['frames'][0]=='record_symbol_offsets','completion is not a genuine native caller return'
    assert events.splitlines().count('exception 80000003 va 0001d943 base 00010000 first 1')==1,'completion original return point differs'
    def read(name):return json.loads((native/(name+'.json')).read_text())
    specifications=json.loads((BINARY/'walks.json').read_text());old=original_walks(events,specifications);new=state['walks']
    from symbol_output import compare as compare_symbols
    symbols=compare_symbols(config,pe,events,read('symbol-output'),old,new,True,state)
    references=compare_references(config,pe,events,read('symbol-references'),old,new,state,read('symbol-blocks'),symbols)
    from public_arena import context as arena_context
    arena=arena_context(config,pe,events,state)
    # The third constructor has published its node, but its parent's following
    # counter increments have NOT executed. Retain raw scalars; count the one
    # explicitly observed pending node only in the ownership walk model.
    p=validate_references(config,pe);assert p['pending_parent_counter']==dict(count=2,bytes=32,published_nodes=3,published_payload=48)
    baseline_path=BINARY/'cases/hello-debug-dll/evidence/0096-record_symbol_offsets/proof.json'
    assert hashlib.sha256(baseline_path.read_bytes()).hexdigest()=='68dccd7c14096f2513acad88ab21b68eed27f8d3f868a2e20ddad7929e98c98b','pending parent baseline proof changed'
    baseline=json.loads(baseline_path.read_text())['matched_application_globals']
    oldcount=int.from_bytes(bytes.fromhex(baseline['global_symbols_count']),'little');oldsize=int.from_bytes(bytes.fromhex(baseline['global_symbols_size']),'little')
    assert(oldcount,oldsize)==(235,4504),'pending parent baseline values differ'
    for group in arena['original_families']+arena['native_families']:
        assert group['name']=='global'and(group['count'],group['total'])==(oldcount+2,oldsize+32),'pending constructor-parent scalar state differs'
        group.update(count=group['count']+1,total=group['total']+16)
    from symbol_blocks import context as blocks_context,check_disjoint
    blocks=blocks_context(config,pe,events,read('symbol-blocks'),old,state,arena,references)
    assert blocks['nodes']==245,'retained old242 plus exact3 references inventory differs'
    from buffer_storage import compare as buffer_compare,parse as parse_buffer
    from packed_arena import compare as packed_compare
    buf=read('buffer-storage');packed=packed_compare(config,pe,events,read('packed-arena'),old,new,state,'hello-debug-dll',parse_buffer(events),buf)
    buffer=buffer_compare(config,pe,events,buf,old,new,state,'hello-debug-dll',packed)
    from recursive_arena import compare as recursive_compare
    recursive=recursive_compare(config,pe,events,read('recursive-arena'),old,new,state,'hello-debug-dll',buffer)
    extra_old={**blocks['original_namespace'],**buffer['original_namespace'],**recursive['original_namespace']};extra_new={**blocks['native_namespace'],**buffer['native_namespace'],**recursive['native_namespace']}
    check_disjoint(blocks,extra_old,extra_new)
    from pdb_consumed import compare as consumed_compare
    consumed=consumed_compare(config,pe,events,read('pdb-consumed'),True)
    from reads import validate as validate_reads,compare as compare_reads
    read_proof=compare_reads(events,read('read-returns'),validate_reads(config,pe))
    from pdb_read import compare as compare_pdb_reads
    pdb_reads=read('pdb-read-returns');pdb_read_proof=compare_pdb_reads(config,pe,events,pdb_reads,True)
    consumed.update(reads_original=pdb_read_proof,reads_native=pdb_reads)
    from read_progress import compare as progress_compare
    frontier=next(r for r in config['functions']if r['name']=='record_symbol_offsets')
    progress=progress_compare(config,pe,events,read('read-progress'),frontier,state)
    from allocations import context as allocation_context
    definitions=allocation_context(config,pe,events,state,frontier,'hello-debug-dll',read('read-returns'),progress)
    observed=compare_walks(specifications,old,new,definitions,arena,consumed,extra_old,extra_new,buffer['comparison']['pdb_writes'],buffer)
    from module_address_returns import compare as module_compare
    modules=module_compare(config,pe,events,read('module-address'),old,new,'hello-debug-dll')
    from resolver_returns import compare as resolver_compare
    resolver=resolver_compare(config,pe,events,read('resolver-returns'),old,new,'hello-debug-dll')
    original_objects={**old,**extra_old,'packed_symbol_allocation':[symbols['original_namespace']]};native_objects={**new,**extra_new,'packed_symbol_allocation':[symbols['native_namespace']]}
    from type_pages import compare as pages_compare
    page=pages_compare(config,pe,events,read('type-pages'),original_objects,native_objects,state,'hello-debug-dll',buffer)
    original_objects['type_pointer_page']=[page['original_namespace']];native_objects['type_pointer_page']=[page['native_namespace']]
    memory={}
    for a,raw in re.findall(r'^memory ([0-9a-f]+) ([0-9a-f]+)$',events,re.M):memory.update((int(a,16)+i,v)for i,v in enumerate(bytes.fromhex(raw)))
    from application_state import compare_globals
    globals=compare_globals(config,pe,memory,state,original_objects,native_objects,definitions,events,symbols)
    assert int.from_bytes(bytes(memory[0x39614+i]for i in range(4)),'little')==state['state']['crt_errno'],'completion CRT errno differs'
    assert state['parameters']['symbols']['address']==symbols['native']['base']and state['parameters']['size']==608 and state['parameters']['module']==3,'completion actual caller formals differ'
    return dict(classification='three-actual-module3-constructor-returns-with-retained-full-contracts',references=references,buffer_storage=buffer,recursive_arena=recursive,packed_arena=packed,symbol_blocks=blocks,packed_symbol_output=symbols,type_page_lifetime=page,module_address=modules,resolver_returns=resolver,observed_objects=observed,matched_application_globals=globals,read_return_observations=read_proof,pdb_header_reads=pdb_read_proof,consumed_undefined_input=consumed,partial_read_observations=progress,pending_parent_counter=dict(raw_count=oldcount+2,raw_size=oldsize+32,ownership_count=oldcount+3,ownership_size=oldsize+48))
