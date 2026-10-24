"""Five complete contribution groups produced before the first PassOne call."""
import struct

GROUPS=[('.edata',0x4021f4,7,10),('.debug$S',0x401d48,11,15),
        ('.debug$T',0x401d20,11,16),('.debug$P',0x401d2c,11,17),
        ('.debug$F',0x401d14,11,18)]


def observed_pairs(events,kind,count):
    selected=[e for e in events if e['event']=='group_observation'and e['kind']==kind]
    assert len(selected)==count*2
    for i in range(count):
        before,after=selected[i*2:i*2+2]
        assert before['index']==after['index']==i
        assert(before['before'],after['before'])==(1,0)
        yield before,after


def records(original,state,trap):
    left=trap['group_records'];right=state['group_records']
    assert len(left)==len(right)==5
    arena=struct.unpack('<4I',(original/'contribution-group-arena.bin').read_bytes())
    narena=struct.unpack('<4I',bytes.fromhex(state['contribution_group_arena']['raw']))
    assert trap['contribution_group_arena']and state['contribution_group_arena']['address']
    assert arena[:3]==narena[:3]==(0,220,4096-220)
    page=(original/'contribution-group-page.bin').read_bytes()
    npage=bytes.fromhex(state['contribution_group_arena']['data'])
    assert len(page)==len(npage)==4096
    following=[None,2,None,1,3]
    ranges=[]
    for i,((text,literal,section,copy),l,r)in enumerate(zip(GROUPS,left,right)):
        assert l['address']==arena[3]+i*44 and r['address']==narena[3]+i*44
        assert l['name']and r['name']and l['text']==r['text']==text
        raw=(original/('contribution-group-%d.bin'%i)).read_bytes();nraw=bytes.fromhex(r['raw'])
        expected=bytearray(44);nexpected=bytearray(44)
        for output,item,groups,sections in [(expected,l,left,trap['section_records']),(nexpected,r,right,state['section_records'])]:
            struct.pack_into('<I',output,0,item['name']);output[24]=1
            struct.pack_into('<I',output,36,groups[following[i]]['address']if following[i]is not None else 0)
            struct.pack_into('<I',output,40,sections[section]['address'])
        assert raw==expected and nraw==nexpected
        assert page[i*44:(i+1)*44]==raw and npage[i*44:(i+1)*44]==nraw
        contents=text.encode()+b'\0'
        assert(original/('contribution-group-name-%d.bin'%i)).read_bytes()==bytes.fromhex(r['name_bytes'])==contents
        ranges.append(dict(original_base=l['name'],native_base=r['name'],bytes=len(contents)))
    assert page[220:]==npage[220:]==bytes(4096-220)
    return dict(original=left,native=right,
        section_groups={7:(left[0]['address'],right[0]['address']),11:(left[4]['address'],right[4]['address'])},
        arena=dict(original_arena=trap['contribution_group_arena'],native_arena=state['contribution_group_arena']['address'],original_base=arena[3],native_base=narena[3],bytes=4096),ranges=ranges)


