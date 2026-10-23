"""Complete consumed cdecl input observations for the public EDIT error family."""
import json

LOGICAL={'editor_rebase_error':2,'editor_bind_error':2,'file_failure':3,'recon_snprintf':4}


def gdb_commands(logical,path):
    assert logical in LOGICAL
    return '''python
import gdb,json,struct
logical=%r
esp=int(gdb.parse_and_eval('$esp'))
read=lambda a,n:bytes(gdb.selected_inferior().read_memory(a,n))
args=struct.unpack('<5I',read(esp,20))
def text(pointer):
    if not pointer:return None
    result=bytearray()
    for n in range(512):
        value=read(pointer+n,1)[0]
        if not value:return result.decode('latin1')
        result.append(value)
    raise AssertionError('Unterminated EDIT actual ABI string')
caller=gdb.newest_frame().older();assert caller
if logical=='file_failure':
    formals=dict(file=text(args[1]),message=args[2],detail=text(args[3]))
elif logical=='recon_snprintf':
    assert caller.name()=='editor_rebase_error'
    caller.select()
    formals=dict(output_is_error_message=args[1]==int(gdb.parse_and_eval('&message[0]')),output_extent=int(gdb.parse_and_eval('sizeof(message)')),capacity=args[2],format=text(args[3]),value=text(args[4]))
else:formals=dict(format=text(args[1]),value=text(args[2]))
state=dict(logical=logical,caller_name=caller.name(),formals=formals)
open(%r,'w').write(json.dumps(state,indent=2)+'\\n')
end
'''%(logical,str(path))


def compare(events,actual):
    logical=actual['logical'];assert logical in LOGICAL
    rows=[e for e in events if e['event']=='editor_error_abi' and e['logical']==logical]
    assert len(rows)==1,'Missing/nonunique EDIT consumed-argument observation'
    original=rows[0];assert original['formals']==actual['formals'],(original,actual)
    if logical=='editor_rebase_error':
        assert actual['caller_name']=='rebase_main' and 0x4fbe0<=original['caller']<0x50260
        assert actual['formals']==dict(format="can't rebase; %s invalid or linked with /FIXED",value='fixed.dll')
    elif logical=='editor_bind_error':
        assert actual['caller_name']=='bind_imports' and 0x4f540<=original['caller']<0x4f880
        assert actual['formals']==dict(format='Unable to locate exports in %s',value='provider.dll')
    elif logical=='recon_snprintf':
        assert actual['caller_name']=='editor_rebase_error' and original['caller']==0x287c5
        assert actual['formals']==dict(output_is_error_message=True,output_extent=340,capacity=340,format="can't rebase; %s invalid or linked with /FIXED",value='fixed.dll')
    else:
        assert actual['caller_name'] in ('new_file_slot','open_file','open_mapped_file','open_buffered_file')
        assert 0x3c000<=original['caller']<0x3d670
        assert actual['formals']['message']==7 and actual['formals']['detail']=='missing.exe'
    return dict(status='pass',formal_slots=LOGICAL[logical],original=original,native=actual,
                scope=('Three fixed cdecl slots plus one consumed variadic DWORD; the caller-owned340-byte output pointer/extent is observed. Prior output bytes are not read on this bounded formatting path; returned text/result are proved separately. No hidden/register input.' if logical=='recon_snprintf' else 'All consumed cdecl formal slots and actual input string data; these diagnostic wrappers/callback have no hidden/register/output formal.'))
