"""Three owned arena vectors, true commit, then distinct dependency/target ends."""
import copy,json,re,struct
from provenance import captured_native_binary,sha,PROJECT
from allocation_state import symbols,call
from allocation_imports import pe_read,export_rva
from scaler_state import semantic,original_fp
from frontier_contract import validate_original_frontier
from environment_argument_state import validate as validate_environment

RETURNS=[0x12beb,0x12bfc,0x12c0d]
DESTINATIONS=[(0x54c74,'defines'),(0x54168,'include_directories'),(0x54b38,'forced_includes')]
ARENA=0x58fd0
UNIT=32768

def registers(original,index,phase):
 return list(struct.unpack('<14I',(original/('vector-%d-%s-registers.bin'%(index,phase))).read_bytes()))
def blob(original,index,phase,suffix):return(original/('vector-%d-%s-target-%s.bin'%(index,phase,suffix))).read_bytes()
def snapshot(original,native,state,record):
 value=copy.deepcopy(state)
 value.update({key:record[key]for key in ('pc','symbol','args','formal_inventory','caller','compiler_state','invocation','module_identity','module_token')})
 value.update(record['crt_state']);value['command_line_fp']=record['fp'];value['pass_mode']=record['compiler_state']['pass_mode']
 return value

def validate(original,native,candidate,report,stage_state=None):
 vectors=[row for row in candidate['functions']if row['name']=='_new_option_vector'];assert len(vectors)==1 and vectors[0]['chunks']==[[0x11e3c,0x11ea4],[0x3016a,0x30176]]
 events=(original/'events.txt').read_text();receipt=json.loads((original/'receipt.json').read_text());role=validate_original_frontier(events,candidate,receipt['probe'])
 binary=captured_native_binary(native);names=symbols(binary);state=stage_state if stage_state is not None else json.loads((native/'state.json').read_text())
 entries=json.loads((native/'vector-entries.json').read_text());returns=json.loads((native/'vector-returns.json').read_text());assert len(entries)==len(returns)==3
 first=snapshot(original,native,state,entries[0]);prefix=validate_environment(original,native,candidate,report,stage_state=first)
 return validate_allocation_state(original,native,candidate,entries,returns,state,first,prefix,following=stage_state is not None)

