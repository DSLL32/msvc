"""Full generated-table proof ending at the real qualified public CRT entry."""
import copy,json,re,struct
from pathlib import Path
from checksum_fields import TABLES,TAPS,read,seed,expected_fields
from allocation_state import symbols,call
from allocation_imports import export_rva
from provenance import captured_native_binary,sha
from scaler_state import original_fp,semantic

def match(events,pattern):
 rows=re.findall(pattern,events,re.M);assert len(rows)==1
 return rows[0]

def preserved(events,seq):
 return tuple(int(v,16)for v in match(events,r'^pool-registers seq=%d ebx=([0-9a-f]+) esi=([0-9a-f]+) edi=([0-9a-f]+) ebp=([0-9a-f]+)$'%seq))

def validate_prefix(original,native,fields,state,sequence,probe):
 expected_fields(fields)
 zero={name:[0]*256 for name,rva,offset in TABLES}
 assert read((original/'compiler-bss-000.bin').read_bytes())==zero
 assert state['compiler_state']['checksum_state']==read((original/('compiler-bss-%03d.bin'%sequence)).read_bytes())==zero
 if probe=='restore':
  events=(original/'events.txt').read_text();observations=json.loads((native/'restore-observations.json').read_text())
  before=int(match(events,r'^trap seq=(\d+) rva=000122a5 .*stack=.*$'))
  after=int(match(events,r'^trap seq=(\d+) rva=000303d5 .*stack=.*$'))
  assert events.count('checksum-reset-seed rva=000122a5 tables=3 bytes=768 pattern=31+53*table+7*byte\n')==1
  assert observations['pre_seed']['checksum_state']==zero
  assert observations['before']['checksum_state']==read((original/('compiler-bss-%03d.bin'%before)).read_bytes())==seed()
  assert observations['post_restore']['checksum_state']==read((original/('compiler-bss-%03d.bin'%after)).read_bytes())==zero

