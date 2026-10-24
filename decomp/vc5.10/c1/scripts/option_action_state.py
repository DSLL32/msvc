"""Three genuine option iterations, borrowed stores and exact public call chain."""
import copy,json,re,struct,subprocess
from allocation_imports import pe_read,export_rva
from allocation_state import symbols,call
from provenance import captured_native_binary,sha
from scaler_state import semantic
from signature_call_state import PROVIDER,fp
from vector_state import snapshot
from option_match_state import validate as validate_matcher
from frontier_contract import validate_original_frontier,validate_native_stop

BASE=0x10600000
RULES=[31,12,23]
VALUES=[b'.\\minimal\0',b'test.c\0',b'1\0']
ARGUMENTS=[b'-il\0',b'-f\0',b'-W\0']

def load(native,name):return json.loads((native/(name+'.json')).read_text())
def registers(original,tag,index):return list(struct.unpack('<16I',(original/('actions-%s-%u-registers.bin'%(tag,index))).read_bytes()))
def blobs(original,tag,index):return {k:(original/('actions-%s-%u-target-%s.bin'%(tag,index,k))).read_bytes()for k in ('data','BSS','snapshot')}
def input_pointer(original,index):
 events=(original/'events.txt').read_text();rows=re.findall(r'^invoke-arg index=%u address=(\w+) bytes=(\d+)$'%index,events,re.M)
 assert len(rows)==1
 return int(rows[0][0],16),int(rows[0][1])

