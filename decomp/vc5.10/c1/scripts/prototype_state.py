"""The reviewed fixed20-byte empty-record prototype, not its larger users' layout."""
import copy,json,re,struct,subprocess
from provenance import captured_native_binary
from pass_state import plain_globals_after_invoke_store,native_after_invoke_store
MODEL=dict(original_rva=0x58f50,native_offset=688,bytes=20,
 purpose='exact-width empty-record byte prototype, no larger record layout inferred',
 original_store_offsets=[0,4,8,12,16,0],static_copy_rvas=[0x48331,0x49ead,0x4a24e,0x4b6c3],
 larger_record_layout_known=False,included_in_initialized_snapshot=False)
PLAN=dict(before_clear_first=1,before_callback_first=0xa0,bytes=20,entry_rva=0x1277b,normal_case=False)
ZERO=[0]*20

def expected_fields(fields):
 assert fields['empty_record_prototype']==MODEL
 row=next(r for r in fields['modeled_bss'] if r['name']=='empty_record_prototype')
 assert row['default']==ZERO and row['native_field']=='c1_bss.empty_record_prototype'
 assert not 0x5e000<=MODEL['original_rva']<0x5f994
 return {'empty_record_prototype':(0x58f50,20,688)}
def read(raw):return list(raw[0x8f50:0x8f64])
def at(original,sequence,native,wanted):
 raw=(original/('compiler-bss-%03d.bin'%sequence)).read_bytes()
 assert read(raw)==native['bss']['empty_record_prototype']==wanted
 return raw

def validate(original,native,fields,state,sequence,probe,candidate,project):
 expected_fields(fields);assert candidate['restore_probe']['empty_record_prototype']==PLAN
 assert read((original/'compiler-bss-000.bin').read_bytes())==ZERO
 current=state['compiler_state'];final=at(original,sequence,current,ZERO)
 events=(original/'events.txt').read_text();module=int(re.search(r'module name=c1n32_l.dll actual=([0-9a-f]+)',events)[1],16)
 entries=re.findall(r'^trap seq=(\d+) rva=0001277b ',events,re.M);assert len(entries)==1
 before_seq=int(entries[0]);assert before_seq<sequence
 function=next(r for r in candidate['functions'] if r['name']=='_initialize_state_1277b')
 assert function['chunks']==[[0x1277b,0x1279c]]
 relocated=(original/'relocated-text.bin').read_bytes()
 assert relocated[0x1277b-0x1000:0x1279c-0x1000]==b'\x33\xc0'+b''.join(b'\xa3'+struct.pack('<I',module+0x58f50+offset) for offset in MODEL['original_store_offsets'])+b'\xc3'
 old=(original/('compiler-bss-%03d.bin'%before_seq)).read_bytes();expected=bytearray(old);expected[0x8f50:0x8f64]=bytes(20)
 assert bytes(expected)==final
 for region in ('compiler-data','compiler-snapshot','state'):
  before_region=(original/('%s-%03d.bin'%(region,before_seq))).read_bytes()
  if region=='state':before_region=plain_globals_after_invoke_store(original,before_region,candidate,module,state.get('boundary_rva'))
  assert before_region==(original/('%s-%03d.bin'%(region,sequence))).read_bytes()
 if probe=='normal':
  assert read(old)==ZERO and not (original/'prototype-probe.txt').exists() and not re.findall(r'^prototype-seed ',events,re.M)
  return
 assert (original/'prototype-probe.txt').read_text()=='58f50 14 01 a0\n'
 assert re.findall(r'^prototype-seed .*$',events,re.M)==['prototype-seed rva=000122a5 bytes=20 first=01','prototype-seed rva=0001277b bytes=20 first=a0']
 obs=json.loads((native/'restore-observations.json').read_text());assert obs['pre_seed']['bss']['empty_record_prototype']==ZERO
 for rva,key in [(0x122a5,'before'),(0x303d5,'post_restore'),(0x12365,'before_callback'),(0x12377,'before_current_callback'),(0x12389,'before_contexts_callback'),(0x12646,'before_value_copy'),(0x12663,'after_value_copy'),(0x12664,'before_signature'),(0x1266f,'after_signature'),(0x12676,'before_pdb_context'),(0x126af,'after_pdb_context'),(0x126b6,'before_output_pdb_context'),(0x126ef,'after_output_pdb_context')]:
  seqs=re.findall(r'^trap seq=(\d+) rva=%08x '%rva,events,re.M);assert len(seqs)==1
  at(original,int(seqs[0]),obs[key],list(range(1,21)) if key=='before' else ZERO)
 for rva,item in obs['buffer_constructors'].items():
  seqs=re.findall(r'^trap seq=(\d+) rva=%08x '%int(rva),events,re.M);assert len(seqs)==1
  at(original,int(seqs[0]),item['before'],ZERO)
 at(original,before_seq,obs['before_prototype'],list(range(0xa0,0xb4)))
 symbols=subprocess.check_output(['nm','-S','--defined-only',str(captured_native_binary(native))],text=True)
 address,size=(int(v,16) for v in re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] initialize_state_1277b$',symbols,re.M).groups())
 assert address<=obs['prototype_pc']<address+size
 before=copy.deepcopy(obs['before_prototype']);before['bss']['empty_record_prototype']=list(ZERO)
 assert native_after_invoke_store(before,candidate,state.get('boundary_rva'))==current
