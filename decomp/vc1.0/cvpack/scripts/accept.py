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

def compare_symbol_sort_key(item,words,actual,events):
    assert(item['qsort_return'],item['qsort_callback'])==(0x1d1d7,0x1d050),'unknown symbol-sort caller'
    frames=[i for i in words if isinstance(i,int)and words[i]==item['qsort_return']and words.get(i+3)==8 and words.get(i+4)==item['qsort_callback']]
    assert len(frames)==1,'missing/ambiguous actual original qsort frame'
    frame=frames[0];base,count=words[frame+1],words[frame+2];pointer=words[item['slot']];array=actual['array']
    assert count>1 and base<=pointer<base+count*8 and not(pointer-base)%8,'original symbol key outside qsort allocation'
    assert array['count']==count and array['stride']==8 and array['index']==(pointer-base)//8 and actual['address']==array['base']+array['index']*8,'symbol sort array/index differs'
    raw=re.findall(r'^input-buffer %08x ([0-9a-f]+)$'%pointer,events,re.M);key=re.findall(r'^symbol-key %08x ([0-9a-f]+) ([0-9a-f]+)$'%pointer,events,re.M)
    assert len(raw)==len(key)==1,'missing actual original symbol-sort inputs'
    raw=bytes.fromhex(raw[0]);node,offset=(int(v,16)for v in key[0]);new=bytes.fromhex(actual['bytes'])
    assert len(raw)==len(new)==8 and int.from_bytes(raw[:4],'little')==node!=0 and int.from_bytes(new[:4],'little')==actual['symbol']['address']!=0,'symbol key cell/pointee differs'
    assert offset==actual['symbol']['offset'],'consumed symbol address differs'
    return dict(original_cell=pointer,native_cell=actual['address'],array_index=array['index'],array_count=count,original_symbol=node,native_symbol=actual['symbol']['address'],consumed_offset=offset,original_raw=raw.hex(),native_raw=new.hex(),scope='Actual qsort array allocation/call; comparator consumes PublicSymbol offset DWORD12 only, no wider node/heap equality claimed')

MAPPED_HEADER_CALL_SHA = '2489041f891d6f17d63d04bc3f1a51b92144762b81e510893800767e347fe7fe'
MAPPED_SUM_CALL_SHA = 'f5bec23e49b7c82f2cef2282daeca681f3872289a2126ead84cee7d5f7216a84'
MAPPED_CHECKSUM_CALL_SHA = '3c19940d38e1d8221942b4f8665c685f0882bc982dc98bfa56199a231c55f32e'

