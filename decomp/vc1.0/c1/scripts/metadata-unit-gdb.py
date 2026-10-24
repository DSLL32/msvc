"""Fresh entry and ABI proof for the bounded metadata interfaces."""
import gdb,json,re,struct
from pathlib import Path
s=json.loads(Path(gdb.convenience_variable('metadata_settings').string()).read_text());out=Path(s['directory']);calls=[];nested=[];hooks=[];result={}
u=lambda e:int(gdb.parse_and_eval(e))&0xffffffff
read=lambda a,n:bytes(gdb.selected_inferior().read_memory(a,n))if n else b''
names=['eip','esp','ebx','esi','edi','ebp','fctrl']
class Returned(gdb.Breakpoint):
    def __init__(self,row,pop):super().__init__('*'+str(row['frame'][0]),temporary=True,internal=True);self.row=row;self.pop=pop
    def stop(self):
        after={n:u('$'+n)for n in names+['eax']};before=self.row['registers'];assert after['esp']==before['esp']+4+self.pop
        for n in names[2:]:assert after[n]==before[n]
        if s['unit']in[1,2]:assert after['eax']==self.row['frame'][1]
        if self.row['function']=='smaller_size':assert after['eax']==min(self.row['frame'][1:])
        elif s['unit']in[3,4]:
            m=re.search(r'^metadata-copy-input cursor (\w+) ',(out/'report').read_text(),re.M);assert m and after['eax']==int(m[1],16)
        elif s['unit']==5:assert after['eax']&65535==13
        elif s['unit']in[6,7]:assert after['eax']==int(s['unit']==6)
        elif s['unit']==9:assert after['eax']==15
        self.row['returned_registers']=after;return False
class Entry(gdb.Breakpoint):
    def stop(self):
        registers={n:u('$'+n)for n in names};frame=list(struct.unpack('<'+'I'*(s['arguments']+1),read(registers['esp'],4*(s['arguments']+1))))
        text=(out/'report').read_text()
        if s['unit']in[3,4]:
            m=re.search(r'^metadata-copy-input cursor \w+ available \d+ source (\w+) bytes(\d+) allocation(\d+)$',text,re.M);assert m
            expected=[int(m[3]),int(m[2])]if s['frontier']else[int(m[1],16),int(m[2]),int(m[3])];assert frame[1:]==expected
        elif s['unit']==5:
            m=re.search(r'^metadata-pascal-input buffer (\w+) allocation256 symbol (\w+) name (\w+) length13 text metadata_sink$',text,re.M);assert m and frame[1:]==[int(m[1],16),int(m[2],16)]
            assert read(frame[1],256)==b'\xa5'*256
        elif s['unit']in[6,7]:
            m=re.search(r'^metadata-qualifier-input symbol (\w+) ',text,re.M);assert m and frame[1:]==[int(m[1],16)]
        elif s['unit']==8:
            assert frame[1:]==[]and read(u('(unsigned)&browser_line_template'),24)==b'\0'*24
            assert u('browser_suppressed')==len(calls)
        elif s['unit']==9:
            m=re.search(r'^metadata-format-input buffer (\w+) owner16 storage (\w+) block2048 format (\w+) name (\w+) number6 count0 trailer-zero8 original-work-distinct1$',text,re.M);assert m
            buffer,storage,format,name=[int(v,16)for v in m.groups()];assert frame[1:]==[buffer,format,name,6]
            assert list(struct.unpack('<4I',read(buffer,16)))==[0,storage,storage,storage]and read(storage+2040,8)==b'\0'*8
            assert read(format,6)==b'%s:%d\0'and read(name,14)==b'metadata_sink\0'
        else:
            pattern=[r'^metadata-argument 0 address (\w+) ',r'^metadata-vector defines (\w+) ',r'^metadata-encoded-vector before owner (\w+) '][s['unit']];matches=re.findall(pattern,text,re.M);assert len(matches)==1 and frame[1]==int(matches[0],16)
        row=dict(function=s['function'],registers=registers,frame=frame);calls.append(row);assert len(calls)<=(2 if s['unit']==8 else 1)
        if s['entry_only']:return True
        Returned(row,s['pop']);return False
class Minimum(gdb.Breakpoint):
    def stop(self):
        registers={n:u('$'+n)for n in names};frame=list(struct.unpack('<3I',read(registers['esp'],12)))
        m=re.search(r'^metadata-copy-input cursor \w+ available \d+ source \w+ bytes(\d+) allocation(\d+)$',(out/'report').read_text(),re.M);assert m and frame[1:]==[int(m[2]),int(m[1])]
        row=dict(function='smaller_size',registers=registers,frame=frame);nested.append(row);assert len(nested)==1;Returned(row,8);return False
class Hook(gdb.Breakpoint):
    def stop(self):
        if not calls:return False
        hooks.append(u('$eip'));assert len(hooks)==1;return False
def exited(event):result['exit']=getattr(event,'exit_code',None)
gdb.execute('set pagination off');gdb.execute('set confirm off');gdb.execute('set disable-randomization off')
Entry('*'+s['function'],internal=True);gdb.events.exited.connect(exited)
if s['unit']in[3,4]and not s['entry_only']:Minimum('*smaller_size',internal=True)
if s['unit']==8 and not s['entry_only']:Hook('*browser_include_begin',internal=True)
gdb.execute('starti '+str(s['unit'])+' '+str(out/'report')+' >'+str(out/'stdout')+' 2>&1');result['inferior_pid']=gdb.selected_inferior().pid;gdb.execute('continue')
assert len(calls)==(2 if s['unit']==8 and not s['entry_only']else 1)
if s['entry_only']:
    assert u('$eip')==u('(unsigned)'+s['function']);gdb.execute('kill')
else:
    assert all('returned_registers'in row for row in calls)and result['exit']==99
    if s['unit']in[3,4]:assert len(nested)==1 and'returned_registers'in nested[0]
    if s['unit']==8:assert len(hooks)==1
result.update(status='entry'if s['entry_only']else'completed',calls=calls,nested_calls=nested,browser_hook_entries=hooks);(out/'state.json').write_text(json.dumps(result,indent=2)+'\n')
