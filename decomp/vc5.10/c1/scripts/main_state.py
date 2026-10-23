"""Independently observed main entry, then genuine main-to-helper state writes."""
import copy,json,re,struct,subprocess
from provenance import captured_native_binary
import main_fields

def boundary(original,native,state,sequence,module,candidate,probe):
 assert (candidate['expected_frontier']['kind'],candidate['expected_frontier']['rva']) in [('main-helper',0x12830),('work-buffer-allocation',0x12873),('main-switches',0x128b1),('scanner-argument',0x12900),('post-internal-switches',0x12998),('post-system-info',0x12999),('arena-initialization',0x129db),('arena-reservation',0x12a18),('arena-size-calculation',0x12a7a),('arena-initialized',0x12ab2),('arenas-complete',0x12ac3),('default-pool-arena',0x12b30)]
 events=(original/'events.txt').read_text()
 rows=re.findall(r'^trap seq=(\d+) rva=000127a7 esp=([0-9a-f]+).*stack=([0-9a-f,]+)\nlast-error=(\d+)\nx87-control=([0-9a-f]+)$',events,re.M)
 assert len(rows)==1;entry_sequence=int(rows[0][0]);assert entry_sequence<sequence
 stack=[int(x,16) for x in rows[0][2].split(',')]
 invoke=re.findall(r'^trap seq=\d+ rva=00012229 .*stack=([0-9a-f,]+)$',events,re.M);assert len(invoke)==1
 invoke_stack=[int(x,16) for x in invoke[0].split(',')]
 assert stack[0]==module+0x12274 and stack[1:3]==invoke_stack[1:3]
 relocated=(original/'relocated-text.bin').read_bytes()
 assert relocated[0x1226f-0x1000]==0xe8 and 0x12274+struct.unpack_from('<i',relocated,0x12270-0x1000)[0]==0x127a7
 assert relocated[0x12258-0x1000:0x12260-0x1000]==b'\x8b\x45\x10\xa3'+struct.pack('<I',module+0x5c08c)
 assert int(rows[0][3])==0 and int(rows[0][4],16)&0xffff==0x027f
 entries=json.loads((native/'main-entry-observations.json').read_text());assert len(entries)==1;entry=entries[0]
 assert entry['boundary_rva']==0x127a7 and entry['symbol']=='c1_main'
 assert entry['formal_inventory']==[dict(name='argc',type='int'),dict(name='argv',type='char **')]
 assert entry['args']=={key:state['invocation']['args'][key] for key in ('argc','argv')}
 assert entry['args']['argc']==candidate['case_contract']['argc'] and entry['esp']
 assert entry['argv']==state['invocation']['argv']==candidate['case_contract']['argv'] and entry['argv_terminator']==0
 binary=captured_native_binary(native);symbols=subprocess.check_output(['nm','-S','--defined-only',str(binary)],text=True)
 def symbol(name):return tuple(int(x,16) for x in re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] '+re.escape(name)+'$',symbols,re.M).groups())
 start,size=symbol('c1_main');assert entry['pc']==start
 invoke_start,invoke_size=symbol('InvokeCompilerPass');assert invoke_start<entry['return']<invoke_start+invoke_size
 code=subprocess.check_output(['objdump','-d','--start-address='+str(entry['return']-5),'--stop-address='+str(entry['return']),str(binary)],text=True)
 assert re.search(r'^\s*%x:\s+e8 (?:[0-9a-f]{2} ){3}[0-9a-f]{2}\s+call\s+%x\b'%(entry['return']-5,start),code,re.M)
 assert entry['compiler_state']['pass_mode']==candidate['case_contract']['BOOL']
 raw_global=(original/('state-%03d.bin'%entry_sequence)).read_bytes();assert struct.unpack_from('<i',raw_global,0x8c)[0]==entry['compiler_state']['pass_mode']
 crt=entry['crt_state'];assert crt['win32_last_error']==0 and crt['x87_control']==0x027f
 assert struct.unpack_from('<i',raw_global,0x90)[0]==crt['crt_attached']==1
 assert struct.unpack_from('<I',raw_global,0x9c)[0]==crt['raw_dllmain']==0
 assert struct.unpack_from('<i',raw_global,0xa0)[0]==crt['crt_adjust_fdiv']==0
 assert struct.unpack_from('<I',raw_global,0x40)[0]==module and crt['dll_instance']==crt['module_token']==state['module_token']
 original_end,original_begin=struct.unpack_from('<II',raw_global,0x94)
 assert original_begin==original_end!=0 and crt['onexit_begin']==crt['onexit_end']!=0 and crt['onexit_first']==0
 heap=re.findall(r'^state seq=%d heap-begin=([0-9a-f]+) heap-first=([0-9a-f]+)$'%entry_sequence,events,re.M)
 assert len(heap)==1 and int(heap[0][0],16)==original_begin and int(heap[0][1],16)==0

 return entry,entry_sequence

def validate_writes(original,native,fields,entry,state,entry_sequence,sequence,module,candidate,probe):
 main_fields.validate(original,native,fields,entry,state,entry_sequence,sequence,probe,candidate)
 binary=captured_native_binary(native);table=subprocess.check_output(['nm','-S','--defined-only',str(binary)],text=True)
 main_address,main_size=[int(x,16) for x in re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] c1_main$',table,re.M).groups()]
 helper_address=int(re.search(r'^([0-9a-f]+) [0-9a-f]+ [Tt] initialize_work_buffers$',table,re.M)[1],16)
 pc=state['caller']['pc'];assert main_address<pc<main_address+main_size
 call=subprocess.check_output(['objdump','-d','--start-address='+str(pc-5),'--stop-address='+str(pc),str(binary)],text=True)
 assert re.search(r'^\s*%x:\s+e8 (?:[0-9a-f]{2} ){3}[0-9a-f]{2}\s+call\s+%x\b'%(pc-5,helper_address),call,re.M)
 relocated=(original/'relocated-text.bin').read_bytes()
 assert relocated[0x127a9-0x1000:0x127b3-0x1000]==b'\xc7\x05'+struct.pack('<II',module+0x5f3d8,1)
 assert relocated[0x127b3-0x1000]==0xe8 and 0x127b8+struct.unpack_from('<i',relocated,0x127b4-0x1000)[0]==0x12830
 before=(original/('compiler-data-%03d.bin'%entry_sequence)).read_bytes();wanted=bytearray(before);struct.pack_into('<I',wanted,0x13d8,1)
 assert bytes(wanted)==(original/('compiler-data-%03d.bin'%sequence)).read_bytes()
 for region in ('compiler-bss','compiler-snapshot','state'):
  assert (original/('%s-%03d.bin'%(region,entry_sequence))).read_bytes()==(original/('%s-%03d.bin'%(region,sequence))).read_bytes()
 for key,value in entry['crt_state'].items():assert (state['crt_state'] if 'crt_state' in state else state)[key]==value
 expected=copy.deepcopy(entry['compiler_state']);expected['initialized']['compiler_phase']=1
 assert expected==state['compiler_state']
