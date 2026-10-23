"""Actual scanner entry followed by its genuine first argument-parser frontier."""
import json,re,struct,subprocess
from provenance import captured_native_binary
from allocation_state import symbols,call

def boundary(original,native,state,sequence,module):
 events=(original/'events.txt').read_text();rows=re.findall(r'^trap seq=(\d+) rva=000128b1 .*stack=([0-9a-f,]+)\nlast-error=(\d+)\nx87-control=([0-9a-f]+)$',events,re.M)
 assert len(rows)==1;entry_sequence=int(rows[0][0]);assert entry_sequence<sequence
 words=[int(x,16)for x in rows[0][1].split(',')]
 invoke=re.findall(r'^trap seq=\d+ rva=00012229 .*stack=([0-9a-f,]+)$',events,re.M);assert len(invoke)==1
 original_invoke=[int(x,16)for x in invoke[0].split(',')]
 assert words[:2]==[module+0x127df,original_invoke[2]] and int(rows[0][2])==0 and int(rows[0][3],16)&0xffff==0x027f
 entries=json.loads((native/'scanner-entry-observations.json').read_text());assert len(entries)==1;entry=entries[0]
 assert entry['boundary_rva']==0x128b1 and entry['symbol']=='scan_internal_switches' and entry['esp']
 assert entry['formal_inventory']==[dict(name='arguments',type='char **')]
 assert entry['args']==dict(arguments=state['invocation']['args']['argv'])
 assert entry['invocation']==state['invocation']
 binary=captured_native_binary(native);names=symbols(binary);assert entry['pc']==names['scan_internal_switches'][0]
 assert entry['return']==entry['caller']['pc'] and entry['caller']['symbol']=='c1_main'
 assert entry['caller']['args']=={key:state['invocation']['args'][key]for key in ('argc','argv')}
 call(binary,entry['return'],entry['pc'])
 return entry,entry_sequence

def validate(original,native,entry,state,entry_sequence,sequence,module,candidate):
 expected=candidate['expected_frontier'];assert expected['kind']=='scanner-argument' and expected['rva']==0x12900
 assert state['caller']['symbol']=='scan_internal_switches'
 assert state['formal_inventory']==[dict(name='argument',type='const char *'),dict(name='mode',type='unsigned int')]
 assert state['args']==dict(argument=entry['invocation']['argv_pointers'][1],mode=0)
 assert state['argument_bytes']==list(candidate['case_contract']['argv'][1].encode('ascii')+b'\0')
 assert state['compiler_state']==entry['compiler_state']
 for key,value in entry['crt_state'].items():assert state[key]==value
 events=(original/'events.txt').read_text();rows=re.findall(r'^trap seq=%d rva=00012900 .*stack=([0-9a-f,]+)$'%sequence,events,re.M);assert len(rows)==1
 words=[int(x,16)for x in rows[0].split(',')]
 argument=re.findall(r'^invoke-arg index=1 address=([0-9a-f]+) bytes=(\d+)$',events,re.M);assert len(argument)==1
 assert words[:3]==[module+0x128d2,int(argument[0][0],16),0] and int(argument[0][1])==len(state['argument_bytes'])
 assert (original/'invoke-arg-001.bin').read_bytes()==bytes(state['argument_bytes'])
 for region in ('compiler-data','compiler-bss','compiler-snapshot','state'):
  assert (original/('%s-%03d.bin'%(region,entry_sequence))).read_bytes()==(original/('%s-%03d.bin'%(region,sequence))).read_bytes()
 raw=(original/'relocated-text.bin').read_bytes()
 assert raw[0x128cd-0x1000]==0xe8 and 0x128d2+struct.unpack_from('<i',raw,0x128ce-0x1000)[0]==0x12900
 assert raw[0x128b1-0x1000:0x128cb-0x1000]==bytes.fromhex('81ec0402000033c0568bb4240c02000083c604578b0e85c97414')
 binary=captured_native_binary(native);names=symbols(binary);call(binary,state['caller']['pc'],names['parse_internal_switch'][0])
 start,size=names['scan_internal_switches'];assert start<state['caller']['pc']<start+size
