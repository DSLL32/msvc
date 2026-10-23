"""Typed actual cdecl inputs and consumed OMF state at the new public frontiers."""
LOGICAL={'omf_insert_relocation_symbol':2,'omf_lookup_relocation_symbol':2,'omf_extend_section':4,'omf_record_symbol_lines':0,'omf_warning':1,'recon_vfprintf':3}


def gdb_commands(logical,path):
    assert logical in LOGICAL
    return '''python
import gdb,json,struct
logical=%r
esp=int(gdb.parse_and_eval('$esp'))
read=lambda p,n:bytes(gdb.selected_inferior().read_memory(p,n))
words=struct.unpack('<5I',read(esp,20))
value=lambda n:int(gdb.parse_and_eval(n))
caller=gdb.newest_frame().older();assert caller
state=dict(logical=logical,caller_name=caller.name(),**{'pass':value('omf_pass')})
if logical in ('omf_insert_relocation_symbol','omf_lookup_relocation_symbol'):
    state['formals']=dict(symbol=words[1],offset=words[2]);bucket=(words[1]+words[2])%%211
    node=gdb.parse_and_eval('omf_relocation_buckets')[bucket];nodes=[]
    while int(node):
        assert len(nodes)<32
        row=dict(symbol=int(node['symbol']),offset=int(node['offset']))
        if logical=='omf_lookup_relocation_symbol':row['resolved']=int(node['resolved'])
        nodes.append(row);node=node['next']
    state.update(bucket=bucket,nodes=nodes,terminated=True)
elif logical=='omf_extend_section':
    section=struct.unpack('<h',struct.pack('<H',words[1]&65535))[0]
    assert words[4]<=256
    state.update(formals=dict(section=section,offset=words[2],length=words[4],data=list(read(words[3],words[4]))),prior_section_size=value('omf_coff_sections[%%d].SizeOfRawData'%%(section-1)),prior_total_bytes=value('omf_coff_raw_bytes'))
elif logical=='omf_record_symbol_lines':
    remaining=value('omf_record_remaining');assert 0<=remaining<=256
    stream=gdb.parse_and_eval('omf_input');assert int(stream['_cnt'])>=remaining
    state.update(formals={},remaining=remaining,wide=value('omf_record_wide'),unread_record=list(read(int(stream['_ptr']),remaining)))
elif logical=='recon_vfprintf':
    out=struct.unpack('<8I',read(words[1],32));used=out[0]-out[2];assert 0<=used<=256
    state['stream']=dict(count=out[1],flags=out[3],file=out[4],buffer_size=out[6],buffered=list(read(out[2],used)))
    state['errors']=dict(errno=value('(int)errno'),doserrno=value('recon_doserrno'),last_error=value("'win32.c'::last_error"))
    caller.select()
    state['formals']=dict(stream_is_stdout=words[1]==value('&recon_iob[1]'),format=gdb.Value(words[2]).cast(gdb.lookup_type('char').pointer()).string(),variadic_is_warning_area=words[3]==value('&format')+4,consumed_variadic_slots=0)
else:
    state.update(formals=dict(format=gdb.Value(words[1]).cast(gdb.lookup_type('char').pointer()).string()),program=gdb.parse_and_eval('omf_program_name').string())
open(%r,'w').write(json.dumps(state,indent=2)+'\\n')
end
'''%(logical,str(path))


def compare(events,actual):
    logical=actual['logical'];assert logical in LOGICAL
    rows=[e for e in events if e['event']=='omf_continuation_abi' and e['logical']==logical]
    assert len(rows)==1,'Missing/nonunique OMF actual ABI inventory'
    original=rows[0];left={k:v for k,v in original.items() if k not in ('event','caller')};right={k:v for k,v in actual.items() if k!='caller_name'}
    if logical=='recon_vfprintf':
        assert 'stream' in actual and 'stream' in original,'Complete public FILE input state required'
        contexts=[e for e in events if e['event']=='context' and e['eip'] in (0x535f0,0x535f1)]
        assert len(contexts)==1,'Missing actual public vfprintf entry error state'
        left['errors']=dict(errno=contexts[0]['crt_errno'],doserrno=contexts[0]['dos_errno'],last_error=contexts[0]['last_error'])
    assert left==right,(left,right)
    if logical in ('omf_insert_relocation_symbol','omf_lookup_relocation_symbol'):
        assert actual['caller_name']=='omf_record_fixups' and 0x45820<=original['caller']<0x45bf0
        assert actual['formals']==dict(symbol=4,offset=4)
        assert actual['pass']==(1 if logical.startswith('omf_insert') else 2)
    elif logical=='omf_extend_section':
        assert actual['caller_name']=='omf_record_comdat' and 0x466a0<=original['caller']<0x46a10
        assert actual['formals']==dict(section=3,offset=8,length=8,data=list(range(17,25)))
        assert actual['prior_section_size']==8 and actual['prior_total_bytes']==42
    elif logical=='omf_record_symbol_lines':
        assert actual['caller_name']=='omf_convert_pass' and 0x47520<=original['caller']<0x479f0
        assert actual['wide']==0 and actual['remaining']==11 and actual['unread_record'][:2]==[0,7]
    elif logical=='recon_vfprintf':
        assert actual['caller_name']=='omf_warning' and original['caller']==0x4817c
        assert actual['formals']==dict(stream_is_stdout=True,format='16-bit segment',variadic_is_warning_area=True,consumed_variadic_slots=0)
    else:
        assert actual['caller_name']=='omf_record_segments' and 0x461f0<=original['caller']<0x466a0
        assert actual['formals']==dict(format='16-bit segment') and actual['program']=='cvtomf'
    return dict(status='pass',formal_slots=LOGICAL[logical],original=original,native=actual,scope='All actual cdecl formals and consumed pointed-to input/state. No hidden/register arguments. Insertion does not read unresolved node+8; lookup observes it after finish_symbols writer. Line handler consumes exact buffered input bytes and record width/count; warning has no consumed variadic argument for actual plain format.')


def compare_result(events,actual):
    rows=[e for e in events if e['event']=='omf_vfprintf_result']
    contexts=[e for e in events if e['event']=='context' and e['eip'] in (0x4817c,0x4817d)]
    assert len(rows)==len(contexts)==len(actual)==1,'Nonempty public vfprintf result inventory required'
    expected={k:v for k,v in rows[0].items() if k!='event'}
    expected['last_error']=contexts[0]['last_error']
    assert actual==[expected],(actual,expected)
    return expected
