"""Complete natural parser-call inventory, initialized fields and getenv boundary."""
import json,re,struct
from allocation_state import symbols,call
from allocation_imports import export_rva
from provenance import captured_native_binary

def validate(original,native,entry,state,entry_sequence,sequence,module,candidate):
 assert (candidate['expected_frontier']['kind'],candidate['expected_frontier']['rva'])in [('post-internal-switches',0x12998),('post-system-info',0x12999),('arena-initialization',0x129db),('arena-reservation',0x12a18)]
 events=(original/'events.txt').read_text();binary=captured_native_binary(native);names=symbols(binary)
 entries=json.loads((native/'internal-parser-entries.json').read_text());returns=json.loads((native/'internal-parser-returns.json').read_text())
 raw_entries=re.findall(r'^internal-parser-entry index=(\d+) seq=(\d+) argument=([0-9a-f]+) mode=(\d+) esp=([0-9a-f]+)$',events,re.M)
 raw_returns=re.findall(r'^internal-parser-return index=(\d+) seq=(\d+) pc=([0-9a-f]+) esp=([0-9a-f]+) entry-esp=([0-9a-f]+) eax=([0-9a-f]+)\nlast-error=(\d+)\nx87-control=([0-9a-f]+)$',events,re.M)
 arguments=candidate['case_contract']['argv'][1:];assert len(arguments)==10
 assert len(entries)==len(returns)==len(raw_entries)==len(raw_returns)==len(arguments)
 previous=entry_sequence
 def original_unchanged(at):
  assert entry_sequence<at<sequence
  for region in ('compiler-data','compiler-bss','compiler-snapshot','state'):
   assert (original/('%s-%03d.bin'%(region,entry_sequence))).read_bytes()==(original/('%s-%03d.bin'%(region,at))).read_bytes()
 for index,(argument,before,after,raw_before,raw_after) in enumerate(zip(arguments,entries,returns,raw_entries,raw_returns)):
  assert before['index']==after['index']==int(raw_before[0])==int(raw_after[0])==index
  assert before['pc']==names['parse_internal_switch'][0]
  assert before['formal_inventory']==[dict(name='argument',type='const char *'),dict(name='mode',type='unsigned int')]
  assert before['argument']==entry['invocation']['argv_pointers'][index+1]
  assert before['mode']==after['eax']==int(raw_before[3])==int(raw_after[5],16)==0
  data=argument.encode('ascii')+b'\0';assert before['bytes']==list(data) and (original/('invoke-arg-%03d.bin'%(index+1))).read_bytes()==data
  assert before['compiler_state']==after['compiler_state']==entry['compiler_state']
  assert after['pc']==before['return'] and after['esp']==before['esp']+4
  assert after['last_error']==0 and after['fpu']==0x027f
  call(binary,before['return'],names['parse_internal_switch'][0])
  start,size=names['scan_internal_switches'];assert start<before['return']<start+size
  original_pointer=re.findall(r'^invoke-arg index=%d address=([0-9a-f]+) bytes=(\d+)$'%(index+1),events,re.M);assert len(original_pointer)==1
  assert int(raw_before[2],16)==int(original_pointer[0][0],16) and int(original_pointer[0][1])==len(data)
  begin_seq,end_seq=int(raw_before[1]),int(raw_after[1]);assert previous<begin_seq<end_seq;previous=end_seq
  trap=re.findall(r'^trap seq=%d rva=00012900 .*stack=([0-9a-f,]+)$'%begin_seq,events,re.M);assert len(trap)==1
  assert [int(x,16)for x in trap[0].split(',')][:3]==[module+0x128d2,int(raw_before[2],16),0]
  assert int(raw_after[2],16)==module+0x128d2 and int(raw_after[4],16)==int(raw_before[4],16) and int(raw_after[3],16)==int(raw_before[4],16)+4
  assert int(raw_after[6])==0 and int(raw_after[7],16)&0xffff==0x027f
  original_unchanged(begin_seq);original_unchanged(end_seq)
 # getenv is actually reached after all ten calls, rather than assumed absent.
 getenv_before=json.loads((native/'internal-getenv-entries.json').read_text());getenv_after=json.loads((native/'internal-getenv-returns.json').read_text());assert len(getenv_before)==len(getenv_after)==1
 before,after=getenv_before[0],getenv_after[0]
 assert before['pc']==names['recon_getenv'][0] and before['name']=='MSC_CMD_FLAGS'
 assert before['formal_inventory']==[dict(name='name',type='const char *')]
 assert after['pc']==before['return'] and after['esp']==before['esp']+4 and after['eax']==0
 assert before['compiler_state']==after['compiler_state']==entry['compiler_state']
 assert after['last_error']==0 and after['fpu']==0x027f
 # Rebuilt Win32 source imports the canonical getenv; native resolves its direct
 # shim call. The input name and actual NULL result are observed independently.
 call(binary,before['return'],names['recon_getenv'][0])
 get=re.findall(r'^internal-getenv seq=(\d+) pc=([0-9a-f]+) provider=([0-9a-f]+) name=(\S+) name-address=([0-9a-f]+) result=([0-9a-f]+) parser-calls=(\d+)$',events,re.M);assert len(get)==1
 g=get[0];get_seq=int(g[0]);assert previous<get_seq<sequence and int(g[1],16)==module+0x128ec and g[3]=='MSC_CMD_FLAGS'
 assert int(g[4],16)==module+0x631e8 and int(g[5],16)==0 and int(g[6])==10
 crt=int(re.findall(r'^module name=MSVCRT.dll actual=([0-9a-f]+) ',events,re.M)[0],16)
 assert int(g[2],16)==crt+export_rva(original/'loaded-msvcrt.bin','getenv')
 original_unchanged(get_seq)
 raw=(original/'relocated-text.bin').read_bytes();assert raw[0x128e6-0x1000:0x128ec-0x1000]==b'\xff\x15'+struct.pack('<I',module+0x5d050)
 table=[module+rva for rva in [0x1291a,0x304c7,0x12970,0x304db]]
 assert list(struct.unpack_from('<4I',raw,0x12988-0x1000))==table
 assert candidate['text_data']==[dict(owner='_parse_internal_switch',start_rva=0x12988,end_rva=0x12998,purpose='actual four-entry mode switch jump table; four padding bytes12984..12988 excluded')]
 assert raw[0x12900-0x1000:0x12908-0x1000]==bytes.fromhex('8b5424045685d257')
 # Only this environment entry is relevant here; private raw remains private.
 environment=(original/'invoke-environment.bin').read_bytes();count=struct.unpack_from('<I',environment,4)[0];at=8
 for i in range(count):
  pointer,length=struct.unpack_from('<II',environment,at);at+=8;item=environment[at:at+length];at+=length
  assert item.endswith(b'\0') and item[:-1].split(b'=',1)[0].lower()!=b'msc_cmd_flags'
 assert at==len(environment)
 assert state['args']=={} and state['formal_inventory']==[] and state['caller']['symbol']=='c1_main'
 assert state['compiler_state']==entry['compiler_state']
 for key,value in entry['crt_state'].items():assert(state['crt_state']if 'crt_state'in state else state)[key]==value
 call(binary,state['caller']['pc'],names['initialize_state_12998'][0])
 for region in ('compiler-data','compiler-bss','compiler-snapshot','state'):
  assert (original/('%s-%03d.bin'%(region,entry_sequence))).read_bytes()==(original/('%s-%03d.bin'%(region,sequence))).read_bytes()
 trap=re.findall(r'^trap seq=%d rva=00012998 .*stack=([0-9a-f,]+)$'%sequence,events,re.M);assert len(trap)==1 and int(trap[0].split(',')[0],16)==module+0x127e7
