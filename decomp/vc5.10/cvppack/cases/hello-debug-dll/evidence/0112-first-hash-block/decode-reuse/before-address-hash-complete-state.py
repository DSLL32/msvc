"""Fresh first-callback history, complete address sorts and current output owners."""
import hashlib
import json
from pathlib import Path
import re
import struct


def prove(config):
    from binary import PE
    from state import ROOT,BINARY,sha
    from capstone import Cs,CS_ARCH_X86,CS_MODE_32
    from capstone.x86 import X86_OP_MEM
    binding=config['address_hash_complete_policy'];path=BINARY/binding['path'];assert sha(path)==binding['sha256'],'changed complete address hash policy'
    policy=json.loads(path.read_text());assert policy['target_sha256']==config['target']['sha256'];pe=PE(ROOT/config['target']['path']);decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True
    extents=[(0x40edc0,0x40edd7,set()),(0x40ede0,0x40f0c6,{0x4161f4,0x4161ac}),(0x40d68f,0x40d7d9,{0x412d34,0x412d38,0x412d40,0x412cf4})]
    if config.get('observe_first_hash_output_block'):extents += [(0x4100c0,0x4100c8,set()),(0x40d7d9,0x40d84c,{0x412d34,0x412d38,0x412d40})]
    assert len(policy['integer_intervals'])==len(extents)
    for row,(start,end,allowed) in zip(policy['integer_intervals'],extents):
        assert row['extent']==[start,end];body=pe.read(start,end-start);assert hashlib.sha256(body).hexdigest()==row['sha256'];code=list(decoder.disasm(body,start));assert sum(i.size for i in code)==len(body)
        assert not any(i.mnemonic.startswith(('f','v','xsave','xrstor')) for i in code),'complete address hash raw FP consumer'
        assert all(o.mem.disp in allowed for i in code for o in i.operands if o.type==X86_OP_MEM and not o.mem.base and not o.mem.index),'complete address hash old root consumer'
    return dict(policy_sha256=sha(path),integer_intervals=policy['integer_intervals'],validator_sha256=sha(Path(__file__)),actual_final_frontier=0x4100d0 if config.get('observe_first_hash_output_block') else 0x4100c0,scope='finite complete three sorts, address serialization, four frees and public-buffer continuation to real names-block stub; no future consumers')


