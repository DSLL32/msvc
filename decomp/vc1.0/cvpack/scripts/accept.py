#!/usr/bin/env python3
"""Accept a candidate only after fresh original/native frontier/state evidence."""
import argparse
import json
import re
import shutil
import subprocess
from pathlib import Path
from state import BINARY,ROOT,load,sha
from objects import original_walks,compare_walks,pointer_identity

def validate_snapshot(config,current,original,native,captured,native_receipt,target,executable):
    """Bind both fresh runs to the same current candidate and original image."""
    expected=sha(current)
    for directory,receipt in ((original,captured),(native,native_receipt)):
        snapshot=directory/'configuration.json'
        assert receipt['configuration_sha256']==sha(snapshot)==expected,'stale or mismatched configuration snapshot'
        assert json.loads(snapshot.read_text())==config,'configuration contents differ'
    assert captured['configuration_sha256']==native_receipt['configuration_sha256']
    assert captured['original_sha256']==native_receipt['original_sha256']==config['target']['sha256']==sha(target),'captured original target differs'
    assert captured['executable_sha256']==sha(original/'CVPACK.EXE'),'captured masked image differs'
    assert captured['debugger_sha256']==sha(BINARY/'build/debug-run.exe'),'original capture controller changed'
    assert native_receipt['executable_sha256']==sha(executable),'native executable changed since run'

def compare_integer(item,descriptions,words,parameters):
    value=words[item.get('register',item.get('slot'))]
    actual=parameters[item['name']]
    condition=item.get('inactive_when')
    if condition:
        assert (BINARY/condition['evidence']).is_file(),'missing conditional ABI-state proof'
        controlling=next(r for r in descriptions if r['name']==condition['argument'])
        assert controlling['kind']=='integer' and controlling.get('bits',32)==32
        old=words[controlling['slot']]&0xffffffff
        new=parameters[controlling['name']]&0xffffffff
        assert old==new,('conditional ABI controlling argument differs',controlling['name'],old,new)
        if old&condition['mask']==condition['value']:
            return dict(slot=item['slot'],original_word=value,native_word=actual&0xffffffff,
                        condition=condition,controlling_word=old)
    mask=(1<<item.get('bits',32))-1
    assert value&mask==actual&mask,('ABI integer differs',item['name'],value,actual)
    return None

