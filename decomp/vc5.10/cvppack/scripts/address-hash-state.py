"""Four current address-hash generations and their genuine first qsort call."""
import struct


def ended_generations(original,native):
    # Exact events in this frozen case, linked to each actual free on BOTH
    # sides. Address containment cannot establish an allocation generation.
    bindings=[('main_free_calls',free,kind,index,size) for free,kind,index,size in (
        (1,'image',3,12332),(2,'image',0,1126),(3,'image',1,8093),(4,'image',2,256),
        (5,'checked',1,12139),(6,'checked',2,12136),(7,'cleared',8,1536),
        (8,'cleared',0,16384),(9,'cleared',2,16384),(10,'cleared',7,16384),
        (11,'cleared',14,384000))]
    bindings += [('source_family_frees',0,'checked',295,12),('source_family_frees',1,'checked',296,16),
        ('name_hash_temp_frees',0,'cleared',107,624),('name_hash_temp_frees',1,'checked',298,4),
        ('name_hash_temp_frees',2,'cleared',108,65504)]
    ended=set()
    for family,free,kind,index,size in bindings:
        identity=(kind,index);assert identity not in ended,'addresshash repeated ended generation';ended.add(identity)
        for state in (original,native):
            allocation=state[kind+'_allocations'][index];released=state[family][free]
            assert allocation['address']==released['address'] and allocation['size']==released['size']==size,'addresshash ended allocation event/free identity differs'
        actual=native[kind+'_allocations'][index]
        if family=='source_family_frees':
            assert actual.get('owner')==('build_source_file_index' if free==0 else 'append_source_name') and actual.get('role')==('module' if free==0 else 'name'),'addresshash ended source generation role differs'
        elif family=='name_hash_temp_frees':
            ordinal=(1,0,2)[free]
            for state in (original,native):
                typed=state['name_hash_allocations'][ordinal]
                assert typed['address']==state[kind+'_allocations'][index]['address'] and typed['size']==size,'addresshash ended namehash generation differs'
            assert actual.get('context')=='build_symbol_name_hash','addresshash ended namehash owner differs'
    return ended


def generations(original,native):
    left,right=original['address_hash_entry'],native['address_hash_entry']
    assert left and right and left['checked_count']==right['checked_count']==len(original['checked_allocations'])==len(native['checked_allocations']),'addresshash actual checked-event entry bound differs'
    begin=left['cleared_count'];assert begin==right['cleared_count'] and len(original['cleared_allocations'])==len(native['cleared_allocations'])==begin+4,'addresshash actual cleared-event entry bound differs'
    old,new=original['address_hash_allocations'],native['address_hash_allocations'];assert len(old)==len(new)==4,'addresshash new allocation inventory differs'
    indices=set(range(begin,begin+4));mapping={};sizes={}
    for index,(a,b) in enumerate(zip(old,new)):
        site=0x40ee42 if index==0 else 0x40eeaf;size=(36,216,232,64)[index]
        assert a['site']==site and a['return_site']==site+5 and a['size']==b['size']==size and a['address'] and b['address'],'addresshash actual allocation site/size/result differs'
        assert a['after_namehash_frees']==b['after_namehash_frees']==12 and b['owner']=='build_symbol_address_hash' and b['sequence']==index and b['formal_types']=={'size':'unsigned int'},'addresshash actual owner/generation/ABI differs'
        assert a['entry_esp']==a['exit_esp'] and b['exit_esp']==b['entry_esp']+4,'addresshash actual allocator cdecl cleanup differs'
        assert len(a['before_nonvolatile'])==len(b['before_nonvolatile'])==4 and a['before_nonvolatile']==a['after_nonvolatile'] and b['before_nonvolatile']==b['after_nonvolatile'],'addresshash actual allocator normal-return registers differ'
        ga,gb=original['cleared_allocations'][begin+index],native['cleared_allocations'][begin+index]
        assert ga['address']==a['address'] and gb['address']==b['address'] and ga['size']==gb['size']==size and gb['context']==gb['owner']=='build_symbol_address_hash','addresshash actual allocation event identity differs'
        assert a['address'] not in mapping and b['address'] not in mapping.values(),'addresshash current generation identity repeats'
        mapping[a['address']]=b['address'];sizes[a['address']]=size
    ended_events=ended_generations(original,native)
    overlaps=[]
    for side,state in [('original',original),('native',native)]:
        fresh=state['address_hash_allocations'];ended=state['main_free_calls']+state['source_family_frees']+state['name_hash_temp_frees']
        for index,row in enumerate(fresh):
            base,size=row['address'],row['size']
            for other in fresh[index+1:]:assert max(base,other['address'])>=min(base+size,other['address']+other['size']),'addresshash current allocations overlap'
            for kind in ('image','checked','cleared','module','subsection'):
                for ordinal,previous in enumerate(state[kind+'_allocations']):
                    if kind=='cleared' and ordinal in indices:continue
                    address,extent=previous['address'],previous['size']
                    retired=(kind,ordinal) in ended_events
                    if not retired:assert max(base,address)>=min(base+size,address+extent),'addresshash current owner overlaps live prior generation'
            protected=[('directory',state['directory_allocation']['address'],state['directory_allocation']['size'])]+[('packed-output',r['address'],r['size']) for r in state['output_buffer_allocations']]+[('mapped-input',int(address),view['length']) for address,view in state['mapped_views'].items()]
            for name,address,extent in protected:assert max(base,address)>=min(base+size,address+extent),'addresshash current owner overlaps live '+name
            for sequence,previous in enumerate(ended):
                if previous['address'] and max(base,previous['address'])<min(base+size,previous['address']+previous['size']):overlaps.append(dict(side=side,new_owner=index,ended_sequence=sequence,original_address=previous['address'],current_address=base))
    return dict(cleared_indices=indices,mapping=mapping,sizes=sizes,actual_ended_address_reuse=overlaps,ended_allocation_events=[dict(kind=kind,index=index) for kind,index in sorted(ended_events)])


