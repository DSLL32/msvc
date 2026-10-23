"""Actual empty-helper boundary and captured GetSystemInfo-only response."""
import copy,json,re,struct
from allocation_state import symbols,call
from allocation_imports import export_rva
from provenance import captured_native_binary,sha,ROOT
from main_fields import SYSTEM_MEMBERS

def boundary(original,native,state,sequence,module):
 events=(original/'events.txt').read_text();rows=re.findall(r'^trap seq=(\d+) rva=00012998 .*stack=([0-9a-f,]+)\nlast-error=(\d+)\nx87-control=([0-9a-f]+)$',events,re.M);assert len(rows)==1
 entry_sequence=int(rows[0][0]);assert entry_sequence<sequence and int(rows[0][1].split(',')[0],16)==module+0x127e7
 assert int(rows[0][2])==0 and int(rows[0][3],16)&0xffff==0x027f
 entries=json.loads((native/'empty-entry-observations.json').read_text());assert len(entries)==1;entry=entries[0]
 assert entry['boundary_rva']==0x12998 and entry['symbol']=='initialize_state_12998' and entry['esp']
 assert entry['args']=={} and entry['formal_inventory']==[] and entry['invocation']==state['invocation']
 names=symbols(captured_native_binary(native));assert entry['pc']==names['initialize_state_12998'][0]
 assert entry['return']==entry['caller']['pc'] and entry['caller']['symbol']=='c1_main'
 assert entry['caller']['args']=={key:state['invocation']['args'][key]for key in ('argc','argv')}
 call(captured_native_binary(native),entry['return'],entry['pc'])
 return entry,entry_sequence

