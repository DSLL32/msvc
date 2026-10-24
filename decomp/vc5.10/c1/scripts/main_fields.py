"""Encountered main datafe fields; original initialized gaps remain UNMODELED."""
import struct
SYSTEM_MEMBERS={'dwOemId':0,'dwPageSize':4,'lpMinimumApplicationAddress':8,'lpMaximumApplicationAddress':12,'dwActiveProcessorMask':16,'dwNumberOfProcessors':20,'dwProcessorType':24,'dwAllocationGranularity':28,'wProcessorLevel':32,'wProcessorRevision':34}
INITIALIZED={'first_call':(0x5f530,4,0),'compiler_phase':(0x5f3d8,4,4),'c9_il':(0x5f2e0,4,8),'compiler_flags':(0x5ede4,4,12),'system_info':(0x5f508,36,16),'internal_bm_flags':(0x5f2fc,4,52),'internal_zb_value':(0x5ef04,4,56),'memory_percent':(0x5f978,4,60)}
COMMAND={'debug_break':(0x55c34,4,776),'command_argc':(0x55c38,4,780),'command_argv':(0x55c3c,4,784)}
SYSTEM_DEFAULT={name:4096 if name=='dwPageSize' else 65536 if name=='dwAllocationGranularity' else 0 for name in SYSTEM_MEMBERS}
DEFAULT=dict(first_call=1,compiler_phase=0,c9_il=0,compiler_flags=0,system_info=SYSTEM_DEFAULT,internal_bm_flags=0,internal_zb_value=0,memory_percent=100)
PLAN=dict(known_initialized_bytes=64,native_initialized_size=64,unknown_initialized_bytes=6484,
 snapshot=dict(internal_bm_flags=37,internal_zb_value=61,memory_percent=83,compiler_phase=7,c9_il=9,compiler_flags=0x13572468,dwOemId=0x56780003,dwPageSize=8192,lpMinimumApplicationAddress=3000,lpMaximumApplicationAddress=3016,dwActiveProcessorMask=0x1122,dwNumberOfProcessors=7,dwProcessorType=586,dwAllocationGranularity=131072,wProcessorLevel=6,wProcessorRevision=0x1234),
 live=dict(internal_bm_flags=41,internal_zb_value=67,memory_percent=89,compiler_phase=11,c9_il=13,compiler_flags=0x24681357,dwOemId=0x12340002,dwPageSize=16384,lpMinimumApplicationAddress=3500,lpMaximumApplicationAddress=3516,dwActiveProcessorMask=0x3344,dwNumberOfProcessors=9,dwProcessorType=486,dwAllocationGranularity=262144,wProcessorLevel=5,wProcessorRevision=0x4321),
 pointer_roles='owned snapshot interior sentinels; preserved without API use or dereference before work-buffer helper')

def read_initialized(raw):
 result={name:struct.unpack_from('<i' if name in ('first_call','compiler_phase','c9_il','internal_bm_flags','internal_zb_value','memory_percent') else '<I',raw,rva-0x5e000)[0] for name,(rva,size,native) in INITIALIZED.items() if name!='system_info'}
 result['system_info']={name:struct.unpack_from('<H' if offset>=32 else '<I',raw,0x1508+offset)[0] for name,offset in SYSTEM_MEMBERS.items()}
 return result

def seeded(which,allocation,first):
 plan=PLAN[which]
 result={name:plan[name] for name in ('compiler_phase','c9_il','compiler_flags','internal_bm_flags','internal_zb_value','memory_percent')};result['first_call']=first
 result['system_info']={name:allocation+plan[name] if name in ('lpMinimumApplicationAddress','lpMaximumApplicationAddress') else plan[name] for name in SYSTEM_MEMBERS}
 return result

def expected_fields(fields):
 assert {r['name']:(r['original_rva'],r['bytes'],r['native_offset']) for r in fields['modeled_initialized']if not r['name'].startswith(('options.','scanner.'))}==INITIALIZED
 rows={r['name']:r for r in fields['modeled_initialized']}
 for name,(rva,size,native) in INITIALIZED.items():
  row=rows[name];assert row['native_field']=='c1_data.'+name and row['snapshot_original_offset']==rva-0x5e000 and row['snapshot_native_offset']==native
  assert row['default']==DEFAULT[name]
 model=fields['main_fields'];assert model['system_info_members']==SYSTEM_MEMBERS
 from command_line_fields import INITIALIZED_SIZE,BSS_SIZE
 initialized,bss=(INITIALIZED_SIZE,BSS_SIZE)if fields.get('command_line')else(64,996)
 if fields.get('multibyte_scanner'):
  import multibyte_fields
  multibyte_fields.expected_fields(fields);initialized=multibyte_fields.INITIALIZED_SIZE
 assert model['initialized_known_bytes']==initialized and model['initialized_unknown_bytes']==6548-initialized
 assert (model['system_info_original_rva'],model['system_info_native_offset'],model['native_initialized_size'],model['native_bss_size'])==(0x5f508,16,initialized,bss)
 for name,(rva,size,native) in COMMAND.items():
  row=next(r for r in fields['modeled_bss'] if r['name']==name)
  assert (row['original_rva'],row['bytes'],row['native_offset'],row['default'],row['native_field'])==(rva,size,native,0,'c1_bss.'+name)
 return COMMAND

