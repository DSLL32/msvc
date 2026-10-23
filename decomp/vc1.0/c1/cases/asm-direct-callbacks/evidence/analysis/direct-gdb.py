"""Loaded only inside GDB by direct-native.py; no original execution."""
import gdb
import json
import struct
from pathlib import Path

settings = json.loads(Path(gdb.convenience_variable('direct_settings').string()).read_text())
output = Path(settings['directory'])
registers = ['eax','ecx','edx','ebx','esi','edi','ebp','esp','eip','eflags','fctrl']
def number(expression):
    return int(gdb.parse_and_eval(expression)) & 0xffffffff
def memory(address, size):
    return bytes(gdb.selected_inferior().read_memory(address,size))
def snapshot():
    operand=gdb.parse_and_eval('asm_operand').dereference()
    address=operand['address'].dereference()
    symbol=address['symbol']
    declaration=symbol.dereference() if int(symbol) else None
    buffers={}
    for name in settings['buffered_streams']:
        stream=number(name)
        if not stream:
            buffers[name]=None
            continue
        ptr,count,base=struct.unpack('<III',memory(stream,12))
        assert 0 <= ptr-base <= 65536,(name,ptr,base)
        buffers[name]=memory(base,ptr-base).hex() if ptr>base else ''
    return dict(registers={r:number('$'+r)for r in registers},
                error_count=number('error_count'),asm_current_token=number('asm_current_token'),
                current_token_value=memory(number('&current_token_value'),8).hex(),
                repair_control=memory(number('&asm_parser_control'),16).hex(),
                type_0000=number('type_0000'),
                asm_state=[number('asm_operand_count'),int(operand['size']),int(operand['flags']),int(address['displacement']),int(address['address_flags']),int(address['flags']),int(address['first_register']),int(address['second_register']),int(bool(int(symbol))),int(declaration['storage'][0]) if declaration else 0,int(declaration['index']) if declaration else 0],
                asm_name=declaration['name'].string() if declaration else '',buffers=buffers)

gdb.execute('set pagination off')
gdb.execute('set confirm off')
gdb.execute('set disable-randomization off')
marker=gdb.Breakpoint('*report_asm_operand_error',internal=True)
gdb.execute('run >'+str(output/'stdout')+' 2>&1')
assert number('$eip')==number('(unsigned)report_asm_operand_error')
before=snapshot()
# GDB's ordinary inferior call allocates a separately owned scratch stack and
# restores the stopped caller context. It never changes compiler-owned globals.
gdb.execute('set $direct_storage = ((void *(*)(unsigned))malloc)(65536)')
storage=number('$direct_storage')
assert storage
assert snapshot()==before,'GDB allocation changed stopped compiler state'
stack=storage+65536-512
words=[number('$eip')]+settings['arguments']
raw=struct.pack('<'+'I'*len(words),*words)+b'\xa5'*(32-4*len(words))
gdb.selected_inferior().write_memory(stack,raw)
function=settings['function']
entry=gdb.Breakpoint(function,internal=True)
gdb.execute('set $esp = '+str(stack))
gdb.execute('set $eip = (unsigned)'+function)
gdb.execute('continue')
assert gdb.newest_frame().name()==function,('wrong direct entry',gdb.newest_frame().name())
slots=[struct.unpack('<I',memory(number('$ebp')+8+4*i,4))[0]for i in range(len(settings['arguments']))]
formals=[number(n)for n in settings['formals']]
assert slots==settings['arguments'] and formals==slots,('direct ABI slots/formals differ',slots,formals,settings['arguments'])
report=dict(before=before,storage=storage,stack=stack,arguments=slots,formals=formals,entry=snapshot(),status='entry')
if settings['body']:
    entry.delete()
    gdb.execute('continue')
    assert number('$eip')==words[0],('direct callback stopped elsewhere',hex(number('$eip')),gdb.newest_frame().name())
    after=snapshot()
    assert after['registers']['esp']==stack+4+4*len(slots),'direct stack cleanup differs'
    for r in ['ebx','esi','edi','ebp','fctrl']:
        assert after['registers'][r]==before['registers'][r],('direct callee-saved register differs',r)
    canary=memory(stack+4+4*len(slots),16)
    assert canary==b'\xa5'*16,'direct stack canary overwritten'
    report.update(after=after,canary=canary.hex(),status='returned')
(output/'state.json').write_text(json.dumps(report,indent=2)+'\n')
gdb.execute('kill')
