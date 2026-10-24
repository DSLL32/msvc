"""Case-bound live name-hash generations and initialized temporary contents."""
import struct


def generations(original,native):
    old=original['name_hash_allocations'];new=native['name_hash_allocations']
    assert len(old)==len(new)==5,'namehash new generation inventory differs'
    checked=[];cleared=[];mapping={};sizes={}
    for index,(a,b) in enumerate(zip(old,new)):
        site=(0x40e9aa,0x40e9e9,0x40eb0a,0x40ff9d,0x40ffaa)[index];size=(4,624,65504,12,4096)[index];kind='checked' if index in (0,4) else 'cleared'
        assert a['site']==site and a['return_site']==site+5 and a['size']==b['size']==size and a['address'] and b['address'],'namehash actual allocator call/size/result differs'
        assert b['owner']==('build_symbol_name_hash' if index<3 else 'copy_type_arena') and b['context']=='build_symbol_name_hash' and b['sequence']==index and b['kind']==kind and b['formal_inventory']=={'size':'unsigned int'},'namehash actual allocation owner/ABI differs'
        assert a['after_frees']==b['after_frees']==12 and a['source_frees']==b['source_frees']==2,'namehash actual allocation generation ordering differs'
        assert a['entry_esp']==a['exit_esp'] and b['exit_esp']==b['entry_esp']+4,'namehash actual allocator cdecl return stack differs'
        assert len(a['before_nonvolatile'])==len(b['before_nonvolatile'])==4 and a['before_nonvolatile']==a['after_nonvolatile'] and b['before_nonvolatile']==b['after_nonvolatile'],'namehash actual allocator return registers differ'
        records=original[kind+'_allocations'];partners=native[kind+'_allocations'];assert len(records)==len(partners),'namehash generic allocator inventory differs'
        indices=[i for i,row in enumerate(partners) if row.get('context')=='build_symbol_name_hash']
        expected_index=(0,0,1,2,1)[index]
        assert len(indices)==(2 if kind=='checked' else 3),'namehash typed/generic allocator event inventory differs'
        position=indices[expected_index];left,right=records[position],partners[position]
        assert left['address']==a['address'] and right['address']==b['address'] and left['size']==right['size']==size,'namehash allocation generation/event identity differs'
        (checked if kind=='checked' else cleared).append(position)
        assert a['address'] not in mapping and b['address'] not in mapping.values(),'namehash new owner identity repeats'
        mapping[a['address']]=b['address'];sizes[a['address']]=size
    overlaps=[]
    # The five new owners are never inserted into old address-keyed aliases.
    # Only actually retired preceding generations may overlap; live source output
    # and every other recorded current owner remain protected at full capacity.
    for side,state in [('original',original),('native',native)]:
        fresh=state['name_hash_allocations'];retired=state['main_free_calls']+state['source_family_frees']
        for index,row in enumerate(fresh):
            base,size=row['address'],row['size']
            for other in fresh[index+1:]:
                assert not max(base,other['address'])<min(base+size,other['address']+other['size']),'namehash new allocations overlap each other'
            for kind in ('image','checked','cleared','module','subsection'):
                for ordinal,previous in enumerate(state[kind+'_allocations']):
                    if kind=='checked' and ordinal in checked or kind=='cleared' and ordinal in cleared:continue
                    before,extent=previous['address'],previous['size']
                    # Identify source generations by native event owner+role and
                    # independently matched ledger; address inclusion alone must
                    # never erase the later live source-file index generation.
                    source_role=None
                    if kind=='checked':
                        partner=native['checked_allocations'][ordinal]
                        if partner.get('owner') in ('build_source_file_index','append_source_name'):source_role=partner.get('role')
                    ended=source_role in ('module','name') if source_role is not None else any(r['address'] and r['address']<=before and before+extent<=r['address']+r['size'] for r in retired)
                    if not ended:assert not max(base,before)<min(base+size,before+extent),'namehash new owner overlaps an existing live generation'
            live_special=[('packed-output',r['address'],r['size']) for r in state['output_buffer_allocations']]+[('directory',state['directory_allocation']['address'],state['directory_allocation']['size'])]
            live_special.extend(('mapped-input',int(address),view['length']) for address,view in state['mapped_views'].items())
            for label,address,extent in live_special:assert not max(base,address)<min(base+size,address+extent),'namehash new owner overlaps live '+label
            for ordinal,previous in enumerate(retired):
                if previous['address'] and max(base,previous['address'])<min(base+size,previous['address']+previous['size']):overlaps.append(dict(side=side,new_owner=index,retired_sequence=ordinal,retired_address=previous['address'],new_address=base))
    return dict(checked_indices=set(checked),cleared_indices=set(cleared),mapping=mapping,sizes=sizes,retired_address_reuse=overlaps)


