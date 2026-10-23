"""Validate explicitly modeled semantic state; unknown bytes remain UNMODELED."""
import json,struct,subprocess,re
from pathlib import Path
import buffer_state
PROJECT=Path(__file__).resolve().parents[1]
def i32(data,offset):return struct.unpack_from('<i',data,offset)[0]
def u32(data,offset):return struct.unpack_from('<I',data,offset)[0]
def validate(original,native,state,sequence,module,candidate,expected,probe):
 fields=json.loads((PROJECT/candidate['field_map']).read_text())
 for key,region in [('modeled_initialized',fields['original_initialized_region_rva']),('modeled_bss',fields['original_bss_region_rva'])]:
  rows=fields[key];ranges=sorted((r['original_rva'],r['original_rva']+r['bytes']) for r in rows)
  assert all(region[0]<=a<b<=region[1] for a,b in ranges)
  assert all(a[1]<=b[0] for a,b in zip(ranges,ranges[1:]))
  native_ranges=sorted((r['native_offset'],r['native_offset']+r['bytes']) for r in rows)
  assert all(a[1]<=b[0] for a,b in zip(native_ranges,native_ranges[1:]))
 assert fields['allocation_bytes']==6548
 assert {r['name']:(r['original_rva'],r['bytes'],r['native_offset']) for r in fields['modeled_initialized']}=={'first_call':(0x5f530,4,0)}
 expected_bss={'saved_data':(0x57818,4,0),'cleanup_count':(0x5781c,4,4),'cleanup_callbacks':(0x57820,128,8),'context':(0x518c8,12,136),'current_context':(0x518b8,12,148),'contexts':(0x51858,72,160)}
 expected_bss.update(buffer_state.expected_fields(fields))
 assert {r['name']:(r['original_rva'],r['bytes'],r['native_offset']) for r in fields['modeled_bss']}==expected_bss
 first=fields['modeled_initialized'][0]
 assert first['snapshot_original_offset']==first['original_rva']-fields['original_initialized_region_rva'][0]==0x1530
 assert first['snapshot_native_offset']==first['native_offset']==0
 assert i32((original/'compiler-data-000.bin').read_bytes(),0x1530)==first['default']==1
 current=state['compiler_state'];layout=current['native_layout']
 assert layout=={'initialized_size':4,'bss_size':232+36*len(fields['output_buffers']),'first_call':0,'saved_data':0,'cleanup_count':4,'cleanup_callbacks':8,'context':136,'current_context':148,'contexts':160,'contexts_stride':12,'output_buffers':232,'output_buffer_count':len(fields['output_buffers']),'output_buffer_size':36,'buffer_members':{name:native for name,(original,native,size) in buffer_state.FIELDS.items()},'context_members':{'object':0,'value':4,'saved_frame':8}}
 data=(original/('compiler-data-%03d.bin'%sequence)).read_bytes();bss=(original/('compiler-bss-%03d.bin'%sequence)).read_bytes()
 assert len(data)==6548 and len(bss)==0xa06c
 buffer_state.validate(fields,original,bss,current)
 assert i32(data,0x1530)==current['initialized']['first_call']
 saved=u32(bss,0x7818);b=current['bss']
 assert bool(saved)==bool(b['saved_data'])
 assert not saved or not module<=saved<module+0x9b000
 assert i32(bss,0x781c)==b['cleanup_count']
 for name,offset in [('context',0x18c8),('current_context',0x18b8)]:
  context=next(r for r in fields['modeled_bss'] if r['name']==name)
  assert context['default']==[0,0,0] and context['members']==[{'name':member,'offset':at,'bytes':4} for member,at in [('object',0),('value',4),('saved_frame',8)]]
  assert struct.unpack_from('<3I',(original/'compiler-bss-000.bin').read_bytes(),offset)==tuple(context['default'])
  assert struct.unpack_from('<3I',bss,offset)==tuple(b[name][key] for key in ('object','value','saved_frame'))==(0,0,0)
 contexts=next(r for r in fields['modeled_bss'] if r['name']=='contexts')
 assert contexts['count']==6 and contexts['stride']==12 and contexts['default']==[[0,0,0] for i in range(6)]
 assert contexts['members']==[{'name':name,'offset':offset,'bytes':4} for name,offset in [('object',0),('value',4),('saved_frame',8)]]
 assert struct.unpack_from('<18I',(original/'compiler-bss-000.bin').read_bytes(),0x1858)==(0,)*18
 assert struct.unpack_from('<18I',bss,0x1858)==(0,)*18
 assert b['contexts']==[dict(object=0,value=0,saved_frame=0) for i in range(6)]
 symbols={name:int(address,16) for address,size,name in re.findall(r'^([0-9a-f]+) ([0-9a-f]+) [TtWw] (\S+)$',subprocess.check_output(['nm','-S','--defined-only',str(PROJECT/'build/native/c1-host')],text=True),re.M)}
 registered=candidate.get('registered_cleanup',[])
 assert len(registered)<=32
 wanted_original=tuple(module+row['original_rva'] for row in registered)+(0,)*(32-len(registered))
 wanted_native=tuple(symbols[row['native_symbol']] for row in registered)+(0,)*(32-len(registered))
 assert struct.unpack_from('<32I',bss,0x7820)==wanted_original
 assert tuple(b['cleanup_callbacks'])==wanted_native
 assert b['cleanup_count']==len(registered)
 globals_data=(original/('state-%03d.bin'%sequence)).read_bytes()
 assert i32(globals_data,0x64)==current['reuse_state']
 assert i32(globals_data,0x8c)==state['pass_mode']==current['pass_mode']
 all_symbols={name:(int(address,16),int(size,16)) for address,size,name in re.findall(r'^([0-9a-f]+) ([0-9a-f]+) [A-Za-z] (\S+)$',subprocess.check_output(['nm','-S','--defined-only',str(PROJECT/'build/native/c1-host')],text=True),re.M)}
 for key,symbol in [('initialized','c1_data'),('bss','c1_bss')]:
  address,size=all_symbols[symbol];assert current['live_addresses'][key]==address
  assert not b['saved_data'] or not address<=b['saved_data']<address+size
 table=(original/'compiler-callbacks-000.bin').read_bytes();assert len(table)==34*4
 assert len(fields['callbacks'])==len(current['initialization_callbacks'])==34
 for index,row in enumerate(fields['callbacks']):
  assert row['index']==index
  assert u32(table,index*4)==(module+row['original_rva'] if row['original_rva'] else 0)
  assert current['initialization_callbacks'][index]==(symbols[row['native_symbol']] if row['native_symbol'] else 0)
 if expected['kind']=='compiler-allocation':
  assert current['initialized']['first_call']==1
  assert saved==b['saved_data']==0 and current['snapshot'] is None
 elif expected['kind'] in ('initialization-callback','initialization-helper'):
  assert current['initialized']['first_call']==0 and saved and b['saved_data']
  snapshot=(original/('compiler-snapshot-%03d.bin'%sequence)).read_bytes();assert len(snapshot)==6548
  expected_first = candidate['restore_probe']['snapshot_first_call'] if probe=='restore' else 1
  assert i32(snapshot,0x1530)==current['snapshot']['first_call']==expected_first
 else:raise AssertionError('new state consumer requires an explicit modeled projection')
 assert current['reuse_state']==0 and b['cleanup_count']==len(registered)

 if probe=='normal' and expected['kind'] in ('initialization-callback','initialization-helper','compiler-allocation'):
  raw=(original/'events.txt').read_text()
  rows=re.findall(r'^trap seq=\d+ rva=0000e799 .*stack=([0-9a-f,]+)$',raw,re.M)
  assert len(rows)==1 and int(rows[0].split(',')[1],16)==6548
  observed=json.loads((native/'allocation-observations.json').read_text())
  assert len(observed)==1 and observed[0]['size']==6548
  match=re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] allocate_compiler_memory$',subprocess.check_output(['nm','-S','--defined-only',str(PROJECT/'build/native/c1-host')],text=True),re.M)
  start,size=(int(v,16) for v in match.groups());assert start<=observed[0]['pc']<start+size
 if probe=='restore':
  validate_restore(original,native,state,sequence,module,candidate,symbols,fields)


