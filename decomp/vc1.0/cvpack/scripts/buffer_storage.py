"""Observed type buffer writes and fully owned published hash nodes.

Malloc payload/padding remain raw evidence: only each observed copy's bytes
become defined. No allocator extent, initial-zero, or universal default claim.
"""
import hashlib,json,re
from objects import namespace,pointer_identity
from state import BINARY

def word(raw,offset,size=4):return int.from_bytes(raw[offset:offset+size],'little')

def required(config,case):return case=='hello-debug-dll'and any(r['name']=='append_buffer'and r['status']!='stub'for r in config['functions'])

def validate(config,pe):
    ref=config.get('buffer_storage_observations')
    assert ref or not required(config,'hello-debug-dll'),'missing type buffer lifetime proof'
    if not ref:return
    path=BINARY/ref['evidence'];assert hashlib.sha256(path.read_bytes()).hexdigest()==ref['evidence_sha256'],'stale type buffer proof'
    p=json.loads(path.read_text())
    assert p['schema']==1 and p['root']==0x3301c and p['block_size']==0x7f00 and p['node_size']==12 and p['maximum_blocks']==1
    assert p['record_indices']==[4115,4133,4134]and p['copy_sizes']==[8,12,8]and p['frontier']==0x262d0
    assert p['original_sites']==dict(entry=0x22f40,allocated=0x22f6d,returned=0x19a65)
    for source,digest in p['sources'].items():assert hashlib.sha256((BINARY/source).read_bytes()).hexdigest()==digest,'type buffer source binding differs'
    for r in p['instructions']:assert hashlib.sha256(pe.read(r['start'],r['end']-r['start'])).hexdigest()==r['sha256'],'type buffer instruction ownership differs'
    lines=(BINARY/'src/types.c').read_text().splitlines();line=p['native_allocation_line']
    assert lines[line-2].strip()=='payload = checked_alloc(buffer->block_size);'and lines[line-1].strip()=='block->data = payload;','type buffer allocation site differs'
    assert p['pdb_defined_writes']==[dict(type_index=4133,field=10,before=4115,after=0x1000),dict(type_index=4134,field=6,before=4133,after=0x1001)],'unreviewed PDB in-place writes'
    assert p['defined_hash_node_ranges']==[[0,2],[4,12]]and p['reserved_hash_node_range']==[2,4]
    if 'local_lookup_frontier'in p:assert p['local_lookup_frontier']==0x1ae40,'unreviewed nested local lookup lifetime'
    if 'global_lookup_frontier'in p:assert p['global_lookup_frontier']==0x1af10,'unreviewed nested global lookup lifetime'
    if p.get('extension'):
        assert p['extension']==dict(frontier=0x1ad40,record_indices=[4115,4133,4134,4193,4194,4195],copy_sizes=[8,12,8,16,12,8],pdb_writes=[dict(type_index=4194,field=10,before=4193,after=0x1003),dict(type_index=4195,field=6,before=4194,after=0x1004)]),'unreviewed type buffer extension'
    return p


def parse(events):
    result=dict(entries=[],returns=[],allocations=[],hash_nodes={});current=None
    for line in events.splitlines():
        f=line.split()
        if line.startswith('buffer-entry '):
            assert len(f)==8
            current=dict(zip(('sequence','buffer','source','size','owner','esp','return_address'),[int(v,10 if i in(0,3)else 16)for i,v in enumerate(f[1:])]))
            result['entries'].append(current)
        elif line.startswith('buffer-allocation '):
            assert len(f)==6
            current=dict(zip(('base','node','size','buffer','esp'),[int(v,10 if i==2 else 16)for i,v in enumerate(f[1:])]))
            result['allocations'].append(current)
        elif line.startswith('buffer-return '):
            assert len(f)==4
            current=dict(sequence=int(f[1]),result=int(f[2],16),esp=int(f[3],16));result['returns'].append(current)
        elif line.startswith('buffer-bytes '):
            assert len(f)==4
            label,address,raw=f[1],int(f[2],16),f[3]
            if label.startswith('hash-'):
                assert label not in result['hash_nodes'],'duplicate type hash object'
                result['hash_nodes'][label]=dict(address=address,bytes=raw);continue
            mapping={'entry-root':'root','source':'source_bytes','birth-payload':'before','birth-node':'node_before','birth-root':'root_before','return-root':'root','return-node':'node','return-payload':'payload'}
            if label.startswith('frontier-'):
                current=result.setdefault('frontier',dict(name='finish_recursive_types'));key=label[9:]
            else:
                assert current is not None and label in mapping,'unowned buffer memory event';key=mapping[label]
            assert key not in current,'duplicate buffer state field'
            current[key]=raw;current[key+'_address']=address
    if 'frontier'in result:
        match=re.search(r'^exception 80000003 va ([0-9a-f]+) ',events,re.M)
        assert match and int(match[1],16)in(0x262d0,0x1ad40,0x1af10,0x1ae40,0x19770,0x256f0,0x1e700,0x27b10,0x1ac80,0x1c640,0x1eac0,0x19ca0,0x1c880,0x19c20,0x257a0,0x25920,0x254f0,0x25530,0x25480,0x1bd60),'unreviewed original type buffer frontier'
        result['frontier']['name']='finish_recursive_types'if int(match[1],16)==0x262d0 else('find_named_type'if int(match[1],16)==0x1ad40 else('find_global_type'if int(match[1],16)==0x1af10 else('find_local_type'if int(match[1],16)==0x1ae40 else('record_derived_types'if int(match[1],16)==0x19770 else('convert_global_data32'if int(match[1],16)==0x256f0 else('get_type_record'if int(match[1],16)==0x1e700 else('add_recursive_offset'if int(match[1],16)==0x27b10 else('intern_type'if int(match[1],16)==0x1ac80 else('hash_recursive_type'if int(match[1],16)==0x1c640 else('commit_recursive_type'if int(match[1],16)==0x1eac0 else('publish_recursive_type'if int(match[1],16)==0x19ca0 else('hash_referenced_type'if int(match[1],16)==0x1c880 else('publish_packed_type'if int(match[1],16)==0x19c20 else('convert_procedure32'if int(match[1],16)==0x257a0 else('address_in_module'if int(match[1],16)==0x25920 else('convert_bp_relative32'if int(match[1],16)==0x254f0 else('convert_register_symbol'if int(match[1],16)==0x25530 else('link_symbol_scopes'if int(match[1],16)==0x1bd60 else 'end_symbol_block'))))))))))))))))))
    return result


