"""Inline recursion state and the native unproved expanded-list guard."""
import copy,json,subprocess,tempfile
from pathlib import Path
from state import BINARY,load
from objects import original_walks
from buffer_storage import compare,parse
from unittest.mock import patch

def check(directory):
    directory=Path(directory);config,pe=load();events=(directory/'original-events').read_text();state=json.loads((directory/'native-gdb.json').read_text());native=json.loads((directory/'native-buffer-storage.json').read_text());old=original_walks(events,json.loads((directory/'walks.json').read_text()));new=state['walks'];rows=parse(events)
    compare(config,pe,events,native,old,new,state,'hello-debug-dll')
    def reject(label,change):
        a,b=copy.deepcopy((old,new));change(a,b)
        try:compare(config,pe,events,native,a,b,state,'hello-debug-dll')
        except(AssertionError,KeyError):print('rejected:',label)
        else:raise AssertionError('accepted '+label)
    def changed(raw,offset,value,size=2):
        data=bytearray.fromhex(raw);data[offset:offset+size]=value.to_bytes(size,'little');return data.hex()
    def missing_write(a,b):
        for w in(a,b):
            table=w['local_type_records'][0];pointer=int.from_bytes(bytes.fromhex(table['bytes'])[(4208-4096)*28:(4208-4096)*28+4],'little');block=w['current_type_server'][0]['objects']['type-storage-1'];at=pointer-block['address']+134;block['bytes']=changed(block['bytes'],at,4207)
    reject('both pending final member conversion writes omitted',missing_write)
    def wrong_offset(a,b):
        row=b['local_type_records'][0];row['bytes']=changed(row['bytes'],(4208-4096)*28+12,53,1)
    reject('inline recursive offset differs from observed argument54',wrong_offset)
    from audit import audited_graph,publics
    f=next(r for r in config['functions']if r['name']=='add_recursive_offset');start,end=f['extent']
    try:audited_graph(pe,start,end)
    except AssertionError:print('rejected: recursive-offset original flag-writing filler undeclared')
    else:raise AssertionError('accepted undeclared recursive filler')
    from binary import PE
    rebuilt=PE(BINARY/'build/CVPACK.EXE');linked=publics();start=linked[f['symbol']];end=min(v for v in linked.values()if v>start)
    expected={linked['_checked_zero'],linked['_checked_realloc']};graph=audited_graph(rebuilt,start,end);assert graph['edges']==expected
    from capstone import Cs,CS_ARCH_X86,CS_MODE_32
    instructions=Cs(CS_ARCH_X86,CS_MODE_32).disasm(rebuilt.read(start,end-start),start);call=next(i for i in instructions if i.mnemonic=='call'and i.op_str=='0x%x'%linked['_checked_zero'])
    mutated=copy.copy(rebuilt);mutated.data=bytearray(rebuilt.data);offset=mutated.offset(call.address+1);mutated.data[offset:offset+4]=((linked['_checked_alloc']-call.address-5)&0xffffffff).to_bytes(4,'little')
    assert audited_graph(mutated,start,end)['edges']!=expected
    print('rejected: expanded recursion allocation loses original zero-helper identity')
    with tempfile.TemporaryDirectory(prefix='expanded-recursion-guard-',dir=Path.home()/'.ai-tmp/vc1.0-cvpack')as folder:
        folder=Path(folder);result=folder/'result.json';commands=folder/'commands.gdb'
        commands.write_text('set pagination off\nset confirm off\nset startup-with-shell off\nset disable-randomization off\nbreak main\nrun\npython\nimport gdb,json\naddress=int(gdb.parse_and_eval("&type_path[0]"))\ngdb.selected_inferior().write_memory(address,b"\\0"*28)\ngdb.execute("set $ecx = %d"%address)\ngdb.execute("set $edx = 256")\ngdb.execute("set $pc = add_recursive_offset")\ngdb.execute("continue")\nf=gdb.newest_frame()\nassert int(gdb.parse_and_eval("$_siginfo.si_signo"))==5 and f.name()=="add_recursive_offset"\nassert int(f.read_var("entry"))==address and int(f.read_var("offset"))==256\npc=int(gdb.parse_and_eval("$pc"));assert bytes(gdb.selected_inferior().read_memory(pc-1,1))==b"\\xcc"\nassert bytes(gdb.selected_inferior().read_memory(address,28))==b"\\0"*28\njson.dump(dict(signal=5,unchanged=True),open('+repr(str(result))+',"w"))\nend\nquit\n')
        completed=subprocess.run(['gdb','-q','-batch','-x',str(commands),str(BINARY/'build/native/CVPACK.EXE')],cwd=folder,capture_output=True)
        assert completed.returncode==0,completed.stdout.decode('latin1')+completed.stderr.decode('latin1')
        assert json.loads(result.read_text())==dict(signal=5,unchanged=True)
    print('rejected: native unproved expanded recursion before allocation/write')
