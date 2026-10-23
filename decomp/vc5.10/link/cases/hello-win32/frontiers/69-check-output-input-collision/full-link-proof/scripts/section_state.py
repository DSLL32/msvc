"""Defined section records, copies and ordered calls before the first group."""
import struct

# Actual InitializeSections inputs, in its observed call order.
SECTIONS=[('.bss',0x401cc8,0xc0000080,0xc0000080),
          ('.sdata',0x402214,0xc0000040,0xc0000040),
          ('.rdata',0x401cdc,0x40000040,0x40000040),
          ('.data',0x401cec,0xc0000040,0xc0000040),
          ('.pdata',0x4021cc,0x40000040,0x40000040),
          ('.idata',0x4071c8,0xc0300040,0xc0000040),
          ('.didat',0x4078f4,0xc0300040,0xc0000040),
          ('.edata',0x4021f4,0x40000040,0x40000040)]
ADDITIONAL_SECTIONS=[('.xdata',0x40220c,0x40000040,0x40000040),
                     ('.arch',0x402204,0x42400040,0x42000040),
                     ('.ppcldr',0x4021b0,0x42000040,0x42000040),
                     ('.debug',0x4021fc,0x42100040,0x42000040)]


def pairs(events,kind,count):
    selected=[e for e in events if e['event']==kind]
    assert len(selected)==count*2,kind
    for index in range(count):
        before,after=selected[index*2:index*2+2]
        assert before['index']==after['index']==index
        assert(before['before'],after['before'])==(1,0)
        yield before,after


def returned(before,after,sites,cleanup):
    assert(before['site'],after['site'])==sites
    assert before['entry_stack']==after['entry_stack']==before['stack']
    assert after['stack']==before['stack']+cleanup


def enum_bytes(next_function,end_function,first,current):
    return struct.pack('<4I',next_function,end_function,first,current)


