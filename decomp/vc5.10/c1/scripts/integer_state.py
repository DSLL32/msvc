"""Whole option loop, numeric leaf, true public ABI and consumed ctype predicates."""
import copy,json,re,struct,subprocess
from pathlib import Path
from provenance import captured_native_binary,sha
from allocation_imports import pe_read,export_rva
from allocation_state import symbols,call
from scaler_state import semantic
from signature_call_state import PROVIDER,fp
from option_action_state import input_pointer,registers,blobs
from option_match_state import validate as validate_first_match
from vector_state import snapshot
from frontier_contract import validate_original_frontier,validate_native_stop
from command_line_fields import DATA,BSS

BASE=0x10600000
RULES=[31,12,23,76,58,53,1]
ARG_INDEX=[1,3,5,7,8,9,10,None]
VALUE_ROLE=[(2,0),(4,0),(6,0),(7,2),None,(9,3),None]
TARGETS=[(0x5f394,'il_prefix'),(0x54170,'source_file'),(0x5f360,'warning_level'),(0x5ef84,'processor'),(0x5efac,'optimize_size'),(0x5ef8c,'inline_level'),(0x550ec,'suppress_banner')]
NUMBERS=[1,5,0]
KINDS=['isspace','isdigit','atol']
PUBLIC=[(0x1c0be,0x13313),(0x1c065,0x13329),(0x5d30,0x1333b)]

def load(p,name):return json.loads((p/(name+'.json')).read_text())
def numeric_registers(original,index,phase):return list(struct.unpack('<16I',(original/('integer-%u-%s-registers.bin'%(index,phase))).read_bytes()))
def api_registers(original,index,kind,phase):return list(struct.unpack('<17I',(original/('integer-%u-api-%s-%s-registers.bin'%(index,kind,phase))).read_bytes()))
def sig_registers(original,index,phase):return list(struct.unpack('<17I',(original/('actions-signature-%u-%s-registers.bin'%(index,phase))).read_bytes()))
def locale_raw(original,phase):return {key:(original/('locale-'+phase+'-target-'+key+'.bin')).read_bytes()for key in ('data','BSS','snapshot')}
def raw_fp(original,phase):return fp((original/('locale-'+phase+'-fp.bin')).read_bytes())

def validate(original,native,candidate,report,stage_state=None):
 first=load(native,'option-action-entries')[0];state=load(native,'state')
 prefix=validate_first_match(original,native,candidate,report,stage_state=snapshot(original,native,state,first))
 result=validate_loop(original,native,candidate,stage_state=stage_state);result['prefix']=prefix
 return result

