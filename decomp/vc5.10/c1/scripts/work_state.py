"""Actual work-buffer entry and first two-argument reserve-call boundary."""
import json,re,struct,subprocess
from provenance import captured_native_binary
FIELDS={'work_buffers[0].data':(0x51850,4,788),'work_buffers[0].limit':(0x5001c,4,792),'work_buffers[1].data':(0x51854,4,796),'work_buffers[1].limit':(0x50024,4,800)}
PLAN=dict(pointer_offsets=[4200,4300,4400,4500],pointer_roles='owned snapshot interiors; cleared before allocator frontier, never dereferenced',normal_case=False)
MODEL=dict(count=2,native_offset=788,native_stride=8,native_members=dict(data=0,limit=4),original_fields=[[0x51850,0x5001c],[0x51854,0x50024]],reserve_bytes=0x500000,commit_bytes=0x4000,limit_offset=0x3ffb,first_call=0x1283a,second_call=0x1285b)
def expected_fields(fields):
 assert fields['work_buffers']==MODEL
 for name,(rva,size,offset) in FIELDS.items():
  row=next(r for r in fields['modeled_bss'] if r['name']==name)
  assert (row['original_rva'],row['bytes'],row['native_offset'],row['default'],row['native_field'])==(rva,size,offset,0,'c1_bss.'+name)
 return FIELDS

def values(bss):return [struct.unpack_from('<I',bss,rva-0x50000)[0] for rva,size,offset in FIELDS.values()]
def native_values(state):return [row[key] for row in state['bss']['work_buffers'] for key in ('data','limit')]
def validate_fields(original,native,fields,entry,state,entry_sequence,sequence,probe,candidate):
 expected_fields(fields);assert candidate['restore_probe']['work_buffers']==PLAN
 assert fields['native_bss_size']==996
 for seq in (0,entry_sequence,sequence):assert values((original/('compiler-bss-%03d.bin'%seq)).read_bytes())==[0]*4
 assert native_values(entry['compiler_state'])==native_values(state['compiler_state'])==[0]*4
 if probe=='restore':
  observations=json.loads((native/'restore-observations.json').read_text());before=observations['before'];saved=before['bss']['saved_data']
  assert native_values(observations['pre_seed'])==native_values(observations['post_restore'])==[0]*4
  assert native_values(before)==[saved+offset for offset in PLAN['pointer_offsets']]
  events=(original/'events.txt').read_text();assert events.count('work-buffers-seed pointers=owned offsets=4200,4300,4400,4500\n')==1
  seq=int(re.findall(r'^trap seq=(\d+) rva=000122a5 ',events,re.M)[0]);raw=(original/('compiler-bss-%03d.bin'%seq)).read_bytes();saved=struct.unpack_from('<I',raw,0x7818)[0]
  assert values(raw)==[saved+offset for offset in PLAN['pointer_offsets']]

def boundary(original,native,state,sequence,module,candidate):
 assert (candidate['expected_frontier']['kind'],candidate['expected_frontier']['rva']) in [('work-buffer-allocation',0x12873),('main-switches',0x128b1),('scanner-argument',0x12900),('post-internal-switches',0x12998),('post-system-info',0x12999),('arena-initialization',0x129db),('arena-reservation',0x12a18),('arena-size-calculation',0x12a7a),('arena-initialized',0x12ab2),('arenas-complete',0x12ac3),('default-pool-arena',0x12b30),('pool-refill',0xaa9c)]
 events=(original/'events.txt').read_text();rows=re.findall(r'^trap seq=(\d+) rva=00012830 .*stack=([0-9a-f,]+)\nlast-error=(\d+)\nx87-control=([0-9a-f]+)$',events,re.M)
 assert len(rows)==1;entry_sequence=int(rows[0][0]);assert entry_sequence<sequence
 words=[int(x,16) for x in rows[0][1].split(',')];assert words[0]==module+0x127b8
 assert int(rows[0][2])==0 and int(rows[0][3],16)&0xffff==0x027f
 entries=json.loads((native/'work-entry-observations.json').read_text());assert len(entries)==1;entry=entries[0]
 assert entry['boundary_rva']==0x12830 and entry['symbol']=='initialize_work_buffers'
 assert entry['args']=={} and entry['formal_inventory']==[] and entry['esp']
 assert entry['caller']['symbol']=='c1_main' and entry['caller']['pc']==entry['return']
 assert entry['caller']['args']=={key:state['invocation']['args'][key] for key in ('argc','argv')}
 binary=captured_native_binary(native);symbols=subprocess.check_output(['nm','-S','--defined-only',str(binary)],text=True)
 start,size=[int(x,16) for x in re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] initialize_work_buffers$',symbols,re.M).groups()]
 assert entry['pc']==start
 assert entry['crt_state']['win32_last_error']==0 and entry['crt_state']['x87_control']==0x027f
 return entry,entry_sequence