def validate_sequence(original,native,candidate,prefix=None):
 expected=candidate['expected_frontier'];assert expected==dict(kind='option-action-integer',rva=0x132f9,native_symbol='parse_option_integer',native_args=['value'],calling_convention='cdecl',caller=dict(native_symbol='apply_option_action',return_rva=0x1320c))
 row=[r for r in candidate['functions']if r['name']=='_apply_option_action'];assert len(row)==1 and row[0]['chunks']==[[0x131aa,0x13285],[0x3092f,0x309c9],[0x13290,0x132d7]]and row[0]['text_data_chunks']==[[0x13290,0x132d7]]
 assert candidate['option_action_observers']==dict(action_entry_rva=0x131aa,action_return_rvas=[0x130bc,0x13145],frontier_rva=0x132f9,first_rules=RULES,signature_returns=[0x12c31,0x130e2,0x13121],borrowed_values=[2,4,6])
 receipt=load(original,'receipt');probe=receipt['probe'];normal=probe=='normal';assert normal or probe=='restore'
 events=(original/'events.txt').read_text();validate_original_frontier(events,candidate,probe);validate_native_stop(native,candidate)
 assert events.count('option-actions-plan ordinary=1 no-seeds=1 action=000131aa frontier=000132f9\n')==1
 assert events.count('option-actions-complete matches=3 actions=3 returned=2 outer=3 inner=3 signatures=%u actual-frontier=000132f9\n'%(3 if normal else 2))==1
 state=load(native,'state');actions=load(native,'option-action-entries');action_returns=load(native,'option-action-returns');matches=load(native,'option-match-entries');match_returns=load(native,'option-match-returns')
 outer=load(native,'argument-cursor-entries');outer_returns=load(native,'argument-cursor-returns');inner=load(native,'option-next-entries');inner_returns=load(native,'option-next-returns')
 apis=load(native,'signature-call-entries');api_returns=load(native,'signature-call-returns')
 assert len(actions)==len(matches)==len(outer)==len(outer_returns)==len(inner)==len(inner_returns)==3
 assert len(action_returns)==len(match_returns)==2 and len(apis)==len(api_returns)==(3 if normal else 2)
 binary=captured_native_binary(native);names=symbols(binary);inv=matches[0]['invocation'];known=copy.deepcopy(outer[0]['compiler_state']);raw={k:bytearray(v)for k,v in blobs(original,'outer-entry',0).items()}
 initial_warning=known['option_state']['initialized']['warning_level']
 crt=outer[0]['crt_state'];architecture=semantic(outer[0]['fp']);assert architecture['live']==[]
 original_crt=(original/('state-%03d.bin'%registers(original,'outer-entry',0)[0])).read_bytes()
 assert len(original_crt)==164
 sequence=[]
 def check(tag,index,record):
  r=registers(original,tag,index);sequence.append(r[0]);assert r[15]==BASE
  assert(original/('state-%03d.bin'%r[0])).read_bytes()==original_crt
  assert record['compiler_state']==known,'complete native modeled state at %s %u'%(tag,index)
  assert blobs(original,tag,index)=={k:bytes(v)for k,v in raw.items()},'complete original state at %s %u'%(tag,index)
  assert record['invocation']==inv and record['crt_state']==crt and record['errors']==[0,0,0]
  assert semantic(record['fp'])==architecture==fp((original/('actions-%s-%u-fp.bin'%(tag,index))).read_bytes())
  return r
 def cursor(tag,index,entry,end,argument_index,value):
  a=check(tag+'-entry',index,entry);rva=0x12c45 if tag=='outer'and not index else 0x12c63 if tag=='outer'else 0x13137 if index==1 else 0x130ae
  assert a[1]==BASE+0x12f9c and a[10]==BASE+rva
  assert entry['symbol']=='next_argument'and entry['pc']==names['next_argument'][0]and entry['args']=={}and entry['formal_inventory']==[]and entry['words']==[entry['caller']['pc']]
  assert entry['caller']['symbol']==('parse_command_line'if tag=='outer'else'match_option')
  if tag=='outer':call(binary,entry['words'][0],entry['pc'])
  else:
   # The callback is the actual next formal at EBP+16, not a direct CALL.
   pc=entry['words'][0];text=subprocess.check_output(['objdump','-d','--start-address='+str(pc-3),'--stop-address='+str(pc),str(binary)],text=True)
   assert re.search(r'ff 55 10\s+call\s+\*0x10\(%ebp\)',text)and entry['caller']['args']['next']==entry['pc']
  argc,argv=struct.unpack_from('<II',raw['BSS'],0x5c38);assert argc==12-argument_index
  struct.pack_into('<II',raw['BSS'],0x5c38,argc-1,argv+4);known['bss']['command_argc']-=1;known['bss']['command_argv']+=4
  b=check(tag+'-return',index,end);oldptr,size=input_pointer(original,argument_index);assert size==len(value)
  assert b[1]==a[10]and b[2]==a[2]+4 and b[6:10]==a[6:10]and b[3]==oldptr
  assert end['pc']==entry['words'][0]and end['esp']==entry['esp']+4 and end['preserved']==entry['preserved']and end['eax']==inv['argv_pointers'][argument_index]
  assert end['result_bytes']==list(value)and(original/('actions-%s-%u-value.bin'%(tag,index))).read_bytes()==value
  return oldptr
 api_proofs=[]
 if normal:
  # Full original/native C entry and return were already checked in the prefix;
  # this independent inventory also binds the repeated-call schema and identity.
  api_proofs.append(validate_signature(original,native,0,apis[0],api_returns[0],names,binary,normal=True))
 for i,rule_index in enumerate(RULES):
  argument_index=1+i*2;oldarg=cursor('outer',i,outer[i],outer_returns[i],argument_index,ARGUMENTS[i])
  a=check('match-entry',i,matches[i]);assert a[1]==BASE+0x12fbe and a[10:15]==[BASE+(0x12c50 if not i else 0x12c6e),BASE+0x62b78,oldarg,BASE+0x12f9c,0]
  match_args=dict(rules=names['option_rules'][0],argument=inv['argv_pointers'][argument_index],next=names['next_argument'][0],copy=0)
  assert matches[i]['args']==match_args and matches[i]['formal_inventory']==matches[0]['formal_inventory'];call(binary,matches[i]['words'][0],names['match_option'][0])
  known['bss']['current_argument']=inv['argv_pointers'][argument_index];struct.pack_into('<I',raw['BSS'],0x38b0,oldarg)
  oldvalue=cursor('inner',i,inner[i],inner_returns[i],argument_index+1,VALUES[i]);assert inner[i]['caller']['args']==match_args
  c=check('action-entry',i,actions[i]);assert c[1]==BASE+0x131aa and c[10:14]==[BASE+(0x13145 if i==1 else 0x130bc),BASE+0x62b78+rule_index*12,oldvalue,0]
  assert a[0]<registers(original,'inner-entry',i)[0]<registers(original,'inner-return',i)[0]<c[0]
  formals=[dict(name=k,type=t)for k,t in [('rule','CompilerOptionRule *'),('value','char *'),('copy','int')]]
  args=dict(rule=names['option_rules'][0]+rule_index*12,value=inv['argv_pointers'][argument_index+1],copy=0)
  assert actions[i]['symbol']=='apply_option_action'and actions[i]['pc']==names['apply_option_action'][0]and actions[i]['formal_inventory']==formals and actions[i]['args']==args and actions[i]['value_bytes']==list(VALUES[i])
  assert actions[i]['words']==[actions[i]['caller']['pc'],args['rule'],args['value'],0]and actions[i]['caller']['symbol']=='match_option'and actions[i]['caller']['args']==match_args;call(binary,actions[i]['words'][0],names['apply_option_action'][0])
  pattern,target,result,action=struct.unpack_from('<IIbB',pe_read(original/'loaded-target.bin',0x62b78+rule_index*12,12))
  assert action==(0x24 if i==2 else 0x22)and result==1
  if i==2:break
  if not i:
   assert target==BASE+0x5f394
   struct.pack_into('<I',raw['data'],0x1394,oldvalue);known['option_state']['initialized']['il_prefix']=args['value']
  else:
   assert target==BASE+0x54170
   struct.pack_into('<I',raw['BSS'],0x4170,oldvalue);known['option_state']['bss']['source_file']=args['value']
  d=check('action-return',i,action_returns[i]);assert d[1]==c[10]and d[2]==c[2]+4 and d[6:10]==c[6:10]
  assert action_returns[i]['index']==i and action_returns[i]['pc']==actions[i]['words'][0]and action_returns[i]['esp']==actions[i]['esp']+4 and action_returns[i]['preserved']==actions[i]['preserved']
  if i==1:
   for j in range(2):
    index=j+(1 if normal else 0);entry,end=apis[index],api_returns[index]
    proof=validate_signature(original,native,index,entry,end,names,binary,normal=normal);api_proofs.append(proof)
    for phase in ('entry','return'):
     seqapi=struct.unpack_from('<I',(original/('actions-signature-%u-%s-registers.bin'%(index,phase))).read_bytes())[0]
     assert(original/('state-%03d.bin'%seqapi)).read_bytes()==original_crt
    assert entry['compiler_state']==end['compiler_state']==known
    stem='actions-signature-%u-'%index
    assert (original/('locale-'+stem+'entry-target-data.bin')).read_bytes()==bytes(raw['data'])and(original/('locale-'+stem+'entry-target-BSS.bin')).read_bytes()==bytes(raw['BSS'])and(original/('locale-'+stem+'entry-target-snapshot.bin')).read_bytes()==bytes(raw['snapshot'])
    assert entry['args']['signature']==known['bss']['pdb_signature']
    known['bss']['pdb_signature']=end['eax'];struct.pack_into('<I',raw['BSS'],0x4de0,end['eax'])
  e=check('match-return',i,match_returns[i]);assert e[1]==a[10]and e[2]==a[2]+4 and e[6:10]==a[6:10]and e[3]==1
  assert match_returns[i]['index']==i and match_returns[i]['pc']==matches[i]['words'][0]and match_returns[i]['esp']==matches[i]['esp']+4 and match_returns[i]['preserved']==matches[i]['preserved']and match_returns[i]['eax']==1
 assert sequence==sorted(set(sequence))
 frontier=re.findall(r'^trap seq=(\d+) rva=000132f9 esp=(\w+).*stack=([\w,]+)$',events,re.M);assert len(frontier)==1
 seq=int(frontier[0][0]);assert(original/('state-%03d.bin'%seq)).read_bytes()==original_crt
 words=[int(v,16)for v in frontier[0][2].split(',')];assert words[:2]==[BASE+0x1320c,input_pointer(original,6)[0]]
 for region,stem in [('data','compiler-data'),('BSS','compiler-bss'),('snapshot','compiler-snapshot')]:assert(original/('%s-%03d.bin'%(stem,seq))).read_bytes()==bytes(raw[region])
 assert state['compiler_state']==known and state['symbol']=='parse_option_integer'and names['parse_option_integer'][0]<state['pc']<=sum(names['parse_option_integer'])
 opcode=subprocess.check_output(['objdump','-d','--start-address='+str(state['pc']-1),'--stop-address='+str(state['pc']),str(binary)],text=True);assert 'int3'in opcode
 assert state['args']==dict(value=inv['argv_pointers'][6])and state['formal_inventory']==[dict(name='value',type='const char *')]
 assert state['caller']['symbol']=='apply_option_action'and state['caller']['args']==actions[2]['args'];call(binary,state['caller']['pc'],names['parse_option_integer'][0])
 assert state['invocation']==inv and semantic(state['command_line_fp'])==architecture
 assert state['module_identity']==actions[0]['module_identity']and state['module_token']==actions[0]['module_token']and state['pass_mode']==known['pass_mode']
 for key,value in crt.items():assert state[key]==value
 assert known['option_state']['initialized']['warning_level']==initial_warning, 'pending integer conversion has not stored warning value'
 return dict(status='matching373B action through three option iterations and genuine integer frontier',candidate='_apply_option_action',code_bytes=373,separate_text_data_bytes=71,action_entries=3,action_returns=2,outer_callbacks=3,inner_callbacks=3,signature_calls=api_proofs,borrowed_assignments=['il_prefix=Invoke argv[2]','source_file=Invoke argv[4]'],integer_frontier='parse_option_integer(Invoke argv[6] = 1)',unvisited_action_branches='full source/graph only',initialized_UNMODELED_bytes=4200,CRT_credit=0,prefix=prefix,execution_receipts=dict(original=sha(original/'receipt.json'),native=sha(native/'receipt.json')))