def validate_loop(original,native,candidate,stage_state=None):
 expected=candidate['expected_frontier']
 if stage_state is None:assert expected==dict(kind='option-integers-complete',rva=0x1335b,native_symbol='resolve_option_path',native_args=['output','input','size'],calling_convention='cdecl',caller=dict(native_symbol='parse_command_line',return_rva=0x12c8b))
 else:assert expected['kind']=='option-path-normalizer'and expected['rva']==0x1344d and candidate['allowed_probes']==['normal']
 fn=[f for f in candidate['functions']if f['name']=='_parse_option_integer'];assert len(fn)==1 and fn[0]['chunks']==[[0x132f9,0x1335b],[0x30a17,0x30a3c]]and fn[0]['role']=='application'
 assert candidate['integer_observers']==dict(entry_rva=0x132f9,return_rva=0x1320c,frontier_rva=0x1335b,expected_inputs=['1','5','0'],isspace_iat_rva=0x5d12c,isdigit_iat_rva=0x5d018,atol_iat_rva=0x5d07c)
 receipt=load(original,'receipt');normal=receipt['probe']=='normal';assert normal or receipt['probe']=='restore'
 events=(original/'events.txt').read_text();validate_original_frontier(events,candidate,receipt['probe']);validate_native_stop(native,candidate)
 assert events.count('option-actions-complete matches=8 actions=7 returned=7 outer=8 inner=3 signatures=%u actual-frontier=%08x\n'%((9 if normal else 8),expected['rva']))==1
 binary=captured_native_binary(native);names=symbols(binary);state=load(native,'state')if stage_state is None else stage_state
 grouped={name:load(native,file)for name,file in [('outer-entry','argument-cursor-entries'),('outer-return','argument-cursor-returns'),('inner-entry','option-next-entries'),('inner-return','option-next-returns'),('match-entry','option-match-entries'),('match-return','option-match-returns'),('action-entry','option-action-entries'),('action-return','option-action-returns'),('numeric-entry','integer-entries'),('numeric-return','integer-returns'),('signature-entry','signature-call-entries'),('signature-return','signature-call-returns'),('public-entry','integer-api-entries'),('public-return','integer-api-returns')]}
 for name,rows in grouped.items():assert len(rows)==(8 if name.startswith(('outer','match'))else 3 if name.startswith(('inner','numeric'))else 7 if name.startswith('action')else 9 if name.startswith('public')else 9 if normal else 8)
 known=copy.deepcopy(grouped['outer-entry'][0]['compiler_state']);inv=grouped['outer-entry'][0]['invocation'];crt=grouped['outer-entry'][0]['crt_state'];architecture=semantic(grouped['outer-entry'][0]['fp'])
 raw={key:bytearray(v)for key,v in blobs(original,'outer-entry',0).items()};first_seq=registers(original,'outer-entry',0)[0];crt_raw=(original/('state-%03d.bin'%first_seq)).read_bytes();assert len(crt_raw)==164
 pending=[]
 for name,rows in grouped.items():
  for index,row in enumerate(rows):
   if name.startswith('signature'):
    if normal and not index:continue # Separately proved actual first C call in prefix.
    r=sig_registers(original,index,name.split('-')[1]);phase='actions-signature-%u-%s'%(index,name.split('-')[1]);regions=locale_raw(original,phase);floating=raw_fp(original,phase)
   elif name.startswith('public'):
    index,part=divmod(index,3);kind=KINDS[part];r=api_registers(original,index,kind,name.split('-')[1]);phase='integer-%u-api-%s-%s'%(index,kind,name.split('-')[1]);regions=locale_raw(original,phase);floating=raw_fp(original,phase)
   elif name.startswith('numeric'):
    r=numeric_registers(original,index,name.split('-')[1]);phase='integer-%u-%s'%(index,name.split('-')[1]);regions=locale_raw(original,phase);floating=raw_fp(original,phase)
   else:
    r=registers(original,name,index);regions=blobs(original,name,index);floating=fp((original/('actions-%s-%u-fp.bin'%(name,index))).read_bytes())
   pending.append((r[0],name,index,row,r,regions,floating))
 assert len({p[0]for p in pending})==len(pending)
 saved_numeric={};proofs=[]
 for sequence,name,index,row,r,regions,floating in sorted(pending):
  phase=name.split('-')[1];enter=phase=='entry'
  if name.startswith(('outer','inner')):
   inner=name.startswith('inner');entry=grouped[('inner'if inner else'outer')+'-entry'][index];a=registers(original,('inner'if inner else'outer')+'-entry',index)
   arg_index=(index*2+2)if inner else ARG_INDEX[index]
   if not enter:
    argc,argv=struct.unpack_from('<II',raw['BSS'],0x5c38);assert argc==known['bss']['command_argc']and argc>0
    known['bss']['command_argc']-=1;struct.pack_into('<I',raw['BSS'],0x5c38,argc-1)
    if argc>1:known['bss']['command_argv']+=4;struct.pack_into('<I',raw['BSS'],0x5c3c,argv+4)
    old=0 if arg_index is None else input_pointer(original,arg_index)[0];new=0 if arg_index is None else inv['argv_pointers'][arg_index]
    value=b''if arg_index is None else inv['argv'][arg_index].encode()+b'\0'
    assert r[1]==a[10]and r[2]==a[2]+4 and r[3]==old and r[6:10]==a[6:10]
    assert row['pc']==entry['words'][0]and row['esp']==entry['esp']+4 and row['eax']==new and row['preserved']==entry['preserved']and row['result_bytes']==list(value)
    assert(original/('actions-%s-%u-value.bin'%('inner'if inner else'outer',index))).read_bytes()==value
   else:
    assert r[1]==BASE+0x12f9c and r[10]==BASE+(0x13137 if index==1 else 0x130ae)if inner else r[1]==BASE+0x12f9c and r[10]==BASE+(0x12c45 if not index else 0x12c63)
    assert row['symbol']=='next_argument'and row['pc']==names['next_argument'][0]and row['formal_inventory']==[]and row['args']=={}and row['words']==[row['caller']['pc']]
    assert row['caller']['symbol']==('match_option'if inner else'parse_command_line')
    if not inner:call(binary,row['words'][0],row['pc'])
    else:
     assert row['caller']['args']==grouped['match-entry'][index]['args'];pc=row['words'][0];ins=subprocess.check_output(['objdump','-d','--start-address='+str(pc-3),'--stop-address='+str(pc),str(binary)],text=True);assert re.search(r'ff 55 10\s+call\s+\*0x10\(%ebp\)',ins)
  elif name.startswith('match'):
   entry=grouped['match-entry'][index];a=registers(original,'match-entry',index);arg_index=ARG_INDEX[index];oldarg=0 if arg_index is None else input_pointer(original,arg_index)[0];newarg=0 if arg_index is None else inv['argv_pointers'][arg_index]
   if enter:
    assert r[1]==BASE+0x12fbe and r[10:15]==[BASE+(0x12c50 if not index else 0x12c6e),BASE+0x62b78,oldarg,BASE+0x12f9c,0]
    assert row['symbol']=='match_option'and row['pc']==names['match_option'][0]and row['args']==dict(rules=names['option_rules'][0],argument=newarg,next=names['next_argument'][0],copy=0)
    assert row['formal_inventory']==[dict(name=k,type=t)for k,t in [('rules','CompilerOptionRule *'),('argument','char *'),('next','char *(*)(void)'),('copy','int')]];call(binary,row['words'][0],row['pc'])
   else:
    assert r[1]==a[10]and r[2]==a[2]+4 and r[6:10]==a[6:10]and r[3]==(0 if index==7 else 1)
    assert row['index']==index and row['pc']==entry['words'][0]and row['esp']==entry['esp']+4 and row['preserved']==entry['preserved']and row['eax']==(0 if index==7 else 1)
  elif name.startswith('action'):
   entry=grouped['action-entry'][index];a=registers(original,'action-entry',index);rule=RULES[index];role=VALUE_ROLE[index]
   oldvalue=0 if role is None else input_pointer(original,role[0])[0]+role[1];newvalue=0 if role is None else inv['argv_pointers'][role[0]]+role[1]
   if enter:
    assert r[1]==BASE+0x131aa and r[10:14]==[BASE+(0x130bc if index in(0,2)else 0x13145 if index==1 else 0x130f8),BASE+0x62b78+rule*12,oldvalue,0]
    assert row['index']==index and row['symbol']=='apply_option_action'and row['pc']==names['apply_option_action'][0]and row['args']==dict(rule=names['option_rules'][0]+rule*12,value=newvalue,copy=0)
    assert row['formal_inventory']==[dict(name=k,type=t)for k,t in [('rule','CompilerOptionRule *'),('value','char *'),('copy','int')]]and row['caller']['args']==grouped['match-entry'][index]['args'];call(binary,row['words'][0],row['pc'])
    value=b''if role is None else inv['argv'][role[0]][role[1]:].encode()+b'\0';assert row['value_bytes']==list(value)
   else:
    target,label=TARGETS[index];table=pe_read(original/'loaded-target.bin',0x62b78+rule*12,12);assert struct.unpack_from('<I',table,4)[0]==BASE+target and table[9]==(0x22 if index<2 else 0x24 if index in(2,3,5)else 1)
    value=oldvalue if index<2 else NUMBERS[(2,3,5).index(index)]if index in(2,3,5)else 1;new=newvalue if index<2 else value
    region='data'if target>=0x5e000 else'BSS';origin=0x5e000 if region=='data'else 0x50000;struct.pack_into('<I',raw[region],target-origin,value)
    known['option_state']['initialized'if region=='data'else'bss'][label]=new
    if index in(2,3,5):assert saved_numeric[(2,3,5).index(index)]==value
    assert r[1]==a[10]and r[2]==a[2]+4 and r[6:10]==a[6:10]
    assert row['index']==index and row['pc']==entry['words'][0]and row['esp']==entry['esp']+4 and row['preserved']==entry['preserved']
  elif name.startswith('numeric'):
   entry=grouped['numeric-entry'][index];a=numeric_registers(original,index,'entry');action=grouped['action-entry'][(2,3,5)[index]];oldptr=registers(original,'action-entry',(2,3,5)[index])[12]
   assert r[14:]==[oldptr,index]and(original/('integer-%u-%s-input.bin'%(index,phase))).read_bytes()==str(NUMBERS[index]).encode()+b'\0'
   if enter:
    assert r[1]==BASE+0x132f9 and r[10:12]==[BASE+0x1320c,oldptr]
    assert row['symbol']=='parse_option_integer'and row['pc']==names['parse_option_integer'][0]and row['index']==index and row['formal_inventory']==[dict(name='value',type='const char *')]and row['args']==dict(value=action['args']['value'])and row['input']==list(str(NUMBERS[index]).encode()+b'\0')
    assert row['caller']['symbol']=='apply_option_action'and row['caller']['args']==action['args'];call(binary,row['words'][0],row['pc'])
   else:
    assert r[1]==BASE+0x1320c and r[2]==a[2]+4 and r[6:10]==a[6:10]and r[3]==NUMBERS[index]
    assert row['index']==index and row['pc']==entry['words'][0]and row['esp']==entry['esp']+4 and row['preserved']==entry['preserved']and row['eax']==NUMBERS[index];saved_numeric[index]=row['eax']
  elif name.startswith('public'):
   kind=row['kind'];part=KINDS.index(kind);assert row['index']==index
   a=api_registers(original,index,kind,'entry');entry=grouped['public-entry'][index*3+part];end=grouped['public-return'][index*3+part]
   assert a[12:14]==[part,index]and a[1]==0x78000000+PUBLIC[part][0]and a[10]==BASE+PUBLIC[part][1]and a[15]==0x78000000
   argument=numeric_registers(original,index,'entry')[14]if part==2 else ord(str(NUMBERS[index]));nativearg=grouped['numeric-entry'][index]['args']['value']if part==2 else argument
   assert a[11]==a[16]==argument and row['locale']['mb_cur_max']==1
   if enter:
    assert row['words']==[row['caller']['pc'],nativearg]and row['caller']['symbol']=='parse_option_integer'and row['caller']['args']==grouped['numeric-entry'][index]['args']
    physical=row['physical_import'];assert physical['resolved']==row['pc']and physical['got']>0 and bytes.fromhex(physical['call_bytes'])[0]==0xe8 and bytes.fromhex(physical['plt_bytes'])[:2]==b'\xff\x25'
    if part==2:assert row['input']==list(str(NUMBERS[index]).encode()+b'\0')
   else:
    assert r[1]==a[10]and r[2]==a[2]+4 and r[6:10]==a[6:10]and r[16]==argument
    assert row['pc']==entry['words'][0]and row['esp']==entry['esp']+4 and row['preserved']==entry['preserved']and row['argument_slot_after']==nativearg
    if part==2:assert row['eax']==r[3]==NUMBERS[index]
    else:
     table=(original/('locale-integer-%u-api-%s-entry-ctype.bin'%(index,kind))).read_bytes();assert len(table)==514
     bits=struct.unpack_from('<H',table,(argument+1)*2)[0];assert r[3]==bits&(8 if part==0 else 4)
     assert bool(row['eax'])==bool(r[3])==(part==1)
    proofs.append(dict(index=index,kind=kind,original_raw=r[3],native_raw=row['eax'],consumed='exact signed32 result'if part==2 else'nonzero predicate only',application_credit=0))
   phase_name='integer-%u-api-%s-'%(index,kind)
   for suffix in ('words','errors','fp','mbctype','ctype','old-ctype','result','CRT-argv','CRT-environment','Invoke-argv','target-data','target-BSS','target-snapshot'):
    assert(original/('locale-'+phase_name+'entry-'+suffix+'.bin')).read_bytes()==(original/('locale-'+phase_name+'return-'+suffix+'.bin')).read_bytes()
   assert(original/('locale-'+phase_name+'entry-errors.bin')).read_bytes()==bytes(12)
   if part==2:assert(original/('integer-%u-api-atol-%s-input.bin'%(index,phase))).read_bytes()==str(NUMBERS[index]).encode()+b'\0'
   assert entry['compiler_state']==end['compiler_state']and entry['crt_state']==end['crt_state']and entry['invocation']==end['invocation']and entry['fp']==end['fp']and entry['errors']==end['errors']
   assert {k:v for k,v in entry['locale'].items()if k not in('phase','fp')}=={k:v for k,v in end['locale'].items()if k not in('phase','fp')}
  elif name.startswith('signature'):
   part=index-(1 if normal else 0);roles=[(4,0),(3,0),(6,0),(5,0),(7,0),(8,0),(9,0),(10,0)];role=roles[part];value=inv['argv'][role[0]].encode()+b'\0';oldptr=input_pointer(original,role[0])[0];newptr=inv['argv_pointers'][role[0]]
   a=sig_registers(original,index,'entry');entry=grouped['signature-entry'][index];end=grouped['signature-return'][index];ret=0x130e2 if part in(0,2)else 0x13121
   assert a[16]==index and a[1]==a[11]+0xf22d and a[10]==BASE+ret and a[12:15]==[oldptr,len(value)-1,known['bss']['pdb_signature']]
   if enter:
    assert row['index']==index and row['args']==dict(bytes=newptr,byte_count=len(value)-1,signature=known['bss']['pdb_signature'])and row['input']==list(value)and row['symbol']=='recon_mspdb_SigForPbCb'and row['pc']==names['recon_mspdb_SigForPbCb'][0]
    assert row['formal_inventory']==[dict(name=k,type=t)for k,t in [('bytes','const void *'),('byte_count','long'),('signature','unsigned long')]];call(binary,row['words'][0],row['pc'])
   else:
    assert r[1]==a[10]and r[2]==a[2]+4 and r[6:10]==a[6:10]and r[12:15]==[oldptr,len(value)-1,r[3]]
    assert row['index']==index and row['pc']==entry['words'][0]and row['esp']==entry['esp']+4 and row['preserved']==entry['preserved']and row['eax']==r[3]and row['argument_slots_after']==[newptr,len(value)-1,r[3]]
   for phase_sig in ('entry','return'):
    stem='actions-signature-%u-%s-'%(index,phase_sig);assert(original/(stem+'input-000.bin')).read_bytes()==value
    assert(original/(stem+'table-000.bin')).read_bytes()==pe_read(original/'loaded-mspdb50.bin',0x21a30,1024)
    assert(original/(stem+'vector-000.bin')).read_bytes()==(original/'vector-committed-002.bin').read_bytes();assert(native/(stem+'vector.bin')).read_bytes()==(native/'vector-committed-002.bin').read_bytes()
   body=bytearray(pe_read(original/'loaded-mspdb50.bin',0xf22d,53));struct.pack_into('<I',body,34,a[11]+0x21a30);body[0]=0xcc;assert(original/('actions-signature-%u-provider-body-000.bin'%index)).read_bytes()==bytes(body)
  assert row['compiler_state']==known,'complete modeled state %s %u'%(name,index)
  assert regions=={k:bytes(v)for k,v in raw.items()},'complete original state %s %u'%(name,index)
  assert(original/('state-%03d.bin'%sequence)).read_bytes()==crt_raw
  assert row['invocation']==inv and row['crt_state']==crt and row['errors']==[0,0,0]and semantic(row['fp'])==floating==architecture
  if name=='match-entry'and ARG_INDEX[index]is not None:
   known['bss']['current_argument']=inv['argv_pointers'][ARG_INDEX[index]];struct.pack_into('<I',raw['BSS'],0x38b0,input_pointer(original,ARG_INDEX[index])[0])
  if name=='signature-return':known['bss']['pdb_signature']=r[3];struct.pack_into('<I',raw['BSS'],0x4de0,r[3])
 assert saved_numeric=={0:1,1:5,2:0}
 validate_public_static(original,native,proofs)
 endpoint=validate_endpoint(original,native,candidate,state,grouped,known,raw,crt,architecture,names,binary,crt_raw,following=stage_state is not None)
 return dict(status='matching whole135B integer parser and complete option loop',application_candidate='_parse_option_integer',candidate_code_bytes=135,numeric_inputs=['1','5','0'],numeric_returns=[1,5,0],public_calls=proofs,matcher_calls=8,action_calls=7,outer_callbacks=8,inner_callbacks=3,signature_calls=9 if normal else 8,endpoint=endpoint,unvisited_paths='whitespace/invalid/range errors remain source+graph only',initialized_known_bytes=2348,initialized_UNMODELED_bytes=4200,CRT_credit=0,execution_receipts=dict(original=sha(original/'receipt.json'),native=sha(native/'receipt.json')))