def decode(events):
    result=dict(sorts=[],callbacks=[],owned={},stream_calls=[],frees=[],initialization={},allocations=[])
    for line in events.splitlines():
        if ' eax ' in line:continue
        fields=line.split()
        if line.startswith('first-block-'):
            row=result.setdefault('first_block',{})
            if fields[0]=='first-block-entry':
                assert 'entry_esp' not in row;_,esp,ret,arg,*regs=fields;row.update(entry_esp=int(esp,16),return_pc=int(ret,16),argument=int(arg,16),before_nonvolatile=[int(v,16)for v in regs])
            elif fields[0]=='first-block-return':
                assert 'result' not in row;_,esp,value,*regs=fields;row.update(exit_esp=int(esp,16),result=int(value,16),after_nonvolatile=[int(v,16)for v in regs])
            elif fields[0]=='first-block-caller-return':
                assert 'caller_esp' not in row;row.update(caller_esp=int(fields[1],16),caller_result=int(fields[2],16))
            elif fields[0]=='first-block-copy-complete':
                assert 'copy_esp' not in row;row.update(copy_esp=int(fields[1],16),copy_block=int(fields[2],16),copy_source_end=int(fields[3],16),copy_destination_end=int(fields[4],16))
            else:
                key={'first-block-input':'before_arena','first-block-after-input':'after_arena','first-block-result':'block'}[fields[0]];assert key not in row and len(bytes.fromhex(fields[3]))==int(fields[2]);row[key]=bytes.fromhex(fields[3]);row[key+'_address']=int(fields[1],16)
        elif line.startswith('address-sort-call '):
            _,index,esp,root,count,width,callback,provider=fields;assert int(index)==len(result['sorts'])
            result['sorts'].append(dict(index=int(index),entry_esp=int(esp,16),args=[int(root,16),int(count),int(width),int(callback,16)],provider=int(provider,16),callback_begin=len(result['callbacks'])))
        elif line.startswith('address-sort-return '):
            _,index,esp,count=fields;r=result['sorts'][-1];assert int(index)==r['index'] and int(count)==len(result['callbacks']) and 'exit_esp' not in r;r.update(exit_esp=int(esp,16),callback_end=int(count))
        elif line.startswith('addresshash-owned '):
            _,phase,address,size,payload=fields
            if phase.startswith('sort'):
                assert phase in {f'sort{i}-{p}' for i in range(3) for p in ('before','after')};table=result['owned'].setdefault(phase,{});address=int(address,16);assert address not in table and len(bytes.fromhex(payload))==int(size);table[address]=bytes.fromhex(payload)
        elif line.startswith('address-callback-call '):
            _,sort,sequence,esp,ret,first,second,payload=fields;assert int(sequence)==len(result['callbacks']) and (not result['callbacks'] or 'result' in result['callbacks'][-1])
            result['callbacks'].append(dict(sort=int(sort),sequence=int(sequence),entry_esp=int(esp,16),return_pc=int(ret,16),first=int(first,16),second=int(second,16),records=bytes.fromhex(payload)))
        elif line.startswith('address-callback-return '):
            _,sort,sequence,esp,ret,value=fields;r=result['callbacks'][-1];assert int(sequence)==r['sequence'] and int(sort)==r['sort'] and 'result' not in r;r.update(exit_esp=int(esp,16),return_pc_after=int(ret,16),result=int(value))
        elif line.startswith(('address-sort-errors ','address-sort-registers ','address-sort-fp ')):
            kind,phase=fields[:2];assert phase in ('before','after','entry','return','frontier')
            if phase=='frontier':target=result.setdefault('frontier_environment',{})
            elif phase in ('before','after'):target=result['sorts'][-1].setdefault(phase,{})
            else:target=result['callbacks'][-1].setdefault('before' if phase=='entry' else 'after',{})
            if kind=='address-sort-errors':
                assert 'raw_errors' not in target;_,_,teb,tid,index,slot,ptd,payload=fields;target.update(teb=int(teb,16),tid=int(tid),tls_index=int(index),tls_slot=int(slot,16),ptd=int(ptd,16),raw_errors=bytes.fromhex(payload))
            elif kind=='address-sort-fp':assert 'fp' not in target;target['fp']=bytes.fromhex(fields[2])
            else:assert 'nonvolatile' not in target;target['nonvolatile']=[int(v,16) for v in fields[2:]]
        elif line.startswith('address-arena-init-call '):
            assert not result['initialization'];_,esp,arena,capacity,*registers=fields;result['initialization']=dict(entry_esp=int(esp,16),arena=int(arena,16),capacity=int(capacity),before_nonvolatile=[int(v,16) for v in registers])
        elif line.startswith('address-arena-init-return '):
            _,esp,*registers=fields;result['initialization'].update(exit_esp=int(esp,16),after_nonvolatile=[int(v,16) for v in registers])
        elif line.startswith('address-arena-initialized '):
            _,address,size,payload=fields;assert int(size)==24 and int(address,16)==result['initialization']['arena'];result['initialization']['after_arena']=bytes.fromhex(payload)
        elif line.startswith('namehash-allocation-call ') and result['initialization']:
            _,site,size,ended,source,esp,*registers=fields
            result['allocations'].append(dict(site=int(site,16),size=int(size),after_frees=int(ended),source_frees=int(source),entry_esp=int(esp,16),before_nonvolatile=[int(v,16) for v in registers]))
        elif line.startswith('namehash-allocation-return ') and result['initialization']:
            _,site,address,esp,*registers=fields;r=result['allocations'][-1];assert 'address' not in r;r.update(return_site=int(site,16),address=int(address,16),exit_esp=int(esp,16),after_nonvolatile=[int(v,16) for v in registers])
        elif line.startswith('address-stream-call '):
            _,kind,site,esp,arena,value,size,*registers=fields;result['stream_calls'].append(dict(kind=int(kind),site=int(site,16),entry_esp=int(esp,16),arena=int(arena,16),value=int(value,16),size=int(size),before_nonvolatile=[int(v,16) for v in registers]))
        elif line.startswith(('address-stream-before ','address-stream-after ','address-stream-input ','address-stream-output ')):
            label,address,size,payload=fields;r=result['stream_calls'][-1];key={'address-stream-before':'before_arena','address-stream-after':'after_arena','address-stream-input':'input','address-stream-output':'output'}[label];assert key not in r;payload=bytes.fromhex(payload);assert len(payload)==int(size);r[key]=payload
            if key=='output':r['output_address']=int(address,16)
            else:assert int(address,16)==(r['value'] if key=='input' else r['arena'])
        elif line.startswith('address-stream-return '):
            _,site,esp,value,*registers=fields;r=result['stream_calls'][-1];assert 'exit_esp' not in r;r.update(return_site=int(site,16),exit_esp=int(esp,16),result=int(value,16),after_nonvolatile=[int(v,16) for v in registers])
        elif line.startswith('address-temp-free-call '):
            _,sequence,site,esp,address,index,provider,*registers=fields;assert int(sequence)==len(result['frees']);result['frees'].append(dict(sequence=int(sequence),site=int(site,16),entry_esp=int(esp,16),address=int(address,16),index=int(index),provider=int(provider,16),before_nonvolatile=[int(v,16) for v in registers]))
        elif line.startswith('address-temp-before-free '):
            _,address,size,payload=fields;r=result['frees'][-1];assert int(address,16)==r['address'] and len(bytes.fromhex(payload))==int(size);r.update(size=int(size),before=bytes.fromhex(payload))
        elif line.startswith('address-temp-free-return '):
            _,site,esp,*registers=fields;r=result['frees'][-1];assert 'exit_esp' not in r;r.update(return_site=int(site,16),exit_esp=int(esp,16),after_nonvolatile=[int(v,16) for v in registers])
        elif line.startswith(('address-complete-arena ','address-complete-block ','address-complete-output ')):
            label,address,size,payload=fields;key={'address-complete-arena':'arena','address-complete-block':'block','address-complete-output':'payload'}[label];r=result.setdefault('current',{});assert key not in r and len(bytes.fromhex(payload))==int(size);r[key]=bytes.fromhex(payload);r[key+'_address']=int(address,16)
        elif line=='address-complete-observation-failed':raise AssertionError('incomplete complete-address observation')
    return result


def native_symbol(name):
    from state import BINARY
    raw=(BINARY/'build/native/CVPACK').read_bytes()
    assert raw[:6]==b'\x7fELF\x01\x01','native checkpoint needs actual little-endian ELF32'
    header=struct.unpack_from('<16sHHIIIIIHHHHHH',raw);assert header[2]==3
    sections=[struct.unpack_from('<10I',raw,header[6]+i*header[11]) for i in range(header[12])]
    found=[]
    for section in sections:
        if section[1]!=2:continue
        strings=sections[section[6]];table=raw[strings[4]:strings[4]+strings[5]]
        for at in range(section[4],section[4]+section[5],section[9]):
            text,value,size,info,other,owner=struct.unpack_from('<IIIBBH',raw,at)
            if table[text:table.find(b'\0',text)].decode()!=name:continue
            assert owner<len(sections) and size;body=sections[owner];offset=body[4]+value-body[3]
            found.append((value,raw[offset:offset+size]))
    assert len(found)==1,'native checkpoint ELF symbol identity differs'
    return found[0]