def validate_signature(original,native,index,entry,end,names,binary,normal):
 provider=original/'loaded-mspdb50.bin';assert sha(provider)==PROVIDER and export_rva(provider,'SigForPbCb')==0xf22d
 a,b=[list(struct.unpack('<17I',(original/('actions-signature-%u-%s-registers.bin'%(index,phase))).read_bytes()))for phase in ('entry','return')]
 first=normal and index==0;following_index=index-(1 if normal else 0);value=b'C\0'if first else(b'test.c\0'if following_index==0 else b'-f\0');returned=0x12c31 if first else 0x130e2 if following_index==0 else 0x13121
 assert a[16]==b[16]==index and a[1]==a[11]+0xf22d and b[1]==BASE+returned and a[10:12]==b[10:12]==[BASE+returned,a[11]]
 assert b[2]==a[2]+4 and b[6:10]==a[6:10]and a[12:14]==b[12:14]and a[13]==len(value)-1 and b[14]==b[3]
 if first:assert a[12:15]==[BASE+0x6318c,1,0xffffffff]
 else:
  assert a[12]==input_pointer(original,4 if following_index==0 else 3)[0]
  assert entry['args']['bytes']==entry['invocation']['argv_pointers'][4 if following_index==0 else 3]
 body=bytearray(pe_read(provider,0xf22d,53));assert body[0]==0x55 and body[-1]==0xc3;struct.pack_into('<I',body,34,a[11]+0x21a30);body[0]=0xcc
 assert(original/('actions-signature-%u-provider-body-000.bin'%index)).read_bytes()==bytes(body)
 for phase in ('entry','return'):
  stem='actions-signature-%u-%s-'%(index,phase)
  assert(original/(stem+'input-000.bin')).read_bytes()==value
  assert(original/(stem+'table-000.bin')).read_bytes()==pe_read(provider,0x21a30,1024)
  assert(original/(stem+'vector-000.bin')).read_bytes()==(original/'vector-committed-002.bin').read_bytes()
  assert(native/(stem+'vector.bin')).read_bytes()==(native/'vector-committed-002.bin').read_bytes()
 assert(original/('locale-actions-signature-%u-entry-errors.bin'%index)).read_bytes()==bytes(12)
 for suffix in ('words','errors','fp','mbctype','ctype','old-ctype','result','CRT-argv','CRT-environment','Invoke-argv','target-data','target-BSS','target-snapshot'):
  assert(original/('locale-actions-signature-%u-entry-%s.bin'%(index,suffix))).read_bytes()==(original/('locale-actions-signature-%u-return-%s.bin'%(index,suffix))).read_bytes()
 assert entry['index']==end['index']==index and entry['symbol']=='recon_mspdb_SigForPbCb'and entry['pc']==names['recon_mspdb_SigForPbCb'][0]
 assert entry['formal_inventory']==[dict(name=k,type=t)for k,t in [('bytes','const void *'),('byte_count','long'),('signature','unsigned long')]]
 assert entry['args']==dict(bytes=entry['words'][1],byte_count=len(value)-1,signature=a[14])and entry['words']==[entry['caller']['pc'],entry['args']['bytes'],len(value)-1,a[14]]and entry['input']==list(value)
 assert entry['caller']['symbol']==('parse_command_line'if first else'match_option');call(binary,entry['words'][0],entry['pc'])
 assert end['pc']==entry['words'][0]and end['esp']==entry['esp']+4 and end['preserved']==entry['preserved']and end['eax']==b[3]
 assert end['argument_slots_after']==[entry['args']['bytes'],len(value)-1,b[3]]
 assert entry['compiler_state']==end['compiler_state']and entry['invocation']==end['invocation']and entry['crt_state']==end['crt_state']and entry['errors']==end['errors']==[0,0,0]
 assert semantic(entry['fp'])==semantic(end['fp'])==fp((original/('locale-actions-signature-%u-entry-fp.bin'%index)).read_bytes())
 left,right=entry['locale'],end['locale'];assert {k:v for k,v in left.items()if k not in ('phase','fp')}=={k:v for k,v in right.items()if k not in ('phase','fp')}
 return dict(index=index,input=value[:-1].decode('ascii'),count=len(value)-1,seed=a[14],result=b[3],caller_return_rva=returned,original_third_incoming_slot='actually changes to result; native actual slot matches; caller discards12',provider_sha256=PROVIDER,application_credit=0)


def validate(original,native,candidate,report):
 state=load(native,'state');first=load(native,'option-action-entries')[0]
 prefix=validate_matcher(original,native,candidate,report,stage_state=snapshot(original,native,state,first))
 return validate_sequence(original,native,candidate,prefix=prefix)