def validate_public_static(original,native,proofs):
 from integer_imports import check_static
 check_static();provider=original/'loaded-msvcrt.bin';assert sha(provider)=='e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210'
 for kind,(rva,ret)in zip(KINDS,PUBLIC):
  assert export_rva(provider,kind)==rva
  assert(original/('integer-provider-'+kind+'-000.bin')).read_bytes()==pe_read(provider,rva,16 if kind=='atol'else 42)
 text=(original/'relocated-text.bin').read_bytes()
 assert text[0x13313-0x1000:0x1331e-0x1000]==bytes.fromhex('83c40485c00f8504d70100')
 assert text[0x13329-0x1000:0x13334-0x1000]==bytes.fromhex('83c40485c00f84f8d60100')
 # Both ctype return values feed TEST/branch only; EAX is overwritten by the
 # next public call before any other use. Exact return encoding is unclaimed.
 providers=load(native,'integer-native-providers');assert set(providers)==set(KINDS)
 entries=load(native,'integer-api-entries');binary=captured_native_binary(native)
 for kind,provider in providers.items():
  path=Path(provider['provider_path']);assert path.name=='libc.so.6'and sha(path)==provider['provider_sha256']
  region=provider['region'];assert region[-1]==str(path)and 'x'in region[1]
  low,high=[int(v,16)for v in region[0].split('-')];address=provider['address'];assert low<=address<high
  data=path.read_bytes();assert data[:6]==b'\x7fELF\x01\x01'and struct.unpack_from('<H',data,18)[0]==3
  offset=int(region[2],16)+address-low;assert 0<=offset<=len(data)-16 and data[offset:offset+16].hex()==provider['entry16']
  symtext=subprocess.check_output(['readelf','--dyn-syms','--wide',str(path)],text=True)
  matches=re.findall(r'^\s*\d+:\s*([0-9a-f]+)\s+\d+\s+FUNC\s+GLOBAL\s+DEFAULT\s+\d+\s+'+kind+r'@@[^\s]+\s*$',symtext,re.M)
  assert len(matches)==1 and int(matches[0],16)==address-low+int(region[2],16)
  rows=[row for row in entries if row['kind']==kind];assert len(rows)==3
  for row in rows:
   assert row['pc']==address and row['formal_inventory']is None
   assert row['formal_observation']=='public libc has no target DWARF formals; actual one-slot cdecl ABI observed directly'
   physical=row['physical_import'];ret=row['words'][0];callbytes=bytes.fromhex(physical['call_bytes']);pltbytes=bytes.fromhex(physical['plt_bytes'])
   assert len(callbytes)==5 and callbytes[0]==0xe8 and physical['plt']==ret+struct.unpack('<i',callbytes[1:])[0]
   assert len(pltbytes)==6 and pltbytes[:2]==b'\xff\x25'and physical['got']==struct.unpack('<I',pltbytes[2:])[0]and physical['resolved']==address
   calltext=subprocess.check_output(['objdump','-d','--start-address='+str(ret-5),'--stop-address='+str(ret),str(binary)],text=True)
   plttext=subprocess.check_output(['objdump','-d','--start-address='+str(physical['plt']),'--stop-address='+str(physical['plt']+6),str(binary)],text=True)
   assert ' '.join('%02x'%b for b in callbytes)in calltext and ' '.join('%02x'%b for b in pltbytes)in plttext
 assert len(proofs)==9


