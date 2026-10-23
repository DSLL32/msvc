"""Encountered six typed arenas and descriptor table; no pointed storage modeled."""
import copy,json,re,struct
from pathlib import Path
from allocation_state import symbols,call
from provenance import captured_native_binary
MEMBERS={'cursor':0,'committed_limit':4,'base':8,'committed_bytes':12,'descriptor':16,'reserve_bytes':20,'reset_value':24,'mapped_view':28}
DESCRIPTOR_MEMBERS={'kind':0,'reserve_bytes':4,'commit_unit':8,'flags':12,'alignment':16}
RECORDS=[dict(zip(DESCRIPTOR_MEMBERS,values))for values in [(0,0x3200000,0x10000,0x21,1),(1,0xa00000,0x8000,0x10,4),(2,0xa00000,0x8000,0x18,4),(3,0x500000,0x4000,0x18,4),(4,0x500000,0x8000,0x18,4),(5,0x500000,0x8000,0x18,4)]]
PLAN=dict(count=6,original_rva=0x58fb0,stride=32,pointer_offsets=[4600,4700,4800,4900],integer_seed=71,scope='owned snapshot interior sentinels and labeled nonzero ints before BSS clear; never dereferenced')
MODEL=dict(original_rva=0x58fb0,count=6,original_stride=32,native_offset=804,native_stride=32,members=MEMBERS,descriptor_rva=0x65780,descriptor_count=6,descriptor_stride=20,descriptor_members=DESCRIPTOR_MEMBERS,snapshot=False)
def expected_fields(fields):
 assert fields['arenas']==MODEL
 rows={}
 for i in range(6):
  for name,offset in MEMBERS.items():
   key='arenas[%d].%s'%(i,name);value=(0x58fb0+i*32+offset,4,804+i*32+offset);row=next(r for r in fields['modeled_bss']if r['name']==key)
   assert (row['original_rva'],row['bytes'],row['native_offset'],row['default'],row['native_field'])==value+(0,'c1_bss.'+key)
   rows[key]=value
 return rows

def read(bss):
 return [dict(zip(MEMBERS,struct.unpack_from('<8I',bss,0x8fb0+i*32)))for i in range(6)]
def seed(saved):
 result=[]
 for i in range(6):
  row={name:saved+offset+i*4 for name,offset in zip(('cursor','committed_limit','base','descriptor'),PLAN['pointer_offsets'])}
  row.update({name:71+i*4+j for j,name in enumerate(('committed_bytes','reserve_bytes','reset_value','mapped_view'))});result.append(row)
 return result

def boundary(original,native,state,sequence,module):
 events=(original/'events.txt').read_text();rows=re.findall(r'^trap seq=(\d+) rva=00012999 .*stack=([0-9a-f,]+)\nlast-error=(\d+)\nx87-control=([0-9a-f]+)$',events,re.M);assert len(rows)==1
 at=int(rows[0][0]);assert at<sequence and int(rows[0][1].split(',')[0],16)==module+0x127f7
 assert int(rows[0][2])==0 and int(rows[0][3],16)&0xffff==0x027f
 entries=json.loads((native/'arena-entry-observations.json').read_text());assert len(entries)==1;entry=copy.deepcopy(entries[0]);names=symbols(captured_native_binary(native))
 assert entry['pc']==names['initialize_compiler_arenas'][0] and entry['boundary_rva']==0x12999
 assert entry['args']=={} and entry['formal_inventory']==[] and entry['symbol']=='initialize_compiler_arenas()'
 assert entry['invocation']==state['invocation'] and entry['return']==entry['caller']['pc']
 assert entry['caller']['symbol']=='c1_main' and entry['caller']['args']=={key:state['invocation']['args'][key]for key in ('argc','argv')}
 call(captured_native_binary(native),entry['return'],entry['pc']);entry.update(entry['crt_state'])
 return entry,at

def validate(original,native,fields,entry,state,entry_sequence,sequence,module,candidate,probe):
 expected_fields(fields);assert candidate['restore_probe']['arenas']==PLAN
 assert candidate['expected_frontier']==dict(rva=0x129db,native_symbol='initialize_primary_arenas',kind='arena-initialization',native_args=[],caller=dict(native_symbol='initialize_compiler_arenas',return_rva=0x129a1))
 assert state['compiler_state']==entry['compiler_state']
 assert state['args']=={} and state['formal_inventory']==[] and state['caller']['symbol']=='initialize_compiler_arenas'
 names=symbols(captured_native_binary(native))
 call(captured_native_binary(native),state['caller']['pc'],names['initialize_primary_arenas'][0])
 zero=[dict.fromkeys(MEMBERS,0)for i in range(6)]
 for at in (0,entry_sequence,sequence):assert read((original/('compiler-bss-%03d.bin'%at)).read_bytes())==zero
 assert entry['compiler_state']['bss']['arenas']==state['compiler_state']['bss']['arenas']==zero
 # Descriptor table contains recovered semantic fields only. Its complete actual
 #120 bytes are bound independently from the reconstructed C literal records.
 target=(original/'loaded-target.bin').read_bytes();pe=struct.unpack_from('<I',target,0x3c)[0];count=struct.unpack_from('<H',target,pe+6)[0];table=pe+24+struct.unpack_from('<H',target,pe+20)[0]
 for i in range(count):
  offset=table+i*40;size,rva,rawsize,raw=struct.unpack_from('<4I',target,offset+8)
  if rva<=0x65780<rva+rawsize:data=target[raw+0x65780-rva:raw+0x657f8-rva];break
 else:raise AssertionError('missing descriptor bytes')
 assert len(data)==120 and [dict(zip(DESCRIPTOR_MEMBERS,struct.unpack_from('<5I',data,i*20)))for i in range(6)]==RECORDS
 assert entry['compiler_state']['arena_descriptors']==state['compiler_state']['arena_descriptors']==RECORDS
 for region in ('compiler-data','compiler-bss','compiler-snapshot','state'):
  assert(original/('%s-%03d.bin'%(region,entry_sequence))).read_bytes()==(original/('%s-%03d.bin'%(region,sequence))).read_bytes()
 raw=(original/'relocated-text.bin').read_bytes();assert raw[0x12999-0x1000:0x129a1-0x1000]==b'\x53\x56\x57\xe8\x3a\0\0\0'
 events=(original/'events.txt').read_text();frontier=re.findall(r'^trap seq=%d rva=000129db .*stack=([0-9a-f,]+)$'%sequence,events,re.M);assert len(frontier)==1 and int(frontier[0].split(',')[0],16)==module+0x129a1
 for key,value in entry['crt_state'].items():assert state[key]==value
 if probe=='restore':
  observations=json.loads((native/'restore-observations.json').read_text());before=observations['before'];native_saved=before['bss']['saved_data']
  assert observations['pre_seed']['bss']['arenas']==observations['post_restore']['bss']['arenas']==zero
  assert before['bss']['arenas']==seed(native_saved)
  assert events.count('arena-seed count=6 stride=32 pointers=owned integer-base=71\n')==1
  at=int(re.findall(r'^trap seq=(\d+) rva=000122a5 ',events,re.M)[0]);bss=(original/('compiler-bss-%03d.bin'%at)).read_bytes();saved=struct.unpack_from('<I',bss,0x7818)[0]
  assert read(bss)==seed(saved)
  assert all(offset+5*4<6548 for offset in PLAN['pointer_offsets'])