def validate_allocation_state(original,native,candidate,entries,returns,state,first,prefix=None,following=False):
 events=(original/'events.txt').read_text();receipt=json.loads((original/'receipt.json').read_text());role=validate_original_frontier(events,candidate,receipt['probe'])
 binary=captured_native_binary(native);names=symbols(binary);assert len(entries)==len(returns)==3
 expected=copy.deepcopy(first['compiler_state']);initial_bss=blob(original,0,'entry','BSS');raw=bytearray(initial_bss)
 oldbase=struct.unpack_from('<I',raw,ARENA-0x50000+8)[0];base=expected['bss']['arenas'][1]['base'];arena=names['c1_bss'][0]+836
 assert expected['bss']['arenas'][1]['cursor']==expected['bss']['arenas'][1]['committed_limit']==base
 assert expected['bss']['arenas'][1]['committed_bytes']==0 and expected['arena_descriptors'][1]['commit_unit']==UNIT
 assert(original/'vector-commit-zero-000.bin').read_bytes()==(native/'vector-commit-zero-000.bin').read_bytes()==bytes(UNIT)
 baseline={suffix:blob(original,0,'entry',suffix)for suffix in ('data','snapshot')}
 old_expected_region=bytearray(UNIT);expected_region=bytearray(UNIT)
 for index,(entry,end)in enumerate(zip(entries,returns)):
  before=registers(original,index,'entry');after=registers(original,index,'return')
  assert before[1]==0x10611e3c and before[8:11]==[0x10600000+RETURNS[index],40,1]and before[11:]==[before[2],0x10600000,index]
  assert after[1]==0x10600000+RETURNS[index]and after[2]==before[2]+4 and after[4:8]==before[4:8]and after[11:]==before[11:]
  assert entry['symbol']=='new_option_vector'and entry['pc']==names['new_option_vector'][0]and entry['words']==[entry['caller']['pc'],40,1]
  assert entry['args']==dict(count=40,mode=1)and entry['formal_inventory']==[dict(name='count',type='unsigned int'),dict(name='mode',type='int')]
  assert entry['layout']==dict(size=12,offsets=dict(count=0,capacity=2,arena_kind=4,items=8))
  assert entry['caller']['symbol']=='parse_command_line'and entry['caller']['args']==first['caller']['args'];call(binary,entry['words'][0],entry['pc'])
  assert entry['compiler_state']==expected and blob(original,index,'entry','BSS')==bytes(raw)
  assert entry['invocation']==end['invocation']==first['invocation']and entry['crt_state']==end['crt_state']==first_crt(first)
  returned=base+index*172;old_returned=oldbase+index*172
  assert end['address']==end['eax']==returned and after[3]==old_returned
  assert end['header']==[0,40,1,returned+12]and bytes.fromhex(end['raw_header'])==struct.pack('<hhII',0,40,1,returned+12)
  assert(original/('vector-header-%03d.bin'%index)).read_bytes()==struct.pack('<hhII',0,40,1,old_returned+12)
  assert end['items']==[0]*160 and(original/('vector-items-%03d.bin'%index)).read_bytes()==bytes(160)
  expected['bss']['arenas'][1].update(cursor=returned+172,committed_limit=base+UNIT,committed_bytes=UNIT)
  struct.pack_into('<II',raw,ARENA-0x50000,old_returned+172,oldbase+UNIT);struct.pack_into('<I',raw,ARENA-0x50000+12,UNIT)
  assert end['compiler_state']==expected and blob(original,index,'return','BSS')==bytes(raw)
  for suffix in ('data','snapshot'):assert blob(original,index,'entry',suffix)==blob(original,index,'return',suffix)==baseline[suffix]
  assert end['pc']==entry['words'][0]and end['esp']==entry['esp']+4 and end['preserved']==entry['preserved']
  assert entry['errors']==end['errors']==[0,0,0]and semantic(entry['fp'])==semantic(end['fp'])==semantic(original_fp(original,before[0]))
  assert(original/('vector-%d-entry-fp.bin'%index)).read_bytes()==(original/('vector-%d-return-fp.bin'%index)).read_bytes()
  struct.pack_into('<hhII',old_expected_region,index*172,0,40,1,old_returned+12);struct.pack_into('<hhII',expected_region,index*172,0,40,1,returned+12)
  assert(original/('vector-committed-%03d.bin'%index)).read_bytes()==bytes(old_expected_region)
  assert(native/('vector-committed-%03d.bin'%index)).read_bytes()==bytes(expected_region)
  destination,name=DESTINATIONS[index];expected['option_state']['bss'][name]=returned;struct.pack_into('<I',raw,destination-0x50000,old_returned)
 validate_commit(original,native,first,base,oldbase,arena,binary)
 assert state['compiler_state']==expected
 if receipt['probe']=='normal':
  validate_dependency(original,native,state,names,binary)
  sequence=(list(struct.unpack('<12I',(original/'signature-entry-registers.bin').read_bytes()))[0]if candidate.get('option_match_observers')else int(role['dependency'][0][2]));assert expected['initialized']['c9_il']==0
 else:
  rows=re.findall(r'^trap seq=(\d+) rva=00012f9c .*stack=([\w,]+)$',events,re.M);rows=[r for r in rows if int(r[1].split(',')[0],16)==0x10612c45];assert len(rows)==1
  sequence=int(rows[0][0]);assert int(rows[0][1].split(',')[0],16)==0x10612c45
  assert expected['initialized']['c9_il']==9 and state['symbol']=='next_argument'and state['args']=={}and state['formal_inventory']==[]
  assert state['pc']==names['next_argument'][0]if following else names['next_argument'][0]<state['pc']<=sum(names['next_argument'])
  call(binary,state['caller']['pc'],names['next_argument'][0])
 assert(original/('compiler-bss-%03d.bin'%sequence)).read_bytes()==bytes(raw)
 for suffix,stem in [('data','compiler-data'),('snapshot','compiler-snapshot')]:assert(original/('%s-%03d.bin'%(stem,sequence))).read_bytes()==baseline[suffix]
 assert state['caller']['symbol']=='parse_command_line'and state['caller']['args']==first['caller']['args']and state['invocation']==first['invocation']
 assert semantic(state['command_line_fp'])==semantic(entries[0]['fp'])
 for key,value in first_crt(first).items():assert state[key]==value
 return dict(status='matching three complete arena vectors and distinct real dependency/target frontier',application_candidate='_new_option_vector',candidate_code_bytes=116,vector_count=3,header_bytes=12,item_bytes=160,committed_bytes=UNIT,opaque_reserved_tail=True,initialized_known_bytes=2348,initialized_unmodeled_bytes=4200,CRT_code_credit=0,provider_execution=False,prefix=prefix,frontier_role=role,execution_receipts=dict(original=sha(original/'receipt.json'),native=sha(native/'receipt.json')))