def compare(config,pe,events,native,old_walks,new_walks,state,case,packed_arena=None):
    p=validate(config,pe)
    if not native:
        assert not required(config,case),'missing mandatory type buffer observations'
        return None
    assert p and case=='hello-debug-dll'and not native.get('error'),'native buffer observer failed'
    old=parse(events);results=[];spaces=[]
    frontier=native['frontier']['name']
    assert frontier in('finish_recursive_types','find_named_type','find_global_type','find_local_type','record_derived_types','convert_global_data32','get_type_record','add_recursive_offset','intern_type','hash_recursive_type','commit_recursive_type','publish_recursive_type','hash_referenced_type','publish_packed_type','convert_procedure32','address_in_module','convert_bp_relative32','convert_register_symbol','end_symbol_block','link_symbol_scopes'),'unsupported type buffer consumer'
    indices=p['record_indices'];sizes=p['copy_sizes'];write_sites=[(1,10,4115,0x1000),(2,6,4133,0x1001)]
    if frontier in('find_named_type','find_global_type','find_local_type','record_derived_types','convert_global_data32','get_type_record','add_recursive_offset','intern_type','hash_recursive_type','commit_recursive_type','publish_recursive_type','hash_referenced_type','publish_packed_type','convert_procedure32','address_in_module','convert_bp_relative32','convert_register_symbol','end_symbol_block','link_symbol_scopes'):
        assert p['extension']==dict(frontier=0x1ad40,record_indices=[4115,4133,4134,4193,4194,4195],copy_sizes=[8,12,8,16,12,8],pdb_writes=[dict(type_index=4194,field=10,before=4193,after=0x1003),dict(type_index=4195,field=6,before=4194,after=0x1004)]),'unreviewed type buffer extension'
        indices=p['extension']['record_indices'];sizes=p['extension']['copy_sizes']
        write_sites +=[(4,10,4193,0x1003),(5,6,4194,0x1004)]
    if frontier=='find_global_type':assert p['global_lookup_frontier']==0x1af10,'unreviewed nested global lookup lifetime'
    if frontier=='find_local_type':assert p['local_lookup_frontier']==0x1ae40,'unreviewed nested local lookup lifetime'
    if frontier=='record_derived_types':
        assert p['derived_frontier']==dict(frontier=0x19770,record_indices=[4115,4133,4134,4193,4194,4195,4191,4192],copy_sizes=[8,12,8,16,12,8,64,24],pdb_write=dict(type_index=4192,field=6,before=4191,after=0x1006)),'unreviewed derived type buffer lifetime'
        indices=p['derived_frontier']['record_indices'];sizes=p['derived_frontier']['copy_sizes']
        write_sites +=[(7,6,4191,0x1006)]
    if frontier=='convert_global_data32':
        assert p['global_data_frontier']=={'frontier': 153328, 'record_indices': [4115, 4133, 4134, 4193, 4194, 4195, 4191, 4192, 4183, 4184, 4185, 4102, 4131, 4132, 4103, 4112, 4108, 4109, 4110, 4169, 4170, 4171, 4106, 4152, 4153, 4111], 'copy_sizes': [8, 12, 8, 16, 12, 8, 64, 24, 12, 108, 32, 16, 12, 8, 12, 8, 8, 12, 8, 12, 28, 32, 12, 12, 8, 8], 'pdb_writes': [[1, 10, 4115, 4096], [2, 6, 4133, 4097], [4, 10, 4193, 4099], [5, 6, 4194, 4100], [7, 6, 4191, 4102], [9, 34, 4097, 4103], [9, 90, 4183, 4104], [10, 6, 4184, 4105], [12, 10, 4102, 4107], [13, 6, 4131, 4108], [14, 10, 4102, 4107], [15, 6, 4103, 4110], [17, 10, 4108, 4112], [18, 6, 4109, 4113], [20, 6, 4169, 4115], [21, 6, 4170, 4116], [23, 10, 4106, 4118], [24, 6, 4152, 4119]]},'unreviewed completed module type copies'
        indices=p['global_data_frontier']['record_indices'];sizes=p['global_data_frontier']['copy_sizes'];write_sites=[tuple(r)for r in p['global_data_frontier']['pdb_writes']]
    if frontier=='get_type_record':
        assert p['resolver_frontier']=={'frontier': 124672, 'record_indices': [4115, 4133, 4134, 4193, 4194, 4195, 4191, 4192, 4183, 4184, 4185, 4102, 4131, 4132, 4103, 4112, 4108, 4109, 4110, 4169, 4170, 4171, 4106, 4152, 4153, 4111, 4099, 4175, 4176, 4177, 4221, 4222, 4223, 4224, 4225], 'copy_sizes': [8, 12, 8, 16, 12, 8, 64, 24, 12, 108, 32, 16, 12, 8, 12, 8, 8, 12, 8, 12, 28, 32, 12, 12, 8, 8, 12, 12, 56, 32, 52, 32, 12, 68, 24], 'pdb_writes': [[1, 10, 4115, 4096], [2, 6, 4133, 4097], [4, 10, 4193, 4099], [5, 6, 4194, 4100], [7, 6, 4191, 4102], [9, 34, 4097, 4103], [9, 90, 4183, 4104], [10, 6, 4184, 4105], [12, 10, 4102, 4107], [13, 6, 4131, 4108], [14, 10, 4102, 4107], [15, 6, 4103, 4110], [17, 10, 4108, 4112], [18, 6, 4109, 4113], [20, 6, 4169, 4115], [21, 6, 4170, 4116], [23, 10, 4106, 4118], [24, 6, 4152, 4119], [28, 42, 4175, 4123], [29, 6, 4176, 4124], [31, 6, 4221, 4126], [33, 54, 4223, 4128], [34, 6, 4224, 4129]], 'pending_write': {'type_index': 4219, 'field': 6, 'before': 4217, 'after': 4130, 'converted_input_type': 4225}},'unreviewed pending resolver type copies'
        indices=p['resolver_frontier']['record_indices'];sizes=p['resolver_frontier']['copy_sizes'];write_sites=[tuple(r)for r in p['resolver_frontier']['pdb_writes']]
    if frontier=='add_recursive_offset':
        assert p['recursive_offset_frontier']=={'frontier': 162576, 'record_indices': [4115, 4133, 4134, 4193, 4194, 4195, 4191, 4192, 4183, 4184, 4185, 4102, 4131, 4132, 4103, 4112, 4108, 4109, 4110, 4169, 4170, 4171, 4106, 4152, 4153, 4111, 4099, 4175, 4176, 4177, 4221, 4222, 4223, 4224, 4225, 4219, 4220, 4211, 4212, 4213, 4145, 4119, 4121, 4122, 4123, 4124, 4146, 4147, 4148, 4214, 4215, 4216, 4126, 4127, 4128, 4129, 4130, 4172, 4173, 4174, 4163, 4164, 4165, 4166, 4167, 4168, 4196, 4197, 4113, 4114], 'copy_sizes': [8, 12, 8, 16, 12, 8, 64, 24, 12, 108, 32, 16, 12, 8, 12, 8, 8, 12, 8, 12, 28, 32, 12, 12, 8, 8, 12, 12, 56, 32, 52, 32, 12, 68, 24, 48, 40, 8, 252, 32, 8, 8, 8, 16, 12, 8, 8, 12, 8, 12, 52, 36, 8, 8, 16, 12, 8, 16, 12, 8, 16, 12, 8, 8, 12, 8, 28, 28, 204, 32], 'pdb_writes': [[14, 10, 4102, 4107], [17, 10, 4108, 4112], [18, 6, 4109, 4113], [15, 6, 4103, 4110], [68, 22, 4112, 4111], [69, 6, 4113, 4164], [41, 6, 4118, 4117], [42, 6, 4120, 4125], [43, 8, 4119, 4137], [43, 10, 4121, 4138], [44, 10, 4122, 4139], [45, 6, 4123, 4140], [52, 6, 4125, 4147], [53, 6, 4126, 4148], [54, 8, 4119, 4137], [54, 10, 4127, 4149], [55, 10, 4128, 4150], [56, 6, 4129, 4151], [12, 10, 4102, 4107], [13, 6, 4131, 4108], [1, 10, 4115, 4096], [2, 6, 4133, 4097], [40, 6, 4144, 4135], [46, 6, 4145, 4136], [47, 10, 4146, 4142], [48, 6, 4147, 4143], [23, 10, 4106, 4118], [24, 6, 4152, 4119], [61, 10, 4163, 4156], [62, 6, 4164, 4157], [64, 10, 4166, 4159], [65, 6, 4167, 4160], [20, 6, 4169, 4115], [21, 6, 4170, 4116], [57, 12, 4111, 4121], [58, 10, 4172, 4153], [59, 6, 4173, 4154], [28, 42, 4175, 4123], [29, 6, 4176, 4124], [9, 34, 4097, 4103], [9, 90, 4183, 4104], [10, 6, 4184, 4105], [7, 6, 4191, 4102], [4, 10, 4193, 4099], [5, 6, 4194, 4100], [67, 6, 4196, 4162], [37, 6, 4210, 4132], [38, 106, 4211, 4133], [39, 6, 4212, 4134], [50, 38, 4214, 4145], [51, 6, 4215, 4146], [35, 6, 4217, 4130], [35, 26, 4218, 4127], [36, 6, 4219, 4131], [31, 6, 4221, 4126], [33, 54, 4223, 4128], [34, 6, 4224, 4129]], 'pending_write': {'type_index': 4199, 'field': 6, 'before': 4198, 'after': 4209, 'target_type': 4209}},'unreviewed recursive offset type copies'
        indices=p['recursive_offset_frontier']['record_indices'];sizes=p['recursive_offset_frontier']['copy_sizes'];write_sites=[tuple(r)for r in p['recursive_offset_frontier']['pdb_writes']]
    if frontier in('intern_type','hash_recursive_type','commit_recursive_type','publish_recursive_type','hash_referenced_type','publish_packed_type','convert_procedure32','address_in_module','convert_bp_relative32','convert_register_symbol','end_symbol_block','link_symbol_scopes'):
        assert p['intern_frontier']=={'frontier': 109696, 'record_indices': [4115, 4133, 4134, 4193, 4194, 4195, 4191, 4192, 4183, 4184, 4185, 4102, 4131, 4132, 4103, 4112, 4108, 4109, 4110, 4169, 4170, 4171, 4106, 4152, 4153, 4111, 4099, 4175, 4176, 4177, 4221, 4222, 4223, 4224, 4225, 4219, 4220, 4211, 4212, 4213, 4145, 4119, 4121, 4122, 4123, 4124, 4146, 4147, 4148, 4214, 4215, 4216, 4126, 4127, 4128, 4129, 4130, 4172, 4173, 4174, 4163, 4164, 4165, 4166, 4167, 4168, 4196, 4197, 4113, 4114, 4207], 'copy_sizes': [8, 12, 8, 16, 12, 8, 64, 24, 12, 108, 32, 16, 12, 8, 12, 8, 8, 12, 8, 12, 28, 32, 12, 12, 8, 8, 12, 12, 56, 32, 52, 32, 12, 68, 24, 48, 40, 8, 252, 32, 8, 8, 8, 16, 12, 8, 8, 12, 8, 12, 52, 36, 8, 8, 16, 12, 8, 16, 12, 8, 16, 12, 8, 8, 12, 8, 28, 28, 204, 32, 12], 'pdb_writes': [[14, 10, 4102, 4107], [17, 10, 4108, 4112], [18, 6, 4109, 4113], [15, 6, 4103, 4110], [68, 22, 4112, 4111], [69, 6, 4113, 4164], [41, 6, 4118, 4117], [42, 6, 4120, 4125], [43, 8, 4119, 4137], [43, 10, 4121, 4138], [44, 10, 4122, 4139], [45, 6, 4123, 4140], [52, 6, 4125, 4147], [53, 6, 4126, 4148], [54, 8, 4119, 4137], [54, 10, 4127, 4149], [55, 10, 4128, 4150], [56, 6, 4129, 4151], [12, 10, 4102, 4107], [13, 6, 4131, 4108], [1, 10, 4115, 4096], [2, 6, 4133, 4097], [40, 6, 4144, 4135], [46, 6, 4145, 4136], [47, 10, 4146, 4142], [48, 6, 4147, 4143], [23, 10, 4106, 4118], [24, 6, 4152, 4119], [61, 10, 4163, 4156], [62, 6, 4164, 4157], [64, 10, 4166, 4159], [65, 6, 4167, 4160], [20, 6, 4169, 4115], [21, 6, 4170, 4116], [57, 12, 4111, 4121], [58, 10, 4172, 4153], [59, 6, 4173, 4154], [28, 42, 4175, 4123], [29, 6, 4176, 4124], [9, 34, 4097, 4103], [9, 90, 4183, 4104], [10, 6, 4184, 4105], [7, 6, 4191, 4102], [4, 10, 4193, 4099], [5, 6, 4194, 4100], [67, 6, 4196, 4162], [37, 6, 4210, 4132], [38, 106, 4211, 4133], [39, 6, 4212, 4134], [50, 38, 4214, 4145], [51, 6, 4215, 4146], [35, 6, 4217, 4130], [35, 26, 4218, 4127], [36, 6, 4219, 4131], [31, 6, 4221, 4126], [33, 54, 4223, 4128], [34, 6, 4224, 4129]], 'pending_write': {'type_index': 4199, 'field': 6, 'before': 4198, 'after': 4209, 'target_type': 4209}, 'pending_copy_write': {'type_index': 4208, 'field': 134, 'before': 4207, 'after': 4166, 'converted_input_type': 4207}},'unreviewed recursive intern type copies'
        indices=p['intern_frontier']['record_indices'];sizes=p['intern_frontier']['copy_sizes'];write_sites=[tuple(r)for r in p['intern_frontier']['pdb_writes']]
    if frontier=='hash_recursive_type':assert p['recursive_hash_frontier']==0x1c640,'unreviewed recursive hash frontier'
    if frontier=='commit_recursive_type':assert p['recursive_commit_frontier']==0x1eac0,'unreviewed recursive commit frontier'
    unpublished=[]
    if frontier=='publish_recursive_type':
        assert p['recursive_publish_frontier']=={'frontier': 105632, 'record_indices': [4115, 4133, 4134, 4193, 4194, 4195, 4191, 4192, 4183, 4184, 4185, 4102, 4131, 4132, 4103, 4112, 4108, 4109, 4110, 4169, 4170, 4171, 4106, 4152, 4153, 4111, 4099, 4175, 4176, 4177, 4221, 4222, 4223, 4224, 4225, 4219, 4220, 4211, 4212, 4213, 4145, 4119, 4121, 4122, 4123, 4124, 4146, 4147, 4148, 4214, 4215, 4216, 4126, 4127, 4128, 4129, 4130, 4172, 4173, 4174, 4163, 4164, 4165, 4166, 4167, 4168, 4196, 4197, 4113, 4114, 4207, 4209], 'copy_sizes': [8, 12, 8, 16, 12, 8, 64, 24, 12, 108, 32, 16, 12, 8, 12, 8, 8, 12, 8, 12, 28, 32, 12, 12, 8, 8, 12, 12, 56, 32, 52, 32, 12, 68, 24, 48, 40, 8, 252, 32, 8, 8, 8, 16, 12, 8, 8, 12, 8, 12, 52, 36, 8, 8, 16, 12, 8, 16, 12, 8, 16, 12, 8, 8, 12, 8, 28, 28, 204, 32, 12, 36], 'unpublished_type_indices': [4167]},'unreviewed pending recursive publication copies'
        indices=p['recursive_publish_frontier']['record_indices'];sizes=p['recursive_publish_frontier']['copy_sizes'];unpublished=p['recursive_publish_frontier']['unpublished_type_indices']
    if frontier in('hash_referenced_type','publish_packed_type','convert_procedure32','address_in_module','convert_bp_relative32','convert_register_symbol','end_symbol_block','link_symbol_scopes'):
        assert p['referenced_hash_frontier']=={'frontier': 116864, 'record_indices': [4115, 4133, 4134, 4193, 4194, 4195, 4191, 4192, 4183, 4184, 4185, 4102, 4131, 4132, 4103, 4112, 4108, 4109, 4110, 4169, 4170, 4171, 4106, 4152, 4153, 4111, 4099, 4175, 4176, 4177, 4221, 4222, 4223, 4224, 4225, 4219, 4220, 4211, 4212, 4213, 4145, 4119, 4121, 4122, 4123, 4124, 4146, 4147, 4148, 4214, 4215, 4216, 4126, 4127, 4128, 4129, 4130, 4172, 4173, 4174, 4163, 4164, 4165, 4166, 4167, 4168, 4196, 4197, 4113, 4114, 4207, 4209, 4208], 'copy_sizes': [8, 12, 8, 16, 12, 8, 64, 24, 12, 108, 32, 16, 12, 8, 12, 8, 8, 12, 8, 12, 28, 32, 12, 12, 8, 8, 12, 12, 56, 32, 52, 32, 12, 68, 24, 48, 40, 8, 252, 32, 8, 8, 8, 16, 12, 8, 8, 12, 8, 12, 52, 36, 8, 8, 16, 12, 8, 16, 12, 8, 16, 12, 8, 8, 12, 8, 28, 28, 204, 32, 12, 36, 164], 'unpublished_type_indices': [4167, 4168]},'unreviewed recursive reference hash copies'
        indices=p['referenced_hash_frontier']['record_indices'];sizes=p['referenced_hash_frontier']['copy_sizes'];unpublished=p['referenced_hash_frontier']['unpublished_type_indices']
        write_sites.append((72,134,4207,4166))
    if frontier=='publish_packed_type':
        assert p['packed_type_frontier']=={'frontier': 105504, 'record_indices': [4115, 4133, 4134, 4193, 4194, 4195, 4191, 4192, 4183, 4184, 4185, 4102, 4131, 4132, 4103, 4112, 4108, 4109, 4110, 4169, 4170, 4171, 4106, 4152, 4153, 4111, 4099, 4175, 4176, 4177, 4221, 4222, 4223, 4224, 4225, 4219, 4220, 4211, 4212, 4213, 4145, 4119, 4121, 4122, 4123, 4124, 4146, 4147, 4148, 4214, 4215, 4216, 4126, 4127, 4128, 4129, 4130, 4172, 4173, 4174, 4163, 4164, 4165, 4166, 4167, 4168, 4196, 4197, 4113, 4114, 4207, 4209, 4208, 4199], 'copy_sizes': [8, 12, 8, 16, 12, 8, 64, 24, 12, 108, 32, 16, 12, 8, 12, 8, 8, 12, 8, 12, 28, 32, 12, 12, 8, 8, 12, 12, 56, 32, 52, 32, 12, 68, 24, 48, 40, 8, 252, 32, 8, 8, 8, 16, 12, 8, 8, 12, 8, 12, 52, 36, 8, 8, 16, 12, 8, 16, 12, 8, 16, 12, 8, 8, 12, 8, 28, 28, 204, 32, 12, 36, 164, 8], 'unpublished_type_indices': [4167, 4168, 4169], 'post_copy_write': {'type_index': 4199, 'field': 6, 'before': 4209, 'after': 4167, 'source': 126127}},'unreviewed packed recursive pointer publication'
        indices=p['packed_type_frontier']['record_indices'];sizes=p['packed_type_frontier']['copy_sizes'];unpublished=p['packed_type_frontier']['unpublished_type_indices']
        write_sites.append((73,6,4198,4209))
    if frontier in('convert_procedure32','address_in_module','convert_bp_relative32','convert_register_symbol','end_symbol_block','link_symbol_scopes'):
        assert p['procedure_frontier']=={'frontier': 153504, 'record_indices': [4115, 4133, 4134, 4193, 4194, 4195, 4191, 4192, 4183, 4184, 4185, 4102, 4131, 4132, 4103, 4112, 4108, 4109, 4110, 4169, 4170, 4171, 4106, 4152, 4153, 4111, 4099, 4175, 4176, 4177, 4221, 4222, 4223, 4224, 4225, 4219, 4220, 4211, 4212, 4213, 4145, 4119, 4121, 4122, 4123, 4124, 4146, 4147, 4148, 4214, 4215, 4216, 4126, 4127, 4128, 4129, 4130, 4172, 4173, 4174, 4163, 4164, 4165, 4166, 4167, 4168, 4196, 4197, 4113, 4114, 4207, 4209, 4208, 4199, 4226, 4227, 4228, 4205, 4206, 4178, 4179, 4180, 4161, 4162, 4136, 4137, 4138, 4139, 4201, 4202, 4203, 4187, 4188, 4189, 4190, 4140, 4141, 4142, 4143, 4159, 4160, 4149, 4150, 4151, 4154, 4155, 4156, 4157, 4181, 4182, 4116, 4117, 4096], 'copy_sizes': [8, 12, 8, 16, 12, 8, 64, 24, 12, 108, 32, 16, 12, 8, 12, 8, 8, 12, 8, 12, 28, 32, 12, 12, 8, 8, 12, 12, 56, 32, 52, 32, 12, 68, 24, 48, 40, 8, 252, 32, 8, 8, 8, 16, 12, 8, 8, 12, 8, 12, 52, 36, 8, 8, 16, 12, 8, 16, 12, 8, 16, 12, 8, 8, 12, 8, 28, 28, 204, 32, 12, 36, 164, 8, 12, 192, 36, 340, 28, 12, 12, 8, 100, 28, 8, 12, 12, 8, 8, 52, 36, 8, 8, 12, 8, 8, 12, 12, 8, 96, 24, 24, 12, 8, 8, 12, 12, 8, 12, 8, 12, 8, 12], 'pdb_writes': [[1, 10, 4115, 4096], [2, 6, 4133, 4097], [4, 10, 4193, 4099], [5, 6, 4194, 4100], [7, 6, 4191, 4102], [9, 34, 4097, 4103], [9, 90, 4183, 4104], [10, 6, 4184, 4105], [12, 10, 4102, 4107], [13, 6, 4131, 4108], [14, 10, 4102, 4107], [15, 6, 4103, 4110], [17, 10, 4108, 4112], [18, 6, 4109, 4113], [20, 6, 4169, 4115], [21, 6, 4170, 4116], [23, 10, 4106, 4118], [24, 6, 4152, 4119], [28, 42, 4175, 4123], [29, 6, 4176, 4124], [31, 6, 4221, 4126], [33, 54, 4223, 4128], [34, 6, 4224, 4129], [35, 6, 4217, 4130], [35, 26, 4218, 4127], [36, 6, 4219, 4131], [37, 6, 4210, 4132], [38, 106, 4211, 4133], [39, 6, 4212, 4134], [40, 6, 4144, 4135], [41, 6, 4118, 4117], [42, 6, 4120, 4125], [43, 8, 4119, 4137], [43, 10, 4121, 4138], [44, 10, 4122, 4139], [45, 6, 4123, 4140], [46, 6, 4145, 4136], [47, 10, 4146, 4142], [48, 6, 4147, 4143], [50, 38, 4214, 4145], [51, 6, 4215, 4146], [52, 6, 4125, 4147], [53, 6, 4126, 4148], [54, 8, 4119, 4137], [54, 10, 4127, 4149], [55, 10, 4128, 4150], [56, 6, 4129, 4151], [57, 12, 4111, 4121], [58, 10, 4172, 4153], [59, 6, 4173, 4154], [61, 10, 4163, 4156], [62, 6, 4164, 4157], [64, 10, 4166, 4159], [65, 6, 4167, 4160], [67, 6, 4196, 4162], [68, 22, 4112, 4111], [69, 6, 4113, 4164], [72, 134, 4207, 4166], [73, 6, 4198, 4209], [75, 150, 4226, 4170], [76, 6, 4227, 4171], [77, 102, 4204, 4172], [78, 6, 4205, 4173], [80, 10, 4178, 4175], [81, 6, 4179, 4176], [83, 6, 4161, 4178], [84, 6, 4135, 4179], [85, 10, 4136, 4180], [86, 10, 4137, 4181], [87, 6, 4138, 4182], [88, 6, 4200, 4174], [89, 6, 4199, 4169], [89, 30, 4201, 4184], [90, 6, 4202, 4185], [96, 8, 4140, 4191], [97, 10, 4141, 4192], [98, 6, 4142, 4193], [99, 86, 4158, 4163], [100, 6, 4159, 4195], [102, 10, 4149, 4197], [103, 6, 4150, 4198], [105, 8, 4154, 4200], [106, 10, 4155, 4201], [107, 6, 4156, 4202], [108, 10, 4166, 4159], [109, 6, 4181, 4204], [110, 10, 4115, 4096], [111, 6, 4116, 4206]], 'intercopy_writes': [{'before_copy': 74, 'type_index': 4209, 'field': 6, 'before': 4208, 'after': 4168, 'target_type': 4208, 'source': 126127}, {'before_copy': 74, 'type_index': 4208, 'field': 54, 'before': 4199, 'after': 4169, 'target_type': 4199, 'source': 126127}, {'before_copy': 74, 'type_index': 4199, 'field': 6, 'before': 4209, 'after': 4167, 'target_type': 4209, 'source': 126127}]},'unreviewed completed recursive type group'
        assert packed_arena,'missing actual packed slab parent ownership'
        indices=p['procedure_frontier']['record_indices'];sizes=p['procedure_frontier']['copy_sizes'];write_sites=[tuple(r)for r in p['procedure_frontier']['pdb_writes']];unpublished=[]
    if frontier=='address_in_module':assert p['procedure_address_frontier']==0x25920,'unreviewed module-address frontier'
    if frontier=='convert_bp_relative32':assert p['bp_frontier']==0x254f0,'unreviewed BP-relative frontier'
    if frontier=='convert_register_symbol':assert p['register_frontier']==0x25530,'unreviewed register-symbol frontier'
    if frontier=='end_symbol_block':assert p['end_frontier']==0x25480,'unreviewed end-symbol frontier'
    if frontier=='link_symbol_scopes':
        extension=p['scope_link_frontier'];prior=p['procedure_frontier']
        assert extension==dict(frontier=0x1bd60,record_indices=prior['record_indices']+[4104,4105,4107],copy_sizes=prior['copy_sizes']+[16,12,12],pdb_writes=prior['pdb_writes']+[[114,10,4104,4209],[115,10,4106,4118]]),'unreviewed completed symbol type copies'
        indices=extension['record_indices'];sizes=extension['copy_sizes'];write_sites=[tuple(r)for r in extension['pdb_writes']]
    count=len(indices);node_count=count-len(unpublished)
    for side,walks in((old,old_walks),(native,new_walks)):
        assert len(side['allocations'])==1 and len(side['entries'])==len(side['returns'])==count,'missing/extra type buffer copy lifetime'
        a=side['allocations'][0];assert a['base']and a['node']and a['size']==0x7f00
        assert len(bytes.fromhex(a['before']))==0x7f00 and bytes.fromhex(a['node_before'])==b'\0'*12,'buffer node calloc ownership differs'
        assert word(bytes.fromhex(a['root_before']),0)==0x7f00 and bytes.fromhex(a['root_before'])[4:]==b'\0'*20,'buffer root was already active at allocation'
        block=walks['current_type_server'][0]['objects']['type-storage-1'];defined=bytes.fromhex(block['bytes']);table=walks['local_type_records'][0];table_bytes=bytes.fromhex(table['bytes'])
        master=(BINARY/'cases/hello-debug-dll/input/msvc.pdb').read_bytes()
        assert hashlib.sha256(master).hexdigest()=='6421bc52a3c6b42958d229e92b0b4612dfb5335be933fe4ec995cd66fb30cd28'
        expected=bytearray(master[64:]);writes=[]
        # These are the two actual defined fields consumed by the observed
        # copies, with their preimage indices retained in the frozen PDB.
        for rank,field,prior,replacement in write_sites:
            entry=side['entries'][rank];offset=entry['source']-block['address']+field
            assert 12<=offset<=4150 and word(expected,offset-12,2)==prior,'PDB conversion field/preimage differs'
            expected[offset-12:offset-10]=replacement.to_bytes(2,'little')
            writes.append(dict(offset=offset,before=prior.to_bytes(2,'little').hex(),after=replacement.to_bytes(2,'little').hex(),type_index=indices[rank],field=field))
        if frontier=='get_type_record':
            pending=p['resolver_frontier']['pending_write'];idx=pending['type_index']
            offsets=bytes.fromhex(walks['current_type_server'][0]['objects']['offsets']['bytes'])
            record=bytes.fromhex(table['bytes'])[(idx-0x1000)*28:(idx-0x1000+1)*28]
            source=word(offsets,(idx-0x1000)*4)+2
            assert word(record,0)==source and word(record,24,2)&0x40,'pending member list does not own active provider record'
            start=source-block['address'];offset=start+pending['field']
            assert 12<=start and offset+2<=start+word(expected,start-12,2)+2<=4152 and word(expected,start-10,2)==0x204,'pending member list extent/kind differs'
            assert word(expected,offset-12,2)==pending['before'],'pending member original preimage differs'
            assert indices[pending['after']-0x1000]==pending['converted_input_type'],'pending member lacks converted global type owner'
            expected[offset-12:offset-10]=pending['after'].to_bytes(2,'little')
            writes.append(dict(offset=offset,before=pending['before'].to_bytes(2,'little').hex(),after=pending['after'].to_bytes(2,'little').hex(),type_index=idx,field=pending['field']))
        if frontier in('add_recursive_offset','intern_type','hash_recursive_type','commit_recursive_type','publish_recursive_type','hash_referenced_type'):
            pending=p['recursive_offset_frontier'if frontier=='add_recursive_offset'else 'intern_frontier']['pending_write'];idx=pending['type_index'];target=pending['target_type']
            offsets=bytes.fromhex(walks['current_type_server'][0]['objects']['offsets']['bytes']);records=bytes.fromhex(table['bytes'])
            record=records[(idx-0x1000)*28:(idx-0x1000+1)*28];target_record=records[(target-0x1000)*28:(target-0x1000+1)*28]
            source=word(offsets,(idx-0x1000)*4)+2
            assert word(record,0)==source and word(record,24,2)&0xc0==0xc0 and word(record,4,2)==target,'pending recursive pointer has different compaction owner/result'
            assert word(target_record,24,2)&0x40 and target==4209,'pending recursive target is not the active parent type'
            start=source-block['address'];offset=start+pending['field']
            assert 12<=start and offset+2<=start+word(expected,start-12,2)+2<=4152 and word(expected,start-10,2)==2,'pending recursive pointer extent/kind differs'
            assert word(expected,offset-12,2)==pending['before'],'pending recursive pointer preimage differs'
            expected[offset-12:offset-10]=pending['after'].to_bytes(2,'little')
            writes.append(dict(offset=offset,before=pending['before'].to_bytes(2,'little').hex(),after=pending['after'].to_bytes(2,'little').hex(),type_index=idx,field=pending['field']))
        if frontier in('intern_type','hash_recursive_type','commit_recursive_type','publish_recursive_type'):
            pending=p['intern_frontier']['pending_copy_write'];idx=pending['type_index']
            offsets=bytes.fromhex(walks['current_type_server'][0]['objects']['offsets']['bytes']);records=bytes.fromhex(table['bytes'])
            record=records[(idx-0x1000)*28:(idx-0x1000+1)*28];source=word(offsets,(idx-0x1000)*4)+2
            assert word(record,0)==source and word(record,24,2)&0xc0==0xc0 and word(record,4,2)==4209,'pending recursive list owner/result differs'
            assert word(record,8,2)==1 and record[12]==54,'actual inline recursive offset/count differs'
            start=source-block['address'];offset=start+pending['field']
            assert 12<=start and offset+2<=start+word(expected,start-12,2)+2<=4152 and word(expected,start-10,2)==0x204,'pending recursive list field extent/kind differs'
            assert word(expected,offset-12,2)==pending['before']and indices[pending['after']-0x1000]==pending['converted_input_type'],'pending list last copied member preimage/target differs'
            expected[offset-12:offset-10]=pending['after'].to_bytes(2,'little')
            writes.append(dict(offset=offset,before=pending['before'].to_bytes(2,'little').hex(),after=pending['after'].to_bytes(2,'little').hex(),type_index=idx,field=pending['field']))
        assert defined[12:4152]==expected,'unexpected PDB in-place modification'
        before=bytes.fromhex(a['before']);used=0;copies=[];last=before
        for i,(entry,returned,index,size)in enumerate(zip(side['entries'],side['returns'],indices,sizes)):
            assert entry['sequence']==returned['sequence']==i+1 and entry['size']==size,'buffer call inventory differs'
            assert entry['buffer']==a['buffer']and entry['owner']==table['address']+(index-0x1000)*28,'buffer call parent ownership differs'
            source=entry['source'];offset=source-block['address'];raw=bytes.fromhex(entry['source_bytes'])
            assert 12<=offset and offset+size<=4152 and raw==defined[offset:offset+size]and word(raw,0,2)+2==size,'buffer source not the complete owned defined PDB record'
            root=bytes.fromhex(entry['root']);assert len(root)==24 and word(root,0)==0x7f00
            assert word(root,4)==(0 if i==0 else 1)and word(root,16)==(0 if i==0 else 0x7f00-used),'buffer pre-copy available state differs'
            assert word(root,8)==word(root,20)==(0 if i==0 else a['node'])and word(root,12)==(0 if i==0 else a['base']+used),'buffer pre-copy owner/cursor differs'
            assert returned['result']==a['base']+used,'buffer return address differs'
            if frontier in('convert_procedure32','address_in_module','convert_bp_relative32','convert_register_symbol','end_symbol_block','link_symbol_scopes')and i==74:
                changed=bytearray(last)
                for update in p['procedure_frontier']['intercopy_writes']:
                    owner=next(c for c in copies if c['index']==update['type_index']);target=next(c for c in copies if c['index']==update['target_type']);at=owner['offset']+update['field']
                    record=table_bytes[(owner['index']-4096)*28:(owner['index']-4096+1)*28]
                    assert word(record,0)==owner['result']and word(record,8,2)==1 and record[12]==update['field']and record[20]==0x80,'intercopy store lacks exact recursive owner/offset/mark'
                    assert word(last,at,2)==update['before']and target['result']==word(table_bytes,(target['index']-4096)*28)and word(table_bytes,(target['index']-4096)*28+4,2)==update['after'],'intercopy reference preimage/target differs'
                    changed[at:at+2]=update['after'].to_bytes(2,'little')
                last=bytes(changed)
            post=bytes.fromhex(returned['payload']);assert len(post)==0x7f00 and post[:used]==last[:used]and post[used:used+size]==raw and post[used+size:]==last[used+size:],'unproved buffer write or untouched-tail mutation'
            used+=size;root=bytes.fromhex(returned['root']);node=bytes.fromhex(returned['node'])
            assert len(root)==24 and [word(root,o)for o in(0,4,8,12,16,20)]==[0x7f00,1,a['node'],a['base']+used,0x7f00-used,a['node']],'buffer post-copy root state differs'
            assert len(node)==12 and [word(node,o)for o in(0,4,8)]==[used,a['base'],0],'buffer post-copy block state differs'
            copies.append(dict(index=index,source_offset=offset,size=size,offset=used-size,bytes=raw.hex(),result=returned['result']))
            last=post
        for i,copy in enumerate(copies):
            record=table_bytes[(copy['index']-0x1000)*28:(copy['index']-0x1000+1)*28]
            assert word(record,0)==copy['result']and word(record,4,2)==0x1000+i,'copied type publication lacks actual pointer/index'
        post_writes=[]
        if frontier=='publish_packed_type':
            change=p['packed_type_frontier']['post_copy_write'];copy=copies[73];offset=copy['offset']+change['field']
            assert copy['index']==4199 and copy['size']==8 and word(last,offset,2)==4209,'post-copy recursive pointer preimage differs'
            owner=bytes.fromhex(table['bytes'])[(4199-0x1000)*28:(4199-0x1000+1)*28];target=bytes.fromhex(table['bytes'])[(4209-0x1000)*28:(4209-0x1000+1)*28]
            assert word(owner,0)==copy['result']and word(owner,4,2)==0x1049 and word(owner,8,2)==1 and owner[12]==6 and owner[20]==0x80 and word(owner,24,2)&4,'post-copy defining store lacks exact recursive owner/mark'
            assert word(target,4,2)==0x1047 and word(target,0)==copies[71]['result'],'post-copy recursive replacement lacks actual target'
            modified=bytearray(last);modified[offset:offset+2]=(0x1047).to_bytes(2,'little');last=bytes(modified)
            post_writes.append(dict(type_index=4199,field=6,before=4209,after=0x1047,offset=offset,source=0x1ecaf))
        f=side['frontier'];assert f['name']==frontier and f['root']==side['returns'][-1]['root']and f['node']==side['returns'][-1]['node']and f['payload']==last.hex(),'buffer frontier changed without an observed write'
        extra={'type_buffer_payload':[dict(address=a['base'],bytes=f['payload'])],'type_buffer_block':[dict(address=a['node'],bytes=f['node'])]}
        pool_nodes={}
        if packed_arena:
            pool_side='original'if side is old else 'native';extra.update(packed_arena[pool_side+'_namespace']);pool_nodes=packed_arena[pool_side+'_nodes']
        nodes=side['hash_nodes'];assert len(nodes)==node_count,'missing/extra published type hash nodes'
        heads=bytes.fromhex(walks['type_hash_buckets'][0]['bytes']);counts=bytes.fromhex(walks['type_hash_counts'][0]['bytes']);seen=set();logical=[]
        by_address={r['address']:(key,r)for key,r in nodes.items()};assert len(by_address)==node_count,'aliased hash node allocation'
        for bucket in range(4096):
            pointer=word(heads,bucket*4);rank=0
            while pointer:
                assert pointer in by_address and pointer not in seen,'unowned/cyclic type hash node'
                seen.add(pointer);key,row=by_address[pointer];raw=bytes.fromhex(row['bytes']);assert len(raw)==12
                index=word(raw,0,2);assert key=='hash-%d-%d'%(bucket,index)and 0x1000<=index<0x1000+count and index not in unpublished,'wrong type hash owner/index'
                copy=copies[index-0x1000];assert word(raw,4)==copy['result'],'type hash record pointer does not own observed copy'
                record=table_bytes[(copy['index']-0x1000)*28:(copy['index']-0x1000+1)*28]
                assert word(record,0)==copy['result']and word(record,4,2)==index,'type record publication differs'
                if pointer in pool_nodes:assert row==pool_nodes[pointer],'packed child differs from its actual parent slab field'
                else:extra['type_hash_node_%04x'%index]=[row]
                logical.append(dict(bucket=bucket,index=index,data_offset=copy['offset'],next_index=word(bytes.fromhex(by_address[word(raw,8)][1]['bytes']),0,2)if word(raw,8)else None))
                pointer=word(raw,8);rank+=1
            assert word(counts,bucket*2,2)==rank,'type hash count does not match owned chain'
        assert len(seen)==node_count
        owners=namespace(walks)+namespace(extra)
        new_owners=namespace(extra)
        for address,size,identity in new_owners:
            assert address%4==0 and all(identity==other or not(address<base+length and base<address+size)for base,length,other in owners),'buffer allocation overlaps another typed owner'
        results.append(dict(pdb_writes=writes,post_copy_writes=post_writes,copies=[{k:v for k,v in r.items()if k!='result'}for r in copies],hash_nodes=logical,defined=used,unpublished_type_indices=unpublished,payload_sha256=hashlib.sha256(last[:used]).hexdigest()))
        spaces.append(extra)
    assert results[0]==results[1],'type buffer defined writes/hash topology differ'
    a,b=old['allocations'][0],native['allocations'][0]
    assert a['buffer']==0x3301c and b['buffer']==state['global_addresses']['type_symbol_storage'],'buffer root identity differs'
    for entry,returned in zip(old['entries'],old['returns']):
        assert entry['return_address']==0x19a65 and returned['esp']==entry['esp']+8,'buffer callee cleanup/caller proof differs'
    for site,next_address,count in((0x22f40,0x22f41,count),(0x22f6d,0x22f70,1),(0x19a65,0x19a67,count)):
        assert events.splitlines().count('buffer-rearm %08x %08x'%(site,next_address))==count,'missing per-call buffer observer rearm'
    return dict(comparison=results[0],original_namespace=spaces[0],native_namespace=spaces[1],original=old,native=native)


