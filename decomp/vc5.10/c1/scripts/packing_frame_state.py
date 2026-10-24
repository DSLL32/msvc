"""Genuine packing member return, linked frame and caller alignment store."""
import copy,json,re,struct,subprocess
from pathlib import Path
from integer_state import load,locale_raw
from provenance import captured_native_binary,sha
from vector_state import snapshot
from packing_allocator_state import validate as validate_prefix
from allocation_state import symbols,call
from scaler_state import semantic
from signature_call_state import fp
from frontier_contract import validate_original_frontier,validate_native_stop
BASE=0x10600000

def registers(original,phase):return list(struct.unpack('<16I',(original/('packing-frame-'+phase+'-registers.bin')).read_bytes()))
def words(original,phase,stem,count):return list(struct.unpack('<%uI'%count,(original/('packing-frame-'+phase+'-'+stem+'.bin')).read_bytes()))

def validate(original,native,candidate,report):
 rows=load(native,'packing-member-entries');assert len(rows)==1
 stage=snapshot(original,native,load(native,'state'),rows[0]);prefix=validate_prefix(original,native,candidate,report,stage_state=stage)
 result=validate_endpoint(original,native,candidate);result['prefix']=prefix;return result

def validate_endpoint(original,native,candidate):
 assert candidate['expected_frontier']==dict(kind='packing-frame-complete',rva=0xd49b,native_symbol='push_packing',native_args=['alignment','tag'],calling_convention='cdecl',caller=dict(native_symbol='initialize_packing',return_rva=0x134c5))and candidate['allowed_probes']==['normal']
 assert candidate['packing_frame_observers']==dict(entry_rva=0xd458,return_rva=0x134a3,frontier_rva=0xd49b,object_bytes=8,frame_bytes=12,arena_kind=0)
 f=[r for r in candidate['functions']if r['name']=='CompilerPacking::Push'];assert len(f)==1 and f[0]['chunks']==[[0xd458,0xd49b],[0x2d999,0x2d9ae]]and f[0]['role']=='application'
 events=(original/'events.txt').read_text();validate_original_frontier(events,candidate,'normal');validate_native_stop(native,candidate)
 a,z,m=[registers(original,phase)for phase in ('entry','return','ready')];arenas=[words(original,phase,'arena',8)for phase in ('entry','return','ready')]
 assert a[1]==BASE+0xd458 and a[2]==a[12]and a[4]==a[10]and a[13]==BASE and a[14]==BASE+0x134a3
 assert a[11]==arenas[0][0]and arenas[0][2]<=a[11]<a[11]+12<=arenas[0][1]and arenas[0][3]==65536
 assert z[1]==BASE+0x134a3 and z[2]==a[2]+4 and z[3]==a[11]and z[6:10]==a[6:10]and z[10:14]==a[10:14]
 expected=arenas[0][:];expected[0]+=12;assert arenas[1]==arenas[2]==expected
 objects=[words(original,phase,'object',2)for phase in ('entry','return','ready')];frames=[words(original,phase,'memory',3)for phase in ('entry','return','ready')]
 assert objects==[[0,0],[a[11],0],[a[11],0]]and frames[:2]==[[0,0,0],[0,0,0]]
 raw={k:bytearray(v)for k,v in locale_raw(original,'packing-frame-entry').items()};assert struct.unpack_from('<I',raw['BSS'],0x8fb0)[0]==a[11]
 alignment=struct.unpack_from('<i',raw['data'],0x1398)[0];alignment=8 if alignment==-1 else alignment&0xffffffff
 assert frames[2]==[0,alignment,0]
 assert m[1]==BASE+0xd49b and m[10:14]==a[10:14]and m[14:16]==[BASE+0x134c5,0]
 rows=re.findall(r'^trap seq=(\d+) rva=0000d49b esp=(\w+).*stack=([\w,]+)$',events,re.M);assert len(rows)==1 and [int(v,16)for v in rows[0][2].split(',')][:3]==[BASE+0x134c5,0,0]
 struct.pack_into('<I',raw['BSS'],0x8fb0,a[11]+12)
 for phase in ('return','ready'):
  for region in raw:assert locale_raw(original,'packing-frame-'+phase)[region]==bytes(raw[region])
  assert(original/('state-%03d.bin'%a[0])).read_bytes()==(original/('state-%03d.bin'%(z[0]if phase=='return'else m[0]))).read_bytes()
 binary=captured_native_binary(native);names=symbols(binary);es,rs=load(native,'packing-member-entries'),load(native,'packing-frame-returns');assert len(es)==len(rs)==1;e,r=es[0],rs[0];t=load(native,'packing-frame-ready');state=load(native,'state')
 identity=load(native,'packing-allocation-entries')[0]
 assert e['module_identity']==identity['module_identity']and e['module_token']==identity['module_token']
 assert e['pc']==names['_ZN15CompilerPacking4PushEv'][0]and e['args']=={'this':e['ecx']}and len(e['formal_inventory'])==1 and e['formal_inventory'][0]['name']=='this'
 assert e['frame_layout']==dict(bytes=12,previous=0,alignment=4,tag=8)and e['object_layout']==dict(bytes=8,active=0,free_frames=4)
 address=e['new_frame_address'];before=e['compiler_state']['bss']['arenas'][0];assert before['cursor']==address and before['base']<=address<address+12<=before['committed_limit']and address-before['base']==a[11]-arenas[0][2]
 assert e['object']==[0]*8 and e['frame']==[0]*12
 expected_state=copy.deepcopy(e['compiler_state']);expected_state['bss']['arenas'][0]['cursor']+=12
 assert r['pc']==e['words'][0]and r['esp']==e['esp']+4 and r['eax']==address and r['preserved']==e['preserved']and r['compiler_state']==expected_state
 assert r['object']==list(struct.pack('<II',address,0))and r['frame']==[0]*12
 assert t['object']==r['object']and t['frame']==list(struct.pack('<III',0,alignment,0))and t['compiler_state']==state['compiler_state']==expected_state
 assert state['symbol']=='push_packing'and state['formal_inventory']==[dict(name='alignment',type='unsigned int'),dict(name='tag',type='const void *')]and state['args']==dict(alignment=0,tag=0)
 assert names['push_packing'][0]<state['pc']<=sum(names['push_packing'])and state['pc']==t['pc']
 opcode=subprocess.check_output(['objdump','-d','--start-address='+str(state['pc']-1),'--stop-address='+str(state['pc']),str(binary)],text=True);assert 'int3'in opcode
 assert state['caller']['symbol']=='initialize_packing'and state['caller']['args']=={};call(binary,state['caller']['pc'],names['push_packing'][0])
 assert state['invocation']==e['invocation']
 for key,value in e['crt_state'].items():assert state[key]==value
 arch=semantic(e['fp']);locale=e['locale']
 for phase,row in zip(('entry','return','ready'),(e,r,t)):
  assert row['invocation']==e['invocation']and row['crt_state']==e['crt_state']and row['errors']==[0,0,0]
  assert semantic(row['fp'])==arch==fp((original/('locale-packing-frame-'+phase+'-fp.bin')).read_bytes())
  for suffix in ('words','errors','fp','mbctype','ctype','old-ctype','result','CRT-argv','CRT-environment','Invoke-argv'):
   assert(original/('locale-packing-frame-'+phase+'-'+suffix+'.bin')).read_bytes()==(original/('locale-packing-frame-entry-'+suffix+'.bin')).read_bytes()
  assert{k:v for k,v in row['locale'].items()if k not in ('phase','fp','compiler_state')}=={k:v for k,v in locale.items()if k not in ('phase','fp','compiler_state')}
  assert row['locale']['compiler_state']==row['compiler_state']and semantic(row['locale']['fp'])==arch
 assert semantic(state['command_line_fp'])==arch
 return dict(status='matching88B packing member through actual frame return and caller alignment',candidate='CompilerPacking::Push',code_bytes=88,object_bytes=8,frame_bytes=12,arena_advance=12,member_receiver='actual physical ECX and implicit this; RET0/EAX frame',active_link='actual previousNULL; active=owned frame; freeNULL',caller_stores=dict(alignment=alignment,opaque_tag=0),next=dict(symbol='push_packing',args=dict(alignment=0,tag=0)),new_global_fields=0,UNMODELED_initialized_bytes=4200,unvisited='free-list clear and commit-overflow source/graph only; next normal Push will exercise nonzero predecessor',CRT_credit=0,execution_receipts=dict(original=sha(original/'receipt.json'),native=sha(native/'receipt.json')))
