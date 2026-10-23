"""Typed output-buffer projection; original gaps are never copied to native."""
import re,struct,subprocess
from pathlib import Path
FIELDS={'data':(0,0,4),'cursor':(4,4,4),'capacity':(8,8,4),'relocated':(12,12,4),'relocation_context':(16,16,4),'stream':(20,20,4),'good':(24,24,4),'mode':(28,28,1),'file_descriptor':(0x120,32,4)}
ZERO={name:0 for name in FIELDS}
INITIALIZED=dict(ZERO,good=1,file_descriptor=-1)
def read(data):
 return {name:struct.unpack_from('<'+('B' if size==1 else 'i' if name=='file_descriptor' else 'I'),data,0x4038+original)[0] for name,(original,native,size) in FIELDS.items()}
def validate(fields,original,bss,current):
 rows={r['name']:r for r in fields['modeled_bss']}
 for name,(offset,native,size) in FIELDS.items():
  row=rows['output_buffer.'+name]
  assert (row['original_rva'],row['native_offset'],row['bytes'],row['default'])==(0x54038+offset,232+native,size,0)
  assert row['native_field']=='c1_bss.output_buffer.'+name
 model=fields['output_buffer']
 assert model['original_object_rva']==0x54038 and model['native_size']==36 and model['native_offset']==232
 assert model['original_unmodeled_offsets']==[[29,288]] and model['object_extent_unknown']
 assert read((original/'compiler-bss-000.bin').read_bytes())==ZERO
 assert read(bss)==current['bss']['output_buffer']==INITIALIZED

def seeded(allocation,callback):
 return dict(data=allocation,cursor=allocation+4,capacity=47,relocated=callback,relocation_context=allocation,stream=allocation+8,good=9,mode=165,file_descriptor=57)
def check_phase(original,sequence,native,module,original_alloc,native_alloc,symbols,seed=False):
 data=(original/('compiler-bss-%03d.bin'%sequence)).read_bytes()
 assert read(data)==(seeded(original_alloc,module+0x12365) if seed else ZERO)
 assert native['bss']['output_buffer']==(seeded(native_alloc,symbols['initialize_state_12365']) if seed else ZERO)
def validate_constructor_probe(project,original,observations,raw,sequence,module,original_alloc,native_alloc,symbols,plan):
 assert observations['pre_seed']['bss']['output_buffer']==ZERO
 seqs=re.findall(r'^trap seq=(\d+) rva=000123ae ',raw,re.M)
 assert len(seqs)==1 and int(seqs[0])<sequence
 before=observations['before_buffer_constructor']
 bss=(original/('compiler-bss-%03d.bin'%int(seqs[0]))).read_bytes()
 check_phase(original,int(seqs[0]),before,module,original_alloc,native_alloc,symbols,seed=True)
 assert re.search(r'^output-buffer-seed rva=000123ae allocation=%08x callback=%08x$'%(original_alloc,module+0x12365),raw,re.M)
 assert plan=={'pointer_roles':'owned snapshot allocation; callback is initialize_state_12365 (never invoked)','capacity':47,'good':9,'mode':165,'file_descriptor':57,'callback_entry_rva':0x123ae,'normal_case':False}
 assert struct.unpack_from('<I',bss,0x7818)[0]==original_alloc
 assert struct.unpack_from('<I',bss,0x781c)[0]==0 and struct.unpack_from('<32I',bss,0x7820)==(0,)*32
 raw_data=(original/('compiler-data-%03d.bin'%int(seqs[0]))).read_bytes()
 raw_snapshot=(original/('compiler-snapshot-%03d.bin'%int(seqs[0]))).read_bytes()
 raw_globals=(original/('state-%03d.bin'%int(seqs[0]))).read_bytes()
 assert struct.unpack_from('<I',raw_data,0x1530)[0]==0
 assert struct.unpack_from('<I',raw_snapshot,0x1530)[0]==0x13579bdf
 assert struct.unpack_from('<I',raw_globals,0x64)[0]==0
 assert struct.unpack_from('<I',raw_globals,0x8c)[0]==before['pass_mode']==1
 assert before['initialized']['first_call']==0 and before['bss']['saved_data']==native_alloc and before['reuse_state']==0
 assert before['snapshot']['first_call']==0x13579bdf
 assert before['bss']['cleanup_count']==0 and before['bss']['cleanup_callbacks']==[0]*32
 assert before['bss']['context']==before['bss']['current_context']==dict(object=0,value=0,saved_frame=0)
 assert before['bss']['contexts']==[dict(object=0,value=0,saved_frame=0) for i in range(6)]
 bss=(original/('compiler-bss-%03d.bin'%int(seqs[0]))).read_bytes()
 assert struct.unpack_from('<3I',bss,0x18b8)==struct.unpack_from('<3I',bss,0x18c8)==(0,0,0)
 assert struct.unpack_from('<18I',bss,0x1858)==(0,)*18
 output=subprocess.check_output(['nm','-S','--defined-only',str(project/'build/native/c1-host')],text=True)
 address,size=re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] initialize_output_buffer_123ae$',output,re.M).groups()
 assert int(address,16)<=observations['buffer_constructor_pc']<int(address,16)+int(size,16)