def compiled_provenance(pe,linked):
    """VC1 emitted allocation result -> same local -> fields -> bucket head."""
    from capstone import Cs,CS_ARCH_X86,CS_MODE_32
    decoder=Cs(CS_ARCH_X86,CS_MODE_32)
    start=linked['@store_type@4'];end=min(v for v in linked.values()if v>start)
    code=list(decoder.disasm(pe.read(start,end-start),start));sites=[i for i,r in enumerate(code)if r.mnemonic=='call'and r.op_str=='0x%x'%linked['_checked_alloc']]
    assert len(sites)==1,'type hash allocator identity differs'
    i=sites[0];call=code[i];assert code[i-1].bytes==b'\x6a\x0c','type hash allocation size differs'
    raw=pe.read(call.address+5,80)
    assert raw[:5]==b'\x83\xc4\x04\x89\x45','type hash allocation return differs'
    slot=raw[5:6];entry=raw[8:9];bucket=raw[33:34]
    heads=linked['_type_hash_buckets'].to_bytes(4,'little');counts=linked['_type_hash_counts'].to_bytes(4,'little')
    expected=(b'\x83\xc4\x04\x89\x45'+slot+b'\x8b\x45'+entry+b'\x8b\x00\x8b\x4d'+slot+b'\x89\x41\x04\x8b\x45'+entry+b'\x66\x8b\x40\x04\x8b\x4d'+slot+b'\x66\x89\x01\x0f\xb7\x45'+bucket+b'\x8b\x0d'+heads+b'\x8b\x04\x81\x8b\x4d'+slot+b'\x89\x41\x08\x8b\x45'+slot+b'\x0f\xb7\x4d'+bucket+b'\x8b\x15'+heads+b'\x89\x04\x8a\x0f\xb7\x45'+bucket+b'\x8b\x0d'+counts+b'\x66\xff\x04\x41')
    assert raw[:len(expected)]==expected,'type hash allocation field/publication def-use differs'
    append=linked['@append_buffer@12'];append_end=min(v for v in linked.values()if v>append)
    instructions=list(decoder.disasm(pe.read(append,append_end-append),append))
    zeros=[j for j,r in enumerate(instructions)if r.mnemonic=='call'and r.op_str=='0x%x'%linked['_checked_zero']]
    mallocs=[j for j,r in enumerate(instructions)if r.mnemonic=='call'and r.op_str=='0x%x'%linked['_checked_alloc']]
    assert len(zeros)==len(mallocs)==1,'buffer allocation callee identity differs'
    j,k=zeros[0],mallocs[0]
    assert instructions[j-1].bytes==b'\x6a\x0c'and k==j+5,'buffer calloc size/ordering differs'
    middle=pe.read(instructions[j].address+5,instructions[k].address-instructions[j].address-5)
    assert middle[:5]==b'\x83\xc4\x04\x89\x45'and middle[6:8]==b'\x8b\x45'and middle[9:]==b'\xff\x30','buffer payload size is not the parent block_size'
    node_slot=middle[5:6];buffer_slot=middle[8:9]
    tail=pe.read(instructions[k].address+5,18)
    assert tail[:5]==b'\x83\xc4\x04\x89\x45','buffer payload result owner differs'
    payload_slot=tail[5:6]
    assert tail[6:]==b'\x8b\x45'+payload_slot+b'\x8b\x4d'+node_slot+b'\x89\x41\x04\x8b\x45'+buffer_slot,'buffer payload->calloc-node->parent def-use differs'
    from type_cache import compiled_provenance as zero_provenance
    zero=zero_provenance(pe,linked)
    return dict(call=call.address,size=12,node_frame_offset=int.from_bytes(slot,'little',signed=True),publication=expected.hex(),zero_helper=zero['zero_helper_sha256'],rebuilt_sha256=hashlib.sha256(pe.data).hexdigest())
