"""Leaf path folding, isolated fullpath and the real parser continuation."""
import copy,json,re,struct,subprocess
from pathlib import Path
from provenance import ROOT,sha,captured_native_binary
from allocation_state import symbols,call
from allocation_imports import pe_read,export_rva
from integer_state import load,locale_raw
from resolver_state import validate as validate_resolver,FORMALS
from vector_state import snapshot
from signature_call_state import fp
from scaler_state import semantic
from frontier_contract import validate_original_frontier,validate_native_stop

BASE=0x10600000

def registers(original,name,count):return list(struct.unpack('<%uI'%count,(original/(name+'-registers.bin')).read_bytes()))

def validate(original,native,candidate,report):
 first=load(native,'normalizer-entries')[0];state=load(native,'state');stage=snapshot(original,native,state,first)
 for key in ('resolver_local_buffer','resolver_path_bytes','resolver_locale'):stage[key]=first[key]
 prefix=validate_resolver(original,native,candidate,report,stage_state=stage)
 result=validate_sequence(original,native,candidate);result['prefix']=prefix;return result

def fold(data,table):
 result=bytearray(data);at=0
 while result[at]:
  value=table[result[at]]
  if value==0xffff:at+=2
  else:result[at]=value&255;at+=1
 return bytes(result)

def validate_namespace(original,native,rows):
 binding=load(native,'native-path-binding');receipt=load(original,'receipt')
 assert binding['kind']=='native-only private S namespace with actual original logical cwd'and binding['original_receipt_sha256']==sha(original/'receipt.json')
 raw=(original/'fullpath-entry-cwd.bin').read_bytes();assert len(raw)%2==0 and raw[-2:]==b'\0\0'
 cwd=raw[:-2].decode('utf-16le');assert cwd[:3]=='S:\\'and cwd.rstrip('\\')==receipt['cwd']
 assert binding['original_cwd_sha256']==sha(original/'fullpath-entry-cwd.bin')and binding['logical_cwd']==cwd
 private=Path(binding['private_S_root']);work=Path(binding['physical_cwd'])
 assert private==native/'drive-S'and work==private/cwd[3:].replace('\\','/').strip('/')and binding['R_root']==str(ROOT)
 assert work.resolve().is_relative_to(native.resolve())and not work.resolve().is_relative_to(original.resolve())
 assert len(binding['inputs'])==1;item=binding['inputs'][0]
 assert item['logical_path']==cwd.rstrip('\\')+'\\test.c'and item['original_path']==str(original/'test.c')and item['physical_path']==str(work/'test.c')
 assert item['sha256']==sha(original/'test.c')==sha(work/'test.c')==sha(ROOT/'decomp/vc5.10/c1/cases/minimal/inputs/test.c')
 assert binding['unknown_S_paths']=='private tree only; no fallback to original raw directory'
 for row in rows:assert row['logical_cwd']==cwd and row['private_S_root']==str(private)and row['physical_cwd']==str(work)
 for phase in ('entry','return'):
  words=struct.unpack('<6I',(original/('fullpath-'+phase+'-cwd-identity.bin')).read_bytes());teb=(original/('locale-fullpath-'+phase+'-teb.bin')).read_bytes()
  assert words[0]==struct.unpack_from('<I',teb,0x18)[0]and words[1]==struct.unpack_from('<I',teb,0x30)[0]and words[2]and words[4]
  length,maximum=words[3]&65535,words[3]>>16;assert length==len(raw)-2 and length%2==0 and maximum>=length+2
  assert(original/('fullpath-'+phase+'-cwd.bin')).read_bytes()==raw
 assert(original/'fullpath-entry-cwd-identity.bin').read_bytes()==(original/'fullpath-return-cwd-identity.bin').read_bytes()
 return cwd