def compare_global_owner(item,words,actual,events):
    if item['root']=='output_buffer':
        assert (item['original_root'],item['size_global'])==(0x335c0,'output_capacity'),'unknown output allocation root'
        scalar=lambda address:int.from_bytes(bytes.fromhex(re.findall(r'^memory %08x ([0-9a-f]+)$'%address,events,re.M)[0]),'little')
        root,capacity,used=scalar(0x335c0),scalar(0x335c4),scalar(0x335c8)
        assert root==words[item['slot']]!=0 and 0<used<=capacity<=65512 and words[item['length_slot']]==used,'output argument/count outside live allocation'
        owner=actual['owner'];assert owner==dict(root='output_buffer',address=actual['address'],requested_size=capacity)and not actual['null'],'native output owner/capacity differs'
        return dict(original_address=root,native_address=actual['address'],owner='output_buffer',requested_size=capacity,defined_prefix=used,scope='initialize_output live allocation; exact defined payload compared by output_buffer walk, no capacity-tail reads')
    if item['root']=='derived_types':
        assert(item['original_root'],item['requested_size'])==(0x3392c,1536)and words[0]==0x1c574,'unreviewed derived table lifetime/caller'
        def scalar(address,size):
            rows=re.findall(r'^memory %08x ([0-9a-f]+)$'%address,events,re.M);assert len(rows)==1 and len(bytes.fromhex(rows[0]))==size
            return int.from_bytes(bytes.fromhex(rows[0]),'little')
        root=scalar(0x3392c,4);capacity=scalar(0x33928,2);count=scalar(0x3718c,2)
        assert root==words[item['slot']]!=0 and capacity==256 and count==128 and words[2]==3072,'first derived table growth differs'
        assert actual['owner']==dict(root='derived_types',address=actual['address'],requested_size=1536,context=dict(capacity=capacity,count=count,stride=12))and not actual['null'],'native derived table owner differs'
        return dict(original_address=root,native_address=actual['address'],owner='derived_types',requested_size=1536,new_requested_size=3072,capacity=capacity,count=count,scope='Actual first128-to256 derived table growth. initialize_type_tables checked_zero(128*12) publishes this root; no prior checked_realloc can return while its entry is masked. add_derived_type updates capacity before this exact call. Wrapper consumes ownership and size, not nested list payloads; no general heap proof.')
    if item['root']=='module_data':
        assert item['original_root']==0x33058 and item['size_global']=='module_capacity','unknown line buffer owner'
        def scalar(address):
            rows=re.findall(r'^memory %08x ([0-9a-f]+)$'%address,events,re.M);assert len(rows)==1 and len(bytes.fromhex(rows[0]))==4
            return int.from_bytes(bytes.fromhex(rows[0]),'little')
        root,capacity=scalar(0x33058),scalar(0x33080)
        assert words[0] in (0x17b4c,0x17b1c) and root==words[item['slot']]!=0 and 0<capacity<=1024,'line buffer/caller/capacity differs'
        entries=re.findall(r'^input-buffer %08x ([0-9a-f]+)$'%words[2],events,re.M);assert len(entries)==1
        entry=bytes.fromhex(entries[0]);assert len(entry)==12
        size=int.from_bytes(entry[8:12],'little');offset=int.from_bytes(entry[4:8],'little')
        assert 0<size<=capacity and actual['owner']==dict(root='module_data',address=actual['address'],requested_size=capacity)and not actual['null'],'native line buffer owner/extent differs'
        rows=re.findall(r'^input-buffer %08x ([0-9a-f]+)$'%root,events,re.M);assert len(rows)==1
        payload=bytes.fromhex(rows[0]);assert len(payload)==size and rows[0]==actual['bytes'],'line buffer defined input differs'
        return dict(original_address=root,native_address=actual['address'],owner='module_data',requested_size=capacity,defined_prefix=size,source_offset=offset,sha256=__import__('hashlib').sha256(payload).hexdigest(),scope='read_image_debug malloc(module_capacity) publication; read_source_module read_file defines exactly directory.size, consumed bytes independently equal; no allocation tail credit')
    if item['root']=='current_module':
        assert(item['original_root'],item['requested_size'],item['input_bytes'])==(0x33050,48,48),'unknown current module contract'
        roots=re.findall(r'^memory 00033050 ([0-9a-f]+)$',events,re.M);raws=re.findall(r'^input-buffer %08x ([0-9a-f]+)$'%words[item['slot']],events,re.M)
        assert len(roots)==len(raws)==1 and int.from_bytes(bytes.fromhex(roots[0]),'little')==words[item['slot']]!=0,'actual module argument is not current_module'
        assert actual['owner']==dict(root='current_module',address=actual['address'],requested_size=48)and not actual['null'],'native current module owner differs'
        old,new=bytes.fromhex(raws[0]),bytes.fromhex(actual['bytes']);assert len(old)==len(new)==48 and old[:4]==new[:4]and old[8:12]==new[8:12],'defined consumed module/signature fields differ'
        return dict(original_address=words[item['slot']],native_address=actual['address'],owner='current_module',requested_size=48,original_raw=old.hex(),native_raw=new.hex(),compared_ranges=[[0,4],[8,12]],scope='Actual pack_module current_module publication and48B find_module object; module/signature consumed by save_precompiled. Raw unconsumed pointer/list/map fields retained, no whole-module-list equality claim.')
    assert (item['root'],item['original_root'],item['requested_size'])==('public_symbol_hash',0x3717c,0x4000),'unreviewed published table allocation'
    matches=re.findall(r'^memory %08x ([0-9a-f]+)$'%item['original_root'],events,re.M)
    assert len(matches)==1 and len(bytes.fromhex(matches[0]))==4,'missing original table root'
    root=int.from_bytes(bytes.fromhex(matches[0]),'little');owner=actual['owner']
    assert root==words[item['slot']]!=0,'original argument differs from published table owner'
    assert owner['root']==item['root']and owner['requested_size']==item['requested_size']and owner['address']==actual['address']!=0 and not actual['null'],'native argument differs from published table owner'
    return dict(original_address=root,native_address=actual['address'],owner=item['root'],requested_size=item['requested_size'],scope='Published checked_zero(0x4000) table identity and requested extent; no bucket/node heap equality claimed at this entry')