def checkpoint(observation,state):
    value,body=native_symbol('compare_symbol_address')
    delta=observation['typed_pc']-observation['entry_pc']
    # Exact emitted frame/GOT setup ends BEFORE filtered tracing and any
    # comparator pointee read. Only the callee's stack is written. CALL+POP is
    # the local PC thunk, with no external call or floating-point effect.
    assert delta==28,'native comparator frame/GOT prologue differs'
    assert body[:15]==bytes.fromhex('5589e55383ec24e8000000005b81c3') and body[19:28]==bytes.fromhex('895dec8b450c8b4508'),'native comparator checkpoint includes semantic instructions'
    assert observation['typed_esp']==observation['entry_esp']-44,'native comparator checkpoint stack delta differs'
    return dict(native_symbol_value=value,typed_checkpoint_delta=delta,code_sha256=hashlib.sha256(body[:delta]).hexdigest(),native_elf_sha256=hashlib.sha256((Path(__file__).resolve().parents[1]/'build/native/CVPACK').read_bytes()).hexdigest(),scope='frame/GOT setup and formal-slot reads only; before instrumentation/pointee reads; historical typed checkpoint')


def first_phase(events,native):
    lines=events.splitlines();begin=[i for i,l in enumerate(lines) if l=='addresshash-phase-entry 0040edc0'];end=[i for i,l in enumerate(lines) if l=='addresshash-phase-end 0040edc0']
    assert len(begin)==len(end)==1 and 0<begin[0]<end[0] and lines[begin[0]-1]=='entry 0040edc0','addresshash historical phase lacks actual candidate-entry bounds'
    assert not any(l.startswith('frontier ') for l in lines[:end[0]+1]),'addresshash phase follows real trap'
    # This is only a historical decoder projection of the actual observed
    # candidate ENTRY. It never changes the complete raw's real final frontier.
    phase=lines[:end[0]+1];phase[begin[0]-1]='frontier 0040edc0'
    observation=native['address_hash_first_callback_observation'];state=native['address_hash_first_callback']
    assert observation['phase']=='physical entry ABI plus typed post-prologue checkpoint before comparator expression; historical only' and observation['entry_pc']==state['function_addresses']['compare_symbol_address'] and observation['typed_pc']==state['pc'] and observation['typed_esp']==state['entry_esp'] and observation['entry_stack'][1:]==[state['address_hash_sort']['args'][0]+8,state['address_hash_sort']['args'][0]],'addresshash native actual phase entry differs'
    assert state['function']=='compare_symbol_address' and state['frontier_caller']=='recon_qsort','addresshash phase owner differs'
    prologue=checkpoint(observation,state)
    return '\n'.join(phase)+'\n',state,dict(prologue=prologue,original_entry_line=begin[0]-1,original_end_line=end[0],original_phase_sha256=hashlib.sha256(('\n'.join(lines[:end[0]+1])+'\n').encode()).hexdigest(),native_physical_entry_pc=observation['entry_pc'],native_typed_checkpoint_pc=observation['typed_pc'],kind='fresh actual candidate-entry historical phase; not a trap')


def mapping_writes(data,original,native,historical,previous,old,new):
    proof=[]
    for side,state,before,complete in [('original',original,historical,old),('native',native,previous,new)]:
        arenas=before['address_hash_allocations']
        destinations=[dict(role='temporary-qsort-owner'+str(i),address=row['address'],capacity=row['size']) for i,row in enumerate(arenas)]
        destinations += [dict(role='address-output-owner'+str(i),address=row['address'],capacity=row['size']) for i,row in enumerate(complete['allocations'])]
        arena=before['address_hash_entry']['stack'][3] if side=='original' else previous['public_writer_frame']['stream_objects'][1]['address']
        output=state['output_buffer_allocations'][0]
        if side=='original':
            method,byte_count=before['address_hash_entry']['stack'][1:3]
            table=int.from_bytes(before['memory'][next(row['original'] for row in data if row['name']=='directory_entries')],'little')
        else:method=previous['public_writer_frame']['header_address']+2;byte_count=previous['public_writer_frame']['header_address']+12;table=previous['public_writer_frame']['entry']-58*12
        destinations += [dict(role='actual-address-arena',address=arena,capacity=24),dict(role='actual-WORD-method-output',address=method,capacity=2),dict(role='actual-DWORD-byte-output',address=byte_count,capacity=4),dict(role='actual-directory-row58',address=table+58*12,capacity=12),dict(role='actual-packed-buffer-capacity',address=output['address'],capacity=output['size'])]
        catalog=before['mapped_views'] if side=='original' else state['mapped_views']
        assert len(catalog)==1,'complete address mapped-input catalog inventory differs'
        for address,view in catalog.items():
            address=int(address);assert view['length']==2097152,'complete address mapped-input catalog extent differs'
            for write in destinations:assert max(address,write['address'])>=min(address+view['length'],write['address']+write['capacity']),'complete address actual write owner overlaps mapped input'
        assert state['mapping_calls']==before['mapping_calls'] and state['file_calls']==before['file_calls'],'complete address unexpected executed mapping/file API'
        proof.append(dict(side=side,destinations=destinations,catalog=[dict(address=int(address),**view) for address,view in catalog.items()],catalog_phase='fresh first-callback lifetime catalog; no final original snapshot' if side=='original' else 'fresh final native registry',actual_executed_sorts=3,actual_stream_calls=139,actual_temporary_frees=4,ordinary_CRT_scope='qsort bounded owned arrays, malloc/free heap-only public boundary; no library internals credit'))
    return proof


