"""Bounded completed source-index family: three new owners, two retirements.

Old typed table bytes remain historical. New output and temporary PRE-free
snapshots use separate generation namespaces even when allocator addresses
coincide with old dangling globals. Unknown consumers are still held.
"""
import struct


def validate(config,data,original,native,objects,allocations):
    expected=(0x4063f0,original['module_bases']['msvcrt.dll']+0x2551e,'recon_qsort') if config.get('observe_output_directory') else (0x4069e0,0x404776,'cvpack_main')
    assert (original['address'],original['stack'][0],native['frontier_caller'])==expected,'source family unknown next consumer'
    rows={row['name']:row for row in data}
    def root(name):return int.from_bytes(original['memory'][rows[name]['original']],'little')
    old=original['source_family_allocations'];new=native['source_family_allocations']
    assert len(old)==len(new)==3,'source family allocation inventory differs'
    for index,(left,right) in enumerate(zip(old,new)):
        role=['module','name','output'][index];size=[12,16,160][index];site=[0x40f183,0x40f0d2,0x40f257][index]
        assert left['site']==site and left['size']==right['size']==size and left['after_frees']==right['after_frees']==12 and left['address'] and right['address'],'source family actual allocation generation differs'
        assert right['role']==role and right['owner']==('append_source_name' if index==1 else 'build_source_file_index'),'source family actual allocation owner differs'
        for a,b in zip(old[:index],new[:index]):
            assert max(a['address'],left['address'])>=min(a['address']+a['size'],left['address']+size) and max(b['address'],right['address'])>=min(b['address']+b['size'],right['address']+size),'source family simultaneously live owners overlap'
    # Derive the supported one-file input from full live module/source payloads.
    modules=[];names=[];seen=set();info=root('module_list')
    while info:
        assert info not in seen and len(seen)<128,'source index module input cycle'
        seen.add(info);module=original['memory'][info];assert len(module)==48
        source=int.from_bytes(module[28:32],'little');size=int.from_bytes(module[32:36],'little')
        number=int.from_bytes(module[:2],'little')
        if source:
            payload=original['memory'][source];other=allocations[source]
            assert len(payload)==size and bytes.fromhex(native['memory'][str(other)])==payload,'source family full initialized source input differs'
            count=int.from_bytes(payload[:2],'little');assert count==1 and size>=8,'source family unmodeled multiple-file input'
            offset=int.from_bytes(payload[4:8],'little');assert offset+4<=size
            offset+=4+int.from_bytes(payload[offset:offset+2],'little')*12
            assert offset<size and offset+1+payload[offset]<=size,'source family counted filename input exceeds live source'
            names.append(dict(address=source+offset,native=other+offset,bytes=payload[offset:offset+1+payload[offset]],source=source,source_offset=offset,module=number))
            modules.append(dict(number=number,count=count,info=info,native_info=objects[info]))
        info=int.from_bytes(module[44:48],'little')
    assert len(seen)==root('module_count')==35 and len(modules)==len(names)==1,'source family unmodeled module/source inventory'
    name=names[0];module=modules[0]
    arguments=old[1]['arguments'];actual_arguments=new[1]['arguments']
    assert arguments['name']==name['address'] and actual_arguments['name']==name['native'] and arguments['offset']==actual_arguments['offset']==0,'source append actual pointer/scalar arguments differ'
    assert arguments['tail_before']==bytes.fromhex(actual_arguments['tail_before'])==bytes(16),'source append initialized sentinel input differs'
    for allocations,records in [(old,old),(new,new)]:
        after_append=records[2]['after_append']
        assert after_append['name_tail']==after_append['sentinel_next']==allocations[1]['address'] and after_append['module_head']==allocations[0]['address'],'source append actual returned node/link aliases differ'
    before=original['source_family_frees'];after=native['source_family_frees']
    assert len(before)==len(after)==2,'source family temporary free inventory differs'
    initialized=0
    for index,(left,right) in enumerate(zip(before,after)):
        a=old[index];b=new[index]
        assert left['site']==[0x40f3ce,0x40f3e7][index] and right['index']==index and right['owner']=='build_source_file_index','source family temporary free call owner/order differs'
        assert left['address']==a['address'] and right['address']==b['address'] and left['size']==right['size']==a['size'],'source family temporary free ownership/extent differs'
        assert left['stack_argument']==a['address'] and left['entry_esp']==left['exit_esp'] and right['exit_esp']==right['entry_esp']+4 and right['returned'] is True,'source family free cdecl argument/return differs'
        assert left['provider']==original['module_bases']['msvcrt.dll']+0x138a,'source family actual canonical free provider differs'
        assert left['entry_last_error']==left['exit_last_error']==right['entry_last_error']==right['exit_last_error']==0,'source family free LastError differs'
        raw=left['before'];actual=bytes.fromhex(right['before']);assert len(raw)==len(actual)==a['size'],'source family complete prefree capacity missing'
        if index==0:
            expected=struct.pack('<HHII',module['number'],1,0,0)
            assert raw==actual==expected,'source family module header/offset initialized input differs'
            initialized+=12
        else:
            assert struct.unpack('<3I',raw[:12])==(name['address'],0,0) and struct.unpack('<3I',actual[:12])==(name['native'],0,0),'source family name pointer/offset/link initialized input differs'
            initialized+=12
            # Reserved+12..15 was never written/read; each run keeps its own.
        for base,size in [(a['address'],a['size'])]:
            assert not any(max(base,location)<min(base+size,location+len(value)) for location,value in original['memory'].items()),'source temporary retired payload present in current memory'
            assert not any(max(base,location)<min(base+size,location+len(value)) for location,value in original['strings'].items()),'source temporary retired payload present in current strings'
        base=b['address'];size=b['size']
        for key in ('memory','strings'):
            assert not any(max(base,int(location))<min(base+size,int(location)+len(bytes.fromhex(value))) for location,value in native[key].items()),'source temporary retired payload present in current memory/string'
    starts=[0]*35;counts=[0]*35;counts[module['number']-1]=1
    expected=struct.pack('<HH',35,1)+struct.pack('<35H',*starts)+struct.pack('<35H',*counts)+struct.pack('<I',0)+name['bytes']
    assert len(expected)==160,'source family output layout has unobserved padding'
    output=original['source_family_output'];actual=native['source_family_output']
    assert output['address']==old[2]['address']==root('source_file_index') and actual['address']==new[2]['address'],'source family output owner/global alias differs'
    assert root('source_file_index_size')==160 and len(output['bytes'])==len(bytes.fromhex(actual['bytes']))==160,'source family output complete initialized extent missing'
    assert output['bytes']==bytes.fromhex(actual['bytes'])==expected,'source family complete initialized output differs'
    # New live output is not a reopening of the old generic root namespace.
    for state,base,hexadecimal in [(original,output['address'],False),(native,actual['address'],True)]:
        for key in ('memory','strings'):
            assert not any(max(base,int(location))<min(base+160,int(location)+len(bytes.fromhex(value) if hexadecimal else value)) for location,value in state[key].items()),'source family output must remain a typed generation namespace'
    overlap=[]
    for index,(a,b) in enumerate(zip(old,new)):
        for free_index,(left,right) in enumerate(zip(original['main_free_calls'],native['main_free_calls'])):
            x=left['address'] and max(a['address'],left['address'])<min(a['address']+a['size'],left['address']+left['size'])
            y=right['address'] and max(b['address'],right['address'])<min(b['address']+b['size'],right['address']+right['size'])
            if x or y:overlap.append(dict(generation=index,old_free=free_index,original_overlap=bool(x),native_overlap=bool(y)))
    return dict(new_allocation_generations=3,temporary_free_calls=2,temporary_initialized_pre_free_bytes=initialized,temporary_own_unwritten_tail_bytes=4,live_output_initialized_bytes=160,source_modules=1,module_count=35,source_name=dict(module=module['number'],source_offset=name['source_offset'],bytes=name['bytes'].hex()),actual_old_generation_address_reuse=overlap,retired_temporary_current_payload_compared=False,next_consumer='flush_packed_file')
