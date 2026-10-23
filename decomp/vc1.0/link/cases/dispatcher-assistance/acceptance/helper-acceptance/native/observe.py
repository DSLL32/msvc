
import gdb,json,struct
from pathlib import Path
events=[];terminal=[];before=None
out=Path('/root/.ai-tmp/msvc2/vc1.0/link/dispatcher-gdb-r5kvjklp/state.json')
def val(s):return int(gdb.parse_and_eval(s))&0xffffffff
def mem(p,n):return bytes(gdb.selected_inferior().read_memory(p,n))
def words(p,n):return list(struct.unpack('<'+'I'*n,mem(p,n*4)))
def string(p):
 result=b''
 for i in range(512):
  c=mem(p+i,1);result+=c
  if c==b'\0':return result.hex()
 raise AssertionError('unterminated actual string')
def persist():out.write_text(json.dumps(dict(events=events,terminal=terminal),indent=2)+'\n')
def snap(event):
 esp=val('$esp');argc=0;argv=0;formals=[];values=[]
 if event in ('entry','frontier') and 'helper_main'=='helper_main':
  args=words(esp,3);argc=args[1];argv=args[2];assert argc<=32;formals=args[1:]
  values=[string(p) for p in words(argv,argc)];assert words(argv+argc*4,1)==[0]
 tools=[]
 for i,size in enumerate([7,16,7,7,7,1]):
  item=gdb.parse_and_eval('tools[%d]'%i)
  tools.append(dict(name_pointer=int(item['name']),length=int(item['length']),padding=int(item['padding']),kind=int(item['kind']),run=int(item['run']),bytes=mem(int(item['name']),size).hex()))
 stream=words(val('&recon_iob[1]'),8);used=stream[0]-stream[2];assert 0<=used<=4096
 maps=[line.split() for line in Path('/proc/'+str(gdb.selected_inferior().pid)+'/maps').read_text().splitlines()]
 writable=[]
 for item,size in zip(tools[:5],[7,16,7,7,7]):
  writable.append(any(int(line[0].split('-')[0],16)<=item['name_pointer'] and item['name_pointer']+size<=int(line[0].split('-')[1],16) and line[1]=='rw-p' for line in maps))
 return dict(writable_names=writable,run_owners=[tools[i]['run']==(val('&'+name) if name else 0) for i,name in enumerate(['linker_main','library_main','dumper_main','editor_main','helper_main',None])],event=event,name='helper_main',eip=val('$eip'),esp=esp,eax=val('$eax'),ebx=val('$ebx'),esi=val('$esi'),edi=val('$edi'),ebp=val('$ebp'),caller=words(esp,1)[0],formal_words=formals,argv=values,program=string(val('program_name')),tools=tools,stdout_file=stream,pending=mem(stream[2],used).hex() if used else '',errors=[val('*dispatcher_test_errno'),val('recon_doserrno'),val("'win32.c'::last_error")],ctype=mem(val('recon_pctype')-2,514).hex(),mb_cur_max=val('recon_mb_cur_max'))
class Return(gdb.Breakpoint):
 def stop(self):
  after=snap('return');events.append(after);persist()
  assert after['esp']==before['esp']+4 and after['eax']==3
  assert all(before[r]==after[r] for r in ['ebx','esi','edi','ebp'])
  self.enabled=False;return False
class Entry(gdb.Breakpoint):
 def stop(self):
  global before
  before=snap('entry' if True else 'frontier');events.append(before);persist()
  if not True:return True
  caller=gdb.newest_frame().older();assert caller
  before['actual_caller']=caller.name();persist();Return('*'+str(before['caller']),internal=True);return False
def exited(e):terminal.append(dict(event='exit',code=getattr(e,'exit_code',None)));persist()
gdb.events.exited.connect(exited)
gdb.execute('set pagination off');gdb.execute('set confirm off');Entry('*'+'helper_main',internal=True)
gdb.execute('run '+'-helper'+' >'+'/root/.ai-tmp/msvc2/vc1.0/link/dispatcher-gdb-r5kvjklp/stdout.bin'+' 2>'+'/root/.ai-tmp/msvc2/vc1.0/link/dispatcher-gdb-r5kvjklp/stderr.bin')
assert events and (terminal or events[-1]['event']=='frontier')
if terminal:assert terminal==[dict(event='exit',code=3)] and len(events)==2
persist()
