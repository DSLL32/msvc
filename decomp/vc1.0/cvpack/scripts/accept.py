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

def compare_counted_name(item,words,actual,events):
    assert (item['owner'],item['local'],item['original_owner_slot'],item['caller'])==('build_source_file_index','module',11,0x1d541),'unknown counted-name context'
    assert words[0]==item['caller'],'unknown counted-name original caller'
    pointer=words[item['slot']];owner=actual['owner']
    matches=re.findall(r'^counted-owner %08x ([0-9a-f]+) (\d+) ([0-9a-f]+) (\d+)$'%pointer,events,re.M)
    assert len(matches)==1,'missing counted-name original owner'
    parent,module,base,size=matches[0];parent=int(parent,16);base=int(base,16);size=int(size);module=int(module)
    assert parent==words[item['original_owner_slot']]and module==owner['module'],'counted-name parent/module differs'
    assert base<=pointer<base+size and size==owner['size']and pointer-base==owner['offset'],'counted-name allocation extent/offset differs'
    assert actual['address']==owner['base']+owner['offset']and not actual['null'],'counted-name native pointer/owner differs'
    match=re.findall(r'^input-buffer %08x ([0-9a-f]+)$'%pointer,events,re.M);assert len(match)==1,'missing actual counted-name bytes'
    raw=bytes.fromhex(match[0]);assert len(raw)==raw[0]+1<=size-(pointer-base)and raw.hex()==actual['bytes'],'counted-name bytes differ'
    return dict(original_address=pointer,native_address=actual['address'],module=module,owner_offset=pointer-base,owner_extent=size,bytes=raw.hex())

def compare_null_pointer(value,actual):
    assert value==0 and actual['null']and actual['address']==0,'non-NULL pointer outside observed NULL-only frontier'
    return dict(original_address=value,native_address=actual['address'],null_only=True)

def compare_literal(item,value,actual,pe):
    raw=bytes.fromhex(item['bytes'])
    assert value==item['original'] and pe.read(value,len(raw))==raw,'literal argument origin differs'
    assert raw.endswith(b'\0') and not actual['null'] and actual['string']==raw[:-1].hex(),'literal argument bytes differ'