def validate(config,data,events,native,artifacts,views,compare,rawdecode):
    policy=prove(config)
    phase_events,previous,phase_proof=first_phase(events,native)
    prefix=compare(dict(config,observe_address_hash_complete=False),data,phase_events,previous,artifacts,views)
    lines=events.splitlines();a=next(i for i,l in enumerate(lines) if l=='addresshash-phase-entry 0040edc0');b=next(i for i,l in enumerate(lines) if l=='addresshash-phase-end 0040edc0');# The debugger emits each view-lifetime catalog row only ONCE, even
    # across typed snapshots. Retain those actual rows at their original point;
    # remove only snapshot state, not the live mapping registry/provenance.
    current_events='\n'.join(lines[:a]+[line for line in lines[a:b+1] if line.startswith('mapped-view ')]+lines[b+1:])+'\n'
    original=rawdecode(current_events);historical=rawdecode(phase_events);old=decode(events);new=native['address_hash_complete']
    phase_proof['original_view_artifacts']=[dict(address=address,file=view['file'],bytes=len(views['original'][view['file']]),sha256=hashlib.sha256(views['original'][view['file']]).hexdigest(),kind='actual first-phase cached view, not fresh final state') for address,view in historical['mapped_views'].items()]
    return validate_current(config,data,original,native,historical,previous,old,new,prefix,phase_proof,policy)


