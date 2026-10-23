"""Explicit complete formal-input comparisons for active resource diagnostics."""
import json

LOGICAL={'resource_name_to_ansi':3,'resource_unicode_to_ansi_byte':1,'fatal_message':3,'recon_unlink':1}


def gdb_commands(logical,path):
    assert logical in LOGICAL
    return '''python
import gdb,json,struct
logical=%r
esp=int(gdb.parse_and_eval('$esp'))
read=lambda a,n:bytes(gdb.selected_inferior().read_memory(a,n))
args=struct.unpack('<4I',read(esp,16))
def cstring(address):
    result=bytearray()
    for n in range(512):
        value=read(address+n,1)[0]
        if not value:return result.decode('latin1')
        result.append(value)
    raise AssertionError('Unterminated actual ABI string')
caller=gdb.newest_frame().older()
assert caller
caller_name=caller.name()
caller.select()
if logical=='resource_name_to_ansi':
    assert caller_name=='insert_resource'
    count=args[3];assert 0<count<=256
    first=int(gdb.parse_and_eval('type_name'))
    second=int(gdb.parse_and_eval('name'))
    assert args[2] in (first+16,second+16)
    owner=gdb.parse_and_eval('type_name' if args[2]==first+16 else 'name')
    formals=dict(count=count,input_is_type_name=args[2]==first+16,input_is_name=args[2]==second+16,count_matches_owner=count==int(owner['length'])+1,
        output_is_insert_resource_display=args[1]==int(gdb.parse_and_eval('&display[0]')),
        output_capacity=int(gdb.parse_and_eval('sizeof(display)')),
        utf16=list(struct.unpack('<'+'H'*count,read(args[2],count*2))))
elif logical=='resource_unicode_to_ansi_byte':
    assert caller_name=='resource_name_to_ansi'
    formals=dict(character=args[1]&0xffff,argument_word=args[1])
elif logical=='fatal_message':
    assert caller_name=='convert_resource_input'
    formals=dict(file=cstring(args[1]),message=args[2]&0xffff,first_variadic_string=cstring(args[3]))
else:
    assert caller_name=='fatal_message'
    formals=dict(file=cstring(args[1]))
state=dict(logical=logical,caller_name=caller_name,formals=formals)
open(%r,'w').write(json.dumps(state,indent=2)+'\\n')
end
'''%(logical,str(path))


def compare(events,actual):
    logical=actual['logical'];assert logical in LOGICAL
    records=[e for e in events if e['event']=='resource_abi' and e['logical']==logical]
    assert len(records)==1, 'Missing/nonunique complete original resource ABI observation'
    original=records[0]
    assert original['formals']==actual['formals'],(original,actual)
    formals=actual['formals']
    if logical=='resource_name_to_ansi':
        assert actual['caller_name']=='insert_resource' and original['caller'] in (0x43172,0x431ce)
        assert formals['input_is_type_name'] != formals['input_is_name']
        assert formals['count_matches_owner'] and formals['output_is_insert_resource_display']
        assert formals['output_capacity']==1024 and len(formals['utf16'])==formals['count']
        assert formals['utf16'][-1]==0
    elif logical=='resource_unicode_to_ansi_byte':
        assert actual['caller_name']=='resource_name_to_ansi' and original['caller']==0x451b9
        assert formals['argument_word']==formals['character']==0xe9
    elif logical=='fatal_message':
        assert actual['caller_name']=='convert_resource_input' and original['caller']==0x210c2
        assert formals==dict(file='identity.res',message=26,first_variadic_string='')
    else:
        assert actual['caller_name']=='fatal_message' and original['caller']==0x2197d
        assert formals==dict(file='identity.exe')
    return dict(status='pass',formal_slots=LOGICAL[logical],original=original,native=actual,
        scope='All actual consumed formal slots; pointer input data and output ownership/count. Output prior bytes are not read by original and are not inputs. No hidden/register formal exists in these decoded cdecl bodies.')
