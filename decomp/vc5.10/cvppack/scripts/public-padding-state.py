"""Actual public buckets/nodes before padding, then one observed end marker."""
import struct


def validate(config,data,original,native,image_aliases,image_sizes):
    name_hash=bool(config.get('observe_name_hash'))
    complete=bool(config.get('observe_name_hash_complete'))
    if complete:assert original['address']==0x40ede0 and original['stack'][0]==0x40d68f and native['frontier_caller']=='write_publics','public padding next addresshash differs'
    elif name_hash:assert original['address']==0x410010 and original['stack'][0]==0x40ebc0 and native['frontier_caller']=='build_symbol_name_hash','public padding next stream-init differs'
    else:assert original['address']==0x40e920 and original['stack'][0]==0x40d66a and native['frontier_caller']=='write_publics','public padding next namehash frontier differs'
    rows={row['name']:row for row in data}
    def root(name):return int.from_bytes(original['memory'][rows[name]['original']],'little')
    def actual(name):return int.from_bytes(bytes.fromhex(native['globals'][name]),'little')
    left=original['public_padding'];right=native['public_padding']
    assert left['reserve']==right['reserve']==4 and left['return_pc']==0x40d62f and left['result']==right['result']==8,'public padding actual inputs/result differ'
    assert left['exit_esp']==left['entry_esp'] and right['exit_esp']==right['entry_esp']+4,'public padding actual cdecl return stack differs'
    assert len(left['before_nonvolatile'])==len(right['before_nonvolatile'])==4 and left['before_nonvolatile']==left['after_nonvolatile'] and right['before_nonvolatile']==right['after_nonvolatile'],'public padding actual ordinary return registers differ'
    assert left['buckets_address']==root('type_table_global') and right['buckets_address']==actual('type_table_global') and image_aliases[left['buckets_address']]==right['buckets_address'],'public padding actual bucket owner differs'
    before=left['buckets'];other=bytes.fromhex(right['buckets']);assert len(before)==len(other)==16384,'public padding complete input bucket extent differs'
    old=left['nodes'];new=right['nodes'];assert len(old)==len(new)==64,'public padding original initialized node inventory differs'
    mapping={};position=0;total=0
    def owned(address,size):
        owners=[base for base,extent in image_sizes.items() if extent==0x1fa4 and base<=address and address+size<=base+extent]
        assert len(owners)==1,'public padding node lacks actual arena/initialized extent';base=owners[0]
        return image_aliases[base]+address-base
    by_address={row['address']:row for row in old};assert len(by_address)==64,'public padding input node identity repeats'
    for a,b in zip(old,new):
        payload=a['bytes'];current=bytes.fromhex(b['bytes']);assert 20<=len(payload)==len(current)==18+int.from_bytes(payload[16:18],'little')<=1042,'public padding full initialized node input extent differs'
        assert a['bucket']==b['bucket'] and a['address'] and b['address']==owned(a['address'],len(payload)),'public padding input node ownership/order differs'
        assert payload[4:]==current[4:] and int.from_bytes(payload[4:8],'little')%4096==a['bucket'],'public padding initialized input record/hash differs'
        mapping[a['address']]=b['address'];length=int.from_bytes(payload[16:18],'little')+2
        assert (position&0xffff)+length+8<=4096,'public padding newly encountered internal page boundary needs actual state contract'
        position+=length;total+=length
    assert total==2108 and root('public_symbol_count')==actual('public_symbol_count')==64,'public padding complete byte/count input differs'
    # Both before tables independently reach exactly the captured complete nodes.
    visited=set()
    for bucket in range(4096):
        a=int.from_bytes(before[bucket*4:bucket*4+4],'little');b=int.from_bytes(other[bucket*4:bucket*4+4],'little')
        while a or b:
            assert a in by_address and a not in visited and mapping[a]==b,'public padding before bucket chain/aliases differ';visited.add(a)
            row=by_address[a];assert row['bucket']==bucket,'public padding before node bucket differs'
            partner=next(row for row in new if row['address']==b);a=int.from_bytes(row['bytes'][:4],'little');b=int.from_bytes(bytes.fromhex(partner['bytes'])[:4],'little')
    assert len(visited)==64,'public padding captured node is unreachable'
    assert left['remaining']==right['remaining']>=24 and right['cursor']==owned(left['cursor'],24),'public padding input bump cursor/remaining differs'
    assert root('public_arena_cursor')==left['cursor']+24 and actual('public_arena_cursor')==right['cursor']+24 and root('public_arena_remaining')==actual('public_arena_remaining')==left['remaining']-24,'public padding actual bump allocation effect differs'
    # No internal alignment record is needed for these actual64 records. The
    # reserve-four end marker is exactly one fully initialized24-byte arena node.
    marker=struct.pack('<IIIHHHHI',0,0xffff,0,0xffff,0,6,0x402,0xffffffff)
    assert len(marker)==24 and original['memory'][left['cursor']]==bytes.fromhex(native['memory'][str(right['cursor'])])==marker,'public padding complete end-marker record differs'
    expected=bytearray(before);expected_other=bytearray(other)
    last=int.from_bytes(before[4095*4:],'little');last_native=int.from_bytes(other[4095*4:],'little')
    if not last:
        struct.pack_into('<I',expected,4095*4,left['cursor']);struct.pack_into('<I',expected_other,4095*4,right['cursor'])
    else:
        while int.from_bytes(by_address[last]['bytes'][:4],'little'):last=int.from_bytes(by_address[last]['bytes'][:4],'little')
        last_native=mapping[last]
    for a,b in zip(old,new):
        payload=bytearray(a['bytes']);current=bytearray.fromhex(b['bytes'])
        if a['address']==last:struct.pack_into('<I',payload,0,left['cursor']);struct.pack_into('<I',current,0,right['cursor'])
        assert original['memory'][a['address']]==bytes(payload) and bytes.fromhex(native['memory'][str(b['address'])])==bytes(current),'public padding changed an existing initialized node'
    assert original['memory'][root('type_table_global')]==bytes(expected) and bytes.fromhex(native['memory'][str(actual('type_table_global'))])==bytes(expected_other),'public padding complete bucket mutation differs'
    assert root('public_symbol_bytes')==actual('public_symbol_bytes')==total+8==2116,'public padding writer byte-count consumption differs'
    expected_parameters={'key':dict(function='symbol_record_key'),'method':dict(owner='write_publics',local='header',field='name_method',width=2,bytes='0000'),'bytes':dict(owner='write_publics',local='header',field='name_bytes',width=4,bytes='00000000'),'stream':dict(owner='write_publics',local='names',width=24),'count':64,'buckets':dict(null=False,global_name='type_table_global')}
    expected_complete={'method':dict(owner='write_publics',local='header',field='address_method',width=2,bytes='0000'),'bytes':dict(owner='write_publics',local='header',field='address_bytes',width=4,bytes='00000000'),'stream':dict(owner='write_publics',local='addresses',width=24),'count':64,'buckets':dict(null=False,global_name='type_table_global')}
    assert native['parameters']==(expected_complete if complete else dict(arena=dict(owner='write_publics',local='names',width=24,description=dict(capacity=4096,blocks=1,remaining=4094,last='namehash-output-block',first='namehash-output-block',cursor_owner='namehash-output-payload',cursor_offset=2)),value=0,size=2) if name_hash else expected_parameters),'public padding actual namehash argument roles differ'
    return dict(input_nodes=64,input_symbol_bytes=2108,padding_nodes=1,padding_symbol_bytes=8,new_initialized_arena_bytes=24,arena_allocation_generation_preserved=True,internal_alignment_path_unvisited=True,unchanged_other_bucket_slots=4095,current_symbol_bytes=2116,hash_stream_uninitialized_bytes_uncompared=24 if name_hash else 48,next_consumer='build_symbol_address_hash' if complete else 'fill_type_arena' if name_hash else 'build_symbol_name_hash(callback,WORDout,DWORDout,stream24,count64,buckets)')
