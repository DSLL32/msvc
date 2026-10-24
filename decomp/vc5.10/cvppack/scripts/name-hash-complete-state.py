"""Finite name-stream writes, actual temporary frees, then address-hash entry."""
import struct


def validate(config,data,original,native,owners,initial):
    if config.get('observe_address_hash'):
        assert original['address']==0x40edc0 and original['stack'][0]==original['module_bases']['msvcrt.dll']+0x2551e and native['frontier_caller']=='recon_qsort','completed namehash actual address callback differs'
    else:assert original['address']==0x40ede0 and original['stack'][0]==0x40d68f and native['frontier_caller']=='write_publics','completed namehash actual next child differs'
    # These are actual observations taken while all five allocations were live,
    # not current-state reads of the three subsequently released temporaries.
    snapshot=original['name_hash_first_fill_stack'];assert len(snapshot['bytes'])==256,'missing complete first-fill stack observation'
    first=next(row for row in original['name_hash_stream_calls'] if row['kind']==1)
    other=next(row for row in native['name_hash_stream_calls'] if row['kind']==1)
    assert snapshot['address']==first['entry_esp']-4,'namehash first-fill actual entry stack differs'
    before=dict(original,address=0x410010,esp=snapshot['address'],stack=list(struct.unpack('<64I',snapshot['bytes'])),memory=dict(original['memory']))
    before['memory'][snapshot['address']]=snapshot['bytes']
    arena=before['stack'][1];relative=arena-snapshot['address'];assert 0<=relative and relative+24<=256
    before['memory'][arena]=snapshot['bytes'][relative:relative+24]
    historical_parent=dict(native['public_writer_frame'],header=native['name_hash_frame']['parent_header'])
    historical_native=dict(native,frontier_caller='build_symbol_name_hash',public_writer_frame=historical_parent,parameters=dict(arena=dict(owner='write_publics',local='names',width=24,description=dict(capacity=4096,blocks=1,remaining=4094,last='namehash-output-block',first='namehash-output-block',cursor_owner='namehash-output-payload',cursor_offset=2)),value=other['value'],size=other['size']))
    assert first['arena']==before['stack'][1] and first['value']==before['stack'][2] and first['size']==before['stack'][3] and other['arena']==historical_parent['stream_objects'][0]['address'],'namehash observed first-fill role projection differs'
    prefix=initial.validate(config,data,before,historical_native,owners)
    old_roots=[r['address'] for r in original['name_hash_allocations']];new_roots=[r['address'] for r in native['name_hash_allocations']]
    table=original['name_hash_owned'][old_roots[1]];overflow=original['name_hash_owned'][old_roots[2]]
    counts=[int.from_bytes(table[i*104:i*104+2],'little') for i in range(6)]
    operations=[(0,0x40ebb0,struct.pack('<H',6),None),(1,0x40ebbb,bytes(2),0)]
    offset=0
    for count in counts:operations.append((0,0x40ec07,struct.pack('<I',offset),None));offset+=count*8
    for count in counts:operations.extend([(0,0x40ec60,struct.pack('<H',count),None),(1,0x40ec6a,bytes(2),0)])
    for index,count in enumerate(counts):
        operations.append((0,0x40ecdc,table[index*104+8:index*104+8+min(count,12)*8],None))
        if count>12:
            pointer=int.from_bytes(table[index*104+4:index*104+8],'little');relative=pointer-old_roots[2];assert 0<=relative and relative+44<=65504 and count<=17
            operations.append((0,0x40ed1d,overflow[relative+4:relative+4+(count-12)*8],None))
    assert len(operations)==28 and sum(kind==0 for kind,_,_,_ in operations)==21,'namehash derived stream operation inventory differs'
    frame=native['name_hash_frame'];assert frame['offset_type']=='unsigned int' and frame['offset_address'],'namehash named DWORD offset local differs'
    original_entry=original['name_hash_entry']['esp'];sources=[(original_entry-80,frame['scalar_addresses']['hash_count']),None]
    sources.extend([(original_entry-60,frame['offset_address'])]*6)
    for index in range(6):sources.extend([(old_roots[1]+index*104,new_roots[1]+index*104),None])
    for index,count in enumerate(counts):
        sources.append((old_roots[1]+index*104+8,new_roots[1]+index*104+8))
        if count>12:
            pointer=int.from_bytes(table[index*104+4:index*104+8],'little');relative=pointer-old_roots[2];sources.append((pointer+4,new_roots[2]+relative+4))
    assert len(sources)==len(operations)

    left,right=original['name_hash_stream_calls'],native['name_hash_stream_calls'];assert len(left)==len(right)==len(operations),'namehash actual complete stream inventory differs'
    output=b''
    for index,((kind,site,payload,value),a,b) in enumerate(zip(operations,left,right)):
        assert a['kind']==b['kind']==kind and a['site']==site and a['return_site']==site+5 and b['owner']=='build_symbol_name_hash','namehash actual stream sequence/owner differs'
        assert a['size']==b['size']==len(payload) and a['arena']==original['name_hash_entry']['stack'][4] and b['arena']==native['name_hash_entry']['stack'][4],'namehash actual stream size/arena differs'
        assert a['exit_esp']==a['entry_esp']+(4 if kind==0 else 0) and b['exit_esp']==b['entry_esp']+(8 if kind==0 else 4),'namehash actual stream fastcall/cdecl cleanup differs'
        expected_types=dict(arena='TypeArena *',data='const unsigned char *',size='unsigned int') if kind==0 else dict(arena='TypeArena *',value='unsigned int',size='unsigned int')
        assert b['formal_types']==expected_types and b['name']==('copy_type_arena' if kind==0 else 'fill_type_arena'),'namehash actual stream formal ABI differs'
        assert len(a['before_nonvolatile'])==len(b['before_nonvolatile'])==4 and a['before_nonvolatile']==a['after_nonvolatile'] and b['before_nonvolatile']==b['after_nonvolatile'],'namehash actual stream nonvolatile preservation differs'
        if kind==0:
            assert (a['value'],b['value'])==sources[index],'namehash actual initialized copy pointer/owner differs'
            assert a['input']==bytes.fromhex(b['input'])==payload,'namehash actual complete initialized copy input differs'
        else:assert a['value']==b['value']==value==0,'namehash actual fill byte differs'
        for row,roots,native_side in [(a,old_roots,False),(b,new_roots,True)]:
            decode=lambda key: bytes.fromhex(row[key]) if native_side else row[key]
            # First copy starts with an empty arena and allocates its sole block.
            expected_before=struct.pack('<6I',4096,0,0,0,0,0) if index==0 else struct.pack('<6I',4096,1,roots[3],roots[4]+len(output),4096-len(output),roots[3])
            assert decode('before_arena')==expected_before and row['result']==roots[4]+len(output),'namehash actual stream input state/result identity differs'
            expected_after=struct.pack('<6I',4096,1,roots[3],roots[4]+len(output)+len(payload),4096-len(output)-len(payload),roots[3])
            assert decode('after_arena')==expected_after and row['output_address']==roots[4] and decode('output')==output+payload,'namehash actual initialized stream writes differ'
        output+=payload
    assert len(output)==564,'namehash complete initialized stream extent differs'
    free_old,free_new=original['name_hash_temp_frees'],native['name_hash_temp_frees'];assert len(free_old)==len(free_new)==12,'namehash actual temporary free inventory differs'
    retired=[]
    for sequence,(a,b) in enumerate(zip(free_old,free_new)):
        index=(1,0,2)[sequence] if sequence<3 else 3;site=(0x40ed87,0x40ed97,0x40eda8)[min(sequence,2)]
        assert a['site']==site and a['return_site']==site+2 and a['index']==b['index']==index,'namehash actual temporary free sequence differs'
        assert a['address']==(old_roots[index] if index<3 else 0) and b['address']==(new_roots[index] if index<3 else 0) and b['owner']=='build_symbol_name_hash','namehash temporary free generation/owner differs'
        assert a['provider']==original['module_bases']['msvcrt.dll']+5002 and b['returned'] is True,'namehash actual canonical free provider/return differs'
        assert a['entry_esp']==a['exit_esp'] and b['exit_esp']==b['entry_esp']+4,'namehash temporary free cdecl stack differs'
        assert len(a['before_nonvolatile'])==len(b['before_nonvolatile'])==4 and a['before_nonvolatile']==a['after_nonvolatile'] and b['before_nonvolatile']==b['after_nonvolatile'],'namehash temporary free normal-return registers differ'
        expected_size=(4,624,65504)[index] if index<3 else 0
        assert a['size']==b['size']==expected_size,'namehash prefree initialized extent differs'
        if index<3:
            assert a['before']==original['name_hash_owned'][old_roots[index]] and bytes.fromhex(b['before'])==bytes.fromhex(native['name_hash_frame']['initialized_allocations'][str(new_roots[index])]),'namehash actual prefree initialized contents differ'
            retired.append(dict(index=index,size=expected_size,original=old_roots[index],native=new_roots[index]))
        else:assert not a['before'] and not b['before'],'NULL free admitted payload'
    # Current live arena/block/output is independent of saved pre-free tables.
    for state,roots,is_native in [(original,old_roots,False),(native,new_roots,True)]:
        current=state['name_hash_complete'];decode=lambda key:bytes.fromhex(current[key]) if is_native else current[key]
        assert current['arena_address']==state['name_hash_entry']['stack'][4] and decode('arena')==struct.pack('<6I',4096,1,roots[3],roots[4]+564,3532,roots[3]),'namehash current live arena identity/fields differ'
        assert current['block_address']==roots[3] and decode('block')==struct.pack('<3I',564,roots[4],0) and current['payload_address']==roots[4] and decode('payload')==output,'namehash complete live output differs'
        # Neither retired private capacities nor live new-owner capacities may
        # leak into the generic historical-address keyed memory namespace.
        for base,size in zip(roots,(4,624,65504,12,4096)):
            for kind in ('memory','strings'):
                for address,payload in state[kind].items():
                    address=int(address);payload=bytes.fromhex(payload) if is_native else payload
                    assert max(base,address)>=min(base+size,address+len(payload)),'namehash current generic namespace overlaps private generation'
    assert native['public_writer_frame']['header']==(struct.pack('<HHIII',10,0,2116,564,0)).hex(),'namehash header method/byte consumption differs'
    return dict(before_first_fill=prefix,actual_copy_calls=21,actual_fill_calls=7,initialized_output_bytes=564,own_unwritten_output_bytes_uncompared=3532,own_uninitialized_second_stream_bytes_uncompared=24,temporary_free_calls=12,ended_temporary_allocations=3,prefree_initialized_bytes=66132,retired=retired,retired_payload_reinstalled=False,next_consumer='compare_symbol_address(first8,second8)' if config.get('observe_address_hash') else 'build_symbol_address_hash(WORDout,DWORDout,addresses24,count64,buckets)')