def validate(config,data,original,native,owners):
    assert original['address']==0x410010 and original['stack'][0]==0x40ebc0 and native['frontier_caller']=='build_symbol_name_hash','namehash actual stream-init frontier differs'
    rows={row['name']:row for row in data}
    def root(name):return int.from_bytes(original['memory'][rows[name]['original']],'little')
    def actual(name):return int.from_bytes(bytes.fromhex(native['globals'][name]),'little')
    frame=native['name_hash_frame'];assert frame['owner']=='build_symbol_name_hash' and frame['caller']=='write_publics','namehash actual local owner differs'
    assert frame['formal_types']==dict(key='unsigned short (*)(const unsigned char *, unsigned int *, unsigned short *, unsigned short *, unsigned int *, unsigned int)',method='unsigned short *',bytes='unsigned int *',stream='TypeArena *',count='unsigned int',buckets='PublicSymbol **'),'namehash actual six formal types differ'
    inputs=original['name_hash_entry'];other=native['name_hash_entry'];assert inputs['esp']==original['esp']+112,'namehash actual current/original entry stack differs'
    assert inputs['stack']==[0x40d66a,0x40cf50,inputs['esp']+48,inputs['esp']+56,inputs['esp']+64,64,root('type_table_global')],'namehash actual six incoming original arguments differ'
    parent=native['public_writer_frame'];names=parent['stream_objects'][0]['address']
    expected_arguments=dict(key=native['function_addresses']['symbol_record_key'],method=parent['header_address'],bytes=parent['header_address']+8,stream=names,count=64,buckets=actual('type_table_global'))
    assert frame['arguments']==expected_arguments and other['stack'][1:]==[expected_arguments[name] for name in ('key','method','bytes','stream','count','buckets')],'namehash native actual incoming/formal roles differ'
    assert native['parameters']==dict(arena=dict(owner='write_publics',local='names',width=24,description=dict(capacity=4096,blocks=1,remaining=4094,last='namehash-output-block',first='namehash-output-block',cursor_owner='namehash-output-payload',cursor_offset=2)),value=0,size=2) and original['stack'][1]==inputs['esp']+64 and original['stack'][2]==0 and original['stack'][3]==2,'namehash stream-init output pointer/scalar ABI differs'
    expected_layouts={'NameHashPair':dict(width=8,fields={'offset':dict(offset=0,width=4),'hash':dict(offset=4,width=4)}),'NameHashOverflow':dict(width=44,fields={'next':dict(offset=0,width=4),'entries':dict(offset=4,width=40)}),'NameHashBucket':dict(width=104,fields={'count':dict(offset=0,width=2),'reserved':dict(offset=2,width=2),'overflow':dict(offset=4,width=4),'entries':dict(offset=8,width=96)})}
    assert frame['layouts']==expected_layouts,'namehash actual pair/bucket/overflow layouts differ'
    assert frame['scalars']==dict(hash_count=6,page_count=1,position=2116,overflow_count=2),'namehash actual initialized scalar state differs'
    assert original['stack'][8]==6 and original['stack'][9]==2116 and original['stack'][11]==2 and original['stack'][16]==1,'namehash original initialized scalar state differs'
    addresses=[row['address'] for row in original['name_hash_allocations']];destinations=[row['address'] for row in native['name_hash_allocations']]
    assert original['stack'][10]==addresses[0] and frame['pages']==destinations[0],'namehash actual pointer-table local/owner differs'
    assert list(original['stack'][18:28])==[addresses[2]]+[0]*9 and bytes.fromhex(frame['overflow_pages'])==struct.pack('<10I',destinations[2],*([0]*9)),'namehash initialized overflow-page pointer array differs'
    reused_old=original['name_hash_reused_calls'];reused_new=native['name_hash_reused_calls']
    assert set(reused_old)==set(reused_new)=={'initialize_arena','copy_type_arena'},'namehash actual reused arena-call inventory differs'
    for name in ('initialize_arena','copy_type_arena'):
        a,b=reused_old[name],reused_new[name];assert a['arena']==inputs['esp']+64 and b['arena']==names and b['owner']=='build_symbol_name_hash','namehash reused actual arena argument owner differs'
        assert len(a['before_nonvolatile'])==len(b['before_nonvolatile'])==4 and a['before_nonvolatile']==a['after_nonvolatile'] and b['before_nonvolatile']==b['after_nonvolatile'],'namehash reused arena ordinary return registers differ'
        assert a['exit_esp']==a['entry_esp']+(0 if name=='initialize_arena' else 4) and b['exit_esp']==b['entry_esp']+(4 if name=='initialize_arena' else 8),'namehash reused arena actual cdecl/fastcall cleanup differs'
        assert b['formal_types']==(dict(arena='TypeArena *',capacity='unsigned int') if name=='initialize_arena' else dict(arena='TypeArena *',data='const unsigned char *',size='unsigned int')),'namehash reused actual formal inventory differs'
    initial=struct.pack('<6I',4096,0,0,0,0,0);a,b=reused_old['initialize_arena'],reused_new['initialize_arena']
    assert a['capacity']==a['return_capacity']==b['capacity']==4096 and a['arena']==a['return_arena'] and a['after_arena']==bytes.fromhex(b['after_arena'])==initial,'namehash actual arena initialization differs'
    a,b=reused_old['copy_type_arena'],reused_new['copy_type_arena']
    assert a['size']==b['size']==2 and a['data']==inputs['esp']-80 and b['data']==frame['scalar_addresses']['hash_count'] and a['input']==bytes.fromhex(b['input'])==bytes.fromhex('0600'),'namehash actual copied WORD input/owner differs'
    assert a['before_arena']==bytes.fromhex(b['before_arena'])==initial and a['result']==addresses[4] and b['result']==destinations[4],'namehash actual reused copy input/result differs'
    current=original['memory'][original['stack'][1]];actual_current=bytes.fromhex(frame['arena_bytes'])
    assert current==a['after_arena']==struct.pack('<6I',4096,1,addresses[3],addresses[4]+2,4094,addresses[3]) and actual_current==bytes.fromhex(b['after_arena'])==struct.pack('<6I',4096,1,destinations[3],destinations[4]+2,4094,destinations[3]),'namehash fully initialized arena fields/aliases differ'
    # Derive bucket membership and stream offsets directly from actual current
    # public records, including their observed S_ALIGN end marker.
    groups=[[] for _ in range(6)];overflow_order=[];position=0;visited=set();table=original['memory'][root('type_table_global')]
    for index in range(4096):
        node=int.from_bytes(table[index*4:index*4+4],'little')
        while node:
            assert node not in visited and len(visited)<65,'namehash input public chain inventory differs';visited.add(node)
            record=original['memory'][node];length=int.from_bytes(record[16:18],'little')+2;kind=int.from_bytes(record[18:20],'little')
            assert len(record)==length+16,'namehash current initialized input extent differs'
            if kind!=0x402:
                value=int.from_bytes(record[4:8],'little');bucket=value%6
                if len(groups[bucket])>=12 and (len(groups[bucket])-12)%5==0:overflow_order.append(bucket)
                groups[bucket].append((position,value))
            position+=length;node=int.from_bytes(record[:4],'little')
    assert len(visited)==65 and position==2116 and [len(group) for group in groups]==[8,13,12,13,10,8] and overflow_order==[3,1],'namehash complete actual input grouping differs'
    old_bytes=original['name_hash_owned'];new_bytes=frame['initialized_allocations'];assert set(old_bytes)==set(addresses) and set(new_bytes)=={str(a) for a in destinations},'namehash current typed ownership namespace differs'
    for side,owned,roots in [('original',old_bytes,addresses),('native',{int(a):bytes.fromhex(b) for a,b in new_bytes.items()},destinations)]:
        assert owned[roots[0]]==struct.pack('<I',roots[1]),'namehash initialized page-table pointer differs'
        buckets=bytearray(624);overflow=bytearray(65504)
        for index,group in enumerate(groups):
            struct.pack_into('<H',buckets,index*104,len(group))
            for slot,pair in enumerate(group[:12]):struct.pack_into('<II',buckets,index*104+8+slot*8,*pair)
            if len(group)>12:
                ordinal=overflow_order.index(index);struct.pack_into('<I',buckets,index*104+4,roots[2]+ordinal*44)
                for slot,pair in enumerate(group[12:]):struct.pack_into('<II',overflow,ordinal*44+4+slot*8,*pair)
        assert owned[roots[1]]==bytes(buckets),'namehash complete initialized bucket contents/aliases differ'
        assert owned[roots[2]]==bytes(overflow),'namehash complete cleared overflow contents/aliases differ'
        assert owned[roots[3]]==struct.pack('<3I',2,roots[4],0),'namehash initialized output block differs'
        assert owned[roots[4]]==bytes.fromhex('0600'),'namehash initialized output payload differs'
        # New role is allowed only in this namespace. Retired payload observations
        # and stale globals remain denied by the existing generic CurrentMemory.
        state=original if side=='original' else native
        for address,size in zip(roots,(4,624,65504,12,4096)):
            for kind in ('memory','strings'):
                for location,payload in state[kind].items():
                    payload=payload if side=='original' else bytes.fromhex(payload);location=int(location)
                    assert not max(address,location)<min(address+size,location+len(payload)),'namehash new generation leaked into generic current memory'
    return dict(input_nodes=65,hashed_records=64,hash_bucket_counts=[len(group) for group in groups],new_owners=5,pointer_table_initialized_bytes=4,bucket_initialized_bytes=624,overflow_initialized_bytes=65504,used_overflow_nodes=2,cleared_overflow_spare_bytes=65416,own_uninitialized_stream_bytes_uncompared=24,initialized_arena_bytes=24,initialized_block_bytes=12,initialized_payload_bytes=2,own_unwritten_payload_bytes=4094,next_consumer='fill_type_arena(names24,value0,size2)',original_dead_mutated_bucket_argument_slot=original['stack'][34],retired_payload_reopened=False,actual_retired_address_reuse=owners['retired_address_reuse'])