def validate(original,native,entry,state,entry_sequence,sequence,module,candidate):
 assert (candidate['expected_frontier']['kind'],candidate['expected_frontier']['rva'])in [('post-system-info',0x12999),('arena-initialization',0x129db),('arena-reservation',0x12a18),('arena-size-calculation',0x12a7a),('arena-initialized',0x12ab2),('arenas-complete',0x12ac3),('default-pool-arena',0x12b30),('pool-refill',0xaa9c),('arena-page-allocation',0xaacb),('arena-commit',0x9883),('command-line-entry',0x12b49)]
 events=(original/'events.txt').read_text()
 before=re.findall(r'^systeminfo-before seq=(\d+) pc=([0-9a-f]+) esp=([0-9a-f]+) argument=([0-9a-f]+) provider=([0-9a-f]+) error=(\d+) fpu=([0-9a-f]+)$',events,re.M)
 after=re.findall(r'^systeminfo-after seq=(\d+) pc=([0-9a-f]+) esp=([0-9a-f]+) before-esp=([0-9a-f]+) error=(\d+) before-error=(\d+) fpu=([0-9a-f]+)$',events,re.M)
 assert len(before)==len(after)==1;b,a=before[0],after[0];bs,ats=int(b[0]),int(a[0])
 assert entry_sequence<bs<ats<sequence
 assert (int(b[1],16),int(a[1],16),int(b[3],16))==(module+0x127ec,module+0x127f2,module+0x5f508)
 assert int(a[2],16)==int(b[2],16)+4 and int(a[3],16)==int(b[2],16)
 assert int(b[5])==int(a[4])==int(a[5])==0 and int(b[6],16)&0xffff==int(a[6],16)&0xffff==0x027f
 kernel=int(re.findall(r'^module name=KERNEL32.dll actual=([0-9a-f]+) ',events,re.M)[0],16)
 assert int(b[4],16)==kernel+export_rva(original/'loaded-kernel32.bin','GetSystemInfo')
 data=(original/'system-info-after-000.bin').read_bytes();assert len(data)==36
 original_before=(original/('compiler-data-%03d.bin'%bs)).read_bytes();assert (original/'system-info-before-000.bin').read_bytes()==original_before[0x1508:0x152c]
 assert original_before==(original/('compiler-data-%03d.bin'%entry_sequence)).read_bytes()
 expected_data=bytearray(original_before);expected_data[0x1508:0x152c]=data
 for at in (ats,sequence):assert (original/('compiler-data-%03d.bin'%at)).read_bytes()==bytes(expected_data)
 for region in ('compiler-bss','compiler-snapshot','state'):
  expected=(original/('%s-%03d.bin'%(region,entry_sequence))).read_bytes()
  assert all((original/('%s-%03d.bin'%(region,at))).read_bytes()==expected for at in (bs,ats,sequence))
 native_before=json.loads((native/'system-info-entries.json').read_text());native_after=json.loads((native/'system-info-returns.json').read_text());assert len(native_before)==len(native_after)==1;nb,na=native_before[0],native_after[0]
 names=symbols(captured_native_binary(native));assert nb['pc']==names['recon_GetSystemInfo'][0]
 assert nb['formal_inventory']==[dict(name='output',type='LPSYSTEM_INFO')]
 assert nb['argument']==entry['compiler_state']['live_addresses']['initialized']+16
 assert na['pc']==nb['return'] and na['esp']==nb['esp']+8
 assert nb['last_error']==na['last_error']==0 and nb['fpu']==na['fpu']==0x027f
 binding=json.loads((native/'system-info-binding.json').read_text());reference=sha(original/'receipt.json')
 assert binding['kind']=='actualGetSystemInfo-only originalresponse closure'
 assert binding['original_receipt_sha256']==reference and binding['candidate_sha256']==sha(original/'candidate.json')
 assert binding['environment_sha256']==sha(original/'invoke-environment.bin') and binding['provider_hfile_sha256']==sha(original/'loaded-kernel32.bin')
 original_receipt=json.loads((original/'receipt.json').read_text())
 assert binding['canonical_target_sha256']==original_receipt['target_sha256'] and binding['canonical_crt_sha256']==original_receipt['canonical_crt_sha256']
 assert binding['provider_export_rva']==export_rva(original/'loaded-kernel32.bin','GetSystemInfo')
 assert binding['encoder_path']=='lib/recon/system_profile.py'
 assert binding['system_info_sha256']==sha(original/'system-info-after-000.bin') and binding['memory_status_supplied']is False
 assert binding['profile_bytes']==104 and binding['profile_sha256']==sha(native/'system-info-profile.bin')
 assert(native/'system-info-profile.bin').read_bytes()==b'RSI1'+reference.encode('ascii')+data
 frozen=json.loads((native/'profile-controller-snapshot.json').read_text())
 assert set(frozen)=={'lib/recon/system_profile.py','decomp/vc5.10/c1/scripts/system_info_reference.py'}
 assert frozen['lib/recon/system_profile.py']['sha256']==binding['encoder_sha256']
 assert frozen['decomp/vc5.10/c1/scripts/system_info_reference.py']['sha256']==binding['factory_sha256']
 for record in frozen.values():assert sha(__import__('pathlib').Path(record['content']))==record['sha256']
 assert nb['profile']==dict(bound=2,system_calls=0,memory_calls=0,reference=reference)
 assert na['profile']==dict(bound=2,system_calls=1,memory_calls=0,reference=reference)

 assert nb['compiler_state']==entry['compiler_state']
 assert na['bytes']==list(data)
 expected=copy.deepcopy(entry['compiler_state']);expected['initialized']['system_info']={key:struct.unpack_from('<H'if offset>=32 else '<I',data,offset)[0]for key,offset in SYSTEM_MEMBERS.items()}
 assert na['compiler_state']==state['compiler_state']==expected
 # The exact pre-call destination contents also include distinct nonzero restore
 # values, proving complete output overwrite and neighboring typed-field safety.
 old=entry['compiler_state']['initialized']['system_info'];encoded=bytearray(36)
 for key,offset in SYSTEM_MEMBERS.items():struct.pack_into('<H'if offset>=32 else '<I',encoded,offset,old[key])
 assert nb['bytes']==list(encoded)
 for key,value in entry['crt_state'].items():assert state[key]==value
 assert state['args']=={} and state['formal_inventory']==[] and state['caller']['symbol']=='c1_main'
 call(captured_native_binary(native),nb['return'],names['recon_GetSystemInfo'][0])
 call(captured_native_binary(native),state['caller']['pc'],names['initialize_compiler_arenas'][0])
 raw=(original/'relocated-text.bin').read_bytes();assert raw[0x12998-0x1000]==0xc3
 assert raw[0x127e7-0x1000:0x127f2-0x1000]==b'\x68'+struct.pack('<I',module+0x5f508)+b'\xff\x15'+struct.pack('<I',module+0x5d1b4)
 frontier=re.findall(r'^trap seq=%d rva=00012999 .*stack=([0-9a-f,]+)$'%sequence,events,re.M);assert len(frontier)==1 and int(frontier[0].split(',')[0],16)==module+0x127f7