def compare_directory_key(item,words,actual,events):
    def scalar(address):
        matches=re.findall(r'^memory %08x ([0-9a-f]+)$'%address,events,re.M)
        assert len(matches)==1 and len(bytes.fromhex(matches[0]))==4,'missing original directory array root/count'
        return int.from_bytes(bytes.fromhex(matches[0]),'little')
    base=scalar(0x33018);count=scalar(0x3329c);pointer=words[item['slot']];array=actual['array']
    output=item['kind']=='directory-output';capacity=count
    if output:
        headers=re.findall(r'^memory 00037880 ([0-9a-f]+)$',events,re.M);assert len(headers)==1 and len(bytes.fromhex(headers[0]))==16,'missing original directory allocation header'
        capacity=int.from_bytes(bytes.fromhex(headers[0])[4:8],'little')+6
        assert array['capacity']==capacity and array['index']==count,'directory output is not the next allocated entry'
    assert base<=pointer<base+capacity*12 and not(pointer-base)%12,'original argument outside directory allocation'
    assert array['count']==count and array['stride']==12 and array['index']==(pointer-base)//12,'directory array/key index differs'
    assert actual['address']==array['base']+array['index']*12 and not actual['null'],'native sort key pointer differs'
    matches=re.findall(r'^%s-buffer %08x ([0-9a-f]+)$'%('output'if output else 'input',pointer),events,re.M)
    assert len(matches)==1,'missing actual directory sort key'
    old=bytes.fromhex(matches[0]);new=bytes.fromhex(actual['bytes']);assert len(old)==len(new)==12 and(output or old[:4]==new[:4]),'directory sort fields differ'
    if item.get('whole_entry'):assert old==new,'consumed directory fields differ'
    return dict(original_address=pointer,native_address=actual['address'],index=array['index'],count=count,compared_fields=[]if output else [0,4],original_raw=old.hex(),native_raw=new.hex(),scope='Next-cell output identity only; raw prebytes no equality credit'if output else 'Comparator consumes subsection/module WORDs only; payload pointer/size retained raw without equality claim')

