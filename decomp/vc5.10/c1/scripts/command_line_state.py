"""Whole encountered option-state projection and actual parser entry/frontier.

No unknown initialized bytes, PDB internals or reserved arena tail are modeled.
The genuine include-alias helper is still masked in the parser-only trial.
"""
import copy,json,re,struct
from pathlib import Path
from command_line_fields import DATA,BSS,DATA_OFFSETS,BSS_OFFSETS,INITIALIZED_SIZE,BSS_SIZE,width,field_rows,read,seed,seed_bss,seed_metadata,targets
from allocation_state import symbols,call
from provenance import captured_native_binary

def expected_fields(fields):
 model=fields['command_line']
 assert model==dict(initialized_known_bytes=INITIALIZED_SIZE,initialized_unknown_bytes=6548-INITIALIZED_SIZE,
  native_initialized_size=INITIALIZED_SIZE,native_bss_size=BSS_SIZE,initialized_offset=64,bss_offset=996,
  persistent_current_pdb_rva=0x5c068,unknown_semantic_consumer_frontier=0x12df6,
  option_rule_rva=0x62b78,option_rule_count=85,z_rule_rva=0x62ae8,z_rule_count=12,
  rule_stride=12,warning_defaults_rva=0x63190,warning_default_count=11)
 for key,rows in [('modeled_initialized',DATA),('modeled_bss',BSS)]:
  actual=[row for row in fields[key]if row['name'].startswith('options.')]
  assert actual==field_rows(rows)
 pointer=next(row for row in fields['outside_bss']if row['name']=='current_pdb_context')
 assert pointer==dict(name='current_pdb_context',original_rva=0x5c068,bytes=4,native_field='c1_current_pdb_context',default=0,lifetime='persistent outside datafe snapshot and BSS reset')
 return {row['name']:(row['original_rva'],row['bytes'],row['native_offset'])for row in field_rows(BSS)}

def expected_defaults(rows,module,strings=None):
 return {name:(strings[default['symbol']]if strings is not None else module+default['rva'])if isinstance(default,dict)else default for name,rva,kind,default in rows}

def pe_reader(raw):
 pe=struct.unpack_from('<I',raw,0x3c)[0];count=struct.unpack_from('<H',raw,pe+6)[0];table=pe+24+struct.unpack_from('<H',raw,pe+20)[0]
 def read_at(rva,size):
  for i in range(count):
   virtual,first,physical,offset=struct.unpack_from('<4I',raw,table+i*40+8)
   if first<=rva and rva+size<=first+physical:return raw[offset+rva-first:offset+rva-first+size]
  raise AssertionError('unbacked parser data RVA')
 return read_at

def validate_tables(original,current,module):
 pe=pe_reader((original/'loaded-target.bin').read_bytes());native=current['tables'];assert native['rule_size']==native['z_rule_size']==12
 assert set(native['targets'])=={str(rva)for rva in targets()}
 for index in range(85):
  pattern,target,result,action=struct.unpack_from('<IIbB',pe(0x62b78+index*12,12))
  row=native['rules'][index]
  if index==84:
   assert pattern==target==result==action==0 and row==dict(pattern=None,target=0,result=0,action=0)
  else:
   text=pe(pattern-0x10600000,80).split(b'\0')[0].decode('ascii')
   # Actual table relocations use the image's preferred base in hFile bytes.
   assert row['pattern']==text and row['result']==result and row['action']==action
   assert row['target']==(native['z_address']if action==0x23 else native['targets'][str(target-0x10600000)])
 assert len(native['rules'])==85 and len(native['z_rules'])==12
 for index in range(12):
  letter,action,target=struct.unpack('<3I',pe(0x62ae8+index*12,12))
  assert native['z_rules'][index]==[letter,action,native['targets'][str(target-0x10600000)]if target else 0]
 # Exact contextual warning records; no code, table-padding or section credit.
 expected=[(1,5),(69,5),(611,5),(226,14),(233,14),(234,14),(235,14),(168,5),(169,5),(530,5),(0,0)]
 assert [(struct.unpack_from('<I',pe(0x63190+i*8,8))[0],pe(0x63190+i*8+4,1)[0])for i in range(11)]==expected

