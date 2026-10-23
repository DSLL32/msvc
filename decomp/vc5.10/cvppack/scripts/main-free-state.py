"""Finite main cleanup lifetimes; retired bytes are historical inputs only."""
import json
import hashlib
from pathlib import Path

NAMES=['debug_directory','line_data','symbol_data','type_data','module_data']
SITES=[0x404726,0x404731,0x40473d,0x404749,0x404754]
CAPACITIES=['module_capacity','type_segment_bytes','symbol_capacity','line_capacity','type_capacity']


def validate(config,data,original,native,image_aliases,image_sizes,reads):
    before=original.get('main_free_calls',[]);after=native.get('main_free_calls',[])
    if not before and not after:return {},dict(calls=0,retired_allocations=0)
    assert config.get('observe_main_frees') and len(before)==len(after)==5,'main free lifetime inventory differs'
    binary=Path(__file__).resolve().parents[1];binding=config['main_free_policy'];path=binary/binding['path']
    assert hashlib.sha256(path.read_bytes()).hexdigest()==binding['sha256'],'changed main free policy'
    policy=json.loads(path.read_text())
    assert policy['target_sha256']==config['target']['sha256'] and policy['allowed_frontiers']==[dict(name='write_debug_directory',original=0x405230,return_pc=0x40475e)],'unreviewed main free consumer policy'
    assert original['address']==0x405230 and original['stack'][0]==0x40475e and native['frontier_caller']=='cvpack_main','unmodeled target consumer after main free'
    rows={row['name']:row for row in data}
    def value(name):return int.from_bytes(original['memory'][rows[name]['original']],'little')
    def actual(name):return int.from_bytes(bytes.fromhex(native['globals'][name]),'little')
    retired={};initialized=0
    for index,(left,right) in enumerate(zip(before,after)):
        name=NAMES[index];address=value(name);other=actual(name)
        assert left['site']==SITES[index] and right['index']==index and right['name']==name,'main free actual sequence/source differs'
        assert left['address']==address and right['address']==other,'main free source-global argument differs'
        size=value(CAPACITIES[index]) if address else 0
        if index==1:size=min(size,0xffe8)
        assert left['size']==right['size']==size and len(left['before'])==len(bytes.fromhex(right['before']))==size,'main free incomplete pre-free capacity'
        assert left['entry_esp']==left['exit_esp'] and left['stack_argument']==address,'main free original cdecl stack/return differs'
        assert right['exit_esp']==right['entry_esp']+4 and right['returned'] is True and right['owner']=='cvpack_main','main free native cdecl return differs'
        assert left['entry_last_error']==left['exit_last_error']==right['entry_last_error']==right['exit_last_error']==0,'main free observable LastError differs'
        assert left['provider']==original['module_bases']['msvcrt.dll']+policy['free_rva'],'main free actual canonical provider differs'
        for root in NAMES:
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
        assert ranges,'main free lacks initialized input provenance'
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
    for row in data:
        if row['kind']!='pointer':continue
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
    return retired,dict(calls=5,retired_allocations=len(retired),initialized_pre_free_bytes_checked=initialized,
        source_pointer_aliases=aliases,retired_payload_current_state_compared=False,allowed_frontier='write_debug_directory',policy_sha256=binding['sha256'])
