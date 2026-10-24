"""Finite main cleanup lifetimes; retired bytes are historical inputs only."""
import json
import hashlib
from pathlib import Path

NAMES=['debug_directory','line_data','symbol_data','type_data','module_data']
SITES=[0x404726,0x404731,0x40473d,0x404749,0x404754]
CAPACITIES=['module_capacity','type_segment_bytes','symbol_capacity','line_capacity','type_capacity']


def validate(config,data,original,native,image_aliases,image_sizes,reads,later_cleared_indices=()):
    before=original.get('main_free_calls',[]);after=native.get('main_free_calls',[])
    if not before and not after:return {},dict(calls=0,retired_allocations=0)
    scratch=bool(config.get('observe_widen_frees'));tables=bool(config.get('observe_type_table_frees'));count=12 if tables else 7 if scratch else 5
    names=NAMES+(['pack_buffer','pack_fixups'] if scratch else [])
    sites=SITES+([0x40523d,0x405249] if scratch else [])
    capacities=CAPACITIES+(['pack_buffer_size','pack_fixup_count'] if scratch else [])
    if tables:
        names+=['type_hash_buckets','type_table_primary','type_table_secondary','type_table_pending','type_records']
        sites+=[0x40b152,0x40b15e,0x40b16a,0x40b175,0x40b181]
        capacities += [None,None,None,None,'type_record_capacity']
    assert config.get('observe_main_frees') and len(before)==len(after)==count,'main free lifetime inventory differs'
    binary=Path(__file__).resolve().parents[1];binding=config['main_free_policy'];path=binary/binding['path']
    assert hashlib.sha256(path.read_bytes()).hexdigest()==binding['sha256'],'changed main free policy'
    policy=json.loads(path.read_text())
    derived=bool(config.get('observe_derived_finalize_entry'));source_index=bool(config.get('observe_source_index_entry'))
    assert bool(policy.get('source_index_entry'))==source_index,'unreviewed source-index consumer policy'
    complete=bool(config.get('observe_source_index_complete'));assert bool(policy.get('source_index_complete'))==complete,'unreviewed complete source-index policy'
    output_directory=bool(config.get('observe_output_directory'));assert bool(policy.get('output_directory'))==output_directory,'unreviewed output directory consumer policy'
    complete_sort=bool(config.get('observe_complete_directory_sort'));assert bool(policy.get('complete_directory_sort'))==complete_sort,'unreviewed complete sort consumer policy'
    buffer_init=bool(config.get('observe_output_buffer_init'));assert bool(policy.get('output_buffer_init'))==buffer_init,'unreviewed output-buffer consumer policy'
    subsections=bool(config.get('observe_subsection_output'));assert bool(policy.get('subsection_output'))==subsections,'unreviewed subsection consumer policy'
    public_prefix=bool(config.get('observe_public_writer_prefix'));assert bool(policy.get('public_writer_prefix'))==public_prefix,'unreviewed publicwriter consumer policy'
    padding=bool(config.get('observe_public_padding'));assert bool(policy.get('public_padding'))==padding,'unreviewed publicpadding consumer policy'
    name_hash=bool(config.get('observe_name_hash'));assert bool(policy.get('name_hash'))==name_hash,'unreviewed namehash consumer policy'
    complete_hash=bool(config.get('observe_name_hash_complete'));assert bool(policy.get('name_hash_complete'))==complete_hash,'unreviewed completed namehash policy'
    address_hash=bool(config.get('observe_address_hash'));assert bool(policy.get('address_hash'))==address_hash,'unreviewed addresshash consumer policy'
    frontier=dict(name='compare_symbol_address',original=0x40edc0,return_pc=0x7802551e) if address_hash else dict(name='build_symbol_address_hash',original=0x40ede0,return_pc=0x40d68f) if complete_hash else dict(name='fill_type_arena',original=0x410010,return_pc=0x40ebc0) if name_hash else dict(name='build_symbol_name_hash',original=0x40e920,return_pc=0x40d66a) if padding else dict(name='insert_symbol_page_padding',original=0x40f960,return_pc=0x40d62f) if public_prefix else dict(name='write_publics',original=0x40d600,return_pc=0x406c5a) if subsections else dict(name='write_subsection',original=0x4068a0,return_pc=0x406c17) if buffer_init else dict(name='begin_buffered_output',original=0x4100e0,return_pc=0x406b84) if complete_sort else dict(name='compare_packed_directory_entries',original=0x4063f0,return_pc=0x7802551e) if output_directory else dict(name='flush_packed_file',original=0x4069e0,return_pc=0x404776) if complete else dict(name='append_source_name',original=0x40f0d0,return_pc=0x40f1d7) if source_index else dict(name='print_statistics',original=0x40f100,return_pc=0x404771) if tables else dict(name='finalize_derived_types',original=0x40df60,return_pc=0x40b146) if derived else dict(name='free_type_tables',original=0x40b140,return_pc=0x404763) if scratch else dict(name='write_debug_directory',original=0x405230,return_pc=0x40475e)
    assert bool(policy.get('type_table_cleanup'))==tables,'unreviewed table cleanup consumer policy'
    assert bool(policy.get('type_finalize_entry'))==derived,'unreviewed type cleanup entry policy'
    assert bool(policy.get('widen_scratch_cleanup'))==scratch and policy['target_sha256']==config['target']['sha256'] and policy['allowed_frontiers']==[frontier],'unreviewed main free consumer policy'
    assert original['address']==frontier['original'] and original['stack'][0]==frontier['return_pc'] and native['frontier_caller']==('recon_qsort' if address_hash else 'write_publics' if complete_hash else 'build_symbol_name_hash' if name_hash else 'write_publics' if public_prefix else 'flush_packed_file' if complete_sort else 'recon_qsort' if output_directory else 'build_source_file_index' if source_index else 'free_type_tables' if derived else 'cvpack_main'),'unmodeled target consumer after main free'
    rows={row['name']:row for row in data}
    def value(name):return int.from_bytes(original['memory'][rows[name]['original']],'little')
    def actual(name):return int.from_bytes(bytes.fromhex(native['globals'][name]),'little')
    retired={};initialized=0
    for index,(left,right) in enumerate(zip(before,after)):
        name=names[index];address=value(name);other=actual(name)
        assert left['site']==sites[index] and right['index']==index and right['name']==name,'main free actual sequence/source differs'
        assert left['address']==address and right['address']==other,'main free source-global argument differs'
        size=(1536 if index==7 else 16384 if index in (8,9,10) else value(capacities[index])) if address else 0
        if index==1:size=min(size,0xffe8)
        if index==6:size*=8
        if index==11:size*=32
        assert left['size']==right['size']==size and len(left['before'])==len(bytes.fromhex(right['before']))==size,'main free incomplete pre-free capacity'
        assert left['entry_esp']==left['exit_esp'] and left['stack_argument']==address,'main free original cdecl stack/return differs'
        assert right['exit_esp']==right['entry_esp']+4 and right['returned'] is True and right['owner']==('cvpack_main' if index<5 else 'free_widen_buffers' if index<7 else 'free_type_tables'),'main free native cdecl return differs'
        assert left['entry_last_error']==left['exit_last_error']==right['entry_last_error']==right['exit_last_error']==0,'main free observable LastError differs'
        assert left['provider']==original['module_bases']['msvcrt.dll']+policy['free_rva'],'main free actual canonical provider differs'
        for root in names:
            assert int.from_bytes(left['globals'][rows[root]['original']],'little')==value(root) and right['global_roots'][root]==actual(root),'main free source pointers changed/cleared'
        assert right['segment0']==actual('line_data') and right['module_symbol_records']==actual('type_data') and right['symbol_cursor']==actual('symbol_input'),'main free native alias inputs differ'
        if not address:
            assert index==0 and not other and not size,'unmodeled NULL main free'
            continue
        assert image_aliases[address]==other and image_sizes[address]==size and address not in retired,'main free allocation ownership/reuse differs'
        ranges=[(0,4)] if index==1 else []
        for row in original['file_reads']:
            if address<=row['buffer']<address+size:
                ranges.append((row['buffer']-address,row['buffer']-address+row['returned']))
        if index in (5,6):
            scale=1 if index==5 else 8;field=0 if index==5 else 1
            written_prefixes=[]
            for emit,actual_emit in zip(original['widen_emit_calls'],native['widen_emit_calls']):
                import struct
                description=struct.unpack('<4I',emit['after'][emit['description']])
                actual_description=struct.unpack('<4I',bytes.fromhex(actual_emit['description_after']))
                if description[field+2]!=address:continue
                assert actual_description[field+2]==other,'scratch emit belongs to a different native allocation generation'
                assert description[field]==actual_description[field] and description[field]*scale<=size,'scratch initialized prefix exceeds own generation'
                written_prefixes.append(description[field]*scale)
            assert written_prefixes,'scratch free has no same-generation initialized emit writes'
            ranges=[(0,max(written_prefixes))]
        if index>=7:
            assert value('derived_type_count')==0 and not value('type_pdb') and not value('type_pdb_types'),'unmodeled derived/PDB cleanup path'
            cleared=[row for ordinal,row in enumerate(original['cleared_allocations']) if ordinal not in later_cleared_indices and row['address']==address]
            assert len(cleared)==1 and cleared[0]['size']==size,'retired table lacks complete initialized allocation provenance'
            # Pointer-bearing tables are compared by the existing typed table
            # validator on their full PRE-free bytes, separately from current
            # memory. Byte-only cleared tables have no relocated fields here.
            if index in (7,10):
                assert not any(left['before']) and not any(bytes.fromhex(right['before'])),'unmodeled nonzero cleanup table'
                ranges=[(0,size)]
        assert ranges or index in (8,9,11),'main free lacks initialized input provenance'
        merged=[]
        for first,last in sorted(ranges):
            if merged and first<=merged[-1][1]:merged[-1]=(merged[-1][0],max(last,merged[-1][1]))
            else:merged.append((first,last))
        ranges=merged
        current=bytes.fromhex(right['before'])
        for first,last in ranges:
            assert 0<=first<=last<=size and left['before'][first:last]==current[first:last],'main free initialized pre-free bytes differ'
            initialized+=last-first
        assert address not in original['memory'] and str(other) not in native['memory'],'released payload was captured as current state'
        retired[address]=dict(name=name,native=other,size=size,before=left['before'],actual_before=current,initialized=ranges)
    for base,state in retired.items():
        assert not any(max(base,address)<min(base+state['size'],address+len(payload)) for address,payload in original['memory'].items()), 'released payload was captured as current state'
        assert not any(max(state['native'],int(address))<min(state['native']+state['size'],int(address)+len(bytes.fromhex(payload))) for address,payload in native['memory'].items()), 'released payload was captured as current state'
        assert not any(max(base,address)<min(base+state['size'],address+len(payload)) for address,payload in original['strings'].items()), 'released payload was captured as current string state'
        assert not any(max(state['native'],int(address))<min(state['native']+state['size'],int(address)+len(bytes.fromhex(payload))) for address,payload in native['strings'].items()), 'released payload was captured as current string state'
    aliases=[]
    allowed={'line_data','symbol_data','type_data','module_data','module_symbol_records','symbol_input'}
    if scratch:allowed.update(('pack_buffer','pack_fixups'))
    if tables:allowed.update(('type_hash_buckets','type_table_primary','type_table_secondary','type_table_pending','type_records'))
    for row in data:
        if row['kind']!='pointer' or (complete and row.get('source_index_output')):continue
        if buffer_init and row.get('output_buffer_generation'):
            assert row['name']=='output_buffer' and original['output_buffer_allocations'] and native['output_buffer_allocations'] and value(row['name'])==original['output_buffer_allocations'][-1]['address'] and actual(row['name'])==native['output_buffer_allocations'][-1]['address'],'output buffer lacks distinct new allocation identity'
            continue
        address=value(row['name']);other=actual(row['name'])
        owners=[(base,state) for base,state in retired.items() if base<=address<base+state['size']]
        if not owners:continue
        assert len(owners)==1 and row['name'] in allowed,'unreviewed released source pointer alias'
        base,state=owners[0]
        assert other==state['native']+address-base,'released source pointer identity/offset differs'
        aliases.append(dict(name=row['name'],owner=state['name'],offset=address-base))
    assert {row['name'] for row in aliases}==allowed,'released source alias inventory differs'
    # These live tables retain addresses only. Their payload extents are proved
    # from captured PRE-free bytes, never from released allocator storage.
    assert value('line_data')==int.from_bytes(original['memory'][value('type_segments')][:4],'little'),'retired segment table alias differs'
    return retired,dict(calls=count,retired_allocations=len(retired),initialized_pre_free_bytes_checked=initialized,
        source_pointer_aliases=aliases,retired_payload_current_state_compared=False,historical_typed_table_bytes_pending=(16384*2+value('type_record_capacity')*32) if tables else 0,allowed_frontier=frontier['name'],policy_sha256=binding['sha256'])