def validate_projection(original,native,fields,state,sequence,module,candidate,probe):
 expected_fields(fields)
 current=state['compiler_state']['option_state'];strings=current['default_strings']
 assert current['layout']==dict(initialized=DATA_OFFSETS,bss=BSS_OFFSETS,initialized_size=INITIALIZED_SIZE,bss_size=BSS_SIZE)
 bases=state['compiler_state']['live_addresses']
 expected_targets={str(rva):bases['initialized']+DATA_OFFSETS[name]for name,rva,kind,default in DATA if '['not in kind}
 expected_targets.update({str(rva):bases['bss']+BSS_OFFSETS[name]for name,rva,kind,default in BSS})
 expected_targets.update({str(0x5f2e0):bases['initialized']+8,str(0x5f2fc):bases['initialized']+52,
  str(0x5ef04):bases['initialized']+56,str(0x55c34):bases['bss']+776})
 assert current['tables']['targets']==expected_targets
 initial=read((original/'compiler-data-000.bin').read_bytes(),DATA,0x5e000)
 assert initial==expected_defaults(DATA,module)
 assert read((original/'compiler-bss-000.bin').read_bytes(),BSS,0x50000)==dict.fromkeys([row[0]for row in BSS],0)
 assert struct.unpack_from('<I',(original/'state-000.bin').read_bytes(),0x68)[0]==0
 assert set(strings)=={default['symbol']for name,rva,kind,default in DATA if isinstance(default,dict)}
 pe=pe_reader((original/'loaded-target.bin').read_bytes());binary=captured_native_binary(native);names=symbols(binary)
 for name,rva,kind,default in DATA:
  if isinstance(default,dict):
   value=default['text'].encode()+b'\0';assert pe(default['rva'],len(value))==value
   assert strings[default['symbol']]==names[default['symbol']][0]
 validate_tables(original,current,module)
 old_saved=struct.unpack_from('<I',(original/('compiler-bss-%03d.bin'%sequence)).read_bytes(),0x7818)[0]
 new_saved=state['compiler_state']['bss']['saved_data'];assert old_saved and new_saved
 wanted_old=seed('snapshot',old_saved)if probe=='restore'else initial
 wanted_new=seed('snapshot',new_saved)if probe=='restore'else expected_defaults(DATA,module,strings)
 assert current['initialized']==current['snapshot']==wanted_new
 assert read((original/('compiler-data-%03d.bin'%sequence)).read_bytes(),DATA,0x5e000)==wanted_old
 assert read((original/('compiler-snapshot-%03d.bin'%sequence)).read_bytes(),DATA,0x5e000)==wanted_old
 assert current['bss']==dict.fromkeys([row[0]for row in BSS],0)
 assert read((original/('compiler-bss-%03d.bin'%sequence)).read_bytes(),BSS,0x50000)==current['bss']
 assert current['current_pdb']==(names['c1_pdb_context'][0]if probe=='restore'else 0)
 assert struct.unpack_from('<I',(original/('state-%03d.bin'%sequence)).read_bytes(),0x68)[0]==(module+0x5c048 if probe=='restore'else 0)
 events=(original/'events.txt').read_text()
 if probe=='normal':
  assert not(original/'command-line-seeds.txt').exists()
  copy_at=int(re.findall(r'^trap seq=(\d+) rva=000122a5 ',events,re.M)[0])
  assert read((original/('compiler-data-%03d.bin'%copy_at)).read_bytes(),DATA,0x5e000)==initial
 else:
  assert(original/'command-line-seeds.txt').read_text()==seed_metadata()
  assert events.count('command-line-seed fields=%d snapshot=%08x persistent=normal-pdb\n'%(len(DATA)+len(BSS),old_saved))==1
  observations=json.loads((native/'restore-observations.json').read_text())
  assert observations['pre_seed']['option_state']['initialized']==expected_defaults(DATA,module,strings)
  assert observations['pre_seed']['option_state']['bss']==dict.fromkeys([row[0]for row in BSS],0)
  for rva,phase,which in [(0x122a5,'before','live'),(0x303d5,'post_restore','snapshot')]:
   at=int(re.findall(r'^trap seq=(\d+) rva=%08x '%rva,events,re.M)[0]);value=observations[phase]['option_state']
   assert value['initialized']==seed(which,new_saved) and value['snapshot']==wanted_new
   assert read((original/('compiler-data-%03d.bin'%at)).read_bytes(),DATA,0x5e000)==seed(which,old_saved)
   expected_bss=seed_bss(new_saved)if which=='live'else dict.fromkeys([row[0]for row in BSS],0)
   assert value['bss']==expected_bss
   assert read((original/('compiler-bss-%03d.bin'%at)).read_bytes(),BSS,0x50000)==(seed_bss(old_saved)if which=='live'else expected_bss)
   assert value['current_pdb']==names['c1_pdb_context'][0]
   assert struct.unpack_from('<I',(original/('state-%03d.bin'%at)).read_bytes(),0x68)[0]==module+0x5c048

