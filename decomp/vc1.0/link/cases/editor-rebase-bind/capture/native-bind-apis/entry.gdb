set pagination off
set confirm off
python
import gdb,json,struct
rows=[]
def errors():return dict(errno=int(gdb.parse_and_eval('(int)errno')),last_error=int(gdb.parse_and_eval("'win32.c'::last_error")))
def words(address,count):return list(struct.unpack('<'+'H'*count,bytes(gdb.selected_inferior().read_memory(address,count*2))))
class Returned(gdb.FinishBreakpoint):
 def __init__(self,name,args):
  super().__init__(gdb.newest_frame(),internal=True);self.name=name;self.args=args
 def stop(self):
  r=dict(name=self.name,before=False,**errors())
  if self.name!='GetSystemTime':r['result']=int(self.return_value)
  if self.name=='GetSystemTime':r['system_time']=words(self.args[0],8)
  if self.name=='SystemTimeToFileTime':r['file_time']=words(self.args[1],4)
  rows.append(r);return False
class Entry(gdb.Breakpoint):
 def __init__(self,name,names):super().__init__('recon_'+name,internal=True);self.name=name;self.names=names
 def stop(self):
  caller=gdb.newest_frame().older()
  if caller is None or caller.name()!='bind_imports':return False
  args=[int(gdb.parse_and_eval(n)) for n in self.names]
  r=dict(name=self.name,before=True,**errors())
  if self.name=='FlushViewOfFile':
   assert args[0]==int(gdb.parse_and_eval('&editor_workspace')) and args[1]==0
   assert int(gdb.parse_and_eval('sizeof(editor_workspace)'))==36
   r.update(base='editor_workspace',bytes=args[1])
  if self.name=='SystemTimeToFileTime':r['system_time']=words(args[0],8)
  if self.name=='SetFileTime':
   assert args[0]==int(gdb.parse_and_eval('editor_workspace.bound.file')) and args[1]==args[2]==0
   r.update(file='current client image',creation=None,access=None,file_time=words(args[3],4))
  rows.append(r);Returned(self.name,args);return False
Entry('FlushViewOfFile',['address','bytes'])
Entry('GetSystemTime',['out'])
Entry('SystemTimeToFileTime',['input','output'])
Entry('SetFileTime',['handle','creation','access','write_time'])
end
run -edit /bind:path=. client.exe >/root/.ai-tmp/msvc2/vc1.0/link/frontiers/editor-api-native-3pd8dgdw/stdout.bin 2>/root/.ai-tmp/msvc2/vc1.0/link/frontiers/editor-api-native-3pd8dgdw/stderr.bin
python
assert len(rows)==8
open('/root/.ai-tmp/msvc2/vc1.0/link/frontiers/editor-api-native-3pd8dgdw/native.json','w').write(json.dumps(rows,indent=2)+'\n')
end