def validate_current(config,data,original,native,historical,previous,old,new,prefix,phase_proof,policy):
    names_copied=config.get('observe_first_hash_output_block',False)
    used=7752 if names_copied else 7188
    assert original['address']==(0x4100d0 if names_copied else 0x4100c0) and original['stack'][0]==(0x40d84c if names_copied else 0x40d7d9) and native['function']==('next_hash_output_block' if names_copied else 'first_hash_output_block') and native['frontier_caller']=='write_publics','complete address hash real next frontier differs'
    assert len(old['sorts'])==len(new['sorts'])==3 and old['callbacks'] and len(old['callbacks'])==len(new['callbacks'])<=4096,'complete address sort inventory differs'
    rows={row['name']:row for row in data}
    def root(state,name):return int.from_bytes(state['memory'][rows[name]['original']],'little')
    def actual(state,name):return int.from_bytes(bytes.fromhex(state['globals'][name]),'little')
    old_roots=[r['address'] for r in historical['address_hash_allocations']];new_roots=[r['address'] for r in previous['address_hash_allocations']]
    pointers={a['address']:b['address'] for a,b in zip(historical['public_padding']['nodes'],previous['public_padding']['nodes'])};pointers[historical['public_padding']['cursor']]=previous['public_padding']['cursor'];reverse={v:k for k,v in pointers.items()}
    def normalized(payload,index,native_side):
        payload=bytes.fromhex(payload) if isinstance(payload,str) else payload
        assert len(payload)==(36,216,232,64)[index],'complete address owned extent differs'
        if not native_side:return payload
        output=bytearray(payload)
        if index==0:
            for group in range(3):
                pointer=int.from_bytes(payload[group*12+8:group*12+12],'little');assert pointer==new_roots[group+1],'complete address group alias differs';struct.pack_into('<I',output,group*12+8,old_roots[group+1])
        else:
            for offset in range(0,len(payload),8):
                pointer=int.from_bytes(payload[offset:offset+4],'little');assert pointer in reverse,'complete address row owner differs';struct.pack_into('<I',output,offset,reverse[pointer])
        return bytes(output)
    def arrays(table,native_side):
        table={int(k):v for k,v in table.items()};roots=new_roots if native_side else old_roots
        assert set(table)==set(roots),'complete address exact owned-array inventory differs'
        return [normalized(table[address],i,native_side) for i,address in enumerate(roots)]
    def environment(row,baseline,native_side):
        assert len(row['nonvolatile'])==4 and row['fp']==baseline['fp'],'complete address own raw FP preservation differs'
        keys=('errno','doserrno','last_error','errno_address','doserrno_address','last_error_address') if native_side else ('raw_errors','teb','tid','tls_index','tls_slot','ptd')
        assert all(row[key]==baseline[key] for key in keys),'complete address own error identity/state differs'
    baseline=historical['sort_environments']['after'];native_baseline=previous['sort_environments']['after']
    previous_arrays=[historical['address_hash_owned']['before-sort'][address] for address in old_roots]
    callbacks_seen=0
    for index,(left,right) in enumerate(zip(old['sorts'],new['sorts'])):
        assert left['index']==right['index']==index and left['args']==[old_roots[index+1],(27,29,8)[index],8,0x40edc0] and right['args']==[new_roots[index+1],(27,29,8)[index],8,native['function_addresses']['compare_symbol_address']],'complete address actual sort arguments differ'
        assert left['provider']==original['module_bases']['msvcrt.dll']+0x2543d and right['owner']=='build_symbol_address_hash','complete address sort provider/owner differs'
        assert left['entry_esp']==left['exit_esp'] and right['exit_esp']==right['entry_esp']+4,'complete address qsort cdecl stack differs'
        assert left['callback_begin']==right['callback_begin']==callbacks_seen and left['callback_end']==right['callback_end']>callbacks_seen,'complete address callback group bounds differ'
        before=arrays(old['owned'][f'sort{index}-before'],False);other=arrays(right['before_owned'],True);assert before==other==previous_arrays,'complete address full before-sort arrays differ'
        after=arrays(old['owned'][f'sort{index}-after'],False);other_after=arrays(right['after_owned'],True);assert after==other_after,'complete address full post-sort arrays differ'
        for group in range(4):
            if group!=index+1:assert after[group]==before[group],'complete address sort changed another owner'
        original_rows=[before[index+1][at:at+8] for at in range(0,len(before[index+1]),8)];after_rows=[after[index+1][at:at+8] for at in range(0,len(after[index+1]),8)]
        assert sorted(original_rows)==sorted(after_rows),'complete address post-sort row inventory differs'
        offsets=[int.from_bytes(historical['memory'][int.from_bytes(row[:4],'little')][8:12],'little') for row in after_rows];assert offsets==sorted(offsets) and max(offsets)-min(offsets)<0x80000000,'complete address actual final offset ordering differs'
        for phase in ('before','after'):
            environment(left[phase],baseline,False);environment(right[phase],native_baseline,True)
        assert left['before']['nonvolatile']==left['after']['nonvolatile'] and right['before']['nonvolatile']==right['after']['nonvolatile'],'complete address sort normal-return registers differ'
        previous_arrays=after;callbacks_seen=left['callback_end']
    assert callbacks_seen==len(old['callbacks'])==len(new['callbacks']),'complete address callback continuation omitted'
    native_qsort_start,native_qsort_body=native_symbol('recon_qsort');symbol_start,_=native_symbol('compare_symbol_address');load_bias=native['function_addresses']['compare_symbol_address']-symbol_start
    from capstone import Cs,CS_ARCH_X86,CS_MODE_32
    instructions=list(Cs(CS_ARCH_X86,CS_MODE_32).disasm(native_qsort_body,native_qsort_start));native_returns={i.address+i.size for i in instructions if i.mnemonic=='call'}
    roles={}
    for sequence,(a,b) in enumerate(zip(old['callbacks'],new['callbacks'])):
        index=a['sort'];assert a['sequence']==b['sequence']==sequence and index==b['sort'] and 0<=index<3,'complete address callback sequence differs'
        assert old['sorts'][index]['callback_begin']<=sequence<old['sorts'][index]['callback_end'],'complete address callback group ownership differs'
        assert a['return_pc']-original['module_bases']['msvcrt.dll'] in (0x2551e,0x25534,0x2560c) and b['return_pc']-load_bias in native_returns,'complete address callback return instruction differs'
        role=a['return_pc']-original['module_bases']['msvcrt.dll'];native_role=b['return_pc']-load_bias
        if role in roles:assert roles[role]==native_role,'complete address callback return role changed'
        else:assert native_role not in roles.values(),'complete address callback roles collapsed';roles[role]=native_role
        assert a['entry_esp']==a['exit_esp'] and a['return_pc']==a['return_pc_after'] and b['exit_esp']==b['entry_esp']+4,'complete address callback cdecl stack/return differs'
        assert b['formal_inventory']==dict(first='const void *',second='const void *'),'complete address callback actual two-formal types differ'
        assert len(a['records'])==len(bytes.fromhex(b['records']))==16,'complete address complete callback inputs differ'
        inputs=[]
        for field,at in [('first',0),('second',8)]:
            delta=a[field]-old_roots[index+1];assert delta==b[field]-new_roots[index+1] and 0<=delta<(27,29,8)[index]*8 and delta%8==0,'complete address callback row-pointer alias differs'
            record=a['records'][at:at+8];other=bytes.fromhex(b['records'])[at:at+8];pointer,offset=struct.unpack('<II',record)
            assert pointer in pointers and other==struct.pack('<II',pointers[pointer],offset) and record in [historical['address_hash_owned']['before-sort'][old_roots[index+1]][i:i+8] for i in range(0,(27,29,8)[index]*8,8)],'complete address callback initialized row contents differ'
            inputs.append(int.from_bytes(historical['memory'][pointer][8:12],'little'))
        expected=((inputs[0]-inputs[1]+0x80000000)&0xffffffff)-0x80000000
        assert a['result']==b['result']==expected,'complete address comparator signed subtraction differs'
        for phase in ('before','after'):environment(a[phase],baseline,False);environment(b[phase],native_baseline,True)
        assert a['before']['nonvolatile']==a['after']['nonvolatile'] and b['before']['nonvolatile']==b['after']['nonvolatile'],'complete address callback normal-return registers differ'
    # The two output allocations are independent new generations, allocated
    # while all four temporary arrays are still live, before their four frees.
    assert len(old['allocations'])==len(new['allocations'])==2 and len(original['checked_allocations'])==len(native['checked_allocations'])==301 and len(original['cleared_allocations'])==len(native['cleared_allocations'])==115,'complete address output allocation inventory differs'
    output_roots=[];native_output_roots=[]
    for index,(a,b) in enumerate(zip(old['allocations'],new['allocations'])):
        site,size,kind,ordinal=(0x40ff9d,12,'cleared',114) if index==0 else (0x40ffaa,4096,'checked',300)
        assert a['site']==site and a['return_site']==site+5 and a['size']==b['size']==size and a['after_frees']==12 and a['source_frees']==2,'complete address actual output allocation site/size differs'
        assert b['sequence']==index and b['owner']=='copy_type_arena' and b['context']=='build_symbol_address_hash' and b['kind']==kind and b['formal_types']==dict(size='unsigned int'),'complete address output allocation owner/ABI differs'
        assert a['entry_esp']==a['exit_esp'] and b['exit_esp']==b['entry_esp']+4 and a['before_nonvolatile']==a['after_nonvolatile'] and b['before_nonvolatile']==b['after_nonvolatile'],'complete address output allocation return ABI differs'
        for state,row in [(original,a),(native,b)]:
            event=state[kind+'_allocations'][ordinal];assert event['address']==row['address'] and event['size']==size,'complete address output generic-event identity differs'
        output_roots.append(a['address']);native_output_roots.append(b['address'])
    for state,before in [(original,historical),(native,previous)]:
        for kind in ('image','checked','cleared','module','subsection'):
            prior=before[kind+'_allocations'];current=state[kind+'_allocations'];extra=1 if kind in ('checked','cleared') else 0
            assert len(current)==len(prior)+extra and current[:len(prior)]==prior,'complete address carried allocation-event prefix differs'
        for name in ('main_free_calls','source_family_frees','name_hash_temp_frees','file_calls','file_reads','mapping_calls','file_handles','address_hash_entry','address_hash_allocations'):
            assert state[name]==before[name],'complete address carried lifecycle/resource prefix differs: '+name
    # Original view bytes/catalog are emitted once at the first typed phase.
    # No mapping API or mapped-input store occurs in the pinned continuation;
    # keep that allocation/view lifetime provenance explicitly historical.
    # Native does freshly observe the same current registry/content hash.
    assert not original['mapped_views'] and native['mapped_views']==previous['mapped_views'],'complete address mapped-view lifetime catalog differs'
    assert original['name_hash_allocations']==historical['name_hash_allocations']+old['allocations'] and native['name_hash_allocations']==previous['name_hash_allocations'],'complete address legacy-labeled output allocation role differs'
    import importlib.util
    spec=importlib.util.spec_from_file_location('address_generation_proof',Path(__file__).with_name('address-hash-state.py'));generations=importlib.util.module_from_spec(spec);spec.loader.exec_module(generations);ended=generations.ended_generations(original,native)
    for side,state,roots in [('original',original,output_roots),('native',native,native_output_roots)]:
        assert roots[0] and roots[1] and max(roots)>=min(roots[0]+12,roots[1]+4096),'complete address output owners overlap'
        for fresh,size in zip(roots,(12,4096)):
            for kind in ('image','checked','cleared','module','subsection'):
                for ordinal,row in enumerate(state[kind+'_allocations']):
                    if (kind,ordinal) in ended or (kind,ordinal) in {('cleared',114),('checked',300)}:continue
                    assert max(fresh,row['address'])>=min(fresh+size,row['address']+row['size']),'complete address output overlaps an earlier live generation'
            specials=[(state['directory_allocation']['address'],912)]+[(r['address'],r['size'])for r in state['output_buffer_allocations']]+[(int(a),r['length'])for a,r in (historical['mapped_views'] if side=='original' else state['mapped_views']).items()]
            assert all(max(fresh,a)>=min(fresh+size,a+s) for a,s in specials),'complete address output overlaps protected capacity'
    original_arena=original['address_hash_entry']['stack'][3];native_arena=previous['public_writer_frame']['stream_objects'][1]['address'];empty=struct.pack('<6I',4096,0,0,0,0,0)
    a,b=old['initialization'],new['initialization'];assert a['arena']==original_arena and b['arena']==native_arena and a['capacity']==b['capacity']==4096 and a['after_arena']==bytes.fromhex(b['after_arena'])==empty,'complete address actual arena initialization differs'
    assert a['entry_esp']==a['exit_esp'] and b['exit_esp']==b['entry_esp']+4 and a['before_nonvolatile']==a['after_nonvolatile'] and b['before_nonvolatile']==b['after_nonvolatile'] and b['formal_types']==dict(arena='TypeArena *',capacity='unsigned int'),'complete address initializer cdecl ABI differs'
    operations=[(0,0x40ef7b,struct.pack('<H',3),(original['address_hash_entry']['esp']-20,None)),(1,0x40ef86,bytes(2),None)]
    offset=0
    for count in (27,29,8):operations.append((0,0x40efa5,struct.pack('<I',offset),(original['address_hash_entry']['esp']-12,None)));offset+=count*8
    for index,count in enumerate((27,29,8)):operations.extend([(0,0x40efda,struct.pack('<H',count),(old_roots[0]+index*12,new_roots[0]+index*12)),(1,0x40efe4,bytes(2),None)])
    for index in range(3):
        contents=previous_arrays[index+1]
        for at in range(0,len(contents),8):
            pointer,stream_offset=struct.unpack_from('<II',contents,at);symbol_offset=historical['memory'][pointer][8:12]
            operations.extend([(0,0x40f01e,struct.pack('<I',stream_offset),(old_roots[index+1]+at+4,new_roots[index+1]+at+4)),(0,0x40f030,symbol_offset,(pointer+8,pointers[pointer]+8))])
    assert len(old['stream_calls'])==len(new['stream_calls'])==len(operations)==139,'complete address actual stream operation inventory differs'
    output=b''
    for index,((kind,site,payload,sources),a,b) in enumerate(zip(operations,old['stream_calls'],new['stream_calls'])):
        assert a['kind']==b['kind']==kind and a['site']==site and a['return_site']==site+5 and b['owner']=='build_symbol_address_hash' and b['name']==('copy_type_arena' if kind==0 else 'fill_type_arena'),'complete address actual stream sequence/owner differs'
        assert a['size']==b['size']==len(payload) and a['arena']==original_arena and b['arena']==native_arena,'complete address stream extent/arena differs'
        assert a['entry_esp']+(4 if kind==0 else 0)==a['exit_esp'] and b['entry_esp']+(8 if kind==0 else 4)==b['exit_esp'],'complete address stream fastcall/cdecl cleanup differs'
        assert a['before_nonvolatile']==a['after_nonvolatile'] and b['before_nonvolatile']==b['after_nonvolatile'],'complete address stream normal-return registers differ'
        assert b['formal_types']==(dict(arena='TypeArena *',data='const unsigned char *',size='unsigned int') if kind==0 else dict(arena='TypeArena *',value='unsigned int',size='unsigned int')),'complete address stream actual formal types differ'
        if kind==0:
            source,other=sources
            if other is None:
                name='maximum_segment' if index==0 else 'offset';schema=b['scalar_sources'][name];assert schema['width']==(2 if name=='maximum_segment' else 4) and schema['type']==('unsigned short' if name=='maximum_segment' else 'unsigned int'),'complete address actual scalar-local ABI differs';other=schema['address']
            assert (a['value'],b['value'])==(source,other) and a['input']==bytes.fromhex(b['input'])==payload,'complete address initialized copy source/bytes differ'
        else:assert a['value']==b['value']==0,'complete address fill byte differs'
        for row,roots,hexadecimal in [(a,output_roots,False),(b,native_output_roots,True)]:
            contents=lambda key:bytes.fromhex(row[key]) if hexadecimal else row[key]
            before=empty if index==0 else struct.pack('<6I',4096,1,roots[0],roots[1]+len(output),4096-len(output),roots[0]);after=struct.pack('<6I',4096,1,roots[0],roots[1]+len(output)+len(payload),4096-len(output)-len(payload),roots[0])
            assert contents('before_arena')==before and contents('after_arena')==after and row['result']==roots[1]+len(output),'complete address stream actual arena/result identity differs'
            assert row['output_address']==roots[1] and contents('output')==output+payload,'complete address complete initialized stream write differs'
        output+=payload
    assert len(output)==540,'complete address independently derived output extent differs'
    assert len(old['frees'])==len(new['frees'])==4,'complete address actual temporary free inventory differs'
    for sequence,(a,b) in enumerate(zip(old['frees'],new['frees'])):
        index=(1,2,3,0)[sequence];site=0x40f09b if sequence<3 else 0x40f0b9
        assert a['sequence']==b['sequence']==sequence and a['index']==b['index']==index and a['site']==site and a['return_site']==site+2 and a['address']==old_roots[index] and b['address']==new_roots[index],'complete address actual free sequence/owner differs'
        assert a['size']==b['size']==(36,216,232,64)[index] and a['before']==previous_arrays[index] and normalized(b['before'],index,True)==previous_arrays[index],'complete address historical pre-free initialized bytes differ'
        assert a['provider']==original['module_bases']['msvcrt.dll']+5002 and b['owner']=='build_symbol_address_hash' and b['returned'] is True,'complete address free canonical provider/return differs'
        assert a['entry_esp']==a['exit_esp'] and b['exit_esp']==b['entry_esp']+4 and a['before_nonvolatile']==a['after_nonvolatile'] and b['before_nonvolatile']==b['after_nonvolatile'],'complete address free cdecl/nonvolatile ABI differs'
    for current,roots,arena,hexadecimal in [(old['current'],output_roots,original_arena,False),(new['current'],native_output_roots,native_arena,True)]:
        contents=lambda key:bytes.fromhex(current[key]) if hexadecimal else current[key]
        assert current['arena_address']==arena and contents('arena')==struct.pack('<6I',4096,1,roots[0],roots[1]+540,3556,roots[0]),'complete address current live arena differs'
        assert current['block_address']==roots[0] and contents('block')==struct.pack('<3I',540,roots[1],0) and current['payload_address']==roots[1] and contents('payload')==output,'complete address current initialized block/output differs'
    # The actual final names argument still aliases the earlier LIVE name
    # arena; the four retired arrays and all unwritten capacities stay private.
    helper_proof=None
    if names_copied:
        helper_proof=first_block(config,original,native,historical,previous,old,new)
    else:
        assert original['stack'][1]==historical['name_hash_entry']['stack'][4] and original['memory'][original['stack'][1]]==historical['name_hash_complete']['arena'],'complete address current names argument identity/fields differ'
        assert native['observed_argument_types']==[dict(name='stream',kind='pointer',width=4,native_type='TypeArena *',pointee_width=24)] and native['parameters']==dict(stream=dict(owner='write_publics',local='names',width=24,bytes=previous['name_hash_complete']['arena'])),'complete address next actual names formal/alias differs'
    assert native['name_hash_complete']==previous['name_hash_complete'],'complete address changed current live names generation'
    for state,roots,live,hexadecimal in [(original,old_roots,output_roots,False),(native,new_roots,native_output_roots,True)]:
        for base,size in list(zip(roots,(36,216,232,64)))+list(zip(live,(12,4096))):
            for namespace in ('memory','strings'):
                for address,payload in state[namespace].items():
                    address=int(address);payload=bytes.fromhex(payload) if hexadecimal else payload
                    assert max(base,address)>=min(base+size,address+len(payload)),'complete address private generation leaked into generic current state'
    # Current global fields remain unchanged except the explicitly derived
    # buffered-write count. This includes all identity-only old dangling roots.
    for row in data:
        name=row['name'];before=historical['memory'][row['original']];after=original['memory'][row['original']]
        if name=='output_used':assert int.from_bytes(after,'little')==actual(native,name)==used
        else:assert after==before and native['globals'][name]==previous['globals'][name],'complete address unexpected changed global: '+name
    old_table=root(historical,'type_table_global');new_table=actual(previous,'type_table_global')
    assert original['memory'][old_table]==historical['memory'][old_table] and native['memory'][str(new_table)]==previous['memory'][str(new_table)],'complete address changed current live public buckets'
    for pointer,other in pointers.items():
        assert original['memory'][pointer]==historical['memory'][pointer] and native['memory'][str(other)]==previous['memory'][str(other)],'complete address changed current initialized public record'
    header=struct.pack('<HHIII',10,12,2116,564,540);records=[];table=historical['memory'][root(historical,'type_table_global')]
    for bucket in range(4096):
        pointer=int.from_bytes(table[bucket*4:bucket*4+4],'little')
        while pointer:
            contents=historical['memory'][pointer];records.append(contents[16:]);pointer=int.from_bytes(contents[:4],'little')
    assert len(records)==65 and sum(map(len,records))==2116,'complete address public record serialization input differs'
    expected=historical['output_buffer_state']['bytes']+header+b''.join(records);assert len(expected)==7188
    if names_copied:expected+=historical['name_hash_complete']['payload'];assert len(expected)==7752
    out=original['output_buffer_state'];other=native['output_buffer_state']
    assert out['bytes']==bytes.fromhex(other['bytes'])==expected and out['used']==other['used']==used and out['capacity']==other['capacity']==65512 and out['file']==other['file']==15 and out['address']==historical['output_buffer_state']['address'] and other['address']==previous['output_buffer_state']['address'],'complete address actual complete packed-output extent differs'
    directory=bytearray(historical['memory'][root(historical,'directory_entries')]);struct.pack_into('<HHII',directory,58*12,0x12a,0xffff,5056,3236)
    native_directory=bytearray(bytes.fromhex(previous['memory'][str(actual(previous,'directory_entries'))]));struct.pack_into('<HHII',native_directory,58*12,0x12a,0xffff,5056,3236)
    assert original['memory'][root(original,'directory_entries')]==directory and bytes.fromhex(native['memory'][str(actual(native,'directory_entries'))])==native_directory,'complete address actual public directory row/tail differs'
    assert native['public_writer_frame']['header']==header.hex() and native['public_writer_frame']['uninitialized_streams']==[] and native['public_writer_frame']['uninitialized_stream_bytes_uncompared']==0,'complete address current initialized public header/stream schema differs'
    environment(old['frontier_environment'],baseline,False);environment(native['output_buffer_state']['environment'],native_baseline,True);environment(out['environment'],baseline,False)
    mapped_store_proof=mapping_writes(data,original,native,historical,previous,old,new)
    return dict(prefix=prefix,fresh_historical_phase=phase_proof,parameters=native['parameters'],policy=policy,mapped_input_store_proof=mapped_store_proof,first_block=helper_proof,address_hash_complete=dict(sorts=3,callback_counts=[r['callback_end']-r['callback_begin']for r in old['sorts']],callbacks=len(old['callbacks']),all_callback_inputs_results_ABI_sequence=True,fully_initialized_temporary_bytes=548,actual_stream_calls=139,initialized_address_stream_bytes=540,own_unwritten_address_tail_bytes_uncompared=3556,temporary_free_calls=4,retired_payload_reinstalled=False,actual_new_live_owners=2,new_live_capacity_bytes=4108,initialized_new_live_bytes=552,initialized_packed_output_bytes=used,own_unwritten_packed_output_bytes_uncompared=65512-used,current_directory_entry=58,current_names_generation_retained=True,original_mapping_scope='once-captured first-phase view lifetime retained through pinned no-mapping-API/no-mapped-input-write continuation; no final fresh original view-byte claim',each_own_full_raw_FP_errors_preserved=True,next_consumer='next_hash_output_block(current names block12)' if names_copied else 'first_hash_output_block(current names24)'))


