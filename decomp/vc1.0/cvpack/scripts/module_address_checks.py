"""Typed return/arguments and bounded native-only malformed contribution guard."""
import copy,json,os,subprocess,tempfile
from pathlib import Path
from unittest.mock import patch
from state import BINARY,load
from objects import original_walks
from module_address_returns import compare,parse

def check(directory):
    directory=Path(directory);config,pe=load();events=(directory/'original-events').read_text();state=json.loads((directory/'native-gdb.json').read_text());rows=json.loads((directory/'native-module-address.json').read_text());old=original_walks(events,json.loads((directory/'walks.json').read_text()));new=state['walks'];original=parse(events)
    compare(config,pe,events,rows,old,new,'hello-debug-dll')
    def run(change):
        a,b=copy.deepcopy((original,rows));change(a,b)
        with patch('module_address_returns.parse',return_value=a):return compare(config,pe,events,b,old,new,'hello-debug-dll')
    def reject(label,action):
        try:action()
        except(AssertionError,KeyError,IndexError):print('rejected:',label)
        else:raise AssertionError('accepted '+label)
    reject('both module-address return inventories missing',lambda:run(lambda a,b:(a.clear(),b.clear())))
    reject('both module-address returns truthy but not original AX1',lambda:run(lambda a,b:[r[0].update(result=2)for r in(a,b)]))
    reject('module-address typed native return wrong',lambda:run(lambda a,b:b[0].update(result=0)))
    reject('module-address native caller wrong',lambda:run(lambda a,b:b[0].update(caller='convert_procedure16')))
    reject('module-address module ownership missing',lambda:run(lambda a,b:b[0].update(module=0)))
    reject('both module-address segment inputs wrong',lambda:run(lambda a,b:[r[0].update(segment=2)for r in(a,b)]))
    reject('both module-address offset inputs wrong',lambda:run(lambda a,b:[r[0].update(offset=0x121)for r in(a,b)]))
    reject('both module-address source record bytes omitted',lambda:run(lambda a,b:[r[0].update(record='')for r in(a,b)]))
    reject('module-address source owner moved',lambda:run(lambda a,b:b[0].update(input=b[0]['input']+2)))
    reject('module-address return probe not rearmed',lambda:compare(config,pe,events.replace('module-address-rearm 000257c5 000257c8\n',''),rows,old,new,'hello-debug-dll'))
    run(lambda a,b:a[0].update(result=a[0]['result']^0x10000,segment=a[0]['segment']^0x10000))
    # No original malformed input is run. Invoke the native-only guard directly
    # with declared48-byte module and64-byte record scratch inside type_path.
    for size,count,label in((7,0,'short module header'),(8,0xffff,'oversized module contribution count')):
        commands='''set pagination off
set confirm off
set disable-randomization off
break main
run
python
import gdb
base=int(gdb.parse_and_eval('&type_path'))
raw=bytearray(128)
raw[12:16]=(base+64).to_bytes(4,'little')
raw[16:20]= (SIZE).to_bytes(4,'little')
raw[68:70]= (COUNT).to_bytes(2,'little')
gdb.selected_inferior().write_memory(base,raw)
stack=int(gdb.parse_and_eval('$esp'))-32
args=int(gdb.parse_and_eval('$eip')).to_bytes(4,'little')+base.to_bytes(4,'little')+(1).to_bytes(4,'little')+(0x120).to_bytes(4,'little')
gdb.selected_inferior().write_memory(stack,args)
gdb.execute('set $esp='+str(stack))
gdb.execute('set $eip=address_in_module')
end
continue
python
assert gdb.newest_frame().name()=='address_in_module'
assert int(gdb.parse_and_eval('$_siginfo.si_signo'))==5
assert bytes(gdb.selected_inferior().read_memory(base,128))==raw
print('NATIVE_MODULE_BOUND_GUARD_PASS')
end
'''.replace('SIZE',str(size)).replace('COUNT',str(count))
        with tempfile.TemporaryDirectory(dir='/root/.ai-tmp/vc1.0-cvpack',prefix='module-guard-')as scratch:
            script=Path(scratch)/'guard.gdb';script.write_text(commands);run_result=subprocess.run(['gdb','--batch','-nx','-x',str(script),str(BINARY/'build/native/CVPACK.EXE')],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env={k:v for k,v in os.environ.items()if k not in('RECON_TRACE_FD','RECON_SEEN_FD')},timeout=20)
            assert run_result.returncode==0 and b'NATIVE_MODULE_BOUND_GUARD_PASS'in run_result.stdout,run_result.stdout.decode('latin1')
            print('rejected: native unsupported '+label)
