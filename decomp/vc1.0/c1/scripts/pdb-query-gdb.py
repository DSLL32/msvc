"""GDB adapter for the nine fixed constructor-owned PDB interface units."""
import gdb,json,struct
from pathlib import Path
settings=json.loads(Path(gdb.convenience_variable('pdb_query_settings').string()).read_text())
output=Path(settings['directory']);unit=settings['unit'];function=settings['function'];calls=[]
u=lambda value:int(gdb.parse_and_eval(value))&0xffffffff
read=lambda address,count:bytes(gdb.selected_inferior().read_memory(address,count))
register_names=['eip','esp','ecx','edx','ebx','esi','edi','ebp','fctrl']
class Returned(gdb.Breakpoint):
    def __init__(self,entry):
        super().__init__('*'+str(entry['return_address']),temporary=True,internal=True);self.entry=entry
    def stop(self):
        entry=self.entry;after={n:u('$'+n)for n in register_names+['eax']}
        assert after['esp']==entry['registers']['esp']+4+settings['pop']
        for n in ['ebx','esi','edi','ebp','fctrl']:assert after[n]==entry['registers'][n]
        entry.update(returned_registers=after,object_after=read(entry['owner'],116).hex())
        return False
class Entry(gdb.Breakpoint):
    def stop(self):
        registers={n:u('$'+n)for n in register_names}
        frame=list(struct.unpack('<'+'I'*(settings['arguments']+1),read(registers['esp'],4*(settings['arguments']+1))))
        text=(output/'report').read_text();import re
        m=re.search(r'^pdb-query-begin unit (\d+) owner (\w+) first (\d+) last (\d+) record (\w+) bytes (\d+) buffer (\w+) length (\w+) allocation (\w+) size65536$',text,re.M);assert m
        owner,index,buffer,length=int(m[2],16),int(m[3]),int(m[7],16),int(m[8],16)
        expected={6:[index],7:[index,buffer,length],8:[],9:[owner,index],10:[owner,index,buffer,length],11:[],12:[owner],13:[owner],14:[owner]}[unit]
        actual=frame[1:]
        if unit in(6,7):actual[0]&=65535
        if unit in(9,10):actual[1]&=65535
        assert actual==expected,('query ABI arguments',actual,expected)
        if unit in(6,7,8):assert registers['ecx']==owner
        entry=dict(function=function,registers=registers,return_address=frame[0],callee_pop=settings['pop'],arguments=frame[1:],owner=owner,object_before=read(owner,116).hex());calls.append(entry)
        if not settings['entry_only']:Returned(entry)
        return settings['entry_only']
gdb.execute('set pagination off');gdb.execute('set confirm off');gdb.execute('set disable-randomization off')
entry=Entry('*'+function,internal=True)
gdb.execute('run '+str(unit)+' '+str(output/'report')+' >'+str(output/'stdout')+' 2>&1')
assert u('$eip')==u('(unsigned)create_program_database')+1
gdb.execute('signal SIGTRAP')
expected_count=1 if settings['entry_only']or unit not in(7,10)else 2
assert len(calls)==expected_count
if settings['entry_only']:
    assert u('$eip')==u('(unsigned)'+function)
else:assert all('returned_registers'in row for row in calls)
(output/'state.json').write_text(json.dumps(dict(status='entry'if settings['entry_only']else'returned',calls=calls),indent=2)+'\n')
if settings['entry_only']:gdb.execute('kill')
