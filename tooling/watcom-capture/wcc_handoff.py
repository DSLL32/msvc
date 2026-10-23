"""Six fixed loader instructions, not an admitted CF application prefix.

No decoder or application policy lives here. The broker independently limits
these exact PCs/bytes/targets and permanently stops after the actual entryCC.
"""
from types import SimpleNamespace
import struct

from ipc import ProtocolError


def require(condition,message):
    if not condition:raise ProtocolError(message)


def enter(broker,event,launch,allocation,entry,initialized):
    base=event['base'];thread=event['tid'];initial_sp=launch['esp']
    stack_base,stack_limit=struct.unpack('<II',broker.read(event['teb']+4,8))
    region=broker.region(initial_sp-12)
    require(region['state']==0x1000 and region['protect'] in (4,8,0x40,0x80),
            'handoff stack is not committed/writable')
    low=max(stack_limit,region['base']);high=min(stack_base,region['base']+region['size'])
    require(low<=initial_sp-12<initial_sp<=high,'handoff implicit stack writes outside actual TEB stack')
    require(not (initial_sp-12<base+0x60000 and base<initial_sp) and
            not (initial_sp-12<allocation+0x91000 and allocation<initial_sp),
            'handoff stack overlaps image/CF allocation')
    require(broker.read(entry,16)==initialized[0x2fe00:0x2fe10],'actual CF entry bytes changed')
    index=broker.wcc_prepare_entry();current=broker.context()
    expected=dict(launch);expected['eip']=base+0x10763
    require(current==expected and broker.read(entry,1)==b'\xcc','CF trap/launch rewind mismatch')
    records=[]
    for instruction,(offset,length) in enumerate(((0x10763,5),(0x10015,1),(0x10016,3),
                                                   (0x10019,2),(0x1001b,2),(0x1001d,2))):
        require(current['eip']==base+offset and not current['eflags']&0x100,'unexpected handoff PC/TF')
        expected=dict(current);expected['eip']+=length
        if instruction==0:expected.update(eip=base+0x10015,esp=current['esp']-4)
        elif instruction==1:expected['esp']-=4
        elif instruction==2:expected['ebx']=(current['ebx']&0xffff0000)|current['cs']
        elif instruction==3:expected['eax']=(current['eax']&0xffff00ff)|0x200
        elif instruction==4:
            expected['ecx']=0;expected['eflags']=(current['eflags']&~0x8d5)|0x44
        else:
            require(current['esi']==entry and broker.read(entry,1)==b'\xcc','unclassified handoff target')
            expected.update(eip=entry,esp=current['esp']-4)
        decision=SimpleNamespace(next_pc=expected['eip'],next_sp=expected['esp'],next_cs=current['cs'])
        armed=broker.arm(current,decision);observed,after=broker.step()
        require(observed['pid']==event['pid'] and observed['tid']==thread and after==expected,
                'handoff actual instruction context differs')
        record={'instruction':instruction,'before':current,'armed':armed,'event':observed,'after':after}
        if instruction in (0,5):
            returned=struct.unpack('<I',broker.read(after['esp'],4))[0]
            require(returned==base+(0x10768 if instruction==0 else 0x1001f),
                    'actual handoff CALL return slot differs')
            record['actual_return']=returned
        elif instruction==1:
            slot=broker.read(after['esp'],4)
            require(struct.unpack_from('<H',slot)[0]==current['cs'],'actual PUSHCS low word differs')
            record['actual_push_cs_hex']=slot.hex()
            record['upper_stack_padding_is_not_a_formal']=True
        records.append(record);current=after
    require(current['esp']==initial_sp-12 and current['eip']==entry and
            broker.read(entry,1)==b'\xcc','actual guarded entry/frame missing')
    frame=broker.read(current['esp'],12)
    require(struct.unpack_from('<I',frame)[0]==base+0x1001f and
            struct.unpack_from('<H',frame,4)[0]==current['cs'] and
            struct.unpack_from('<I',frame,8)[0]==base+0x10768,'actual near/far launch frame differs')
    trap_event,trap_context=broker.wcc_entry_trap(current)
    require(trap_event['pid']==event['pid'] and trap_event['tid']==thread and
            broker.read(entry,1)==b'\xcc' and broker.read(current['esp'],12)==frame,
            'actual entry trap changed frame/target')
    return {'stack':{'teb_base':stack_base,'teb_limit':stack_limit,'region':region,
                     'owned_start':low,'owned_end':high},'steps':records,'trap_index':index,
            'actual_frame_hex':frame.hex(),'entry_db_context':current,
            'actual_int3_event':trap_event,'actual_int3_context':trap_context,
            'original_cf_instructions_executed':0,'ordinary_cf_loop_ready':False}