def validate(original,native,fields,entry,state,entry_sequence,sequence,probe,candidate):
 import copy,json,re
 expected_fields(fields);assert candidate['restore_probe']['main_fields']==PLAN
 assert read_initialized((original/'compiler-data-000.bin').read_bytes())==DEFAULT
 original_bss=(original/'compiler-bss-000.bin').read_bytes()
 assert [struct.unpack_from('<I',original_bss,rva-0x50000)[0] for rva,size,offset in COMMAND.values()]==[0,0,0]
 original_saved=struct.unpack_from('<I',(original/('compiler-bss-%03d.bin'%entry_sequence)).read_bytes(),0x7818)[0]
 native_saved=entry['compiler_state']['bss']['saved_data']
 if probe=='normal':
  expected=copy.deepcopy(DEFAULT);expected['first_call']=0
  snapshot_expected=copy.deepcopy(DEFAULT)
  assert entry['compiler_state']['initialized']==expected
  assert entry['compiler_state']['snapshot']==snapshot_expected
  assert read_initialized((original/('compiler-data-%03d.bin'%entry_sequence)).read_bytes())==expected
  assert read_initialized((original/('compiler-snapshot-%03d.bin'%entry_sequence)).read_bytes())==snapshot_expected
  events=(original/'events.txt').read_text();copy_source=re.findall(r'^trap seq=(\d+) rva=000122a5 ',events,re.M);assert len(copy_source)==1
  assert (original/('compiler-snapshot-%03d.bin'%entry_sequence)).read_bytes()==(original/('compiler-data-%03d.bin'%int(copy_source[0]))).read_bytes()
  assert not (original/'main-fields-probe.txt').exists()
 else:
  assert (original/'main-fields-probe.txt').read_text()=='5f3d8 5f2e0 5ede4 5f508 40 55c34\n'
  observations=json.loads((native/'restore-observations.json').read_text())
  assert observations['pre_seed']['initialized']==DEFAULT
  events=(original/'events.txt').read_text()
  assert re.findall(r'^main-fields-seed .*$',events,re.M)==['main-fields-seed bytes=64 snapshot-phase=7 live-phase=11 snapshot=%08x pointers=owned'%original_saved]
  original_expected=seeded('snapshot',original_saved,0);native_expected=seeded('snapshot',native_saved,0)
  assert entry['compiler_state']['initialized']==native_expected
  assert read_initialized((original/('compiler-data-%03d.bin'%entry_sequence)).read_bytes())==original_expected
  first=candidate['restore_probe']['snapshot_first_call']
  assert entry['compiler_state']['snapshot']==seeded('snapshot',native_saved,first)
  assert read_initialized((original/('compiler-snapshot-%03d.bin'%entry_sequence)).read_bytes())==seeded('snapshot',original_saved,first)
  assert observations['before']['initialized']==seeded('live',native_saved,0)
  assert observations['before']['snapshot']==seeded('snapshot',native_saved,first)
  assert observations['post_restore']['initialized']==seeded('snapshot',native_saved,first)
  assert observations['post_restore']['snapshot']==seeded('snapshot',native_saved,first)
  for rva,kind,wanted_first in [(0x122a5,'live',0),(0x303d5,'snapshot',first)]:
   seqs=re.findall(r'^trap seq=(\d+) rva=%08x '%rva,events,re.M);assert len(seqs)==1
   assert read_initialized((original/('compiler-data-%03d.bin'%int(seqs[0]))).read_bytes())==seeded(kind,original_saved,wanted_first)
  before_seq=int(re.findall(r'^trap seq=(\d+) rva=000122a5 ',events,re.M)[0])
  original_before=(original/('compiler-bss-%03d.bin'%before_seq)).read_bytes()
  assert [struct.unpack_from('<I',original_before,rva-0x50000)[0] for rva,size,offset in COMMAND.values()]==[17,23,original_saved+4000]
  assert [observations['before']['bss'][name] for name in COMMAND]==[17,23,native_saved+4000]
  assert all(observations['pre_seed']['bss'][name]==observations['post_restore']['bss'][name]==0 for name in COMMAND)
 for name,(rva,size,offset) in COMMAND.items():
  assert entry['compiler_state']['bss'][name]==state['compiler_state']['bss'][name]==0
  for seq in (entry_sequence,sequence):assert struct.unpack_from('<I',(original/('compiler-bss-%03d.bin'%seq)).read_bytes(),rva-0x50000)[0]==0
 final_original=read_initialized((original/('compiler-data-%03d.bin'%sequence)).read_bytes())
 expected_final=copy.deepcopy(entry['compiler_state']['initialized']);expected_final['compiler_phase']=1
 assert state['compiler_state']['initialized']==expected_final
 # Original pointer values retain their ORIGINAL owned-allocation identities.
 expected_original=read_initialized((original/('compiler-data-%03d.bin'%entry_sequence)).read_bytes());expected_original['compiler_phase']=1
 assert final_original==expected_original