def first_block(config,original,native,historical,previous,old,new):
    a=old['first_block'];b=new['first_block'];left=historical['name_hash_complete'];right=previous['name_hash_complete']
    assert a['return_pc']==0x40d7d9 and b['return_pc'] and b['owner']=='write_publics' and b['local']=='names' and b['formal_type']=='TypeArena *','first block actual caller/formal identity differs'
    for row,known,hexadecimal in [(a,left,False),(b,right,True)]:
        raw=lambda key:bytes.fromhex(row[key])if hexadecimal else row[key]
        value=lambda key:bytes.fromhex(known[key])if hexadecimal else known[key]
        assert row['argument']==known['arena_address'] and raw('before_arena')==raw('after_arena')==value('arena'),'first block live arena/input preservation differs'
        assert row['result']==known['block_address'] and raw('block')==value('block'),'first block actual result/block identity differs'
        assert row['before_nonvolatile']==row['after_nonvolatile'] and len(row['before_nonvolatile'])==4,'first block nonvolatile ABI differs'
    assert a['entry_esp']==a['exit_esp'] and a['caller_esp']==a['entry_esp']+4 and a['caller_result']==a['result'],'first block original cdecl return differs'
    assert b['exit_esp']==b['entry_esp']+4,'first block native cdecl return differs'
    assert a['copy_esp']==a['entry_esp']+8 and a['copy_block']==left['block_address'] and a['copy_source_end']==left['payload_address']+564 and a['copy_destination_end']==original['output_buffer_state']['address']+7752,'first block actual inline copy boundary differs'
    assert original['stack'][1]==left['block_address'] and original['memory'][left['block_address']]==left['block'],'first block next actual live block argument differs'
    assert native['observed_argument_types']==[dict(name='block',kind='pointer',width=4,native_type='TypeArenaBlock *',pointee_width=12)],'first block next formal ABI differs'
    assert native['parameters']==dict(block=dict(owner='write_publics',local='block',width=12,bytes=right['block'])),'first block next local-pointer value/contents differ'
    # Native return PCs are independently bound to the actual caller CALL.
    from capstone import Cs,CS_ARCH_X86,CS_MODE_32
    start,body=native_symbol('write_publics');helper,_=native_symbol('first_hash_output_block');bias=native['function_addresses']['write_publics']-start
    calls={i.address+i.size for i in Cs(CS_ARCH_X86,CS_MODE_32).disasm(body,start) if i.mnemonic=='call' and i.op_str==hex(helper)}
    assert b['return_pc']-bias in calls,'first block native return PC is not its genuine caller'
    return dict(cdecl_arguments=1,ret_cleanup=0,initialized_arena_bytes=24,initialized_block_bytes=12,initialized_names_bytes_copied=564,source_and_result_preserved=True,actual_next_frontier=0x4100d0,own_unwritten_names_tail_bytes_uncompared=3532)
