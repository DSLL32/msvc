
import gdb,json,struct
from pathlib import Path
out=Path('/root/.ai-tmp/msvc2/vc1.0/link/diagnostic-native-observations/menu-03bv93df/state.json')
def val(x):return int(gdb.parse_and_eval(x))&0xffffffff
def memory(address,size):return bytes(gdb.selected_inferior().read_memory(address,size))
def words(address,count):return list(struct.unpack('<'+'I'*count,memory(address,count*4)))
def snap():
    esp=val('$esp');args=words(esp,3);stream=words(val('&recon_iob[1]'),8);n=stream[0]-stream[2]
    assert 0<=n<=4096
    pointer=val('diagnostic_input');size=val('diagnostic_input_size')
    return dict(eip=val('$eip'),esp=esp,arguments=args,registers={n:val('$'+n) for n in ['eax','ebx','esi','edi','ebp']},
        input_pointer=pointer,input_size=size,payload=memory(pointer,size).hex(),guards=memory(pointer-32,32).hex()+memory(pointer+size,32).hex(),
        stdout_file=stream,buffered=memory(stream[2],n).hex() if n else '',
        errors=[val('*diagnostic_errno'),val('recon_doserrno'),val("'win32.c'::last_error")],
        mb_cur_max=val('recon_mb_cur_max'),ctype=memory(val('recon_pctype')-2,514).hex())
gdb.execute('set pagination off');gdb.execute('set confirm off')
gdb.execute('break *'+'display_legacy_menu')
gdb.execute('run '+'4 /root/msvc2/decomp/vc1.0/link/cases/legacy-resource-diagnostics/inputs/menu.bin'+' >'+'/root/.ai-tmp/msvc2/vc1.0/link/diagnostic-native-observations/menu-03bv93df/stdout.bin'+' 2>'+'/root/.ai-tmp/msvc2/vc1.0/link/diagnostic-native-observations/menu-03bv93df/stderr.bin')
assert val('$eip')==val('&'+'display_legacy_menu'),'wrong native entry'
state=dict(entry=snap())
if True:
    before=state['entry'];gdb.execute('tbreak *'+str(before['arguments'][0]));gdb.execute('continue')
    state['return']=snap();after=state['return']
    assert after['eip']==before['arguments'][0] and after['esp']==before['esp']+4,'cdecl stack return mismatch'
    for n in ['ebx','esi','edi','ebp']:assert before['registers'][n]==after['registers'][n],n
    assert after['payload']==before['payload'] and after['guards']==before['guards'],'input/guard mutation'
    gdb.execute('continue')
    assert gdb.selected_inferior().pid==0,'native did not exit'
out.write_text(json.dumps(state,indent=2)+'\n')