def validate_writes(original,native,entry,state,entry_sequence,sequence,module):
 # No stores occur before the genuine first allocation call. Compare complete
 # original regions and every currently modeled native field, not just buffers.
 for region in ('compiler-data','compiler-bss','compiler-snapshot','state'):
  assert (original/('%s-%03d.bin'%(region,entry_sequence))).read_bytes()==(original/('%s-%03d.bin'%(region,sequence))).read_bytes()
 assert entry['compiler_state']==state['compiler_state']
 for key,value in entry['crt_state'].items():assert (state['crt_state'] if 'crt_state' in state else state)[key]==value
 assert state['caller']['symbol']=='initialize_work_buffers'
 assert state['formal_inventory']==[dict(name='reserve_bytes',type='unsigned int'),dict(name='commit_bytes',type='unsigned int')]
 assert state['args']==dict(reserve_bytes=0x500000,commit_bytes=0x4000)
 events=(original/'events.txt').read_text();rows=re.findall(r'^trap seq=%d rva=00012873 .*stack=([0-9a-f,]+)$'%sequence,events,re.M);assert len(rows)==1
 words=[int(x,16) for x in rows[0].split(',')];assert words[:3]==[module+0x1283f,0x500000,0x4000]
 raw=(original/'relocated-text.bin').read_bytes();body=raw[0x12830-0x1000:0x12873-0x1000]
 # Full canonical 67-byte body includes both real calls, four distinct field
 # stores, exact limit arithmetic, stack cleanup and RET0.
 expected=(b'\x68\x00\x40\x00\x00\x68\x00\x00\x50\x00\xe8\x34\x00\x00\x00\x83\xc4\x08\xa3'+struct.pack('<I',module+0x51850)+b'\x05\xfb\x3f\x00\x00\x68\x00\x40\x00\x00\x68\x00\x00\x50\x00\xa3'+struct.pack('<I',module+0x5001c)+b'\xe8\x13\x00\x00\x00\xa3'+struct.pack('<I',module+0x51854)+b'\x05\xfb\x3f\x00\x00\x83\xc4\x08\xa3'+struct.pack('<I',module+0x50024)+b'\xc3')
 assert body==expected
 binary=captured_native_binary(native);symbols=subprocess.check_output(['nm','-S','--defined-only',str(binary)],text=True)
 start,size=[int(x,16) for x in re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] initialize_work_buffers$',symbols,re.M).groups()]
 callee=int(re.search(r'^([0-9a-f]+) [0-9a-f]+ [Tt] reserve_work_buffer$',symbols,re.M)[1],16)
 pc=state['caller']['pc'];assert start<pc<start+size
 code=subprocess.check_output(['objdump','-d','--start-address='+str(pc-5),'--stop-address='+str(pc),str(binary)],text=True)
 assert re.search(r'^\s*%x:\s+e8 (?:[0-9a-f]{2} ){3}[0-9a-f]{2}\s+call\s+%x\b'%(pc-5,callee),code,re.M)