def compare_literal(item,value,actual,pe):
    raw=bytes.fromhex(item['bytes'])
    assert value==item['original'] and pe.read(value,len(raw))==raw,'literal argument origin differs'
    assert raw.endswith(b'\0') and not actual['null'] and actual['string']==raw[:-1].hex(),'literal argument bytes differ'


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--function',required=True)
    parser.add_argument('--check-only',action='store_true',help='validate the complete fresh pair without accepting or copying evidence')
    parser.add_argument('--original-rebind',type=Path,help='explicit audited native-only source-line rebind sidecar')
    parser.add_argument('--case',required=True)
    parser.add_argument('--original',required=True,type=Path)
    parser.add_argument('--native',required=True,type=Path)
    args=parser.parse_args()
    config,pe=load()
    function=next(r for r in config['functions'] if r['name']==args.function)
    assert function['status']=='candidate'
    original=args.original.resolve()
    native=args.native.resolve()
    captured=json.loads((original/'receipt.json').read_text())
    native_receipt=json.loads((native/'receipt.json').read_text())[0]
    rebind=None
    if args.original_rebind:
        from native_line_rebind import validate as validate_rebind
        rebind=validate_rebind(original,config,BINARY/'functions.json',native/args.case,native_receipt,args.original_rebind)
    else:
        validate_snapshot(config,BINARY/'functions.json',original,native/args.case,captured,native_receipt,
                          ROOT/config['target']['path'],BINARY/'build/native/CVPACK.EXE')
    assert captured['case']==native_receipt['case']==args.case
    assert captured['candidates']==[args.function]
    assert not captured.get('guard_probe'),'guard controller evidence cannot accept application functions'
    assert native_receipt['executable_sha256']==sha(BINARY/'build/native/CVPACK.EXE')
    events=(original/'events').read_text()
    from seams import approved,compare_returns,check_policy_snapshot
    seam_records=approved(config,pe,json.loads((BINARY/'runtime.json').read_text()),args.case)
    assert captured.get('conditional_runtime',[])==seam_records,'stale conditional CRT entry evidence'
    policy=sha(BINARY/'runtime-support.json')if (BINARY/'runtime-support.json').exists()else None
    assert captured.get('conditional_support_sha256')==native_receipt.get('conditional_support_sha256')==policy,'stale conditional CRT policy'
    check_policy_snapshot(original,policy)
    check_policy_snapshot(native/args.case,policy)
    observations=native/args.case/'crt-returns.json'
    crt_proof=compare_returns(events,json.loads(observations.read_text())if observations.exists()else [],seam_records,function.get('required_conditional_calls_by_case',{}).get(args.case))
    from reads import validate as validate_reads,compare as compare_reads
    read_sites=validate_reads(config,pe)if args.case=='hello-debug-dll'else []
    native_reads=json.loads((native/args.case/'read-returns.json').read_text())if read_sites else []
    read_proof=compare_reads(events,native_reads,read_sites)if read_sites else []
    from pdb_read import compare as compare_pdb_reads
    pdb_reads=json.loads((native/args.case/'pdb-read-returns.json').read_text())if (native/args.case/'pdb-read-returns.json').exists()else []
    required_pdb=args.case=='hello-debug-dll'and function.get('required_pdb_record_proof',False)
    pdb_read_proof=compare_pdb_reads(config,pe,events,pdb_reads,required_pdb)
    from pdb_consumed import compare as compare_pdb_consumed
    consumed_path=native/args.case/'pdb-consumed.json'
    consumed_proof=compare_pdb_consumed(config,pe,events,json.loads(consumed_path.read_text())if consumed_path.exists()else [],required_pdb)
    consumed_proof.update(reads_original=pdb_read_proof,reads_native=pdb_reads)
    old_work,new_work=original/'work',native/args.case/'work'
    assert {p.name for p in old_work.iterdir()}=={p.name for p in new_work.iterdir()},'artifact set differs'
    for path in old_work.iterdir():assert path.read_bytes()==(new_work/path.name).read_bytes(),'complete artifact bytes differ: '+path.name
    match=re.search(r'^exception ([0-9a-f]+) va ([0-9a-f]+)',events,re.M)
    completed=match is None
    assert not captured['unmasked'],'acceptance must use a masked original'
    if completed:
        case_record=json.loads((BINARY/'cases'/args.case/'case.json').read_text())
        expected=case_record['complete_reference']
        terminal=re.search(r'^exit (\d+)$',events,re.M)
        assert terminal and int(terminal[1])==expected['exit']==native_receipt['status']
        assert native_receipt['terminal']=='exit'
        address=None;frontier=None
        state=dict(argc=None,argv=[],parameters={})
    else:
        assert int(match[1],16)==0x80000003,'unknown original exception'
        state=json.loads((native/args.case/'gdb.json').read_text())
        address=int(match[2],16)
        frontier=next(r for r in config['functions'] if r['original']==address)
        assert frontier['status']=='stub','frontier is not a new named application function'
        assert state['frames'][0]==frontier['native'] and state['signal']==5
    argv=[bytes.fromhex(line.split()[2]).decode('latin1') for line in events.splitlines() if line.startswith('argv ')]
    if not completed:assert argv==state['argv'] and len(argv)==state['argc'],'argv/state mismatch'
    memory={}
    for line in events.splitlines():
        if line.startswith('memory '):
            _,a,value=line.split()
            start=int(a,16)
            memory.update((start+i,byte)for i,byte in enumerate(bytes.fromhex(value)))
    matched={}
    if crt_proof and not completed:
        old_errno=int.from_bytes(bytes(memory[0x39614+i]for i in range(4)),'little')
        assert old_errno==state['state']['crt_errno'],'CRT errno differs at application boundary'
    from read_progress import compare as compare_read_progress
    progress_path=native/args.case/'read-progress.json'
    progress_proof=compare_read_progress(config,pe,events,json.loads(progress_path.read_text())if progress_path.exists()else [],frontier,state)
    object_proof={}
    buffer_proof=None;recursive_proof=None;packed_proof=None;blocks_proof=None
    if not completed and (BINARY/'walks.json').exists():
        specifications=json.loads((BINARY/'walks.json').read_text())
        from allocations import context as allocation_context
        definitions=allocation_context(config,pe,events,state,frontier,args.case,native_reads,progress_proof)
        from public_arena import context as public_arena_context
        arena_context=public_arena_context(config,pe,events,state)
        from symbol_blocks import context as retained_symbol_blocks
        blocks_path=native/args.case/'symbol-blocks.json'
        blocks_proof=retained_symbol_blocks(config,pe,events,json.loads(blocks_path.read_text())if blocks_path.exists()else None,original_walks(events,specifications),state,arena_context)
        from buffer_storage import compare as compare_buffer_storage
        buffer_path=native/args.case/'buffer-storage.json'
        from packed_arena import compare as compare_packed_arena
        from buffer_storage import parse as parse_buffer_storage
        packed_path=native/args.case/'packed-arena.json'
        buffer_native=json.loads(buffer_path.read_text())if buffer_path.exists()else None
        packed_proof=compare_packed_arena(config,pe,events,json.loads(packed_path.read_text())if packed_path.exists()else None,original_walks(events,specifications),state['walks'],state,args.case,parse_buffer_storage(events),buffer_native)
        buffer_proof=compare_buffer_storage(config,pe,events,buffer_native,original_walks(events,specifications),state['walks'],state,args.case,packed_proof)
        from recursive_arena import compare as compare_recursive_arena
        recursive_path=native/args.case/'recursive-arena.json'
        recursive_proof=compare_recursive_arena(config,pe,events,json.loads(recursive_path.read_text())if recursive_path.exists()else None,original_walks(events,specifications),state['walks'],state,args.case,buffer_proof)
        old_extra=dict(buffer_proof['original_namespace'])if buffer_proof else {};new_extra=dict(buffer_proof['native_namespace'])if buffer_proof else {}
        if blocks_proof:old_extra.update(blocks_proof['original_namespace']);new_extra.update(blocks_proof['native_namespace'])
        if recursive_proof:old_extra.update(recursive_proof['original_namespace']);new_extra.update(recursive_proof['native_namespace'])
        if blocks_proof:
            from symbol_blocks import check_disjoint as check_symbol_block_owners
            check_symbol_block_owners(blocks_proof,old_extra,new_extra)
        object_proof=compare_walks(specifications,original_walks(events,specifications),state['walks'],definitions,arena_context,consumed_proof,old_extra or None,new_extra or None,buffer_proof['comparison']['pdb_writes']if buffer_proof else None,buffer_proof)
    original_objects=original_walks(events,specifications)if not completed else {}
    native_objects=dict(state['walks'])if not completed else {}
    if buffer_proof:
        original_objects.update(buffer_proof['original_namespace']);native_objects.update(buffer_proof['native_namespace'])
    if recursive_proof:
        original_objects.update(recursive_proof['original_namespace']);native_objects.update(recursive_proof['native_namespace'])
    if blocks_proof:
        original_objects.update(blocks_proof['original_namespace']);native_objects.update(blocks_proof['native_namespace'])
    from module_address_returns import compare as compare_module_address
    module_address_path=native/args.case/'module-address.json'
    module_address_proof=compare_module_address(config,pe,events,json.loads(module_address_path.read_text())if module_address_path.exists()else [],original_objects,native_objects,args.case)if args.case=='hello-debug-dll'else None
    from resolver_returns import compare as compare_resolver_returns
    resolver_path=native/args.case/'resolver-returns.json'
    resolver_proof=compare_resolver_returns(config,pe,events,json.loads(resolver_path.read_text())if resolver_path.exists()else None,original_objects,native_objects,args.case)if args.case=='hello-debug-dll'else None
    from symbol_output import compare as compare_symbol_output
    symbol_path=native/args.case/'symbol-output.json'
    symbol_proof=compare_symbol_output(config,pe,events,json.loads(symbol_path.read_text())if symbol_path.exists()else [],original_objects,native_objects,args.case=='hello-debug-dll'and function.get('required_symbol_output_observation',False),state)if args.case=='hello-debug-dll'else None
    if symbol_proof:
        original_objects['packed_symbol_allocation']=[symbol_proof['original_namespace']]
        native_objects['packed_symbol_allocation']=[symbol_proof['native_namespace']]
    from type_pages import compare as compare_type_pages
    page_path=native/args.case/'type-pages.json'
    page_proof=compare_type_pages(config,pe,events,json.loads(page_path.read_text())if page_path.exists()else [],original_objects,native_objects,state,args.case,buffer_proof)if args.case=='hello-debug-dll'else None
    if page_proof:
        original_objects['type_pointer_page']=[page_proof['original_namespace']]
        native_objects['type_pointer_page']=[page_proof['native_namespace']]
    for item in ([] if completed else json.loads((BINARY/'data.json').read_text())):
        raw=bytes(memory[item['original']+i]for i in range(item['size']))
        actual=state['globals'][item['name']]
        if item.get('kind')=='pointer-structure':
            expected=bytearray(raw);observed=bytearray.fromhex(actual)
            assert len(expected)==len(observed)==item['size']
            for offset in item['pointer_fields']:
                assert pointer_identity(original_objects,int.from_bytes(expected[offset:offset+4],'little'))==pointer_identity(native_objects,int.from_bytes(observed[offset:offset+4],'little')),'global structure pointer owner differs'
                expected[offset:offset+4]=observed[offset:offset+4]=b'\0'*4
            assert expected==observed,'global structure defined bytes differ'
            expected=actual
        elif item.get('kind')=='callback-table':
            from callback_table import describe
            expected=describe(item,raw,config['functions'])
        elif item.get('kind')=='string-pointer-array':
            expected=[]
            for index in range(item['size']//4):
                pointer=int.from_bytes(raw[index*4:index*4+4],'little')
                value=[]
                while memory[pointer]:value.append(memory[pointer]);pointer+=1
                expected.append(bytes(value).hex())
        elif item.get('kind')=='pointer-array':
            assert item['size']%4==0 and len(actual)==item['size']//4,'global pointer-array extent differs'
            expected=[]
            for index,entry in enumerate(actual):
                value=int.from_bytes(raw[index*4:index*4+4],'little')
                assert entry['null']==(value==0)and (entry['address']==0)==(value==0),'global pointer-array null state differs'
                identity=dict(null=not value)
                if value:
                    old_id=pointer_identity(original_objects,value);new_id=pointer_identity(native_objects,entry['address'])
                    assert old_id==new_id,('global pointer-array owner differs',item['name'],index,old_id,new_id)
                    identity['object']=old_id
                expected.append(identity)
            actual=expected
        elif isinstance(actual,dict):
            value=int.from_bytes(raw,'little')
            assert actual['null']==(value==0) and (actual['address']==0)==(value==0)
            expected={'null':value==0}
            if value:
                targets=[r for r in config['functions']if r['original']==value]
                owners=[dict(name=r['name'],offset=value-r['original'])for r in json.loads((BINARY/'data.json').read_text())if r['original']<=value<r['original']+r['size']]
                if targets:
                    assert len(targets)==1 and actual['function']==targets[0]['native']
                    expected['function']=targets[0]['native']
                elif owners:
                    assert len(owners)==1
                    owner=owners[0]
                    assert actual['address']==state['global_addresses'][owner['name']]+owner['offset']
                    expected['global']=owner
                else:
                    old_id=pointer_identity(original_objects,value)
                    new_id=pointer_identity(native_objects,actual['address'])
                    assert old_id==new_id,('global object pointer differs',item['name'],old_id,new_id)
                    expected['object']=old_id
            actual=expected
        else:expected=raw.hex()
        assert actual==expected,('application state differs',item['name'],expected,actual)
        matched[item['name']]=actual
    words={int(line.split()[1]):int(line.split()[2],16) for line in events.splitlines() if line.startswith('stack ')}
    registers=re.search(r'^registers (.*)$',events,re.M)
    if registers:
        fields=registers[1].split();words.update((fields[i],int(fields[i+1],16))for i in range(0,len(fields),2))
    descriptions=frontier.get('parameters',[]) if frontier else []
    inactive={}
    assert {r['name'] for r in descriptions}==set(state['parameters']),'missing ABI parameter specification'
    for item in descriptions:
        value=words[item.get('register',item.get('slot'))]
        actual=state['parameters'][item['name']]
        kind=item['kind']
        if kind=='integer':
            proof=compare_integer(item,descriptions,words,state['parameters'])
            if proof:inactive[item['name']]=proof
        elif kind in ('stack-output','stack-input'):
            from stack_output import compare as compare_output
            inactive[item['name']]=compare_output(item,descriptions,words,state['parameters'],events,pe)
        elif kind in ('pointer','cstring'):
            assert actual['null']==(value==0)
            if value:
                context=json.loads((BINARY/'data.json').read_text())
                owners=[dict(name=r['name'],offset=value-r['original']) for r in context if r['original']<=value<r['original']+r['size']]
                if owners:
                    assert len(owners)==1 and actual.get('global')==owners[0],('mismatched global pointer argument',item['name'],value,actual)
                else:
                    old_pointer=pointer_identity(original_objects,value)
                    new_pointer=pointer_identity(native_objects,actual['address'])
                    assert old_pointer==new_pointer,('mismatched pool pointer argument',item['name'],old_pointer,new_pointer)
                if kind=='cstring':
                    text=[];pointer=value
                    while memory[pointer]:text.append(memory[pointer]);pointer+=1
                    assert bytes(text).hex()==actual['string'],('string argument differs',item['name'])
        elif kind=='transient-pointer':
            from transient import compare as compare_transient
            inactive[item['name']]=compare_transient(item,words,state['parameters'],events,pe)
        elif kind=='literal-string':
            compare_literal(item,value,actual,pe)
        elif kind=='argv':assert not actual['null']
        elif kind=='function':
            target=next(r for r in config['functions'] if r['original']==value)
            assert not actual['null'] and actual['function']==target['native']
        else:raise AssertionError('unsupported ABI proof kind: '+kind)
    for stream in ('stdout','stderr'):
        assert (original/stream).read_bytes()==(native/args.case/stream).read_bytes(),stream+' mismatch'
        if completed:
            assert (original/stream).read_bytes()==(BINARY/'cases'/args.case/expected[stream]).read_bytes()
    applications={r['original']:r for r in config['functions']}
    entries=[]
    runtime=json.loads((BINARY/'runtime.json').read_text())
    identities={(r['start'],r['end']):r for r in runtime['functions']+runtime['imports']}
    support=[]
    for line in events.splitlines():
        if not line.startswith('runtime '):continue
        _,a,b=line.split();a,b=int(a,16),int(b,16)
        if a in applications:entries.append(applications[a]['native'])
        else:
            identity=identities[(a,b)]
            support.append(dict(name=identity['name'],start=a,end=b))
    if frontier:entries.append(frontier['native'])
    assert function['native']in entries,'candidate function was not reached'
    native_entries=(native/args.case/'trace').read_text().splitlines()
    assert entries==native_entries,('entry trace mismatch',entries,native_entries)
    subprocess.run(['python3',str(BINARY/'scripts/audit.py')],check=True)
    if args.check_only:
        print('Fresh original/native candidate proof passes without acceptance:',args.function)
        return
    sequence=len(config['accepted_order'])+1
    directory=BINARY/'cases'/args.case/'evidence'/('%04d-%s'%(sequence,args.function))
    directory.mkdir(parents=True,exist_ok=False)
    for name in ('events','receipt.json','stdout','stderr','runtime-plan','configuration.json'):
        shutil.copyfile(original/name,directory/('original-'+name))
    for name in ('gdb.json','gdb.stdout','gdb.stderr','commands.gdb','trace','stdout','stderr','crt-returns.json','read-returns.json','pdb-read-returns.json','pdb-consumed.json','read-progress.json','symbol-output.json','type-pages.json','buffer-storage.json','resolver-returns.json','recursive-arena.json','packed-arena.json','symbol-blocks.json','module-address.json'):
        if (native/args.case/name).is_file():shutil.copyfile(native/args.case/name,directory/('native-'+name))
    shutil.copyfile(native/args.case/'configuration.json',directory/'native-configuration.json')
    for side,source in (('original',original),('native',native/args.case)):
        if (source/'runtime-support.json').exists():shutil.copyfile(source/'runtime-support.json',directory/(side+'-runtime-support.json'))
    shutil.copyfile(native/'receipt.json',directory/'native-receipt.json')
    if rebind:
        shutil.copytree(args.original_rebind.parent,directory/'native-line-rebind')
    shutil.copyfile(BINARY/'walks.json',directory/'walks.json')
    proof_path=str(directory.relative_to(BINARY)/'proof.json')
    prior={(r['start'],r['end'])for r in config.get('support_unmasked',[])}
    for record in support:
        if (record['start'],record['end']) not in prior:
            record['evidence']=proof_path
            config.setdefault('support_unmasked',[]).append(record)
    proof=dict(function=args.function,case=args.case,sequence=sequence,extent=function['extent'],
        original_frontier=address,native_frontier=frontier['native'] if frontier else 'exit',application_entries=entries,
        argc=state['argc'],argv=argv,matched_application_globals=matched,
        matched_frontier_parameters=state['parameters'],runtime_unmasked=support,pdb_header_reads=pdb_read_proof,consumed_undefined_input=consumed_proof,
        conditionally_inactive_parameters=inactive,
        conditional_CRT_support=crt_proof,
        read_return_observations=read_proof,partial_read_observations=progress_proof,packed_symbol_output=symbol_proof,type_page_lifetime=page_proof,buffer_storage=buffer_proof,
        observed_objects=object_proof,resolver_returns=resolver_proof,recursive_arena=recursive_proof,packed_arena=packed_proof,symbol_blocks=blocks_proof,module_address=module_address_proof,
        process_boundary=native_receipt.get('process_boundary'),
        graph_ABI_data='Complete original/compiler CFG/callee/RET ABI and contextual initial data passed',
        seconds=native_receipt['seconds'])
    if rebind:proof['original_native_line_rebind']=dict(path='native-line-rebind/rebind.json',sha256=sha(args.original_rebind))
    if config.get('type_cache_observations'):
        from type_cache import validate as validate_type_cache
        from binary import PE
        from audit import publics
        proof['type_cache_allocation_provenance']=validate_type_cache(config,pe,PE(BINARY/'build/CVPACK.EXE'),publics())['compiled_allocation']
    from symbol_type_fields import validate as validate_symbol_type_fields
    from binary import PE
    from audit import publics
    proof['symbol_type_field']=validate_symbol_type_fields(config,pe,PE(BINARY/'build/CVPACK.EXE'),publics())
    (directory/'proof.json').write_text(json.dumps(proof,indent=2)+'\n')
    function['status']='accepted';function['evidence']=proof_path
    config['accepted_order'].append(dict(function=args.function,case=args.case,kind='application',sequence=sequence))
    (BINARY/'functions.json').write_text(json.dumps(config,indent=2)+'\n')
    print('Accepted',args.function,'as application function',sequence,'; next',frontier['name'] if frontier else 'case complete')

if __name__=='__main__':main()