def validate_sequence(original,native,candidate):
 assert candidate['allowed_probes']==['normal']and load(original,'receipt')['probe']==load(native,'receipt')['probe']=='normal'
 assert candidate['expected_frontier']==dict(kind='path-normalizer-complete',rva=0x1348d,native_symbol='initialize_packing',native_args=[],calling_convention='cdecl',caller=dict(native_symbol='parse_command_line',return_rva=0x12d55))
 assert candidate['normalizer_observers']==dict(entry_rva=0x1344d,return_rvas=[0x133c9,0x1343f],calls=2,fullpath_iat_rva=0x5d040,fullpath_export_rva=0x1dabe,fullpath_return_rva=0x1342e,resolver_return_rva=0x12c8b,frontier_rva=0x1348d)
 fn=[r for r in candidate['functions']if r['name']=='_normalize_path'];assert len(fn)==1 and fn[0]['chunks']==[[0x1344d,0x1348d],[0x30af0,0x30af8]]and fn[0]['role']=='application'
 events=(original/'events.txt').read_text();validate_original_frontier(events,candidate,'normal');validate_native_stop(native,candidate)
 entries=load(native,'normalizer-entries');ends=load(native,'normalizer-returns');api=load(native,'fullpath-entries');returns=load(native,'fullpath-returns');resolver=load(native,'resolver-entries');resolved=load(native,'resolver-returns');state=load(native,'state')
 assert len(entries)==len(ends)==2 and len(api)==len(returns)==len(resolver)==len(resolved)==1
 allrows=entries+ends+api+returns+resolved;cwd=validate_namespace(original,native,allrows)
 expected_output=(cwd.rstrip('\\')+'\\vc50.pdb').encode('ascii')+b'\0';assert len(expected_output)<256
 binary=captured_native_binary(native);names=symbols(binary);known=copy.deepcopy(entries[0]['compiler_state']);table=known['path_fold'];assert len(table)==256
 architecture=semantic(entries[0]['fp']);crt=entries[0]['crt_state'];inv=entries[0]['invocation'];baseline=locale_raw(original,'resolver-entry');entry_r=registers(original,'resolver-entry',16);outside=(original/('state-%03d.bin'%entry_r[0])).read_bytes()
 def phase(label,row,sequence):
  assert locale_raw(original,label)==baseline and(original/('state-%03d.bin'%sequence)).read_bytes()==outside
  assert row['compiler_state']==known and row['invocation']==inv and row['crt_state']==crt and row['errors']==[0,0,0]
  assert semantic(row['fp'])==fp((original/('locale-'+label+'-fp.bin')).read_bytes())==architecture
  assert(original/('locale-'+label+'-fp.bin')).read_bytes()==(original/'locale-resolver-entry-fp.bin').read_bytes()
  for suffix in ('words','errors','mbctype','ctype','old-ctype','result','CRT-argv','CRT-environment','Invoke-argv'):
   assert(original/('locale-'+label+'-'+suffix+'.bin')).read_bytes()==(original/('locale-resolver-entry-'+suffix+'.bin')).read_bytes()
  for suffix in ('mbctype','ctype','old-ctype','result'):
   assert(native/('locale-native-'+label+'-'+suffix+'.bin')).read_bytes()==(original/('locale-'+label+'-'+suffix+'.bin')).read_bytes()
  for suffix in ('CRT-argv','CRT-environment','Invoke-argv'):
   assert(native/('locale-native-'+label+'-'+suffix+'.bin')).read_bytes()==(native/('locale-native-resolver-entry-'+suffix+'.bin')).read_bytes()
  assert row['locale']['errors']==[0,0,0]
  assert {k:v for k,v in row['locale'].items()if k not in('phase','fp')}=={k:v for k,v in entries[0]['locale'].items()if k not in('phase','fp')}
  assert semantic(row['locale']['fp'])==architecture
 for index,(entry,end)in enumerate(zip(entries,ends)):
  before=registers(original,'normalizer-%u-entry'%index,16);after=registers(original,'normalizer-%u-return'%index,16);ret=(0x133c9,0x1343f)[index]
  assert before[1]==BASE+0x1344d and before[10]==BASE+ret and before[12:14]==[index,BASE]and before[14]==before[11]and before[15]==(260 if index==0 else 520)
  assert after[1]==before[10]and after[2]==before[2]+4 and after[3]==before[11]and after[6:10]==before[6:10]and after[10:14]==before[10:14]
  assert after[14]==before[11]&0xffffff00,'actual low-byte private argument reuse'
  assert entry['index']==end['index']==index and entry['symbol']=='normalize_path'and entry['pc']==names['normalize_path'][0]and entry['formal_inventory']==[dict(name='path',type='char *')]
  assert entry['args']==dict(path=entry['words'][1])and len(entry['words'])==2 and entry['caller']['symbol']=='resolve_option_path'and entry['caller']['args']==resolver[0]['args']and entry['caller']['formal_inventory']==FORMALS
  assert entry['args']['path']==(entry['resolver_local_buffer']['address']if index==0 else resolver[0]['args']['output'])
  call(binary,entry['words'][0],entry['pc'])
  assert end['pc']==entry['words'][0]and end['esp']==entry['esp']+4 and end['eax']==entry['args']['path']and end['preserved']==entry['preserved']
  assert end['argument_slot_after']==entry['args']['path'],'observed native by-value private slot remains unchanged'
  value=b'vc50.pdb\0'if not index else expected_output
  assert(original/('normalizer-%u-entry-path.bin'%index)).read_bytes()==value and entry['path_bytes']==list(value)
  normalized=fold(value,table);assert(original/('normalizer-%u-return-path.bin'%index)).read_bytes()==normalized and end['path_bytes']==list(normalized)
  phase('normalizer-%u-entry'%index,entry,before[0]);phase('normalizer-%u-return'%index,end,after[0])
 # The original compiler stores a char temporary in its private argument slot.
 # Both call sites discard that slot; no target state/output reads its bits.
 text=(original/'relocated-text.bin').read_bytes()
 assert text[0x133c9-0x1000:0x133de-0x1000]==bytes.fromhex('0fbe742414a194d065108bd681e2ff00000083c404')
 assert text[0x1343f-0x1000:0x13442-0x1000]==bytes.fromhex('83c404')
 a=registers(original,'fullpath-entry',18);z=registers(original,'fullpath-return',18)
 provider=original/'loaded-msvcrt.bin';assert export_rva(provider,'_fullpath')==0x1dabe
 assert(original/'fullpath-provider-entry.bin').read_bytes()==pe_read(provider,0x1dabe,32)
 assert a[1]==a[14]+0x1dabe and a[10]==BASE+0x1342e and a[11:14]==[entry_r[11],entry_r[12],520]and a[15:18]==a[11:14]
 assert z[1]==a[10]and z[2]==a[2]+4 and z[3]==a[11]and z[6:10]==a[6:10]and z[11:15]==a[11:15]
 assert z[15:17]==a[11:13]and a[11]<=z[17]<a[11]+520,'observed private capacity-slot pointer; no semantic type inferred'
 public=api[0];returned=returns[0]
 assert public['symbol']=='recon_fullpath'and public['pc']==names['recon_fullpath'][0]and public['formal_inventory']==[dict(name=k,type=t)for k,t in [('out','char *'),('name','const char *'),('capacity','size_t')]]
 assert public['args']==dict(out=resolver[0]['args']['output'],name=resolver[0]['args']['input'],capacity=520)
 assert public['words']==[public['caller']['pc']]+[public['args'][k]for k in ('out','name','capacity')]and public['caller']['symbol']=='resolve_option_path'and public['caller']['args']==resolver[0]['args'];call(binary,public['words'][0],public['pc'])
 assert public['input_bytes']==list(b'vc50.pdb\0')and returned['output_bytes']==list(expected_output)
 assert returned['pc']==public['words'][0]and returned['esp']==public['esp']+4 and returned['eax']==public['args']['out']and returned['preserved']==public['preserved']and returned['argument_slots_after']==public['words'][1:]
 for side,prefix in ((original,'fullpath-'),(native,'fullpath-native-')):
  before=(side/(prefix+'entry-buffer.bin')).read_bytes();after=(side/(prefix+'return-buffer.bin')).read_bytes();assert len(before)==len(after)==520
  assert after[:len(expected_output)]==expected_output and before[len(expected_output):]==after[len(expected_output):],'defined output and own untouched tail'
 assert(original/'fullpath-entry-input.bin').read_bytes()==(original/'fullpath-return-input.bin').read_bytes()==b'vc50.pdb\0'
 assert text[0x1342e-0x1000:0x13439-0x1000]==bytes.fromhex('83c40c85c00f8472d60100')
 phase('fullpath-entry',public,a[0]);phase('fullpath-return',returned,z[0])
 rr=registers(original,'resolver-return',4);native_return=resolved[0]
 assert rr[1]==entry_r[2]+4 and rr[2]==entry_r[11]and rr[3]==BASE
 assert native_return['pc']==resolver[0]['words'][0]and native_return['esp']==resolver[0]['esp']+4 and native_return['eax']==resolver[0]['args']['output']and native_return['preserved']==resolver[0]['preserved']
 assert native_return['output_bytes']==list(fold(expected_output,table));phase('resolver-return',native_return,rr[0])
 endpoint=validate_parser(original,native,state,known,baseline,outside,fold(expected_output,table),names,binary,crt,inv,architecture)
 return dict(status='matching72B normalizer, actual fullpath and parser continuation',application_candidate='_normalize_path',candidate_code_bytes=72,normalizer_calls=2,normalizer_returns=2,fullpath_calls=1,logical_cwd=cwd,defined_output_bytes=len(expected_output),output_tail='each own remaining520-byte buffer tail unchanged, cross-target uninitialized equality unclaimed',private_argument_slots='actual original low-byte normalizer temporaries and fullpath capacity reuse retained; only dead discarded caller-area representation differs',CRT_credit=0,UNMODELED_initialized_bytes=4200,UNMODELED_literal_tail_bytes=16,endpoint=endpoint,execution_receipts=dict(original=sha(original/'receipt.json'),native=sha(native/'receipt.json')))

