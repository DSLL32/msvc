"""First source-name callback: live source owner and new temporary generation."""
import struct


def root(original,data,name):
    row=next(row for row in data if row['name']==name)
    return int.from_bytes(original['memory'][row['original']],'little')


def name_argument(original,data,value):
    candidates=[];seen=set();info=root(original,data,'module_list')
    while info:
        assert info not in seen and len(seen)<128,'source-name module list cycle'
        seen.add(info);module=original['memory'][info];source=int.from_bytes(module[28:32],'little');size=int.from_bytes(module[32:36],'little')
        if source:
            payload=original['memory'][source];assert len(payload)==size>=4
            count=int.from_bytes(payload[:2],'little');assert 4+count*4<=size,'source-file array exceeds initialized source module'
            for index in range(count):
                offset=int.from_bytes(payload[4+index*4:8+index*4],'little');assert offset+4<=size
                segments=int.from_bytes(payload[offset:offset+2],'little');offset+=4+segments*12
                assert offset<size and offset+1+payload[offset]<=size,'counted source name exceeds initialized source module'
                if source+offset==value:candidates.append(dict(module=int.from_bytes(module[:2],'little'),file_index=index,source_offset=offset,bytes=payload[offset:offset+1+payload[offset]].hex()))
        info=int.from_bytes(module[44:48],'little')
    assert len(candidates)==1,'original source-name argument lacks exact initialized owner'
    return candidates[0]


def validate(config,data,original,native,image_aliases,image_sizes,object_aliases,allocation_aliases):
    assert original['address']==0x40f0d0 and original['stack'][0]==0x40f1d7 and native['frontier_caller']=='build_source_file_index','unmodeled source-index callback context'
    old=original['source_index_allocations'];new=native['source_index_allocations']
    assert len(old)==len(new)==1 and old[0]['after_frees']==new[0]['after_frees']==12 and new[0]['owner']=='build_source_file_index','source-index actual new generation inventory differs'
    allocation=old[0];actual=new[0];address=allocation['address'];other=actual['address'];size=allocation['size']
    assert actual['size']==size and original['checked_allocations'][-1]['address']==address and native['checked_allocations'][-1]==actual,'source-index generation allocation identity/size differs'
    stack=original['stack'];info,source,block,cursor=stack[8],stack[13],stack[11],stack[12]
    state=native['source_index_context'];assert state['generation']==dict(owner='build_source_file_index',after_frees=12),'source-index native generation role differs'
    assert block==address and state['block_address']==other and state['block_size']==size,'source-index temporary block alias differs'
    module=original['memory'][info];count=int.from_bytes(original['memory'][source][:2],'little')
    assert object_aliases[info]==state['module_address'] and int.from_bytes(module[28:32],'little')==source and allocation_aliases[source]==state['source_address'],'source-index actual live source owner differs'
    assert int.from_bytes(module[32:36],'little')==state['source_size']==len(original['memory'][source]) and size==8+count*4,'source-index module source/capacity differs'
    observed=original['source_index_module'];assert observed['address']==address and len(observed['bytes'])==size,'incomplete original source module allocation capacity'
    before=observed['bytes'];after=bytes.fromhex(state['block_bytes']);assert len(after)==size,'incomplete new source module allocation capacity'
    assert before[:4]==after[:4]==module[:2]+count.to_bytes(2,'little') and before[4:8]==after[4:8]==bytes(4),'source-index initialized temporary header differs'
    assert cursor==address+8 and state['offset_cursor']==other+8,'source-index unwritten offset cursor differs'
    assert stack[10]==state['total_files']==original['registers']['ebx']==state['file_index']==0,'source-index first-file counter differs'
    assert original['stack'][3]==original['registers']['esi']==state['name_bytes']==0,'source-index first filename offset differs'
    assert stack[9]==original['stack'][2]==original['esp']+64,'source-index original names sentinel alias differs'
    # Names sentinel is actual16B initialized caller storage; the standard
    # typed argument controller independently binds native DWARF &names.
    assert stack[16:20]==(0,0,0,0),'source-index original name sentinel initial state differs'
    assert stack[14]>>16==0 and stack[15]==address,'source-index original module sentinel count/link differs'
    sentinel=bytes.fromhex(state['module_sentinel_bytes']);assert len(sentinel)==8 and sentinel[2:4]==bytes(2) and int.from_bytes(sentinel[4:8],'little')==other,'source-index native module sentinel count/link differs'
    # moduleWORD at sentinel+0 and offsets+8..end have never been written.
    # They keep each side's own values and are not application input yet.
    assert address not in original['memory'] and str(other) not in native['memory'],'new generation must remain separate from stale-root current memory'
    overlaps=[]
    for index,(left,right) in enumerate(zip(original['main_free_calls'],native['main_free_calls'])):
        if not left['address']:continue
        a=max(address,left['address'])<min(address+size,left['address']+left['size'])
        b=max(other,right['address'])<min(other+size,right['address']+right['size'])
        if a or b:overlaps.append(dict(free_index=index,original_overlap=a,native_overlap=b))
    return dict(allocator_generations_checked=1,temporary_bytes_initialized=8,temporary_unwritten_bytes=size-8,sentinel_own_uninitialized_module_word=True,new_generation_after_frees=12,new_generation_reuses_retired_regions=overlaps,source_module=int.from_bytes(module[:2],'little'),source_count=count,filename=name_argument(original,data,original['stack'][1]))
