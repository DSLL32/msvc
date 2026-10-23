"""Actual reserve/commit calls, defined bytes, owned buffers and main continuation."""
import copy,json,re,struct,subprocess
from pathlib import Path
from provenance import captured_native_binary
import allocation_imports,work_state

def read_json(directory,name):return json.loads((directory/name).read_text())
def record(events,pattern):
 rows=re.findall(pattern,events,re.M);assert rows;return rows

def symbols(binary):
 return {name:(int(address,16),int(size,16))for address,size,name in re.findall(r'^([0-9a-f]+) ([0-9a-f]+) [A-Za-z] (\S+)$',subprocess.check_output(['nm','-S','--defined-only',str(binary)],text=True),re.M)}
def call(binary,pc,destination):
 text=subprocess.check_output(['objdump','-d','--start-address='+str(pc-5),'--stop-address='+str(pc),str(binary)],text=True)
 assert re.search(r'^\s*%x:\s+e8 (?:[0-9a-f]{2} ){3}[0-9a-f]{2}\s+call\s+%x\b'%(pc-5,destination),text,re.M)
def regions(maps,base):
 rows=[]
 for line in maps.splitlines():
  m=re.match(r'^([0-9a-f]+)-([0-9a-f]+) ([-rwxps]+)\s',line);assert m
  start,end=int(m[1],16),int(m[2],16)
  if start<base+0x500000 and end>base:rows.append((max(start,base),min(end,base+0x500000),m[3]))
 assert rows and rows[0][0]==base and rows[-1][1]==base+0x500000 and all(a[1]==b[0]for a,b in zip(rows,rows[1:]))
 return rows