def calls(original,state,trap,events,model,resolutions):
    # Local import avoids coupling section_state's older, still immutable phases
    # to this completed initializer contract.
    from section_state import pairs,returned,enum_bytes
    left=model['original'];right=model['native']
    ls=trap['section_records'];ns=state['section_records']
    original_initializers=list(pairs(events,'group_enum_observation',5))
    finds=list(observed_pairs(events,0,5));names=list(observed_pairs(events,1,5))
    steps=list(observed_pairs(events,2,11));ends=list(observed_pairs(events,3,5))
    for key,count in [('group_find_calls',5),('group_enum_calls',5),('group_next_calls',11),('group_end_calls',5)]:
        assert len(state[key])==count,key
    native_callbacks=state['group_callback_addresses']
    callback=native_callbacks['next_contribution_group'];end=native_callbacks['end_section_enumerator']
    histories=[[],[],[1],[1,2],[3,1,2]]
    step=0
    for i,(text,literal,section,copy)in enumerate(GROUPS):
        b,a=finds[i];native=state['group_find_calls'][i]
        returned(b,a,(0x40e70c,0x40e711),8)
        assert b['arguments']==a['arguments']==[ls[section]['address'],literal]
        assert b['text']==a['text']==native['text']==text and a['result']==native['result']==0
        assert native['section']==ns[section]['address']and native['name']
        assert native['return_stack']==native['entry_stack']+12
        b,a=resolutions[[8,13,14,15,16][i]];resolved=state['section_resolve_calls'][[8,13,14,15,16][i]]
        returned(b,a,(0x40e67e,0x40e683),4)
        assert b['argument']==a['argument']==a['result']==ls[section]['address']
        assert resolved['section']==resolved['result']==ns[section]['address']
        assert resolved['return_stack']==resolved['entry_stack']+8
        b,a=original_initializers[i];initialized=state['group_enum_calls'][i]
        returned(b,a,(0x40e689,0x40e68e),8)
        enum=b['arguments'][0];nenum=initialized['enumerator']
        assert b['arguments']==a['arguments']==[b['stack']+24,ls[section]['address']]
        assert nenum==initialized['owner_local']and nenum
        assert initialized['section']==ns[section]['address']
        assert initialized['return_stack']==initialized['entry_stack']+12
        initial=enum_bytes(0x40fc70,0x40fd90,ls[section]['address'],0)
        ninitial=enum_bytes(callback,end,ns[section]['address'],0)
        assert len((original/('group-enumerator-before-%d.bin'%i)).read_bytes())==len(bytes.fromhex(initialized['before']))==16
        assert(original/('group-enumerator-after-%d.bin'%i)).read_bytes()==initial
        assert bytes.fromhex(initialized['after'])==ninitial
        history=histories[i]
        for j in range(len(history)+1):
            b,a=steps[step];call=state['group_next_calls'][step]
            returned(b,a,(0x40e693,0x40e698)if j==0 else(0x40e6da,0x40e6df),4)
            assert b['arguments']==a['arguments']==[enum]and call['enumerator']==nenum
            previous=history[j-1]if j else None;current=history[j]if j<len(history)else None
            for filename,field,index in [('before','before',previous),('after','after',current)]:
                expected=enum_bytes(0x40fc70,0x40fd90,ls[section]['address'],left[index]['address']if index is not None else 0)
                nexpected=enum_bytes(callback,end,ns[section]['address'],right[index]['address']if index is not None else 0)
                assert(original/('group-next-%s-%d.bin'%(filename,step))).read_bytes()==expected
                assert bytes.fromhex(call[field])==nexpected
            assert a['byte_result']==(a['result']&255)==call['result']==(1 if current is not None else 0)
            assert call['return_stack']==call['entry_stack']+8
            step+=1
        b,a=ends[i];call=state['group_end_calls'][i]
        returned(b,a,(0x40e6ec,0x40e6f1),4)
        assert b['arguments']==a['arguments']==[enum]and call['enumerator']==nenum
        assert(original/('group-end-before-%d.bin'%i)).read_bytes()==(original/('group-end-after-%d.bin'%i)).read_bytes()==initial
        assert bytes.fromhex(call['before'])==bytes.fromhex(call['after'])==ninitial
        assert call['normal_return']is True and call['return_stack']==call['entry_stack']+8
        b,a=names[i];call=state['link_string_calls'][copy];duplicate=state['duplicate_calls'][copy+5]
        returned(b,a,(0x40e73c,0x40e741),4)
        assert b['arguments']==a['arguments']==[literal]and a['result']==left[i]['name']
        assert b['text']==a['text']==call['text']==duplicate['text']==text
        assert(original/('group-copied-name-%d.bin'%i)).read_bytes()==text.encode()+b'\0'
        assert call['input']==duplicate['input']==native['name']
        assert call['result']==duplicate['result']==right[i]['name']
        assert call['return_stack']==call['entry_stack']+8
    assert step==11