def compare_counted_name(item,words,actual,events):
    assert (item['owner'],item['local'])==('build_source_file_index','module'),'unknown counted-name owner'
    contexts={0x1d541:11,0x1d39c:15,0x1d55c:12}
    assert contexts.get(item['caller'])==item['original_owner_slot'],'unknown counted-name context'
    if item['caller']==0x1d39c:assert item['outer_return_slot']==4 and item['outer_return']==words[4]==0x1d541,'unknown counted-name nested frame'
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
    assert captured['helper_status']==0,'original controller did not finish normally'
    events=(original/'events').read_text();traps=re.findall(r'^exception ([0-9a-f]+) va ([0-9a-f]+) ',events,re.M)
    apps={r['original']:r for r in config['functions']};frontier=None;complete=current['terminal']=='exit'
    if complete:
        assert isinstance(current['exit'],int) and 0<=current['exit']<=255 and current['exit']==record['complete_reference']['exit'],'terminal status is not the cached original oracle'
        assert receipt['terminal']=='exit'and receipt['status']==current['exit'],'native completion status differs'
        assert not traps and re.findall(r'^exit (\d+)$',events,re.M)==[str(current['exit'])],'original terminal status differs'
    else:
        assert receipt['terminal']=='trap'and receipt['status']==-5,'native genuine trap missing'
        assert len(traps)==1 and traps[0][0]=='80000003'and 'exit 99\n'in events,'unknown original terminal'
        frontier=apps[int(traps[0][1],16)];assert frontier['status']=='stub'and frontier['native']==current['native_function'],'original next entry is not cached next stub'
    allocator=record.get('native_host_allocator')
    if allocator:
        profile=BINARY/allocator['profile'];assert sha(profile)==allocator['sha256'],'stale host allocator profile'
        expected_profile=json.loads(profile.read_text());assert receipt.get('native_host_allocator')==dict(profile_sha256=allocator['sha256'],launch=expected_profile),'native host allocator receipt differs'
        assert sha(Path(expected_profile['libc']['path']))==expected_profile['libc']['sha256'],'native host libc changed'
        assert json.loads((work/'host-environment.json').read_text())==dict(host_controls_absent=True,unbound_CRT_uses_host_vector=True),'host allocator controls visible to target'
    state=json.loads((work/'gdb.json').read_text());assert state['later_heap_equality']is False
    if complete:
        assert state['signal']==0 and state['exit']==current['exit'] and state['frames']==[]and state['parameters']=={}and state['globals']=={}and state['walks']=={},'native terminal debugger observation differs'
    else:assert state['signal']==5 and state['frames'][0]==frontier['native'],'native next named trap differs'
    words={int(a):int(b,16)for a,b in re.findall(r'^stack (\d+) ([0-9a-f]+)$',events,re.M)};parameters={}
    registers=re.search(r'^registers (.*)$',events,re.M)
    if not complete:
        assert registers,'missing original ABI register state'
        words.update({k:int(v,16)for k,v in zip(registers[1].split()[::2],registers[1].split()[1::2])})
    assert set(state['parameters'])=={r['name']for r in frontier['parameters']}if frontier else not state['parameters'],'new frontier ABI observation incomplete'
    recursive_input=None
    if frontier and frontier['name']=='compare_recursive_type':
        from recursive_input import compare as compare_recursive_input
        recursive_input=compare_recursive_input(events,words,state['parameters'])
    for item in frontier['parameters']if frontier else []:
        if item['kind']=='integer':parameters[item['name']]=compare_integer(item,frontier['parameters'],words,state['parameters'])
        elif item['kind']=='module-symbol-prefix':
            from recursive_input import compare_symbol_prefix
            parameters[item['name']]=compare_symbol_prefix(events,words,state['parameters'][item['name']])
        elif item['kind']=='module-type-record':
            from recursive_input import compare_precompiled
            parameters[item['name']]=compare_precompiled(events,words,state['parameters'][item['name']])
        elif item['kind']=='module-type-slice':
            from recursive_input import compare_numeric
            parameters[item['name']]=compare_numeric(events,words,state['parameters'][item['name']])
        elif item['kind']in('local-type-record','published-type-record'):
            assert recursive_input is not None,'unproved recursive comparison input'
            parameters[item['name']]=dict(typed_owner=recursive_input,role=item['kind'])
        elif item['kind']=='symbol-sort-key':parameters[item['name']]=compare_symbol_sort_key(item,words,state['parameters'][item['name']],events)
        elif item['kind']=='function':
            callback=next(r for r in config['functions']if r['name']==item['target']);actual=state['parameters'][item['name']]
            assert words[item['slot']]==item['original_target']==callback['original']and actual['function']==callback['native']and not actual['null'],'callback ABI identity differs'
            parameters[item['name']]=dict(original_address=words[item['slot']],native_address=actual['address'],function=callback['name'])
        elif item['kind']=='mapped-image':
            pointer=words[item['slot']];actual=state['parameters'][item['name']];size=words[item['length_slot']]*item.get('size_scale',1)
            assert 0<size<=524288,'mapped image extent differs'
            assert __import__('hashlib').sha256(pe.read(0x2bf79,0x2bfce-0x2bf79)).hexdigest()==MAPPED_CHECKSUM_CALL_SHA,'mapped image original return/carrier proof changed'
            if words[0]==0x2bfce:assert pointer==words['ebx']and item['slot']==1 and item['length_slot']==2,'mapped image direct carrier differs'
            elif words[0]==0x2be88:
                assert pointer==words['ebp']and words[8]==0x2bfce and words[9]==pointer and words[10]==size and item['slot']==2 and item['length_slot']==3 and item['size_scale']==2,'mapped checksum loop caller/owner/count differs'
                assert __import__('hashlib').sha256(pe.read(0x2be60,0x28)).hexdigest()==MAPPED_SUM_CALL_SHA,'mapped checksum loop def-use proof changed'
            else:
                assert words[0]==0x2be94 and pointer==words['ebp']and words[6]==0x2bfce and words[7]==pointer and item['slot']==1 and item['length_slot']==8,'mapped PE-header caller/owner differs'
                assert __import__('hashlib').sha256(pe.read(0x2be60,0x34)).hexdigest()==MAPPED_HEADER_CALL_SHA,'mapped header lookup def-use proof changed'
            assert actual['mapping']==dict(size=size,owner='update_image_checksum')and not actual['null'],'native mapped view ownership differs'
            rows=re.findall(r'^input-buffer %08x ([0-9a-f]+)$'%pointer,events,re.M);assert len(rows)==1,'missing actual mapped input'
            payload=bytes.fromhex(rows[0]);assert len(payload)==size and rows[0]==actual['bytes'],'full mapped image input differs'
            assert payload==(original/'work/hello-debug.dll').read_bytes(),'mapped bytes differ from whole current file'
            parameters[item['name']]=dict(original_address=pointer,native_address=actual['address'],size=size,sha256=__import__('hashlib').sha256(payload).hexdigest(),scope='Actual MapViewOfFile return carried in EBX at reviewed caller, registered native full view; every mapped input byte compared')
        elif item['kind']=='file-slot':
            pointer=words[item['slot']];actual=state['parameters'][item['name']]
            rows=re.findall(r'^file-slot-owner %08x (\d+) ([0-9a-f]+) (\d+)$'%pointer,events,re.M);assert len(rows)==1,'missing actual published FileSlot owner'
            handle,table,limit=rows[0];handle,limit=int(handle),int(limit);assert actual['owner']==dict(handle=handle,limit=limit)and 0<=handle<limit<=64,'FileSlot table index/count differs'
            raw=re.findall(r'^input-buffer %08x ([0-9a-f]+)$'%pointer,events,re.M);assert len(raw)==1,'missing FileSlot raw input'
            old,new=bytes.fromhex(raw[0]),bytes.fromhex(actual['bytes']);assert len(old)==len(new)==40 and int.from_bytes(old[8:12],'little')==handle and not actual['null'],'FileSlot handle/extent differs'
            source_proof=None
            if item.get('source_win32_handle'):
                from mapped_slot import compare as compare_mapped_slot
                assert args.case=='embedded-cpp-mapped'and frontier['name']in('map_file','seek_mapped_file','read_mapped_file','write_mapped_file','truncate_mapped_file','close_mapped_file'),'unknown active HANDLE consumer'
                source_proof=compare_mapped_slot(pointer,old,new,actual,events,json.loads((work/'mapped-open.json').read_text()),item.get('mapped_seek',False),bool(item.get('mapped_write')or item.get('mapped_cursor')),item.get('mapped_close',False))
                if item.get('mapped_seek'):
                    from mapped_slot import compare_creation
                    source_proof['mapping_creation']=compare_creation(pointer,old,new,actual,events,json.loads((work/'mapping-returns.json').read_text()))
                if item.get('mapped_write'):
                    from mapped_slot import compare_destination
                    source_proof['destination']=compare_destination(pointer,old,actual,events,words)
                if item.get('mapped_read'):
                    from mapped_slot import compare_view_input
                    source_proof['consumed_view_input']=compare_view_input(pointer,old,actual,events,words,original/'work/embedded-cpp.dll')
            else:assert old[4:20]==new[4:20]and old[28:32]==new[28:32]and not(old[4]&8),'FileSlot buffered scalar fields differ'
            parameters[item['name']]=dict(original_address=pointer,native_address=actual['address'],handle=handle,table_limit=limit,original_raw=old.hex(),native_raw=new.hex(),source_HANDLE=source_proof,compared_ranges=[[4,16],[28,36]]if source_proof else [[4,20],[28,32]],scope='Published file_slots[handle] identity and buffered scalar inputs; pointer raw retained, no all-page/heap equality claim')
        elif item['kind']=='global-owner':parameters[item['name']]=compare_global_owner(item,words,state['parameters'][item['name']],events)
        elif item['kind']in('directory-key','directory-output'):parameters[item['name']]=compare_directory_key(item,words,state['parameters'][item['name']],events)
        elif item['kind']=='counted-string':parameters[item['name']]=compare_counted_name(item,words,state['parameters'][item['name']],events)
        elif item['kind']=='pointer'and item.get('null_only'):parameters[item['name']]=compare_null_pointer(words[item.get('register',item.get('slot'))],state['parameters'][item['name']])
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
    if frontier:entries.append(frontier['native'])
    assert function['native']in entries and entries==(work/'trace').read_text().splitlines()==current['application_trace'],'complete entry trace differs'
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
    from freebuf import validate as validate_freebuf,compare as compare_freebuf
    freebuf_policy=validate_freebuf(config,pe);freebuf_proof=None
    if freebuf_policy and args.case==freebuf_policy['case']:
        freebuf_proof=compare_freebuf(events,json.loads((work/'freebuf-returns.json').read_text()),freebuf_policy)
    for stream in('stdout','stderr'):assert(original/stream).read_bytes()==(work/stream).read_bytes(),'streams differ'
    assert {p.name for p in(original/'work').iterdir()}=={p.name for p in(work/'work').iterdir()},'artifact inventory differs'
    for path in(original/'work').iterdir():assert path.read_bytes()==(work/'work'/path.name).read_bytes(),'full artifact differs'
    if complete:
        for artifact in current['artifacts']:
            path=work/'work'/artifact['name']
            assert path.stat().st_size==artifact['bytes']and sha(path)==artifact['sha256'],'terminal artifact differs from immutable master'
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
    sequence=len(config['accepted_order'])+1;directory=BINARY/'cases'/args.case/'evidence'/('%04d-%s'%(sequence,args.function));directory.mkdir(parents=True,exist_ok=False)
    for label,source,names in(('original',original,('events','receipt.json','configuration.json','runtime-support.json','runtime-plan','stdout','stderr')),('native',work,('gdb.json','gdb.stdout','gdb.stderr','commands.gdb','trace','configuration.json','runtime-support.json','crt-returns.json','mapped-open.json','mapping-returns.json','freebuf-returns.json','host-environment.json','stdout','stderr'))):
        for name in names:
            if(source/name).exists():shutil.copyfile(source/name,directory/(label+'-'+name))
    if args.original_rebind:shutil.copytree(args.original_rebind.parent,directory/'native-only-rebind')
    shutil.copyfile(native/'receipt.json',directory/'native-receipt.json');path=str((directory/'proof.json').relative_to(BINARY));prior={(r['start'],r['end'])for r in config.get('support_unmasked',[])}
    for r in support:
        if(r['start'],r['end'])not in prior:config.setdefault('support_unmasked',[]).append(dict(r,evidence=path))
    proof=dict(function=function['name'],sequence=sequence,case=args.case,extent=function['extent'],original_frontier=frontier['original']if frontier else None,native_frontier=frontier['native']if frontier else 'exit',application_entries=entries,matched_frontier_parameters=state['parameters'],typed_frontier_parameters=parameters,matched_application_globals=matched,observed_objects=owners,conditional_CRT_support=crt,freebuf_noop_boundary=freebuf_proof,runtime_unmasked=support,seconds=receipt['seconds'],classification='routine-whole-function-and-exact-terminal-output'if complete else 'routine-whole-function-and-genuine-next-entry',later_module_heap_equality=False,prior_module3='cases/hello-debug-dll/evidence/0097-create_symbol_reference/proof.json',scope='Root streamlined normal loop; actual formal/selected consumed globals checked, old full module3 contracts retained; additional state probes only for concrete ambiguities/mismatches')
    (directory/'proof.json').write_text(json.dumps(proof,indent=2)+'\n');function.update(status='accepted',evidence=path);config['accepted_order'].append(dict(function=function['name'],case=args.case,kind='application',sequence=sequence));(BINARY/'functions.json').write_text(json.dumps(config,indent=2)+'\n');print('Accepted',sequence,function['name'],'next',frontier['native']if frontier else 'case complete')

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