def compare(original,state,trap,events):
    after_collision=state['function']=='open_argument_file'
    collision=state['function']=='check_output_input_collision'or after_collision
    completed=state['function']=='LinkContext::PassOne'or collision
    sections=SECTIONS+ADDITIONAL_SECTIONS if completed else SECTIONS
    count=len(sections)
    left=trap['section_records'];right=state['section_records']
    assert len(left)==len(right)==count
    assert trap['output_section_count']==state['output_section_count']==count
    assert state['comdat_arena']==0
    assert trap['section_initializer_this']==trap['validation_context']
    assert state['section_initializer_this']==state['context_this']
    group_model=None
    if completed:
        context=trap['validation_context'];ncontext=state['context_this']
        formals=trap['pass_one_formals'];nformals=state['pass_one_formals']
        assert formals==dict(this=context,stack_formals=[],other_hidden_inputs=[])
        if collision:
            assert nformals==dict(this=ncontext,stack_formals=[],other_hidden_inputs=[])
            entry,=[e for e in events if e['event']=='pass_one_entry']
            assert entry['this']==formals['this']==context and entry['stack_formals']==entry['other_hidden_inputs']==[]
            assert trap['esp']==entry['stack_pointer']-(48 if after_collision else 44)
            assert trap['stack'][0]==(0x44b8e5 if after_collision else 0x44b8d0)
            nentry=state['pass_one_entry']
            assert nentry['this']==ncontext and nentry['stack_formals']==nentry['other_hidden_inputs']==[]
            if after_collision:
                from collision_state import compare as compare_collision
                compare_collision(original,state,trap,events)
                current=trap['open_file_formals'];ncurrent=state['open_file_formals']
                assert current['name']==trap['stack'][1]==trap['output_name_pointer']==trap['crt_argv_addresses'][3]+5
                assert ncurrent['name']==state['output_name_pointer']==state['crt_argv_addresses'][3]+5
                assert current['flags']==ncurrent['flags']==trap['stack'][2]==0x8302
                assert current['mode']==ncurrent['mode']==trap['stack'][3]==0x180
                assert current['text']==ncurrent['text']=='hello.exe'
                assert trap['frontier_inputs']==dict(formals=[current[k]for k in ('name','flags','mode')],hidden_register_inputs=[])
                assert state['frontier_inputs']==dict(formals=[ncurrent[k]for k in ('name','flags','mode')],hidden_register_inputs=[])
            else:
                current=trap['collision_formals'];ncurrent=state['collision_formals']
                lists={e['name']:e for e in events if e['event']=='argument_list'}
                assert current['output']==trap['stack'][1]==trap['output_name_pointer']==trap['crt_argv_addresses'][3]+5
                assert current['inputs']==trap['stack'][2]==lists['link_files']['header'][0]
                assert ncurrent['output']==state['output_name_pointer']==state['crt_argv_addresses'][3]+5
                assert ncurrent['inputs']==state['argument_lists']['link_files']['header'][0]
                assert current['text']==ncurrent['text']=='hello.exe'
                assert trap['frontier_inputs']==dict(formals=[current['output'],current['inputs']],hidden_register_inputs=[])
                assert state['frontier_inputs']==dict(formals=[ncurrent['output'],ncurrent['inputs']],hidden_register_inputs=[])
            assert trap['pass_one_flags']==state['pass_one_flags']==dict(verbose_link=0,output_file_open=0,current_image_file=-1,arch_data_ignored=0)
        else:
            assert trap['ecx']==context
            assert nformals==dict(this=ncontext,raw_ecx=ncontext,stack_formals=[],other_hidden_inputs=[])
            assert trap['frontier_inputs']==dict(formals=[],hidden_register_inputs=[dict(register='ecx',value=context)])
            assert state['frontier_inputs']==dict(formals=[],hidden_register_inputs=[dict(register='ecx',value=ncontext)])
        assert trap['default_phase']==dict(address=0x407a50,text='Pass1')
        assert state['default_phase']['address']and state['default_phase']['text']=='Pass1'
        observed=[e for e in events if e['event']=='sections_initialize_observation']
        assert len(observed)==2
        before,after=observed
        assert(before['before'],after['before'])==(1,0)
        returned(before,after,(0x442fa1,0x442fa6),0)
        assert before['this']==after['this']==context
        call=state['sections_initializer_call']
        assert call['this']==ncontext and call['normal_return']is True
        assert call['return_stack']==call['entry_stack']+4
        from group_state import records
        group_model=records(original,state,trap)
    else:
        formals=trap['section_group_formals'];native_formals=state['section_group_formals']
        finder=state['function']=='find_contribution_group'
        group_initializer=state['function']=='initialize_group_enumerator'
        group_step=state['function']=='next_contribution_group'
        assert formals['name']==0x4021f4
        assert formals['section']==left[-1]['address']
        assert native_formals['name'] and native_formals['section']==right[-1]['address']
        assert formals['text']==native_formals['text']=='.edata'
        if group_initializer or group_step:
            current=trap['group_enumerator_formals'];ncurrent=state['group_enumerator_formals']
            assert current['enumerator']==trap['stack'][1]==trap['esp']+(0x1c if group_initializer else 0x18)
            assert current['section']==formals['section']
            if group_initializer:assert current['section']==trap['stack'][2]
            assert current['owner_stack']==trap['esp']+(0x30 if group_initializer else 0x2c)
            assert struct.unpack('<2I',(original/'group-finder-inputs.bin').read_bytes())==(formals['section'],formals['name'])
            assert ncurrent['enumerator']==ncurrent['owner_local']and ncurrent['enumerator']
            assert ncurrent['section']==native_formals['section']
            if group_initializer:
                assert current['initial_value_contract']==ncurrent['initial_value_contract']=='uninitialized output record; no incoming bytes consumed'
                assert len((original/'group-enumerator-before.bin').read_bytes())==len(bytes.fromhex(ncurrent['before']))==16
                assert trap['frontier_inputs']==dict(formals=[current['enumerator'],current['section']],hidden_register_inputs=[])
                assert state['frontier_inputs']==dict(formals=[ncurrent['enumerator'],ncurrent['section']],hidden_register_inputs=[])
            else:
                assert current['initial_value_contract']==ncurrent['initial_value_contract']=='fully initialized group enumerator record'
                assert trap['frontier_inputs']==dict(formals=[current['enumerator']],hidden_register_inputs=[])
                assert state['frontier_inputs']==dict(formals=[ncurrent['enumerator']],hidden_register_inputs=[])
                initialized=enum_bytes(0x40fc70,0x40fd90,formals['section'],0)
                ninitialized=enum_bytes(state['group_callback_addresses']['next_contribution_group'],state['group_callback_addresses']['end_section_enumerator'],native_formals['section'],0)
                assert (original/'group-enumerator.bin').read_bytes()==initialized
                assert bytes.fromhex(ncurrent['before'])==ninitialized
                before,after=list(pairs(events,'group_enum_observation',1))[0]
                assert len(state['group_enum_calls'])==1
                call=state['group_enum_calls'][0]
                returned(before,after,(0x40e689,0x40e68e),8)
                assert before['arguments']==after['arguments']==[current['enumerator'],formals['section']]
                assert len((original/'group-enumerator-before-0.bin').read_bytes())==len(bytes.fromhex(call['before']))==16
                assert (original/'group-enumerator-after-0.bin').read_bytes()==initialized
                assert call['enumerator']==ncurrent['enumerator']and call['section']==native_formals['section']
                assert call['return_stack']==call['entry_stack']+12
                assert bytes.fromhex(call['after'])==ninitialized
        else:
            assert formals['name']==trap['stack'][2 if finder else 1]
            assert formals['section']==trap['stack'][1 if finder else 2]
            assert trap['frontier_inputs']==dict(formals=[formals['section'],formals['name']]if finder else[formals['name'],formals['section']],hidden_register_inputs=[])
            assert state['frontier_inputs']==dict(formals=[native_formals['section'],native_formals['name']]if finder else[native_formals['name'],native_formals['section']],hidden_register_inputs=[])
    assert trap['section_list']==[left[0]['address'],left[-1]['address']+68]
    assert state['section_list']==[right[0]['address'],right[-1]['address']+68]
    expected_handles=[0,1,2,3,4,5,5,5,6,7,None,8,9,10,11,None,None,None,None]if completed else[0,1,2,3,4,5,5,5,6,7,None,None,None,None,None,None,None,None,None]
    handles=[left[i]['address']if i is not None else 0 for i in expected_handles]
    nhandles=[right[i]['address']if i is not None else 0 for i in expected_handles]
    if completed:
        for slot,group in [(10,0),(15,1),(16,2),(17,3),(18,4)]:
            handles[slot]=group_model['original'][group]['address'];nhandles[slot]=group_model['native'][group]['address']
    assert trap['section_handles']==handles and state['section_handles']==nhandles
    arena=struct.unpack('<4I',(original/'output-section-arena.bin').read_bytes())
    narena=struct.unpack('<4I',bytes.fromhex(state['output_section_arena']['raw']))
    assert arena[:3]==narena[:3]==(0,count*72,4096-count*72)
    assert trap['output_section_arena'] and state['output_section_arena']['address']
    assert arena[3]==left[0]['address']and narena[3]==right[0]['address']
    page=(original/'output-section-page.bin').read_bytes()
    npage=bytes.fromhex(state['output_section_arena']['data'])
    assert len(page)==len(npage)==4096
    ranges=[]
    for i,((name,literal,flags,normalized),l,r)in enumerate(zip(sections,left,right)):
        assert l['address']==arena[3]+i*72 and r['address']==narena[3]+i*72
        assert l['text']==r['text']==name and l['name']and r['name']
        assert l['next']==(left[i+1]['address']if i<count-1 else 0)
        assert r['next']==(right[i+1]['address']if i<count-1 else 0)
        raw=(original/('output-section-%d.bin'%i)).read_bytes();nraw=bytes.fromhex(r['raw'])
        assert len(raw)==len(nraw)==72
        expected=bytearray(72);nexpected=bytearray(72)
        for data,item in [(expected,l),(nexpected,r)]:
            struct.pack_into('<I',data,0,item['name'])
            struct.pack_into('<II',data,32,normalized,normalized)
            struct.pack_into('<I',data,68,item['next'])
        if completed and i in group_model['section_groups']:
            lg,ng=group_model['section_groups'][i]
            struct.pack_into('<I',expected,64,lg);struct.pack_into('<I',nexpected,64,ng)
        assert raw==expected and nraw==nexpected
        assert page[i*72:(i+1)*72]==raw and npage[i*72:(i+1)*72]==nraw
        contents=name.encode()+b'\0'
        assert(original/('output-section-name-%d.bin'%i)).read_bytes()==bytes.fromhex(r['name_bytes'])==contents
        ranges.append(dict(original_base=l['name'],native_base=r['name'],bytes=len(contents)))
    assert page[count*72:]==npage[count*72:]==bytes(4096-count*72)
    check_calls(original,state,trap,events,left,right,sections,group_model)
    # The named copies remain distinct from other live strings and arenas.
    for i,length in enumerate([23,24]):
        ranges.append(dict(original_base=trap['output_names'][i],native_base=state['output_names'][i]['address'],bytes=length))
    lists={e['name']:e for e in events if e['event']=='argument_list'}
    for l,r in zip(lists['link_files']['nodes'],state['canonicalized_arguments']):
        ranges.append(dict(original_base=l['words'][0],native_base=r['result'],bytes=len(l['text'])+1))
    for lp,np,text,ntext in zip(trap['crt_argv_addresses'],state['crt_argv_addresses'],trap['crt_argv'],state['crt_argv']):
        assert text==ntext
        ranges.append(dict(original_base=lp,native_base=np,bytes=len(text)+1))
    if completed:ranges+=group_model['ranges']
    later=[dict(original_arena=trap['output_section_arena'],native_arena=state['output_section_arena']['address'],original_base=arena[3],native_base=narena[3],bytes=4096)]
    if completed:later.append(group_model['arena'])
    return dict(context_roles={0x1b4:(left[0]['address'],right[0]['address']),0x1b8:(left[-1]['address']+68,right[-1]['address']+68)},
                later_arenas=later,extra_ranges=ranges)