def validate(config,data,original,native,owners):
    assert original['address']==0x40edc0 and original['stack'][0]==original['module_bases']['msvcrt.dll']+0x2551e and native['frontier_caller']=='recon_qsort','addresshash actual first comparator frontier differs'
    rows={row['name']:row for row in data}
    def root(name):return int.from_bytes(original['memory'][rows[name]['original']],'little')
    def actual(name):return int.from_bytes(bytes.fromhex(native['globals'][name]),'little')
    entry,other=original['address_hash_entry'],native['address_hash_entry'];parent=native['public_writer_frame'];streams=parent['stream_objects'];header=parent['header_address']
    assert len(entry['parent_stack'])==128 and list(struct.unpack('<6I',entry['parent_stack'][:24]))==entry['stack'],'addresshash actual original entry stack differs'
    esp=entry['esp'];assert entry['stack']==[0x40d68f,esp+46,esp+56,esp+84,64,root('type_table_global')],'addresshash actual original five incoming arguments differ'
    assert other['owner']=='write_publics' and other['stack'][1:]==[header+2,header+12,streams[1]['address'],64,actual('type_table_global')],'addresshash actual native five incoming roles differ'
    assert other['formal_types']==dict(method='unsigned short *',bytes='unsigned int *',stream='TypeArena *',count='unsigned int',buckets='PublicSymbol **') and other['after_namehash_frees']==12,'addresshash actual formal types/lifetime differs'
    old_roots=[row['address'] for row in original['address_hash_allocations']];new_roots=[row['address'] for row in native['address_hash_allocations']]
    first,second=original['address_hash_sort'],native['address_hash_sort']
    assert first['entry_esp']==esp-52 and first['provider']==original['module_bases']['msvcrt.dll']+0x2543d,'addresshash actual canonical qsort caller/ABI differs'
    assert first['args']==[old_roots[1],27,8,0x40edc0] and second['args']==[new_roots[1],27,8,native['function_addresses']['compare_symbol_address']] and second['owner']=='build_symbol_address_hash','addresshash actual qsort arguments/owner differs'
    assert first['groups']==old_roots[0] and second['groups']==new_roots[0] and first['scalars']==second['scalars']==dict(maximum_segment=3,absolute_count=0,position=2116,segment=0),'addresshash actual local scalars/group pointer differ'
    expected_layouts=dict(AddressHashEntry=dict(width=8,fields=dict(symbol=dict(offset=0,width=4),stream_offset=dict(offset=4,width=4))),AddressHashGroup=dict(width=12,fields=dict(count=dict(offset=0,width=4),written=dict(offset=4,width=4),entries=dict(offset=8,width=4))))
    assert second['layouts']==expected_layouts,'addresshash actual group/entry field layout differs'
    mapping={a['address']:b['address'] for a,b in zip(original['public_padding']['nodes'],native['public_padding']['nodes'])};mapping[original['public_padding']['cursor']]=native['public_padding']['cursor']
    table=original['memory'][root('type_table_global')];groups=[[],[],[]];position=0;visited=set();skipped=0;absolute=0
    for index in range(4096):
        address=int.from_bytes(table[index*4:index*4+4],'little')
        while address:
            assert address not in visited and address in mapping,'addresshash actual public input ownership/cycle differs';visited.add(address);payload=original['memory'][address];size=int.from_bytes(payload[16:18],'little')+2
            assert len(payload)==size+16,'addresshash complete initialized public input differs'
            if int.from_bytes(payload[18:20],'little')==0x402:skipped+=1
            else:
                segment=int.from_bytes(payload[14:16],'little')
                if not segment:absolute+=1
                else:
                    assert 1<=segment<=3,'addresshash unobserved segment requires fresh contract';groups[segment-1].append((address,position))
            position+=size;address=int.from_bytes(payload[:4],'little')
    counts=[len(group) for group in groups];assert counts==[27,29,8] and len(visited)==65 and skipped==1 and not absolute and position==2116,'addresshash independently derived public segment grouping differs'
    for side,state,roots in [('original',original,old_roots),('native',native,new_roots)]:
        before=state['address_hash_owned']['before-sort'] if side=='original' else {int(address):bytes.fromhex(payload) for address,payload in state['address_hash_sort']['before_owned'].items()}
        current=state['address_hash_owned']['first-callback'] if side=='original' else {int(address):bytes.fromhex(payload) for address,payload in state['address_hash_current_owned'].items()}
        assert set(before)==set(current)==set(roots),'addresshash exact initialized ownership namespace differs'
        expected_groups=b''.join(struct.pack('<iiI',count,count,roots[index+1]) for index,count in enumerate(counts));assert before[roots[0]]==current[roots[0]]==expected_groups,'addresshash complete initialized group counts/aliases differ'
        for index,group in enumerate(groups):
            expected=b''.join(struct.pack('<II',address if side=='original' else mapping[address],position) for address,position in group)
            assert before[roots[index+1]]==expected,'addresshash complete before-sort initialized rows differ'
            observed=bytearray(expected)
            if index==0:
                middle=counts[0]//2;observed[:8],observed[middle*8:middle*8+8]=observed[middle*8:middle*8+8],observed[:8]
            assert current[roots[index+1]]==bytes(observed),'addresshash actual first callback array mutation differs'
        for address,size in zip(roots,(36,216,232,64)):
            for kind in ('memory','strings'):
                for location,payload in state[kind].items():
                    location=int(location);payload=bytes.fromhex(payload) if side=='native' else payload
                    assert max(address,location)>=min(address+size,location+len(payload)),'addresshash new generation leaked into generic current namespace'
    assert list(original['stack'][1:3])==[old_roots[1]+8,old_roots[1]] and native['parameters']==dict(first=dict(segment=1,index=1,width=8),second=dict(segment=1,index=0,width=8)),'addresshash actual first comparator argument aliases differ'
    assert native['observed_argument_types']==[dict(name=name,kind='pointer',width=4,native_type='const void *') for name in ('first','second')],'addresshash actual comparator two-formal ABI differs'
    return dict(current_new_owners=4,fully_initialized_capacity_bytes=548,segment_counts=counts,input_nodes=65,absolute_records=0,skipped_alignment_records=1,input_symbol_bytes=2116,actual_first_sort=dict(segment=1,count=27,width=8,pivot_exchange=[0,13],argument_rows=[1,0]),complete_sort_result_unclaimed=True,own_uninitialized_address_stream_bytes_uncompared=24,prior_name_stream_initialized_bytes=564,prior_name_stream_own_unwritten_bytes_uncompared=3532,ended_allocation_events=owners['ended_allocation_events'],historical_namehash_entry_checked_bound=entry['checked_count'],historical_namehash_entry_cleared_bound=entry['cleared_count'],actual_ended_address_reuse=owners['actual_ended_address_reuse'],retired_payload_reinstalled=False,next_consumer='compare_symbol_address(two initialized8B rows, actual live PublicSymbol owners)')
