set pagination off
set confirm off
break *recon_unlink
run -link /nodefaultlib /entry:main /subsystem:console /debug:none /out:identity.exe test.obj identity.res >/root/.ai-tmp/msvc2/vc1.0/link/frontiers/resource-identity-duplicate-controlled-recon_unlink-d66thm9d/native.stdout 2>/root/.ai-tmp/msvc2/vc1.0/link/frontiers/resource-identity-duplicate-controlled-recon_unlink-d66thm9d/native.stderr
printf "NATIVE_MATCH recon_unlink\n"
x/6wx $esp
bt 5
python
import gdb,json,struct
logical='recon_unlink'
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
open('/root/.ai-tmp/msvc2/vc1.0/link/frontiers/resource-identity-duplicate-controlled-recon_unlink-d66thm9d/native-resource-abi.json','w').write(json.dumps(state,indent=2)+'\n')
end
quit