def validate_parser(original,native,state,known,baseline,outside,value,names,binary,crt,inv,architecture):
 expected=copy.deepcopy(known);options=expected['option_state']['initialized'];raw={k:bytearray(v)for k,v in baseline.items()};external=bytearray(outside)
 assert not options['debug_old_format']and not options['japanese_identifiers']and not options['packing_stack']and options['split_pdbs']==1
 options['database_path'][:len(value)]=list(value);raw['data'][0xfc8:0xfc8+len(value)]=value
 expected['initialized']['c9_il']|=int(options['incremental_compile']==0);struct.pack_into('<I',raw['data'],0x12e0,expected['initialized']['c9_il'])
 options['function_records']&=options['incremental_compile'];struct.pack_into('<I',raw['data'],0x12f0,options['function_records'])
 options['line_records']&=options['debug_info'];struct.pack_into('<I',raw['data'],0xfc4,options['line_records'])
 expected['option_state']['current_pdb']=names['c1_output_pdb_context'][0];struct.pack_into('<I',external,0x68,BASE+0x5c070)
 events=(original/'events.txt').read_text();rows=re.findall(r'^trap seq=(\d+) rva=0001348d esp=(\w+).*stack=([\w,]+)$',events,re.M);assert len(rows)==1
 seq=int(rows[0][0]);assert int(rows[0][2].split(',')[0],16)==BASE+0x12d55
 for region,stem in [('data','compiler-data'),('BSS','compiler-bss'),('snapshot','compiler-snapshot')]:assert(original/('%s-%03d.bin'%(stem,seq))).read_bytes()==bytes(raw[region])
 assert(original/('state-%03d.bin'%seq)).read_bytes()==bytes(external)
 assert state['compiler_state']==expected and state['symbol']=='initialize_packing'and state['args']=={}and state['formal_inventory']==[]
 assert names['initialize_packing'][0]<state['pc']<=sum(names['initialize_packing'])
 opcode=subprocess.check_output(['objdump','-d','--start-address='+str(state['pc']-1),'--stop-address='+str(state['pc']),str(binary)],text=True);assert 'int3'in opcode
 assert state['caller']['symbol']=='parse_command_line'and state['caller']['args']=={k:inv['args'][k]for k in ('argc','argv')};call(binary,state['caller']['pc'],names['initialize_packing'][0])
 assert state['invocation']==inv and semantic(state['command_line_fp'])==architecture and state['pass_mode']==known['pass_mode']
 for key,item in crt.items():assert state[key]==item
 return dict(symbol='initialize_packing',formals=[],known_parser_changes=['database_path defined prefix','c9_il','function_records','line_records','persistent current_pdb context identity'],snapshot_unchanged=True,unknown_bytes_unread=True)