def validate_endpoint(original,native,candidate,state,grouped,known,raw,crt,architecture,names,binary,crt_raw,following=False):
 events=(original/'events.txt').read_text();rows=re.findall(r'^trap seq=(\d+) rva=0001335b esp=(\w+).*stack=([\w,]+)$',events,re.M);assert len(rows)==1
 seq=int(rows[0][0]);esp=int(rows[0][1],16);stack=[int(v,16)for v in rows[0][2].split(',')];assert stack[0]==BASE+0x12c8b and stack[1]==esp+32 and stack[3]==520
 assert stack[2]==struct.unpack_from('<I',raw['data'],0x1358)[0]
 for region,stem in [('data','compiler-data'),('BSS','compiler-bss'),('snapshot','compiler-snapshot')]:assert(original/('%s-%03d.bin'%(stem,seq))).read_bytes()==bytes(raw[region])
 assert(original/('state-%03d.bin'%seq)).read_bytes()==crt_raw
 assert state['compiler_state']==known and state['symbol']=='resolve_option_path'and names['resolve_option_path'][0]<=state['pc']<=sum(names['resolve_option_path'])
 if following:assert state['pc']==names['resolve_option_path'][0]
 else:
  opcode=subprocess.check_output(['objdump','-d','--start-address='+str(state['pc']-1),'--stop-address='+str(state['pc']),str(binary)],text=True);assert 'int3'in opcode
 assert state['formal_inventory']==[dict(name=k,type=t)for k,t in [('output','char *'),('input','const char *'),('size','unsigned int')]]
 assert state['args']['size']==520 and state['args']['input']==known['option_state']['initialized']['database_file']
 frame=grouped['match-return'][-1]['preserved']['ebp'];assert state['args']['output']==frame-0x20c
 caller=state['caller'];assert caller['symbol']=='parse_command_line'and caller['args']=={k:state['invocation']['args'][k]for k in ('argc','argv')};call(binary,caller['pc'],names['resolve_option_path'][0])
 assembly=subprocess.check_output(['objdump','-d','--start-address='+str(caller['pc']-31),'--stop-address='+str(caller['pc']),str(binary)],text=True);assert '8d 8d f4 fd ff ff'in assembly and '08 02 00'in assembly
 assert state['module_identity']==grouped['action-entry'][0]['module_identity']and state['module_token']==grouped['action-entry'][0]['module_token']and state['invocation']==grouped['outer-entry'][0]['invocation']and state['pass_mode']==known['pass_mode']
 assert semantic(state['command_line_fp'])==architecture
 for key,value in crt.items():assert state[key]==value
 assert known['bss']['command_argc']==0 and known['bss']['command_argv']==state['invocation']['args']['argv']+40
 return dict(symbol='resolve_option_path',formals=['output','input','size'],size=520,output_role='actual caller-owned520-byte stack array; uninitialized contents not compared',input_role='actual typed database_file pointer; normal canonical vc50.pdb identity, restore owned snapshot sentinel remains opaque',input_read=False,initialized_UNMODELED_bytes=4200)
