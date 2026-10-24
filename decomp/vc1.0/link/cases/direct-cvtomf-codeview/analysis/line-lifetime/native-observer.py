import gdb,json,struct,hashlib
from pathlib import Path
out=Path('/root/.ai-tmp/msvc2/vc1.0/link/cvtomf-lifetime-67zniv_u');events=[];released=False;before=None

def value(s):return int(gdb.parse_and_eval(s))&0xffffffff
def memory(p,n):return bytes(gdb.selected_inferior().read_memory(p,n))
def snapshot(label):
 p=value('omf_line_data');n=value('omf_line_count');assert p and n==9
 raw=memory(p,n*8);rows=[list(struct.unpack_from('<IH',raw,i*8)) for i in range(n)]
 d=dict(event=label,pointer=p,count=n,raw=raw.hex(),rows=rows,coff_line_count=value('omf_coff_line_count'),esp=value('$esp'),errors=[value('*cvtomf_test_errno'),value('recon_doserrno'),value("'win32.c'::last_error")]);events.append(d);(out/'lifetime.json').write_text(json.dumps(events,indent=2)+'\n');return d
class Released(gdb.Breakpoint):
 def stop(self):snapshot('line_release_return');self.enabled=False;return False
class Free(gdb.Breakpoint):
 def stop(self):
  global released
  esp=value('$esp');arg=struct.unpack('<I',memory(esp+4,4))[0]
  if not value('omf_line_data') or arg!=value('omf_line_data'):return False
  assert not released;released=True;d=snapshot('line_release_entry');d['actual_formal']=arg;d['caller']=gdb.newest_frame().older().name();Released('*'+str(struct.unpack('<I',memory(esp,4))[0]),internal=True);return False
class ReaderReturn(gdb.Breakpoint):
 def stop(self):snapshot('line_reader_return');self.enabled=False;return False
class Reader(gdb.Breakpoint):
 def stop(self):
  assert released;d=snapshot('line_reader_entry');esp=value('$esp');ReaderReturn('*'+str(struct.unpack('<I',memory(esp,4))[0]),internal=True);return False
Free('*recon_free',internal=True);Reader('*omf_write_lines',internal=True)
gdb.execute('set pagination off');gdb.execute('set confirm off');gdb.execute('run -g -o converted.obj legacy.obj >'+'/root/.ai-tmp/msvc2/vc1.0/link/cvtomf-lifetime-67zniv_u/stdout.bin'+' 2>'+'/root/.ai-tmp/msvc2/vc1.0/link/cvtomf-lifetime-67zniv_u/stderr.bin')
assert released and len(events)==4
