"""Fresh native entry/ABI observations for the four fixed resource units."""
import gdb,json,re,struct
from pathlib import Path
s=json.loads(Path(gdb.convenience_variable('resource_settings').string()).read_text());out=Path(s['directory']);calls=[];forward=[];exits=[];result={}
u=lambda e:int(gdb.parse_and_eval(e))&0xffffffff
read=lambda a,n:bytes(gdb.selected_inferior().read_memory(a,n))
names=['eip','esp','ebx','esi','edi','ebp','fctrl']
class Returned(gdb.Breakpoint):
    def __init__(self,row):super().__init__('*'+str(row['frame'][0]),temporary=True,internal=True);self.row=row
    def stop(self):
        after={n:u('$'+n)for n in names+['eax']};before=self.row['registers'];assert after['esp']==before['esp']+4+s['pop']
        for n in names[2:]:assert after[n]==before[n]
        self.row['returned_registers']=after;return False
class Entry(gdb.Breakpoint):
    def stop(self):
        registers={n:u('$'+n)for n in names};frame=list(struct.unpack('<'+'I'*(s['arguments']+1),read(registers['esp'],4*(s['arguments']+1))))
        text=(out/'report').read_text();assert'resource-state before depth 4 errno 2 dos 2'in text
        expected={0:[96],1:[],3:[2]}.get(s['unit'])
        if s['unit']==2:
            m=re.search(r'resource-native-report code83 kind (\w+) file (\w+) source (\w+) line (\d+)',text);assert m;expected=[83,int(m[1],16),int(m[2],16),int(m[3],16),int(m[4])]
        assert frame[1:]==expected
        row=dict(function=s['function'],registers=registers,frame=frame);calls.append(row)
        if s['entry_only']:return True
        if s['unit']<2:Returned(row)
        return False
class Forward(gdb.Breakpoint):
    def stop(self):
        frame=list(struct.unpack('<7I',read(u('$esp'),28)));args=calls[0]['frame'][1:];assert frame[1:4]==args[:3]and frame[5:7]==args[3:5];forward.append(frame);return False
class Exit(gdb.Breakpoint):
    def stop(self):
        frame=list(struct.unpack('<2I',read(u('$esp'),8)));assert frame[1]==(1 if s['unit']==2 else 32768);exits.append(frame);return False
def exited(event):result['exit']=event.exit_code

gdb.execute('set pagination off');gdb.execute('set confirm off');gdb.execute('set disable-randomization off')
Entry('*'+s['function'],internal=True)
if not s['entry_only']and s['unit']==2:Forward('*fatal_error',internal=True)
if not s['entry_only']and s['unit']>=2:Exit('*recon_exit',internal=True)
gdb.events.exited.connect(exited)
gdb.execute('starti '+str(s['unit'])+' '+str(out/'report')+' >'+str(out/'stdout')+' 2>&1');result['inferior_pid']=gdb.selected_inferior().pid;gdb.execute('continue')
assert len(calls)==1
if s['entry_only']:
    assert u('$eip')==u('(unsigned)'+s['function']);gdb.execute('kill')
elif s['unit']<2:assert'returned_registers'in calls[0]and result['exit']==99
else:
    assert len(exits)==1 and result['exit']==(1 if s['unit']==2 else 0)
    if s['unit']==2:assert len(forward)==1
result.update(status='entry'if s['entry_only']else'completed',calls=calls,forwarded=forward,exits=exits);(out/'state.json').write_text(json.dumps(result,indent=2)+'\n')