def first_crt(first):
 return {key:first[key]for key in ('win32_last_error','x87_control','crt_attached','raw_dllmain','onexit_begin','onexit_end','onexit_first','crt_adjust_fdiv','dll_instance','module_token')}

def validate_commit(original,native,first,base,oldbase,arena,binary):
 events=(original/'events.txt').read_text();names=symbols(binary)
 before=re.findall(r'^vector-commit-before seq=(\d+) esp=(\w+) address=(\w+) bytes=(\w+) kind=(\w+) protection=(\w+) provider=(\w+) error=(\d+)$',events,re.M)
 after=re.findall(r'^vector-commit-after seq=(\d+) esp=(\w+) before-esp=(\w+) result=(\w+) error=(\d+) before-error=(\d+)$',events,re.M);assert len(before)==len(after)==1
 b,a=before[0],after[0];kernel=int(re.findall(r'^module name=KERNEL32.dll actual=(\w+)',events,re.M)[0],16)
 assert[int(v,16)for v in b[2:7]]==[oldbase,UNIT,0x1000,4,kernel+export_rva(original/'loaded-kernel32.bin','VirtualAlloc')]
 assert int(a[1],16)==int(b[1],16)+16 and int(a[2],16)==int(b[1],16)and int(a[3],16)==oldbase and b[7]==a[4]==a[5]=='0'
 regions=re.findall(r'^vector-commit-region part=(\d+) base=(\w+) allocation=(\w+) allocation-protect=(\w+) bytes=(\w+) state=(\w+) protect=(\w+) type=(\w+)$',events,re.M)
 reserve=first['compiler_state']['bss']['arenas'][1]['reserve_bytes'];assert[tuple(int(v,16 if i else 10)for i,v in enumerate(r))for r in regions]==[(0,oldbase,oldbase,4,UNIT,0x1000,4,0x20000),(1,oldbase+UNIT,oldbase,4,reserve-UNIT,0x2000,0,0x20000)]
 entries=json.loads((native/'vector-commit-entries.json').read_text());returns=json.loads((native/'vector-commit-returns.json').read_text());api=json.loads((native/'vector-api-entries.json').read_text());ret=json.loads((native/'vector-api-returns.json').read_text());assert len(entries)==len(returns)==len(api)==len(ret)==1
 entry,end,p,q=entries[0],returns[0],api[0],ret[0]
 method=names['_ZN13CompilerArena12CommitExtentEv'][0]
 assert entry['pc']==method and entry['ecx']==arena and entry['args']==dict(this=arena)and entry['formal_inventory']==[dict(name='this',type='CompilerArena *')]
 assert entry['caller']['symbol']=='new_option_vector'and entry['caller']['args']==dict(count=40,mode=1);call(binary,entry['words'][0],method)
 assert end['pc']==entry['words'][0]and end['esp']==entry['esp']+4 and end['preserved']==entry['preserved']
 expected=copy.deepcopy(first['compiler_state']);expected['bss']['arenas'][1]['cursor']=base+12
 assert entry['compiler_state']==p['compiler_state']==q['compiler_state']==expected
 expected['bss']['arenas'][1].update(committed_limit=base+UNIT,committed_bytes=UNIT);assert end['compiler_state']==expected
 assert p['pc']==names['recon_VirtualAlloc'][0]and p['args']==[base,UNIT,0x1000,4]
 assert p['formal_inventory']==[dict(name=k,type=t)for k,t in [('address','LPVOID'),('bytes','DWORD'),('kind','DWORD'),('protection','DWORD')]]
 assert p['caller']['symbol']=='CompilerArena::CommitExtent'and p['caller']['args']==dict(this=arena);call(binary,p['words'][0],p['pc'])
 assert q['pc']==p['words'][0]and q['esp']==p['esp']+20 and q['eax']==base and q['preserved']==p['preserved']
 for row in [entry,end,p,q]:
  assert row['errors']==[0,0,0]and semantic(row['fp'])==semantic(first['command_line_fp'])
  assert row['crt_state']==first_crt(first)and row['invocation']==first['invocation']
 assert len(q['allocations'])==8 and q['allocations'].count(dict(address=base,length=reserve))==1
 mapped=[]
 for line in q['maps'].splitlines():
  m=re.match(r'^([0-9a-f]+)-([0-9a-f]+) ([-rwxps]+)\s',line);assert m
  start,stop=int(m[1],16),int(m[2],16)
  if start<base+reserve and stop>base:mapped.append((max(start,base),min(stop,base+reserve),m[3]))
 assert mapped==[(base,base+UNIT,'rw-p'),(base+UNIT,base+reserve,'---p')]
 olda,oldb=[list(struct.unpack('<12I',(original/('vector-member-'+phase+'-registers.bin')).read_bytes()))for phase in ('entry','return')]
 assert olda[1]==0x10609883 and olda[8:]==[0x10658fd0,0x10611ea2,olda[2],0]
 assert oldb[1]==0x10611ea2 and oldb[2]==olda[2]+4 and oldb[4:8]==olda[4:8]
 assert olda[0]<int(b[0])<int(a[0])<oldb[0]<registers(original,0,'return')[0]
 assert(original/'vector-member-entry-fp.bin').read_bytes()==(original/'vector-member-return-fp.bin').read_bytes()
 # Exact modeled and neighboring writes for the previously accepted method.
 raw=bytearray((original/'vector-member-entry-target-BSS.bin').read_bytes());struct.pack_into('<I',raw,ARENA-0x50000+4,oldbase+UNIT);struct.pack_into('<I',raw,ARENA-0x50000+12,UNIT)
 assert bytes(raw)==(original/'vector-member-return-target-BSS.bin').read_bytes()
 for suffix in ('data','snapshot'):assert(original/('vector-member-entry-target-'+suffix+'.bin')).read_bytes()==(original/('vector-member-return-target-'+suffix+'.bin')).read_bytes()

