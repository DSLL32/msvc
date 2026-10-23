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
    left=trap['section_records'];right=state['section_records']
    assert len(left)==len(right)==8
    assert trap['output_section_count']==state['output_section_count']==8
    assert state['comdat_arena']==0
    assert trap['section_initializer_this']==trap['validation_context']
    assert state['section_initializer_this']==state['context_this']
    formals=trap['section_group_formals'];native_formals=state['section_group_formals']
    assert formals['name']==trap['stack'][1]==0x4021f4
    assert formals['section']==trap['stack'][2]==left[-1]['address']
    assert native_formals['name'] and native_formals['section']==right[-1]['address']
    assert formals['text']==native_formals['text']=='.edata'
    assert trap['frontier_inputs']==dict(formals=[formals['name'],formals['section']],hidden_register_inputs=[])
    assert state['frontier_inputs']==dict(formals=[native_formals['name'],native_formals['section']],hidden_register_inputs=[])
    assert trap['section_list']==[left[0]['address'],left[-1]['address']+68]
    assert state['section_list']==[right[0]['address'],right[-1]['address']+68]
    expected_handles=[0,1,2,3,4,5,5,5,6,7,None,None,None,None,None,None,None,None,None]
    assert trap['section_handles']==[left[i]['address']if i is not None else 0 for i in expected_handles]
    assert state['section_handles']==[right[i]['address']if i is not None else 0 for i in expected_handles]
    arena=struct.unpack('<4I',(original/'output-section-arena.bin').read_bytes())
    narena=struct.unpack('<4I',bytes.fromhex(state['output_section_arena']['raw']))
    assert arena[:3]==narena[:3]==(0,8*72,4096-8*72)
    assert trap['output_section_arena'] and state['output_section_arena']['address']
    assert arena[3]==left[0]['address']and narena[3]==right[0]['address']
    page=(original/'output-section-page.bin').read_bytes()
    npage=bytes.fromhex(state['output_section_arena']['data'])
    assert len(page)==len(npage)==4096
    ranges=[]
    for i,((name,literal,flags,normalized),l,r)in enumerate(zip(SECTIONS,left,right)):
        assert l['address']==arena[3]+i*72 and r['address']==narena[3]+i*72
        assert l['text']==r['text']==name and l['name']and r['name']
        assert l['next']==(left[i+1]['address']if i<7 else 0)
        assert r['next']==(right[i+1]['address']if i<7 else 0)
        raw=(original/('output-section-%d.bin'%i)).read_bytes();nraw=bytes.fromhex(r['raw'])
        assert len(raw)==len(nraw)==72
        expected=bytearray(72);nexpected=bytearray(72)
        for data,item in [(expected,l),(nexpected,r)]:
            struct.pack_into('<I',data,0,item['name'])
            struct.pack_into('<II',data,32,normalized,normalized)
            struct.pack_into('<I',data,68,item['next'])
        assert raw==expected and nraw==nexpected
        assert page[i*72:(i+1)*72]==raw and npage[i*72:(i+1)*72]==nraw
        contents=name.encode()+b'\0'
        assert(original/('output-section-name-%d.bin'%i)).read_bytes()==bytes.fromhex(r['name_bytes'])==contents
        ranges.append(dict(original_base=l['name'],native_base=r['name'],bytes=len(contents)))
    assert page[8*72:]==npage[8*72:]==bytes(4096-8*72)
    check_calls(original,state,trap,events,left,right)
    # The named copies remain distinct from other live strings and arenas.
    for i,length in enumerate([23,24]):
        ranges.append(dict(original_base=trap['output_names'][i],native_base=state['output_names'][i]['address'],bytes=length))
    lists={e['name']:e for e in events if e['event']=='argument_list'}
    for l,r in zip(lists['link_files']['nodes'],state['canonicalized_arguments']):
        ranges.append(dict(original_base=l['words'][0],native_base=r['result'],bytes=len(l['text'])+1))
    for lp,np,text,ntext in zip(trap['crt_argv_addresses'],state['crt_argv_addresses'],trap['crt_argv'],state['crt_argv']):
        assert text==ntext
        ranges.append(dict(original_base=lp,native_base=np,bytes=len(text)+1))
    return dict(context_roles={0x1b4:(left[0]['address'],right[0]['address']),0x1b8:(left[-1]['address']+68,right[-1]['address']+68)},
                later_arenas=[dict(original_arena=trap['output_section_arena'],native_arena=state['output_section_arena']['address'],original_base=arena[3],native_base=narena[3],bytes=4096)],extra_ranges=ranges)


def check_calls(original,state,trap,events,left,right):
    lookups=list(pairs(events,'section_lookup_observation',8))
    names=list(pairs(events,'section_name_observation',8))
    flags=list(pairs(events,'section_flags_observation',16))
    initializers=list(pairs(events,'section_enum_observation',8))
    steps=list(pairs(events,'section_next_observation',36))
    ends=list(pairs(events,'section_end_observation',8))
    resolutions=list(pairs(events,'section_resolve_observation',8))
    for key,count in [('section_lookup_calls',8),('section_create_calls',8),('section_flag_calls',16),('section_enum_calls',8),('section_next_calls',36),('section_end_calls',8),('section_resolve_calls',8),('link_string_calls',10),('duplicate_calls',15)]:
        assert len(state[key])==count,key
    context=trap['validation_context'];ncontext=state['context_this']
    next_function=state['section_callback_addresses']['next_output_section']
    end_function=state['section_callback_addresses']['end_section_enumerator']
    step=0
    for i,(name,literal,requested,normalized)in enumerate(SECTIONS):
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
        b,a=resolutions[i];call=state['section_resolve_calls'][i]
        returned(b,a,(0x40f219,0x40f21e),4)
        assert b['argument']==a['argument']==a['result']==call['section']==call['result']==0
        assert call['return_stack']==call['entry_stack']+8
        b,a=names[i];call=state['link_string_calls'][2+i];duplicate=state['duplicate_calls'][7+i]
        returned(b,a,(0x40efe2,0x40efe7),4)
        assert b['argument']==a['argument']==literal
        assert a['result']==left[i]['name']and b['text']==a['text']==call['text']==duplicate['text']==name
        assert(original/('section-copied-name-%d.bin'%i)).read_bytes()==name.encode()+b'\0'
        assert call['input']==duplicate['input']==lookup['arguments'][1]
        assert call['result']==duplicate['result']==right[i]['name']
        assert call['return_stack']==call['entry_stack']+8
    assert step==36
