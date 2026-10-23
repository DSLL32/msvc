set pagination off
set confirm off
break *editor_rebase_error
run -edit /rebase:base=0x500000 fixed.dll >/root/.ai-tmp/msvc2/vc1.0/link/frontiers/editor-errors-rebase-editor_rebase_error-ioi1_2_p/native.stdout 2>/root/.ai-tmp/msvc2/vc1.0/link/frontiers/editor-errors-rebase-editor_rebase_error-ioi1_2_p/native.stderr
printf "NATIVE_MATCH editor_rebase_error\n"
x/6wx $esp
bt 5
python
import gdb,json,struct
logical='editor_rebase_error'
esp=int(gdb.parse_and_eval('$esp'))
read=lambda a,n:bytes(gdb.selected_inferior().read_memory(a,n))
args=struct.unpack('<4I',read(esp,16))
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
else:formals=dict(format=text(args[1]),value=text(args[2]))
state=dict(logical=logical,caller_name=caller.name(),formals=formals)
open('/root/.ai-tmp/msvc2/vc1.0/link/frontiers/editor-errors-rebase-editor_rebase_error-ioi1_2_p/native-editor-error-abi.json','w').write(json.dumps(state,indent=2)+'\n')
end
quit