def check_calls(original,state,trap,events,left,right,sections,group_model):
    count=len(sections);completed=group_model is not None
    lookups=list(pairs(events,'section_lookup_observation',count))
    names=list(pairs(events,'section_name_observation',count))
    flags=list(pairs(events,'section_flags_observation',count*2))
    initializers=list(pairs(events,'section_enum_observation',count))
    steps=list(pairs(events,'section_next_observation',count*(count+1)//2))
    ends=list(pairs(events,'section_end_observation',count))
    group_initializer=state['function']in ('initialize_group_enumerator','next_contribution_group')
    resolution_count=17 if completed else 9 if group_initializer else 8
    resolutions=list(pairs(events,'section_resolve_observation',resolution_count))
    for key,expected_count in [('section_lookup_calls',count),('section_create_calls',count),('section_flag_calls',count*2),('section_enum_calls',count),('section_next_calls',count*(count+1)//2),('section_end_calls',count),('section_resolve_calls',resolution_count),('link_string_calls',19 if completed else 10),('duplicate_calls',24 if completed else 15)]:
        assert len(state[key])==expected_count,key
    context=trap['validation_context'];ncontext=state['context_this']
    next_function=state['section_callback_addresses']['next_output_section']
    end_function=state['section_callback_addresses']['end_section_enumerator']
    step=0
    for i,(name,literal,requested,normalized)in enumerate(sections):
        before,after=lookups[i];lookup=state['section_lookup_calls'][i];created=state['section_create_calls'][i]
        arguments=[0,literal,requested,context+0x1b4,context+0x44]
        returned(before,after,(0x40efac,0x40efb1),20)
        assert before['arguments']==after['arguments']==arguments and after['result']==0
        assert lookup['arguments']==created['arguments']
        assert lookup['arguments'][0]==0 and lookup['arguments'][1]
        assert lookup['arguments'][2:]==[requested,ncontext+0x1b4,ncontext+0x44]
        assert lookup['text']==created['text']==name and lookup['result']==0
        assert lookup['return_stack']==lookup['entry_stack']+24
        assert created['result']==right[i]['address']and created['return_stack']==created['entry_stack']+24
        for kind in range(2):
            b,a=flags[i*2+kind];call=state['section_flag_calls'][i*2+kind]
            assert b['kind']==a['kind']==kind
            returned(b,a,[(0x40f15a,0x40f15f),(0x40efbc,0x40efc1)][kind],12)
            assert b['arguments']==a['arguments']
            assert b['arguments'][1:]==[literal,context+0x44]
            assert b['value']==call['before']==requested and a['value']==call['after']==normalized
            assert b['text']==a['text']==call['text']==name
            assert b['initialize_bss']==a['initialize_bss']==call['initialize_bss']==0
            assert b['subsystem']==a['subsystem']==call['subsystem']==0
            assert call['name']==lookup['arguments'][1]and call['header']==ncontext+0x44
            assert call['flags']==call['owner_slot']and call['flags']
            assert call['owner_kind']==('find_matching_output_section'if kind==0 else 'create_output_section')
            assert call['return_stack']==call['entry_stack']+16
            if kind:assert b['arguments'][0]==b['stack']+0x24
        b,a=initializers[i];call=state['section_enum_calls'][i]
        returned(b,a,(0x40f16e,0x40f173),8)
        assert b['arguments']==a['arguments']and b['arguments'][1]==context+0x1b4
        enum=b['arguments'][0];nenum=call['enumerator']
        assert flags[i*2][0]['arguments'][0]==enum-4
        assert call['sections']==ncontext+0x1b4 and call['return_stack']==call['entry_stack']+12
        first=left[0]['address']if i else 0;nfirst=right[0]['address']if i else 0
        initialized=enum_bytes(0x40fc00,0x40fd90,first,0)
        ninitialized=enum_bytes(next_function,end_function,nfirst,0)
        assert(original/('section-enumerator-after-%d.bin'%i)).read_bytes()==initialized
        assert bytes.fromhex(call['after'])==ninitialized
        assert len((original/('section-enumerator-before-%d.bin'%i)).read_bytes())==len(bytes.fromhex(call['before']))==16
        for j in range(i+1):
            b,a=steps[step];call=state['section_next_calls'][step]
            returned(b,a,(0x40f178,0x40f17d)if j==0 else(0x40f1c8,0x40f1cd),4)
            assert b['argument']==a['argument']==enum
            old=left[j-1]['address']if j else 0;nold=right[j-1]['address']if j else 0
            current=left[j]['address']if j<i else 0;ncurrent=right[j]['address']if j<i else 0
            assert(original/('section-next-before-%d.bin'%step)).read_bytes()==enum_bytes(0x40fc00,0x40fd90,first,old)
            assert(original/('section-next-after-%d.bin'%step)).read_bytes()==enum_bytes(0x40fc00,0x40fd90,first,current)
            assert call['enumerator']==nenum
            assert bytes.fromhex(call['before'])==enum_bytes(next_function,end_function,nfirst,nold)
            assert bytes.fromhex(call['after'])==enum_bytes(next_function,end_function,nfirst,ncurrent)
            assert a['result']==(a['eax']&255)==call['result']==(1 if j<i else 0)
            assert call['return_stack']==call['entry_stack']+8
            step+=1
        b,a=ends[i];call=state['section_end_calls'][i]
        returned(b,a,(0x40f1d6,0x40f1db),4)
        assert b['argument']==a['argument']==enum
        assert(original/('section-end-before-%d.bin'%i)).read_bytes()==(original/('section-end-after-%d.bin'%i)).read_bytes()==initialized
        assert call['enumerator']==nenum and bytes.fromhex(call['before'])==bytes.fromhex(call['after'])==ninitialized
        assert call['normal_return']is True and call['return_stack']==call['entry_stack']+8
        resolution=i+1 if completed and i>=8 else i
        b,a=resolutions[resolution];call=state['section_resolve_calls'][resolution]
        returned(b,a,(0x40f219,0x40f21e),4)
        assert b['argument']==a['argument']==a['result']==call['section']==call['result']==0
        assert call['return_stack']==call['entry_stack']+8
        copy=2+i+(1 if completed and i>=8 else 0)
        b,a=names[i];call=state['link_string_calls'][copy];duplicate=state['duplicate_calls'][copy+5]
        returned(b,a,(0x40efe2,0x40efe7),4)
        assert b['argument']==a['argument']==literal
        assert a['result']==left[i]['name']and b['text']==a['text']==call['text']==duplicate['text']==name
        assert(original/('section-copied-name-%d.bin'%i)).read_bytes()==name.encode()+b'\0'
        assert call['input']==duplicate['input']==lookup['arguments'][1]
        assert call['result']==duplicate['result']==right[i]['name']
        assert call['return_stack']==call['entry_stack']+8
    assert step==count*(count+1)//2
    if completed:
        from group_state import calls
        calls(original,state,trap,events,group_model,resolutions)
    if group_initializer:
        before,after=resolutions[-1];native=state['section_resolve_calls'][-1]
        returned(before,after,(0x40e67e,0x40e683),4)
        assert before['argument']==after['argument']==after['result']==left[-1]['address']
        assert native['section']==native['result']==right[-1]['address']
        assert native['return_stack']==native['entry_stack']+8