def validate_dependency(original,native,state,names,binary):
 provider=original/'loaded-mspdb50.bin';assert provider.stat().st_size==167424 and sha(provider)=='cd4536b0f88ef633dbba509062edae7423407b206199a89ea971b444ff249a33'
 assert export_rva(provider,'SigForPbCb')==0xf22d and(original/'signature-provider-entry-000.bin').read_bytes()==pe_read(provider,0xf22d,3)==bytes.fromhex('558bec')
 w=list(struct.unpack('<12I',(original/'signature-entry-registers.bin').read_bytes()));assert w[1]==w[11]+0xf22d and w[7:11]==[0x10612c31,0x1066318c,1,0xffffffff]
 assert(original/'signature-input-000.bin').read_bytes()==b'C\0'and pe_read(original/'loaded-target.bin',0x6318c,2)==b'C\0'
 source=(original/'relocated-text.bin').read_bytes();assert source[0x12c2b-0x1000:0x12c31-0x1000]==bytes.fromhex('ff15d4d16510')
 assert state['symbol']=='recon_mspdb_SigForPbCb'and state['pc']==names['recon_mspdb_SigForPbCb'][0]
 assert state['formal_inventory']==[dict(name='bytes',type='const void *'),dict(name='byte_count',type='long'),dict(name='signature',type='unsigned long')]
 assert state['args']['byte_count']==1 and state['args']['signature']==0xffffffff
 d=state['dependency_entry'];assert d['stack']==[state['caller']['pc'],state['args']['bytes'],1,0xffffffff]and d['input']==[67]and d['errors']==[0,0,0]
 call(binary,state['caller']['pc'],state['pc'])
 assert list(struct.unpack('<3I',(original/'locale-signature-entry-errors.bin').read_bytes()))==[0,0,0]
 raw=(original/'locale-signature-entry-fp.bin').read_bytes();assert len(raw)==112
 cw,sw,tag=[v&65535 for v in struct.unpack_from('<3I',raw)];top=(sw>>11)&7
 assert semantic(state['command_line_fp'])==dict(cw=cw,sw=sw,tag=tag,top=top,live=[dict(logical=i,tag=(tag>>(2*((top+i)&7)))&3,raw80=raw[28+10*i:38+10*i].hex())for i in range(8)if((tag>>(2*((top+i)&7)))&3)!=3])
 assert 'unreviewed MSPDB SigForPbCb'not in(native/'gdb.txt').read_text()