def boundary(original,native,state,sequence,module):
 events=(original/'events.txt').read_text();rows=re.findall(r'^trap seq=(\d+) rva=00012b49 esp=([0-9a-f]+).*stack=([0-9a-f,]+)$',events,re.M);assert len(rows)==1
 at=int(rows[0][0]);assert at<sequence
 entries=json.loads((native/'command-line-entry-observations.json').read_text());assert len(entries)==1
 entry=copy.deepcopy(entries[0]);entry.update(entry['crt_state']);entry['pass_mode']=entry['compiler_state']['pass_mode']
 assert entry['pc']==symbols(captured_native_binary(native))['parse_command_line'][0] and entry['boundary_rva']==0x12b49
 assert entry['formal_inventory']==[dict(name='argc',type='int'),dict(name='argv',type='char **')]
 assert entry['invocation']==state['invocation'] and entry['args']=={key:state['invocation']['args'][key]for key in ('argc','argv')}
 assert entry['caller']['symbol']=='c1_main' and entry['caller']['args']==entry['args']
 assert entry['return']==entry['caller']['pc'];call(captured_native_binary(native),entry['return'],entry['pc'])
 return entry,at

def validate_parser(original,native,entry,state,entry_sequence,sequence,module,candidate):
 assert candidate['expected_frontier']==dict(rva=0x12df6,native_symbol='initialize_include_aliases',kind='command-line-helper',calling_convention='cdecl',native_args=[],caller=dict(native_symbol='parse_command_line',return_rva=0x12b61))
 assert state['symbol']=='initialize_include_aliases' and state['args']=={} and state['formal_inventory']==[]
 assert state['caller']['symbol']=='parse_command_line' and state['caller']['args']==entry['args']
 assert state['compiler_state']==entry['compiler_state']
 names=symbols(captured_native_binary(native));call(captured_native_binary(native),state['caller']['pc'],names['initialize_include_aliases'][0])
 events=(original/'events.txt').read_text();words=re.findall(r'^trap seq=%d rva=00012df6 .*stack=([0-9a-f,]+)$'%sequence,events,re.M);assert len(words)==1
 assert int(words[0].split(',')[0],16)==module+0x12b61
 raw=(original/'relocated-text.bin').read_bytes();assert raw[0x12b49-0x1000:0x12b4e-0x1000]==b'\xa1'+struct.pack('<I',module+0x5e414)
 assert raw[0x12b5c-0x1000:0x12b61-0x1000]==b'\xe8\x95\x02\0\0'
 assert entry['compiler_state']['option_state']['initialized']['include_aliases']==0
 for region in ('compiler-data','compiler-bss','compiler-snapshot','state'):
  assert(original/('%s-%03d.bin'%(region,entry_sequence))).read_bytes()==(original/('%s-%03d.bin'%(region,sequence))).read_bytes()
 from scaler_state import semantic,original_fp
 assert semantic(entry['fp'])==semantic(state['command_line_fp'])==semantic(original_fp(original,entry_sequence))==semantic(original_fp(original,sequence))
 for key,value in entry['crt_state'].items():assert state[key]==value

def check_preservation_static(project,root):
 from allocation_imports import pe_read,export_rva
 from provenance import sha
 packet=json.loads((project/'cases/minimal/command-line-preservation.json').read_text())
 target=root/packet['target']['path'];assert sha(target)==packet['target']['sha256']
 caller=packet['caller'];assert caller['cached_iat_rva']==0x5d058 and caller['callee_save_register']=='ebx'
 assert caller['load']==dict(rva=0x3055f,bytes='8b1d58d06510')
 assert caller['register_calls']==[dict(rva=rva,bytes='ffd3')for rva in (0x3057a,0x30608)]
 for row in [caller['load']]+caller['register_calls']:
  assert pe_read(target,row['rva'],len(bytes.fromhex(row['bytes'])))==bytes.fromhex(row['bytes'])
 provider=packet['provider'];path=root/provider['path'];assert sha(path)==provider['sha256']
 assert export_rva(path,'fgets')==provider['rva']==0x2880e and provider['end']==0x28893
 raw=pe_read(path,provider['rva'],provider['end']-provider['rva']);assert raw.hex()==provider['bytes'] and raw[0]==0x53
 assert provider['restore_return_rvas']==[0x28827,0x28870,0x28891]
 assert all(pe_read(path,rva,2)==b'\x5b\xc3'for rva in provider['restore_return_rvas'])
 assert [row['name']for row in packet['target_helpers']]==['_read_env_arguments','_duplicate_argument']
 for row in packet['target_helpers']:
  for key in ('hot','cold'):
   if key in row:
    block=row[key];assert pe_read(target,block['rva'],len(bytes.fromhex(block['bytes'])))==bytes.fromhex(block['bytes'])
  assert pe_read(target,row['save_rva'],1)==b'\x53' and pe_read(target,row['restore_rva'],1)==b'\x5b'

def check_register_report(report):
 result=report['functions']['_parse_command_line']['register_imports']
 assert result['require_all_register_calls_resolved']is True
 assert result['original']['resolved_register_calls']=={str(0x10600000+rva):dict(dll='msvcrt.dll',name='fgets')for rva in (0x3057a,0x30608)}
 for side in ('original','rebuilt'):
  assert result[side]['unresolved_register_calls']==result[side]['other_unresolved_calls']==[]