def validate(original,native,candidate,report,public_state=None):
 project=Path(__file__).resolve().parents[1];fields=json.loads((project/candidate['field_map']).read_text())
 expected_fields(fields);stage=public_state is not None
 if stage:
  assert (candidate['expected_frontier']['kind'],candidate['expected_frontier']['rva'])in [('multibyte-scanner-complete',0x12f3e),('path-fold-complete',0x12f87),('environment-arguments-complete',0x11e3c),('option-vectors-complete',0x12f9c),('argument-cursor-complete',0x12fbe),('option-match-action',0x131aa),('option-action-integer',0x132f9),('option-integers-complete',0x1335b),('option-path-normalizer',0x1344d),('path-normalizer-complete',0x1348d),('packing-initializer-allocator',0x134c9),('packing-allocation-complete',0xd458),('packing-frame-complete',0xd49b)]
  expected=dict(kind='public-crt-entry',rva=0x5729,provider='MSVCRT.dll',native_symbol='recon_setlocale',calling_convention='cdecl',native_args=['category','locale'],iat_rva=0x5d120,caller=dict(native_symbol='parse_command_line',return_rva=0x12b73),following_target_stub_rva=0x12f08,projection_frontier_rva=0x12f08)
 else:expected=candidate['expected_frontier']
 assert expected==dict(kind='public-crt-entry',rva=0x5729,provider='MSVCRT.dll',native_symbol='recon_setlocale',calling_convention='cdecl',native_args=['category','locale'],iat_rva=0x5d120,caller=dict(native_symbol='parse_command_line',return_rva=0x12b73),following_target_stub_rva=0x12f08,projection_frontier_rva=0x12f08)
 assert candidate['checksum_observers']==dict(entry_rva=0x12e1c,return_rva=0x12b66,provider_rva=0x5729,iat_rva=0x5d120,locale_rva=0x601c1,table_bytes=768)
 assert(original/'checksum-locale-observers.txt').read_text()=='12e1c 5d120 5729\n'
 events=(original/'events.txt').read_text();assert'failure 'not in events
 module=int(match(events,r'^module name=c1n32_l.dll actual=([0-9a-f]+) preferred=.*$'),16)
 crt=int(match(events,r'^module name=MSVCRT.dll actual=([0-9a-f]+) preferred=.*$'),16)
 assert module==0x10600000 # Other bases remain uncertified.
 seq,esp,words=match(events,r'^trap seq=(\d+) rva=00012e1c esp=([0-9a-f]+).*stack=([0-9a-f,]+)$');seq=int(seq);esp=int(esp,16)
 assert int(words.split(',')[0],16)==module+0x12b66
 returned=match(events,r'^checksum-return seq=(\d+) pc=([0-9a-f]+) esp=([0-9a-f]+) error=(\d+)$')
 returned_seq=int(returned[0]);assert(int(returned[1],16),int(returned[2],16),int(returned[3]))==(module+0x12b66,esp+4,0)
 row=match(events,r'^public-entry seq=(\d+) module=MSVCRT.dll name=setlocale provider=([0-9a-f]+) rva=00005729 iat=0005d120 esp=([0-9a-f]+) return=([0-9a-f]+) category=(\d+) locale=([0-9a-f]+) error=(\d+)$')
 public_seq=int(row[0]);assert seq<returned_seq<public_seq
 assert tuple(int(v,16)for v in row[1:4])==(crt+0x5729,esp-8,module+0x12b73)
 assert int(row[4])==2 and int(row[5],16)==module+0x601c1 and int(row[6])==0
 assert export_rva(original/'loaded-msvcrt.bin','setlocale')==0x5729
 assert(original/'setlocale-provider-entry-000.bin').read_bytes()==bytes.fromhex('81ec90000000')
 assert(original/'setlocale-input-000.bin').read_bytes()==b'\0'
 assert report['import_catalogs']['original']['slots'][str(module+0x5d120)]==dict(dll='msvcrt.dll',name='setlocale')
 raw=(original/'relocated-text.bin').read_bytes()
 assert raw[0x12b61-0x1000:0x12b73-0x1000]==bytes.fromhex('e8b602000068')+struct.pack('<I',module+0x601c1)+bytes.fromhex('6a02ff15')+struct.pack('<I',module+0x5d120)
 from command_line_state import pe_reader
 pe=pe_reader((original/'loaded-target.bin').read_bytes())
 assert list(struct.unpack('<8I',pe(0x611a8,32)))==TAPS and pe(0x601c1,1)==b'\0'
 entries=json.loads((native/'checksum-entries.json').read_text());returns=json.loads((native/'checksum-returns.json').read_text());assert len(entries)==len(returns)==1
 entry=copy.deepcopy(entries[0]);end=returns[0];entry.update(entry['crt_state']);entry['command_line_fp']=entry['fp'];entry['pass_mode']=entry['compiler_state']['pass_mode']
 binary=captured_native_binary(native);names=symbols(binary)
 assert entry['pc']==names['initialize_checksums'][0] and entry['symbol']=='initialize_checksums'and entry['boundary_rva']==0x12e1c
 assert entry['args']=={} and entry['formal_inventory']==[] and entry['return']==entry['caller']['pc'] and entry['feedback_taps']==TAPS
 assert entry['caller']['symbol']=='parse_command_line';call(binary,entry['return'],entry['pc'])
 prefix=copy.deepcopy(candidate);prefix['projection_actual_frontier']=fields['value_copy']['hard_frontier_rva']
 prefix['expected_frontier']=dict(rva=0x12e1c,native_symbol='initialize_checksums',kind='command-line-helper',calling_convention='cdecl',native_args=[],caller=dict(native_symbol='parse_command_line',return_rva=0x12b66))
 from compiler_state import validate as validate_compiler
 probe=json.loads((original/'receipt.json').read_text())['probe']
 validate_compiler(original,native,entry,seq,module,prefix,prefix['expected_frontier'],probe)
 validate_prefix(original,native,fields,entry,seq,probe)
 state=public_state if stage else json.loads((native/'state.json').read_text());wanted=copy.deepcopy(entry['compiler_state'])
 generated=read((original/('compiler-bss-%03d.bin'%returned_seq)).read_bytes());assert set(generated)=={'transform','low','high'} and any(generated['transform'])
 wanted['checksum_state']=generated
 assert end['compiler_state']==state['compiler_state']==wanted
 old=bytearray((original/('compiler-bss-%03d.bin'%seq)).read_bytes())
 for name,rva,offset in TABLES:old[rva-0x50000:rva-0x50000+256]=bytes(generated[name])
 assert bytes(old)==(original/('compiler-bss-%03d.bin'%returned_seq)).read_bytes()==(original/('compiler-bss-%03d.bin'%public_seq)).read_bytes()
 for region in ('compiler-data','compiler-snapshot','state'):
  assert(original/('%s-%03d.bin'%(region,seq))).read_bytes()==(original/('%s-%03d.bin'%(region,returned_seq))).read_bytes()==(original/('%s-%03d.bin'%(region,public_seq))).read_bytes()
 assert end['pc']==entry['return'] and end['esp']==entry['esp']+4
 assert end['preserved']==entry['preserved']==state['public_entry']['preserved']
 assert preserved(events,seq)==preserved(events,returned_seq)==preserved(events,public_seq)
 assert semantic(entry['fp'])==semantic(end['fp'])==semantic(state['command_line_fp'])==semantic(original_fp(original,seq))==semantic(original_fp(original,returned_seq))==semantic(original_fp(original,public_seq))
 for key,value in entry['crt_state'].items():assert end['crt_state'][key]==state[key]==value
 assert state['symbol']=='recon_setlocale'and state['pc']==names['recon_setlocale'][0]
 assert state['formal_inventory']==[dict(name='category',type='int'),dict(name='locale',type='const char *')]
 assert state['args']==dict(category=2,locale=names['c1_empty_string'][0])
 assert state['caller']['symbol']=='parse_command_line'and state['caller']['args']==entry['caller']['args']
 assert state['public_entry']['return']==state['caller']['pc'];call(binary,state['caller']['pc'],state['pc'])
 assert state['public_entry']['stack']==[state['public_entry']['return'],2,state['args']['locale']]and state['public_entry']['locale_bytes']==[0]
 assert state['module_identity']==dict(name='C1.DLL',path='R:\\toolchains\\5.10\\bin\\C1.DLL',sha256=json.loads((original/'receipt.json').read_text())['target_sha256'],flags=2)
 assert state['module_token']and state['crt_attached']==1 and state['raw_dllmain']==0
 assert state['onexit_begin']==state['onexit_end']and state['onexit_begin']and state['onexit_first']==0 and state['crt_adjust_fdiv']==0
 assert state['dll_instance']==state['module_token']and state['win32_last_error']==0 and state['x87_control']==0x027f
 globals=(original/('state-%03d.bin'%public_seq)).read_bytes();end_old,begin_old=struct.unpack_from('<2I',globals,0x94)
 assert begin_old==end_old and begin_old and struct.unpack_from('<I',globals,0x40)[0]==module
 assert struct.unpack_from('<i',globals,0x90)[0]==1 and struct.unpack_from('<I',globals,0x9c)[0]==0
 assert struct.unpack_from('<i',globals,0xa0)[0]==0 and struct.unpack_from('<I',globals,0)[0]==0
 heap=match(events,r'^state seq=%d heap-begin=([0-9a-f]+) heap-first=([0-9a-f]+)$'%public_seq)
 assert(int(heap[0],16),int(heap[1],16))==(begin_old,0)
 invoke=match(events,r'^trap seq=\d+ rva=00012229 .*stack=([0-9a-f,]+)$')
 words=[int(v,16)for v in invoke.split(',')];contract=candidate['case_contract'];actual=state['invocation']
 assert words[1]==actual['args']['argc']==contract['argc']and words[3]==actual['args']['mode']==contract['BOOL']
 assert actual['symbol']=='InvokeCompilerPass'and actual['argv_terminator']==0
 original_args=[(original/('invoke-arg-%03d.bin'%i)).read_bytes()for i in range(contract['argc'])]
 assert all(raw.endswith(b'\0')and b'\0'not in raw[:-1]for raw in original_args)
 assert[raw[:-1].decode('ascii')for raw in original_args]==actual['argv']==contract['argv']
 transcript=(native/'gdb.txt').read_text()
 if stage:
  assert state==json.loads((native/'locale-native-setlocale-entry-state.json').read_text())and'NativeLocaleEntry'in(native/'commands.gdb').read_text()
 else:assert re.search(r'Breakpoint \d+, recon_setlocale \(',transcript)and'break *recon_setlocale'in(native/'commands.gdb').read_text()
 assert'recon_unsupported'not in transcript
 if not stage:assert'Program received signal'not in transcript
 # The original's later locale return / target trap are separate original-only
 # analysis. Native stops BEFORE the unsupported provider body; no locale success.
 if candidate['expected_frontier']['kind']in ('option-vectors-complete','argument-cursor-complete','option-match-action','option-action-integer','option-integers-complete','option-path-normalizer','path-normalizer-complete','packing-initializer-allocator','packing-allocation-complete','packing-frame-complete'):
  from frontier_contract import validate_original_frontier
  validate_original_frontier(events,candidate,probe)
 else: assert match(events,r'^frontier rva=([0-9a-f]+)$')==('%08x'%candidate['expected_frontier']['rva']if stage else'00012f08')
 for item in candidate.get('imports',[]):
  assert raw[item['start_rva']-0x1000:item['end_rva']-0x1000]==b'\xff\x25'+struct.pack('<I',module+item['iat_rva'])
 return dict(status='matching genuine public CRT entry; no locale behavior accepted',native_symbol=state['symbol'],public_frontier=dict(provider='MSVCRT.dll',name='setlocale',rva=0x5729,category=2,locale_bytes=[0]),generated_bss_bytes=768,initialized_known_bytes=fields['main_fields']['initialized_known_bytes'],initialized_unmodeled_bytes=fields['main_fields']['initialized_unknown_bytes'],promotion='held for concrete public-entry proof review',execution_receipts={side:sha(directory/'receipt.json')for side,directory in [('original',original),('native',native)]})
