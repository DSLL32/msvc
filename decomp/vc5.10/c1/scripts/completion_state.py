"""Typed target lifecycle projection from actual original/native raw observations."""
import json,re,struct

def validate(original,native,o,n):
 raw=(original/'events.txt').read_text()
 state=json.loads((native/'state.json').read_text())
 assert o['exit']==state['exit']==0 and n['kind']=='native-gdb-completion'
 assert 'exited normally' in (native/'gdb.txt').read_text()
 assert 'exit=0 target=1 crt=1 initialized=1' in raw
 assert 'frontier ' not in raw and 'failure ' not in raw
 assert (original/'stdout.bin').read_bytes()==(native/'stdout.bin').read_bytes()==b'attach=1\r\nexports=1\r\ndetach=1\r\n'
 assert (original/'stderr.bin').read_bytes()==(native/'stderr.bin').read_bytes()==b''
 base=int(re.search(r'module name=c1n32_l.dll actual=([0-9a-f]+)',raw)[1],16)
 rows=re.findall(r'^trap seq=(\d+) rva=([0-9a-f]+).*stack=([0-9a-f,]+)$',raw,re.M)
 names={0x1202a:'c1_crt_entry',0x120ce:'c1_crt_init',0x1218e:'DllMain'}
 projected=[]
 for seq,rva,stack in rows:
  rva=int(rva,16);words=[int(v,16) for v in stack.split(',')]
  if rva==0x12223:
   assert words[1:3]==[base+0x5c000,base+0x5c004];continue
  assert rva in names and words[1]==base and words[3]==0
  projected.append((names[rva],words[2],words[3]))
 entries=[r for r in state['events'] if r['kind']=='entry']
 assert projected==[(r['symbol'],r['args']['reason'],r['args']['reserved']) for r in entries]
 assert projected==[('c1_crt_entry',1,0),('c1_crt_init',1,0),('DllMain',1,0),('c1_crt_entry',0,0),('DllMain',0,0),('c1_crt_init',0,0)]
 returns=re.findall(r'^crt-return seq=(\d+) reason=(\d+) status=(\d+) pc=[0-9a-f]+\nlast-error=(\d+)\nx87-control=([0-9a-f]+)$',raw,re.M)
 observations=[r for r in state['events'] if r['kind']=='return']
 assert len(returns)==len(observations)==2
 original_allocation=native_allocation=None
 for index,((seq,reason,status,error,cw),r) in enumerate(zip(returns,observations)):
  reason=int(reason);assert reason==r['reason']==1-index
  assert int(status)==r['status']==1
  assert int(error)==r['win32_last_error']==14007
  assert (int(cw,16)&0xffff)==r['x87_control']==0x027f
  data=(original/('state-%03d.bin'%int(seq))).read_bytes()
  count=struct.unpack_from('<i',data,0x90)[0]
  end,begin,hook,adjust=struct.unpack_from('<4I',data,0x94)
  instance=struct.unpack_from('<I',data,0x40)[0]
  assert count==r['crt_attached']==reason
  assert hook==r['raw_dllmain']==0 and adjust==r['crt_adjust_fdiv']==0
  assert instance==base and r['dll_instance']==r['module_token']!=0
  assert r['module_identity']=={'name':'C1.DLL','path':'R:\\toolchains\\5.10\\bin\\C1.DLL','sha256':o['target_sha256'],'flags':2}
  assert r['thread_calls_disabled']==1
  if reason:
   assert begin==end!=0 and r['onexit_begin']==r['onexit_end']!=0
   assert r['onexit_first']==0
   heap=re.search(r'^state seq='+seq+r' heap-begin=([0-9a-f]+) heap-first=([0-9a-f]+)$',raw,re.M)
   assert heap and int(heap[1],16)==begin and int(heap[2],16)==0
   original_allocation=begin;native_allocation=r['onexit_begin']
  else:
   assert begin==r['onexit_begin']==0 and r['onexit_first'] is None
   assert end==original_allocation and r['onexit_end']==native_allocation
 for entry in entries:assert entry['args']['module']==observations[0]['module_token']
 assert observations[0]['module_token']==observations[1]['module_token']
 return {'status':'fresh independent original/native lifecycle completion','application_functions':1,'application_bytes':35,'crt_functions':2,'crt_bytes':451,'import_thunks':1,'import_bytes':6,'state':['ordered attach/detach entry ABIs','successful return/status/streams','registered module identity and thread notifications','CRT count/raw callback/adjust_fdiv','onexit allocation and retained dangling end','Win32 LastError14007','x87 CW027f'],'promotion':'eligible under reviewed same-protocol lifecycle continuation'}
