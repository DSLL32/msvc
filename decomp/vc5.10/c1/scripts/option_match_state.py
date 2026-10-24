"""Whole matcher source, genuine callback/value path and three-formal action."""
import copy,json,re,struct,subprocess
from provenance import PROJECT,captured_native_binary,sha
from allocation_state import symbols,call
from allocation_imports import pe_read
from scaler_state import semantic
from vector_state import validate as validate_vectors,snapshot
from argument_cursor_state import validate_boundary as validate_first_argument
from frontier_contract import validate_original_frontier,validate_native_stop
from option_match_fields import validate_reset


def raw_registers(original,phase):return list(struct.unpack('<16I',(original/('option-match-'+phase+'-registers.bin')).read_bytes()))
def raw_blob(original,phase,region):return(original/('option-match-'+phase+'-target-'+region+'.bin')).read_bytes()

def validate(original,native,candidate,report,stage_state=None):
 rows=[f for f in candidate['functions']if f['name']=='_match_option'];assert len(rows)==1
 assert rows[0]['chunks']==[[0x12fbe,0x1315f],[0x30919,0x3092f],[0x13160,0x131aa]]and rows[0]['text_data_chunks']==[[0x13160,0x131aa]]
 assert candidate['option_match_observers']==dict(entry_rva=0x12fbe,action_rva=0x131aa,argument_index=1,value_index=2,rule_index=31)
 receipt=json.loads((original/'receipt.json').read_text());probe=receipt['probe'];validate_original_frontier((original/'events.txt').read_text(),candidate,probe);validate_native_stop(native,candidate)
 state=stage_state if stage_state is not None else json.loads((native/'state.json').read_text());matcher=json.loads((native/'option-match-entries.json').read_text());assert len(matcher)==(3 if stage_state else 1)
 first_next=json.loads((native/'argument-cursor-entries.json').read_text());first_ret=json.loads((native/'argument-cursor-returns.json').read_text());assert len(first_next)==len(first_ret)==(3 if stage_state else 1)
 sig=json.loads((native/'signature-call-entries.json').read_text());sig_ret=json.loads((native/'signature-call-returns.json').read_text())
 if probe=='normal':assert len(sig)==len(sig_ret)==(3 if stage_state else 1);stage=snapshot(original,native,state,sig[0]);stage['dependency_entry']=sig[0]['dependency_entry']
 else:assert probe=='restore'and len(sig)==len(sig_ret)==(2 if stage_state else 0);stage=snapshot(original,native,state,first_next[0])
 prefix=validate_vectors(original,native,candidate,report,stage_state=stage)
 from signature_call_state import validate as validate_signature
 signature=validate_signature(original,native,sig[0],sig_ret[0],first_next[0])if probe=='normal'else dict(executed=False,reason='actual restored c9_il9 skips public call')
 match_entry=snapshot(original,native,state,matcher[0]);iterator=validate_first_argument(original,native,candidate,first_next[0],first_ret[0],match_entry,following=True)
 validate_reset(original,native)
 next_entries=json.loads((native/'option-next-entries.json').read_text());next_returns=json.loads((native/'option-next-returns.json').read_text());assert len(next_entries)==len(next_returns)==(3 if stage_state else 1)
 proof=validate_boundary(original,native,candidate,matcher[0],next_entries[0],next_returns[0],state,following=stage_state is not None)
 from counterpart_correction import validate as correction
 corrected=correction(original)
 if stage_state is None:assert corrected is not None
 return dict(status='matching439B option matcher through genuine callback and action entry',application_candidate='_match_option',candidate_code_bytes=439,separate_text_data_bytes=74,case_rule='-il$',case_rule_index=31,argument='-il',borrowed_value='.\\minimal',action_executed=False,other_matcher_branches='whole source/graph only; no runtime branch credit',initialized_known_bytes=2348,initialized_UNMODELED_bytes=4200,native_bss_bytes=2376,new_known_bss_bytes=4,signature=signature,iterator=iterator,proof=proof,prefix=prefix,original_reuse=(dict(kind='explicit corrected counterpart; original execution unchanged',packet_sha256=corrected['packet_sha256'],original_receipt_sha256=corrected['original_receipt_sha256'])if corrected else None),execution_receipts=dict(original=sha(original/'receipt.json'),native=sha(native/'receipt.json')))

