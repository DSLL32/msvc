"""Borrowed option argument has BSS lifetime, separate from the argv cursor."""
import json,re,struct
RVA=0x538b0
OFFSET=2372

def expected_fields(fields):
 rows=[r for r in fields['modeled_bss']if r['name']=='current_argument'];assert len(rows)==1
 row=rows[0];assert(row['original_rva'],row['bytes'],row['native_offset'],row['default'],row['native_field'],row['c_type'],row['snapshot'])==(RVA,4,OFFSET,0,'c1_bss.current_argument','char *',False)
 assert fields['option_match']==dict(current_argument_rva=RVA,native_offset=OFFSET,bytes=4,default=0,bss_reset=True,initialized_snapshot=False,source_chunks=[[0x12fbe,0x1315f],[0x30919,0x3092f]],text_data=[[0x13160,0x131aa]],next_semantic_frontier=fields['option_match']['next_semantic_frontier'])
 assert fields['option_match']['next_semantic_frontier']in (0x131aa,0x132f9,0x1335b,0x1344d,0x1348d,0x134c9,0xd458)
 assert fields['native_bss_size']==2376
 return {'current_argument':(RVA,4,OFFSET)}

def read(raw):return struct.unpack_from('<I',raw,RVA-0x50000)[0]

def validate_reset(original,native):
 assert read((original/'compiler-bss-000.bin').read_bytes())==0
 receipt=json.loads((original/'receipt.json').read_text())
 if receipt['probe']=='restore':
  observations=json.loads((native/'restore-observations.json').read_text());events=(original/'events.txt').read_text()
  seq=re.findall(r'^trap seq=(\d+) rva=000122a5 ',events,re.M);assert len(seq)==1
  old=(original/('compiler-bss-%03d.bin'%int(seq[0]))).read_bytes();saved=struct.unpack_from('<I',old,0x7818)[0]
  assert read(old)==saved+4096
  assert observations['pre_seed']['bss']['current_argument']==0
  assert observations['before']['bss']['current_argument']==observations['before']['bss']['saved_data']+4096
  assert observations['post_restore']['bss']['current_argument']==0