def validate_restore(original,native,state,sequence,module,candidate,symbols,fields):
 raw=(original/'events.txt').read_text();plan=candidate['restore_probe']
 assert not re.findall(r'^trap seq=\d+ rva=0000e799 ',raw,re.M)
 assert json.loads((native/'allocation-observations.json').read_text())==[]
 assert re.search(r'^restore-seed snapshot=[0-9a-f]+ bytes=6548 first=13579bdf count=7 mode=1 reuse=9$',raw,re.M)
 preseqs=re.findall(r'^trap seq=(\d+) rva=000122a5 ',raw,re.M)
 postseqs=re.findall(r'^trap seq=(\d+) rva=000303d5 ',raw,re.M)
 assert len(preseqs)==len(postseqs)==1
 preseq,postseq=int(preseqs[0]),int(postseqs[0]);assert preseq<postseq<sequence
 seed=(original/'restore-original-input.bin').read_bytes();assert len(seed)==6548
 observations=json.loads((native/'restore-observations.json').read_text())
 pre,post=observations['before'],observations['post_restore']
 assert observations['pre_seed']['bss']['contexts']==[dict(object=0,value=0,saved_frame=0) for i in range(6)]
 assert all(observations['pre_seed']['bss'][name]==dict(object=0,value=0,saved_frame=0) for name in ('context','current_context'))
 last=state['compiler_state']
 original_alloc=None;native_alloc=None
 for seq,n,phase in [(preseq,pre,'before'),(postseq,post,'post_restore')]:
  d=(original/('compiler-data-%03d.bin'%seq)).read_bytes();b=(original/('compiler-bss-%03d.bin'%seq)).read_bytes();g=(original/('state-%03d.bin'%seq)).read_bytes()
  saved=u32(b,0x7818);assert saved and n['bss']['saved_data']
  if original_alloc is None:original_alloc=saved;native_alloc=n['bss']['saved_data']
  assert saved==original_alloc and n['bss']['saved_data']==native_alloc
  assert i32(d,0x1530)==n['initialized']['first_call']==(plan['live_first_call'] if phase=='before' else plan['snapshot_first_call'])
  assert i32(b,0x781c)==n['bss']['cleanup_count']==(plan['cleanup_count'] if phase=='before' else 0)
  original_callbacks=list(struct.unpack_from('<32I',b,0x7820))
  wanted_original=([module+rva for rva in plan['callback_rvas']]+[0]*30) if phase=='before' else [0]*32
  wanted_native=([symbols['initialize_state_%x'%rva] for rva in plan['callback_rvas']]+[0]*30) if phase=='before' else [0]*32
  assert original_callbacks==wanted_original and n['bss']['cleanup_callbacks']==wanted_native
  for name,offset,value in [('context',0x18c8,23),('current_context',0x18b8,29)]:
   original_context=struct.unpack_from('<3I',b,offset)
   native_context=tuple(n['bss'][name][key] for key in ('object','value','saved_frame'))
   assert original_context==((module+0x5f530,value,original_alloc) if phase=='before' else (0,0,0))
   assert native_context==((n['live_addresses']['initialized'],value,native_alloc) if phase=='before' else (0,0,0))
   assert plan[name]=={'object':'first_call-address','value':value,'saved_frame':'owned-snapshot-allocation'}
  assert plan['contexts']=={'object':'first_call-address','values':list(range(31,37)),'saved_frame':'owned-snapshot-allocation'}
  for index in range(6):
   assert struct.unpack_from('<3I',b,0x1858+index*12)==((module+0x5f530,31+index,original_alloc) if phase=='before' else (0,0,0))
   assert n['bss']['contexts'][index]==(dict(object=n['live_addresses']['initialized'],value=31+index,saved_frame=native_alloc) if phase=='before' else dict(object=0,value=0,saved_frame=0))
  buffer_state.check_phase(fields,original,seq,n,module,original_alloc,native_alloc,symbols,seed=phase=='before')
  assert i32(g,0x64)==n['reuse_state']==plan['reuse_state']
  assert i32(g,0x8c)==n['pass_mode']==plan['pass_mode']
  assert n['snapshot']['first_call']==plan['snapshot_first_call']
  assert n['initialization_callbacks']==last['initialization_callbacks']
  if phase=='post_restore':
   # Complete original-to-original runtime copy; no native opaque-byte claim.
   assert d==seed
   zero=bytearray(b);struct.pack_into('<I',zero,0x7818,0);assert not any(zero)
 assert u32((original/('compiler-bss-%03d.bin'%sequence)).read_bytes(),0x7818)==original_alloc
 assert last['bss']['saved_data']==native_alloc and last['reuse_state']==0
 assert last['initialized']['first_call']==0
 for rva,name,offset,value,key,pc_key,plan_key in [
   (0x12365,'context',0x18c8,23,'before_callback','callback_pc','callback_context_auxiliary'),
   (0x12377,'current_context',0x18b8,29,'before_current_callback','current_callback_pc','current_context_callback_auxiliary')]:
  callbackseqs=re.findall(r'^trap seq=(\d+) rva=%08x '%rva,raw,re.M)
  assert len(callbackseqs)==1 and postseq<int(callbackseqs[0])<sequence
  callbackseq=int(callbackseqs[0]);before_callback=observations[key]
  callback_bss=(original/('compiler-bss-%03d.bin'%callbackseq)).read_bytes()
  assert re.search(r'^context-seed rva=%08x object=%08x value=%d frame=%08x$'%(rva,module+0x5f530,value,original_alloc),raw,re.M)
  assert struct.unpack_from('<3I',callback_bss,offset)==(module+0x5f530,value,original_alloc)
  assert before_callback['bss'][name]==dict(object=before_callback['live_addresses']['initialized'],value=value,saved_frame=native_alloc)
  other='current_context' if name=='context' else 'context'
  buffer_state.check_phase(fields,original,callbackseq,before_callback,module,original_alloc,native_alloc,symbols)
  assert before_callback['bss'][other]==dict(object=0,value=0,saved_frame=0)
  assert struct.unpack_from('<3I',callback_bss,0x18b8 if name=='context' else 0x18c8)==(0,0,0)
  assert struct.unpack_from('<18I',callback_bss,0x1858)==(0,)*18
  assert before_callback['bss']['contexts']==[dict(object=0,value=0,saved_frame=0) for i in range(6)]
  assert before_callback['initialized']['first_call']==0 and before_callback['snapshot']['first_call']==plan['snapshot_first_call']
  assert before_callback['pass_mode']==plan['pass_mode']
  assert before_callback['reuse_state']==0 and before_callback['bss']['saved_data']==native_alloc
  assert before_callback['bss']['cleanup_count']==0 and before_callback['bss']['cleanup_callbacks']==[0]*32
  symbol='initialize_state_%x'%rva;callback_address=int(symbols[symbol])
  callback_size=re.search(r'^[0-9a-f]+ ([0-9a-f]+) [Tt] '+symbol+'$',subprocess.check_output(['nm','-S','--defined-only',str(PROJECT/'build/native/c1-host')],text=True),re.M)[1]
  assert callback_address<=observations[pc_key]<callback_address+int(callback_size,16)
  assert plan[plan_key]=={'entry_rva':rva,'object':'first_call-address','value':value,'saved_frame':'owned-snapshot-allocation','expected_result':[0,0,0],'normal_case':False}
 arrayseqs=re.findall(r'^trap seq=(\d+) rva=00012389 ',raw,re.M)
 assert len(arrayseqs)==1 and postseq<int(arrayseqs[0])<sequence
 array_bss=(original/('compiler-bss-%03d.bin'%int(arrayseqs[0]))).read_bytes()
 before_array=observations['before_contexts_callback']
 buffer_state.check_phase(fields,original,int(arrayseqs[0]),before_array,module,original_alloc,native_alloc,symbols)
 assert re.search(r'^contexts-seed rva=00012389 count=6 object=%08x frame=%08x$'%(module+0x5f530,original_alloc),raw,re.M)
 assert plan['contexts_callback_auxiliary']=={'entry_rva':0x12389,'count':6,'normal_case':False}
 for index in range(6):
  assert struct.unpack_from('<3I',array_bss,0x1858+index*12)==(module+0x5f530,31+index,original_alloc)
  assert before_array['bss']['contexts'][index]==dict(object=before_array['live_addresses']['initialized'],value=31+index,saved_frame=native_alloc)
 for name,offset in [('context',0x18c8),('current_context',0x18b8)]:
  assert before_array['bss'][name]==dict(object=0,value=0,saved_frame=0)
  assert struct.unpack_from('<3I',array_bss,offset)==(0,0,0)
 assert before_array['initialized']['first_call']==0 and before_array['snapshot']['first_call']==plan['snapshot_first_call']
 assert before_array['pass_mode']==plan['pass_mode']
 assert before_array['reuse_state']==0 and before_array['bss']['saved_data']==native_alloc
 assert before_array['bss']['cleanup_count']==0 and before_array['bss']['cleanup_callbacks']==[0]*32
 array_address=int(symbols['initialize_state_12389'])
 array_size=re.search(r'^[0-9a-f]+ ([0-9a-f]+) [Tt] initialize_state_12389$',subprocess.check_output(['nm','-S','--defined-only',str(PROJECT/'build/native/c1-host')],text=True),re.M)[1]
 assert array_address<=observations['contexts_callback_pc']<array_address+int(array_size,16)
 buffer_state.validate_constructor_probe(fields,PROJECT,original,observations,raw,sequence,module,original_alloc,native_alloc,symbols,plan['output_buffers'])
 address,size=re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] initialize_compiler_pass$',subprocess.check_output(['nm','-S','--defined-only',str(PROJECT/'build/native/c1-host')],text=True),re.M).groups()
 assert int(address,16)<=observations['post_restore_pc']<int(address,16)+int(size,16)
 line=next(i for i,text in enumerate((PROJECT/'src/pass.c').read_text().splitlines(),1) if 'c1_reuse_state = !c1_pass_mode;' in text)
 assert observations['post_restore_line']==line