def validate(original,native,fields,work_entry,state,work_sequence,sequence,module,candidate,probe):
 events=(original/'events.txt').read_text();binary=captured_native_binary(native);names=symbols(binary)
 packet=allocation_imports.check_static()
 for provider in packet['providers']:
  actual=original/('loaded-'+provider['name'][:-4]+'.bin')
  assert actual.stat().st_size==provider['bytes'] and allocation_imports.sha(actual)==provider['sha256']
  assert allocation_imports.export_rva(actual,'VirtualAlloc')==provider['rva']
 kernel_base=int(record(events,r'^module name=KERNEL32.dll actual=([0-9a-f]+) ')[0],16)
 provider=int(record(events,r'^allocation-provider iat=0005d1c4 resolved=([0-9a-f]+)$')[0],16)
 assert provider==kernel_base+packet['providers'][0]['rva']
 entries=read_json(native,'work-reserve-entries.json');returns=read_json(native,'work-reserve-returns.json')
 assert len(entries)==len(returns)==2
 original_entries=record(events,r'^allocation-helper-entry index=(\d+) pc=([0-9a-f]+) esp=([0-9a-f]+) edi=([0-9a-f]+) esi=([0-9a-f]+)$')
 original_returns=record(events,r'^allocation-helper-return index=(\d+) pc=([0-9a-f]+) esp=([0-9a-f]+) eax=([0-9a-f]+) edi=([0-9a-f]+) esi=([0-9a-f]+)$')
 assert len(original_entries)==len(original_returns)==2
 native_bases=[];original_bases=[]
 for index,(entry,returned,original_entry,original_return) in enumerate(zip(entries,returns,original_entries,original_returns)):
  assert entry['index']==returned['index']==index and int(original_entry[0])==int(original_return[0])==index
  assert entry['pc']==names['reserve_work_buffer'][0] and entry['symbol']=='reserve_work_buffer' and entry['boundary_rva']==0x12873
  assert entry['args']==dict(reserve_bytes=0x500000,commit_bytes=0x4000) and entry['raw_args']==[0x500000,0x4000]
  assert entry['formal_inventory']==[dict(name='reserve_bytes',type='unsigned int'),dict(name='commit_bytes',type='unsigned int')]
  assert entry['caller']==dict(symbol='initialize_work_buffers',pc=entry['return'])
  call(binary,entry['return'],entry['pc'])
  assert returned['pc']==entry['return'] and returned['esp']==entry['esp']+4
  assert returned['edi']==entry['edi'] and returned['esi']==entry['esi']
  assert int(original_entry[1],16)==module+0x12873 and int(original_return[1],16)==module+(0x1283f if index==0 else 0x12860)
  assert int(original_return[2],16)==int(original_entry[2],16)+4
  assert original_return[4:]==original_entry[3:]
  assert returned['eax'] and int(original_return[3],16)
  native_bases.append(returned['eax']);original_bases.append(int(original_return[3],16))
 assert len(set(native_bases))==len(set(original_bases))==2
 assert all(a+0x500000<=b or b+0x500000<=a for a,b in [tuple(native_bases),tuple(original_bases)])
 # Retain the first-call frontier as a separately observed physical entry.
 first_sequence=int(record(events,r'^trap seq=(\d+) rva=00012873 ')[0])
 work_state.validate_fields(original,native,fields,work_entry,entries[0],work_sequence,first_sequence,probe,candidate)
 work_state.validate_writes(original,native,work_entry,entries[0],work_sequence,first_sequence,module)
 for index in range(2):
  wanted=copy.deepcopy(work_entry['compiler_state'])
  if index:wanted['bss']['work_buffers'][0]=dict(data=native_bases[0],limit=native_bases[0]+0x3ffb)
  assert entries[index]['compiler_state']==returns[index]['compiler_state']==wanted
 before=record(events,r'^allocation-before index=(\d+) pc=([0-9a-f]+) esp=([0-9a-f]+) edi=([0-9a-f]+) args=([0-9a-f,]+)$')
 after=record(events,r'^allocation-after index=(\d+) pc=([0-9a-f]+) esp=([0-9a-f]+) eax=([0-9a-f]+) edi=([0-9a-f]+) before-esp=([0-9a-f]+) before-edi=([0-9a-f]+)$')
 errors=record(events,r'^allocation-error index=(\d+) value=(\d+) fpu=([0-9a-f]+)$')
 native_before=read_json(native,'work-api-entries.json');native_after=read_json(native,'work-api-returns.json')
 assert len(before)==len(after)==len(errors)==len(native_before)==len(native_after)==4
 for index,(left,right,error,nb,na) in enumerate(zip(before,after,errors,native_before,native_after)):
  assert int(left[0])==int(right[0])==int(error[0])==nb['index']==na['index']==index
  commit=index%2;allocation=index//2
  oa=[int(x,16)for x in left[4].split(',')]
  wanted=[original_bases[allocation] if commit else 0,0x4000 if commit else 0x500000,0x1000 if commit else 0x2000,4]
  assert oa==wanted and nb['args']==[native_bases[allocation] if commit else 0]+wanted[1:]
  assert {r['name']:r['type']for r in nb['formal_inventory']}==dict(address='LPVOID',bytes='DWORD',kind='DWORD',protection='DWORD')
  assert nb['pc']==names['recon_VirtualAlloc'][0]
  assert int(left[1],16)==module+(0x128a2 if commit else 0x12889)
  assert int(right[1],16)==module+(0x128a4 if commit else 0x1288b)
  assert int(left[2],16)==int(right[5],16) and int(right[2],16)==int(left[2],16)+16
  assert int(left[3],16)==int(right[4],16)==int(right[6],16)==provider
  assert int(right[3],16)==original_bases[allocation] and na['eax']==native_bases[allocation]
  assert na['pc']==nb['return'] and na['esp']==nb['esp']+20
  assert all(na[r]==nb[r]for r in ('edi','esi','ebx'))
  assert int(error[1])==nb['last_error']==na['last_error']==0 and int(error[2],16)&0xffff==nb['fpu']==na['fpu']==0x027f
  query=record(events,r'^allocation-region index=%d part=(\d+) base=([0-9a-f]+) allocation=([0-9a-f]+) allocation-protect=([0-9a-f]+) bytes=([0-9a-f]+) state=([0-9a-f]+) protect=([0-9a-f]+) type=([0-9a-f]+)$'%index)
  expected_query=[(0,original_bases[allocation],original_bases[allocation],4,0x4000,0x1000,4,0x20000),(1,original_bases[allocation]+0x4000,original_bases[allocation],4,0x4fc000,0x2000,0,0x20000)]if commit else [(0,original_bases[allocation],original_bases[allocation],4,0x500000,0x2000,0,0x20000)]
  assert [tuple(int(x,10 if j==0 else 16)for j,x in enumerate(row))for row in query]==expected_query
  native_regions=regions(na['maps'],native_bases[allocation]);base=native_bases[allocation]
  assert native_regions==([(base,base+0x4000,'rw-p'),(base+0x4000,base+0x500000,'---p')]if commit else [(base,base+0x500000,'---p')])
  if commit:
   original_bytes=(original/('work-committed-%03d.bin'%allocation)).read_bytes();native_bytes=(native/('work-committed-%03d.bin'%allocation)).read_bytes()
   assert original_bytes==native_bytes==bytes(0x4000)
 # Genuine main continues after both buffer returns, then registers cleanup and
 # stores its live argc/argv before calling the new named commandline stub.
 expected=copy.deepcopy(work_entry['compiler_state'])
 expected['bss']['work_buffers']=[dict(data=base,limit=base+0x3ffb)for base in native_bases]
 expected['bss']['command_argc']=candidate['case_contract']['argc'];expected['bss']['command_argv']=state['invocation']['args']['argv']
 old_count=expected['bss']['cleanup_count'];assert old_count==10
 expected['bss']['cleanup_callbacks'][old_count]=names['release_work_buffers_238da'][0];expected['bss']['cleanup_count']+=1
 assert state['compiler_state']==expected
 original_before=(original/('compiler-bss-%03d.bin'%work_sequence)).read_bytes();wanted=bytearray(original_before)
 for name,(rva,size,offset) in work_state.FIELDS.items():
  index=int(name[13]);value=original_bases[index]+(0x3ffb if name.endswith('.limit')else 0);struct.pack_into('<I',wanted,rva-0x50000,value)
 args=record(events,r'^trap seq=%d rva=000128b1 .*stack=([0-9a-f,]+)$'%sequence);assert len(args)==1;words=[int(x,16)for x in args[0].split(',')]
 invoke_words=[int(x,16)for x in record(events,r'^trap seq=\d+ rva=00012229 .*stack=([0-9a-f,]+)$')[0].split(',')]
 assert words[0]==module+0x127df and words[1]==invoke_words[2]
 struct.pack_into('<II',wanted,0x5c38,invoke_words[1],invoke_words[2]);struct.pack_into('<I',wanted,0x781c,11);struct.pack_into('<I',wanted,0x7820+10*4,module+0x238da)
 assert bytes(wanted)==(original/('compiler-bss-%03d.bin'%sequence)).read_bytes()
 for region in ('compiler-data','compiler-snapshot','state'):
  assert (original/('%s-%03d.bin'%(region,work_sequence))).read_bytes()==(original/('%s-%03d.bin'%(region,sequence))).read_bytes()
 for key,value in work_entry['crt_state'].items():assert state[key]==value
 assert state['caller']['symbol']=='c1_main' and state['args']==dict(arguments=state['invocation']['args']['argv'])
 assert state['formal_inventory']==[dict(name='arguments',type='char **')]
 call(binary,state['caller']['pc'],names['scan_internal_switches'][0])
 raw=(original/'relocated-text.bin').read_bytes();assert raw[0x127da-0x1000]==0xe8 and 0x127df+struct.unpack_from('<i',raw,0x127db-0x1000)[0]==0x128b1
