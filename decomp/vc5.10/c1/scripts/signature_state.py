"""The independently encountered rolling PDB-signature scalar."""
import json,re,struct,subprocess
from provenance import captured_native_binary
PLAN=dict(before_clear=0x13572468,before_callback=0x24681357,result=0xffffffff,entry_rva=0x12664,normal_case=False)
def expected_fields(fields):
 m=fields['pdb_signature']
 assert (m['original_rva'],m['native_offset'],m['bytes'],m['default'],m['initialized'])==(0x54de0,540,4,0,0xffffffff)
 row=next(r for r in fields['modeled_bss'] if r['name']=='pdb_signature')
 assert row['default']==0 and row['native_field']=='c1_bss.pdb_signature'
 return {'pdb_signature':(0x54de0,4,540)}
def read(raw):return struct.unpack_from('<I',raw,0x4de0)[0]
def at(original,seq,n,wanted):
 raw=(original/('compiler-bss-%03d.bin'%seq)).read_bytes()
 assert read(raw)==n['bss']['pdb_signature']==wanted
 return raw

def validate(original,native,fields,state,sequence,probe,candidate,project):
 expected_fields(fields)
 assert candidate['restore_probe']['pdb_signature']==PLAN
 assert read((original/'compiler-bss-000.bin').read_bytes())==0
 final=at(original,sequence,state['compiler_state'],0xffffffff)
 events=(original/'events.txt').read_text()
 entries=re.findall(r'^trap seq=(\d+) rva=00012664 ',events,re.M)
 assert len(entries)==1 and int(entries[0])<sequence
 before=(original/('compiler-bss-%03d.bin'%int(entries[0]))).read_bytes()
 if probe=='normal':
  assert read(before)==0
  assert not (original/'signature-probe.txt').exists()
  assert not re.findall(r'^signature-seed ',events,re.M)
  return
 assert (original/'signature-probe.txt').read_text()=='13572468 24681357\n'
 obs=json.loads((native/'restore-observations.json').read_text())
 assert obs['pre_seed']['bss']['pdb_signature']==0
 for rva,key,wanted in [(0x122a5,'before',PLAN['before_clear']),(0x303d5,'post_restore',0),(0x12365,'before_callback',0),(0x12377,'before_current_callback',0),(0x12389,'before_contexts_callback',0),(0x12646,'before_value_copy',0),(0x12663,'after_value_copy',0),(0x12664,'before_signature',PLAN['before_callback'])]:
  seqs=re.findall(r'^trap seq=(\d+) rva=%08x '%rva,events,re.M);assert len(seqs)==1
  at(original,int(seqs[0]),obs[key],wanted)
 for rva,row in obs['buffer_constructors'].items():
  seqs=re.findall(r'^trap seq=(\d+) rva=%08x '%int(rva),events,re.M);assert len(seqs)==1
  at(original,int(seqs[0]),row['before'],0)
 assert re.findall(r'^signature-seed .*$',events,re.M)==['signature-seed rva=000122a5 value=13572468','signature-seed rva=00012664 value=24681357']
 symbols=subprocess.check_output(['nm','-S','--defined-only',str(captured_native_binary(native))],text=True)
 address,size=re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] initialize_state_12664$',symbols,re.M).groups()
 assert int(address,16)<=obs['signature_pc']<int(address,16)+int(size,16)
 # Every other mapped field and pointer identity must survive this scalar write.
 old=obs['before_signature'];now=state['compiler_state']
 for key in old:
  if key=='bss':assert {k:v for k,v in old[key].items() if k!='pdb_signature'}=={k:v for k,v in now[key].items() if k!='pdb_signature'}
  else:assert old[key]==now[key]
 expected=bytearray(before);struct.pack_into('<I',expected,0x4de0,0xffffffff)
 assert bytes(expected)==final