def accept_entry_flow(args,config,pe,function,original,native,captured,receipt):
    """Routine whole-function/real frontier proof; grow state only for actual gaps."""
    import tempfile
    record=json.loads((BINARY/'cases'/args.case/'case.json').read_text());current=record['current'];work=native/args.case
    assert current.get('proof_mode')=='entry-flow'and captured['entry_progress_only']and not(captured.get('reference_completion')or captured.get('scalar_completion')or captured.get('guard_probe')or captured.get('stop_at')or captured['unmasked']),'unapproved flow capture mode'
    assert receipt['terminal']=='trap'and receipt['status']==-5 and captured['helper_status']==0,'not corresponding genuine traps'
    events=(original/'events').read_text();traps=re.findall(r'^exception ([0-9a-f]+) va ([0-9a-f]+) ',events,re.M);assert len(traps)==1 and traps[0][0]=='80000003'and 'exit 99\n'in events,'unknown original terminal'
    apps={r['original']:r for r in config['functions']};frontier=apps[int(traps[0][1],16)];assert frontier['status']=='stub'and frontier['native']==current['native_function'],'original next entry is not cached next stub'
    state=json.loads((work/'gdb.json').read_text());assert state['signal']==5 and state['frames'][0]==frontier['native']and state['later_heap_equality']is False,'native next named trap differs'
    words={int(a):int(b,16)for a,b in re.findall(r'^stack (\d+) ([0-9a-f]+)$',events,re.M)};parameters={}
    registers=re.search(r'^registers (.*)$',events,re.M);assert registers,'missing original ABI register state'
    words.update({k:int(v,16)for k,v in zip(registers[1].split()[::2],registers[1].split()[1::2])})
    assert set(state['parameters'])=={r['name']for r in frontier['parameters']},'new frontier ABI observation incomplete'
    for item in frontier['parameters']:
        if item['kind']=='integer':parameters[item['name']]=compare_integer(item,frontier['parameters'],words,state['parameters'])
        elif item['kind']=='counted-string':parameters[item['name']]=compare_counted_name(item,words,state['parameters'][item['name']],events)
        elif item['kind']=='pointer'and item.get('null_only'):parameters[item['name']]=compare_null_pointer(words[item['slot']],state['parameters'][item['name']])
        elif item['kind']in('stack-output','stack-input','allocation-output'):
            from stack_output import compare as compare_output
            parameters[item['name']]=compare_output(item,frontier['parameters'],words,state['parameters'],events,pe)
        else:raise AssertionError('concrete typed pointer proof required before consumption')
    entries=[];support=[];runtime=json.loads((BINARY/'runtime.json').read_text());identities={(r['start'],r['end']):r for r in runtime['functions']+runtime['imports']}
    for a,b in re.findall(r'^runtime ([0-9a-f]+) ([0-9a-f]+)$',events,re.M):
        a,b=int(a,16),int(b,16)
        if a in apps:
            f=apps[a];assert f['status']=='accepted'or f['name']==function['name'],'unaccepted app body restored';assert f['extent']==[a,b];entries.append(f['native'])
        else:
            identity=identities[(a,b)];support.append(dict(name=identity['name'],start=a,end=b))
    entries.append(frontier['native']);assert function['native']in entries and entries==(work/'trace').read_text().splitlines()==current['application_trace'],'complete entry trace differs'
    # The executable itself must be the exact declared mask, with only accepted
    # application entries trapped for first-entry tracing. No hidden body grants.
    from state import mask
    with tempfile.TemporaryDirectory(dir=Path.home()/'.ai-tmp/vc1.0-cvpack')as directory:
        expected=Path(directory)/'mask.exe';mask(config,pe,expected,[function['name']]);image=bytearray(expected.read_bytes())
        for f in config['functions']:
            if f['status']=='accepted'or f['name']==function['name']:image[pe.offset(f['original'])]=0xcc
        assert bytes(image)==(original/'CVPACK.EXE').read_bytes(),'executed original mask differs'
    from seams import approved,compare_returns,check_policy_snapshot
    seams=approved(config,pe,runtime,args.case);assert captured['conditional_runtime']==seams;policy=captured['conditional_support_sha256'];assert policy==receipt['conditional_support_sha256'];check_policy_snapshot(original,policy);check_policy_snapshot(work,policy)
    crt=compare_returns(events,json.loads((work/'crt-returns.json').read_text()),seams,function.get('required_conditional_calls_by_case',{}).get(args.case))
    for stream in('stdout','stderr'):assert(original/stream).read_bytes()==(work/stream).read_bytes(),'streams differ'
    assert {p.name for p in(original/'work').iterdir()}=={p.name for p in(work/'work').iterdir()},'artifact inventory differs'
    for path in(original/'work').iterdir():assert path.read_bytes()==(work/'work'/path.name).read_bytes(),'full artifact differs'
    memory={}
    for a,raw in re.findall(r'^memory ([0-9a-f]+) ([0-9a-f]+)$',events,re.M):memory.update((int(a,16)+i,v)for i,v in enumerate(bytes.fromhex(raw)))
    definitions={r['name']:r for r in json.loads((BINARY/'data.json').read_text())};matched={};assert set(state['globals'])==set(current.get('observed_globals',[])),'consumed global observation inventory differs'
    for name in current.get('observed_globals',[]):
        d=definitions[name];raw=bytes(memory[d['original']+i]for i in range(d['size']));assert raw.hex()==state['globals'][name],('concrete application global differs',name);matched[name]=raw.hex()
    specs=[r for r in json.loads((BINARY/'walks.json').read_text())if r['name']in current.get('observed_walks',[])]
    assert len(specs)==len(current.get('observed_walks',[])),'unknown typed observed owner'
    old_walks=original_walks(events,specs);new_walks=state.get('walks',{});assert set(new_walks)=={r['name']for r in specs},'typed consumed owner inventory differs'
    owners=compare_walks(specs,old_walks,new_walks)if specs else {}
    subprocess.run(['python3',str(BINARY/'scripts/audit.py')],check=True)
    if args.check_only:print('Fresh whole-function entry-flow proof passes:',args.function);return
    sequence=len(config['accepted_order'])+1;directory=BINARY/'cases'/args.case/'evidence'/('%04d-%s'%(sequence,args.function));directory.mkdir(exist_ok=False)
    for label,source,names in(('original',original,('events','receipt.json','configuration.json','runtime-support.json','runtime-plan','stdout','stderr')),('native',work,('gdb.json','gdb.stdout','gdb.stderr','commands.gdb','trace','configuration.json','runtime-support.json','crt-returns.json','stdout','stderr'))):
        for name in names:
            if(source/name).exists():shutil.copyfile(source/name,directory/(label+'-'+name))
    shutil.copyfile(native/'receipt.json',directory/'native-receipt.json');path=str((directory/'proof.json').relative_to(BINARY));prior={(r['start'],r['end'])for r in config.get('support_unmasked',[])}
    for r in support:
        if(r['start'],r['end'])not in prior:config.setdefault('support_unmasked',[]).append(dict(r,evidence=path))
    proof=dict(function=function['name'],sequence=sequence,case=args.case,extent=function['extent'],original_frontier=frontier['original'],native_frontier=frontier['native'],application_entries=entries,matched_frontier_parameters=state['parameters'],typed_frontier_parameters=parameters,matched_application_globals=matched,observed_objects=owners,conditional_CRT_support=crt,runtime_unmasked=support,seconds=receipt['seconds'],classification='routine-whole-function-and-genuine-next-entry',later_module_heap_equality=False,prior_module3='cases/hello-debug-dll/evidence/0097-create_symbol_reference/proof.json',scope='Root streamlined normal loop; actual formal/selected consumed globals checked, old full module3 contracts retained; additional state probes only for concrete ambiguities/mismatches')
    (directory/'proof.json').write_text(json.dumps(proof,indent=2)+'\n');function.update(status='accepted',evidence=path);config['accepted_order'].append(dict(function=function['name'],case=args.case,kind='application',sequence=sequence));(BINARY/'functions.json').write_text(json.dumps(config,indent=2)+'\n');print('Accepted',sequence,function['name'],'next',frontier['native'])

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
    if json.loads((BINARY/'cases'/args.case/'case.json').read_text())['current'].get('proof_mode')=='entry-flow':
        return accept_entry_flow(args,config,pe,function,original,native,captured,native_receipt)
    assert not(captured.get('entry_progress_only')or captured.get('reference_completion')or captured.get('scalar_completion')),'bounded sliced captures require their separate reviewed validator'
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
    if not completed:
        from application_state import compare_globals
        matched=compare_globals(config,pe,memory,state,original_objects,native_objects,definitions,events,symbol_proof)
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
        elif kind in ('stack-output','stack-input','allocation-output'):
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
    proof['register_type_field']=validate_symbol_type_fields(config,pe,PE(BINARY/'build/CVPACK.EXE'),publics(),'convert_register_symbol')
    (directory/'proof.json').write_text(json.dumps(proof,indent=2)+'\n')
    function['status']='accepted';function['evidence']=proof_path
    config['accepted_order'].append(dict(function=args.function,case=args.case,kind='application',sequence=sequence))
    (BINARY/'functions.json').write_text(json.dumps(config,indent=2)+'\n')
    print('Accepted',args.function,'as application function',sequence,'; next',frontier['name'] if frontier else 'case complete')

if __name__=='__main__':main()