def validate_boundary(original,native,candidate,entry,next_entry,end,state,following=False):
 binary=captured_native_binary(native);names=symbols(binary);a,b,c,d=[raw_registers(original,phase)for phase in ('entry','next-entry','next-return','action')]
 assert a[1]==0x10612fbe and a[10:]==[0x10612c50,0x10662b78,a[12],0x10612f9c,0,0x10600000]
 assert b[1]==0x10612f9c and b[10]==0x106130ae and c[1]==0x106130ae and c[2]==b[2]+4 and c[6:10]==b[6:10]
 assert a[0]<b[0]<c[0]<d[0]and d[1]==0x106131aa and d[10:14]==[0x106130bc,0x10662cec,c[3],0]
 old_argument=a[12];old_value=c[3];events=(original/'events.txt').read_text()
 for index,pointer,raw in [(1,old_argument,b'-il\0'),(2,old_value,b'.\\minimal\0')]:
  rows=re.findall(r'^invoke-arg index=%d address=(\w+) bytes=(\d+)$'%index,events,re.M);assert len(rows)==1 and int(rows[0][0],16)==pointer and int(rows[0][1])==len(raw)
 assert(original/'option-match-value-000.bin').read_bytes()==b'.\\minimal\0'
 raw=bytearray(raw_blob(original,'entry','BSS'));assert struct.unpack_from('<I',raw,0x38b0)[0]==0;struct.pack_into('<I',raw,0x38b0,old_argument);assert bytes(raw)==raw_blob(original,'next-entry','BSS')
 assert struct.unpack_from('<II',raw,0x5c38)==(10,struct.unpack_from('<I',raw_blob(original,'entry','BSS'),0x5c3c)[0])
 argc,argv=struct.unpack_from('<II',raw,0x5c38);struct.pack_into('<II',raw,0x5c38,9,argv+4);assert bytes(raw)==raw_blob(original,'next-return','BSS')==raw_blob(original,'action','BSS')
 for region in ('data','snapshot'):
  assert all(raw_blob(original,phase,region)==raw_blob(original,'entry',region)for phase in ('next-entry','next-return','action'))
 assert len((original/'option-match-entry-fp.bin').read_bytes())==112 and all((original/('option-match-'+phase+'-fp.bin')).read_bytes()==(original/'option-match-entry-fp.bin').read_bytes()for phase in ('next-entry','next-return','action'))
 assert entry['symbol']=='match_option'and entry['pc']==names['match_option'][0]and entry['formal_inventory']==[dict(name=k,type=t)for k,t in [('rules','CompilerOptionRule *'),('argument','char *'),('next','char *(*)(void)'),('copy','int')]]
 inv=entry['invocation'];argument=inv['argv_pointers'][1];value=inv['argv_pointers'][2]
 assert entry['args']==dict(rules=names['option_rules'][0],argument=argument,next=names['next_argument'][0],copy=0)
 expected=copy.deepcopy(entry['compiler_state']);assert expected['bss']['current_argument']==0 and expected['bss']['command_argc']==10;expected['bss']['current_argument']=argument
 assert next_entry['compiler_state']==expected and next_entry['symbol']=='next_argument'and next_entry['pc']==names['next_argument'][0]and next_entry['args']=={}and next_entry['formal_inventory']==[]
 assert next_entry['caller']['symbol']=='match_option'and next_entry['caller']['args']==entry['args']and next_entry['words']==[next_entry['caller']['pc']]
 assert end['pc']==next_entry['words'][0]and end['esp']==next_entry['esp']+4 and end['preserved']==next_entry['preserved']and end['eax']==value and end['result_bytes']==list(b'.\\minimal\0')
 expected['bss']['command_argc']=9;expected['bss']['command_argv']+=4;assert end['compiler_state']==state['compiler_state']==expected
 assert state['symbol']=='apply_option_action'
 assert state['pc']==names['apply_option_action'][0]if following else names['apply_option_action'][0]<state['pc']<=sum(names['apply_option_action'])
 opcode=subprocess.check_output(['objdump','-d','--start-address='+str(state['pc']-1),'--stop-address='+str(state['pc']),str(binary)],text=True);assert following or 'int3'in opcode
 assert state['args']==dict(rule=names['option_rules'][0]+31*12,value=value,copy=0)and state['formal_inventory']==[dict(name=k,type=t)for k,t in [('rule','CompilerOptionRule *'),('value','char *'),('copy','int')]]
 assert state['caller']['symbol']=='match_option'and state['caller']['args']==(entry['args']if following else{})
 # The current generic JSON collector only names argc/argv caller formals.
 # GDB's independently recorded actual frame contains all matcher formals.
 if not following:
  actual=re.findall(r'^#1  (0x[0-9a-f]+) in match_option \(rules=(0x[0-9a-f]+) <option_rules>, argument=(0x[0-9a-f]+) "-il", next=(0x[0-9a-f]+) <next_argument>, copy=(\d+)\) at ',(native/'gdb.txt').read_text(),re.M);assert len(actual)==1
  pc,rules,arg,nextfn,copyarg=actual[0];assert int(pc,16)==state['caller']['pc']and dict(rules=int(rules,16),argument=int(arg,16),next=int(nextfn,16),copy=int(copyarg))==entry['args']
 call(binary,state['caller']['pc'],names['apply_option_action'][0])
 text=(original/'relocated-text.bin').read_bytes();assert text[0x130aa-0x1000:0x130ae-0x1000]==bytes.fromhex('ff542420')and text[0x130b7-0x1000]==0xe8 and 0x130bc+struct.unpack_from('<i',text,0x130b8-0x1000)[0]==0x131aa
 assert text[0x13160-0x1000:0x131aa-0x1000]==pe_read(original/'loaded-target.bin',0x13160,74)
 pattern,target,result,action=struct.unpack_from('<IIbB',pe_read(original/'loaded-target.bin',0x62cec,12));assert(pattern,target,result,action)==(0x106630cc,0x1065f394,1,0x22)and pe_read(original/'loaded-target.bin',0x630cc,5)==b'-il$\0'
 for row in (entry,next_entry,end):
  assert row['errors']==[0,0,0]and row['crt_state']==entry['crt_state']and row['invocation']==inv and semantic(row['fp'])==semantic(entry['fp'])
 assert state['invocation']==inv and semantic(state['command_line_fp'])==semantic(end['fp'])
 for key,val in end['crt_state'].items():assert state[key]==val
 seq=int(re.findall(r'^trap seq=(\d+) rva=000131aa ',events,re.M)[0]);assert(original/('compiler-bss-%03d.bin'%seq)).read_bytes()==bytes(raw)
 return dict(rule_index=31,action=0x22,callback='real next_argument; zeroformals and RET0',argc_before=10,argc_after=9,argv_advance=4,borrowed_current_argument='Invoke argv[1]',borrowed_value='Invoke argv[2]',unmodeled_tail_consumer_reached=False)
