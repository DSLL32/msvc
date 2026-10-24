"""Actual NULL-tokenizer entry/RET followed by authentic parser writes/frontier."""
import copy,json,re,struct
from pathlib import Path
from allocation_state import symbols,call
from provenance import captured_native_binary,sha
from scaler_state import semantic,original_fp
from command_line_fields import DATA,read as read_options
from path_fold_state import validate as validate_path_fold

WARNINGS=[(1,5),(69,5),(611,5),(226,14),(233,14),(234,14),(235,14),(168,5),(169,5),(530,5)]

def original_state(original,candidate):
 assert candidate['functions'][-1]['name']=='_read_env_arguments'and candidate['functions'][-1]['chunks']==[[0x12f87,0x12f9c],[0x307dc,0x30919]]
 assert candidate['expected_frontier']==dict(rva=0x11e3c,native_symbol='new_option_vector',kind='environment-arguments-complete',calling_convention='cdecl',native_args=['count','mode'],caller=dict(native_symbol='parse_command_line',return_rva=0x12beb))
 assert candidate['environment_argument_observers']==dict(entry_rva=0x12f87,return_rva=0x12bb3,argument=None,accepted_input_scope='NULL only; full non-NULL tokenizer body retained, allocator pending')
 events=(original/'events.txt').read_text();assert'failure 'not in events
 assert re.findall(r'^frontier rva=(\w+)$',events,re.M)==['00011e3c']
 module=int(re.findall(r'^module name=c1n32_l.dll actual=(\w+) ',events,re.M)[0],16);assert module==0x10600000
 a,b=[list(struct.unpack('<12I',(original/('environment-'+phase+'-registers.bin')).read_bytes()))for phase in ('entry','return')]
 assert a[1]==module+0x12f87 and a[8:10]==[module+0x12bb3,0]and a[10:]==[a[2],module]
 assert b[1]==module+0x12bb3 and b[2]==a[2]+4 and b[4:8]==a[4:8]and b[10:]==a[10:]
 entry=re.findall(r'^trap seq=(\d+) rva=00012f87 esp=(\w+).*stack=([\w,]+)$',events,re.M);assert len(entry)==1
 assert int(entry[0][0])==a[0]and int(entry[0][1],16)==a[2]and[int(v,16)for v in entry[0][2].split(',')][:2]==a[8:10]
 final=re.findall(r'^trap seq=(\d+) rva=00011e3c esp=(\w+).*stack=([\w,]+)$',events,re.M);assert len(final)==1
 seq=int(final[0][0]);words=[int(v,16)for v in final[0][2].split(',')];assert words[:3]==[module+0x12beb,40,1]
 assert a[0]<b[0]<seq
 source=(original/'relocated-text.bin').read_bytes()
 assert source[0x12f87-0x1000:0x12f9c-0x1000]==bytes.fromhex('83ec08568b74241085f60f8545d801005e83c408c3')
 assert source[0x12be6-0x1000]==0xe8 and 0x12beb+struct.unpack_from('<i',source,0x12be7-0x1000)[0]==0x11e3c
 assert source[0x12be2-0x1000:0x12be6-0x1000]==bytes.fromhex('6a016a28')
 initial=(original/'environment-entry-target-data.bin').read_bytes();returned=(original/'environment-return-target-data.bin').read_bytes()
 assert initial==returned
 for suffix in ('BSS','snapshot'):
  assert(original/('environment-entry-target-'+suffix+'.bin')).read_bytes()==(original/('environment-return-target-'+suffix+'.bin')).read_bytes()
 assert(original/'environment-entry-fp.bin').read_bytes()==(original/'environment-return-fp.bin').read_bytes()==(original/('fpu-%03d.bin'%seq)).read_bytes()
 expected=bytearray(returned)
 for index,flags in WARNINGS:expected[0x5e9ec-0x5e000+index]=flags
 assert bytes(expected)==(original/('compiler-data-%03d.bin'%seq)).read_bytes()
 assert(original/'environment-return-target-BSS.bin').read_bytes()==(original/('compiler-bss-%03d.bin'%seq)).read_bytes()
 assert(original/'environment-return-target-snapshot.bin').read_bytes()==(original/('compiler-snapshot-%03d.bin'%seq)).read_bytes()
 return dict(entry=a,returned=b,final_sequence=seq,warnings=read_options(bytes(expected),DATA,0x5e000)['warning_levels'])

def validate_native_boundary(original,native,candidate,entries,returns,state):
 raw=original_state(original,candidate);binary=captured_native_binary(native);names=symbols(binary)
 assert len(entries)==len(returns)==1
 entry,end=entries[0],returns[0]
 assert entry['pc']==names['read_env_arguments'][0]and entry['symbol']=='read_env_arguments'and entry['args']==dict(arguments=0)
 assert entry['formal_inventory']==[dict(name='arguments',type='char *')]and entry['words']==[entry['caller']['pc'],0]
 assert entry['caller']['symbol']=='parse_command_line';call(binary,entry['words'][0],entry['pc'])
 assert end['pc']==entry['words'][0]and end['esp']==entry['esp']+4 and end['preserved']==entry['preserved']
 for key in ('compiler_state','crt_state','invocation','crt'):assert end[key]==entry[key]
 assert semantic(entry['fp'])==semantic(end['fp'])==semantic(original_fp(original,raw['entry'][0]))
 stage=copy.deepcopy(state)
 stage.update({key:entry[key]for key in ('pc','symbol','args','formal_inventory','caller','compiler_state','invocation','module_identity','module_token')})
 stage.update(entry['crt_state']);stage['command_line_fp']=entry['fp'];stage['pass_mode']=entry['compiler_state']['pass_mode']
 wanted=copy.deepcopy(end['compiler_state'])
 for index,flags in WARNINGS:wanted['option_state']['initialized']['warning_levels'][index]=flags
 assert state['compiler_state']==wanted
 assert wanted['option_state']['initialized']['warning_levels']==raw['warnings']
 assert state['symbol']=='new_option_vector'and names['new_option_vector'][0]<state['pc']<=sum(names['new_option_vector'])
 assert state['args']==dict(count=40,mode=1)and state['formal_inventory']==[dict(name='count',type='unsigned int'),dict(name='mode',type='int')]
 assert state['caller']['symbol']=='parse_command_line'and state['caller']['args']==entry['caller']['args'];call(binary,state['caller']['pc'],names['new_option_vector'][0])
 assert semantic(state['command_line_fp'])==semantic(end['fp'])
 for key,value in entry['crt_state'].items():assert state[key]==value
 assert state['invocation']==entry['invocation']and state['module_identity']==entry['module_identity']and state['module_token']==entry['module_token']
 assert state['pass_mode']==entry['compiler_state']['pass_mode']
 return stage

def validate(original,native,candidate,report):
 entries=json.loads((native/'environment-entries.json').read_text());returns=json.loads((native/'environment-returns.json').read_text());state=json.loads((native/'state.json').read_text())
 stage=validate_native_boundary(original,native,candidate,entries,returns,state)
 prefix=validate_path_fold(original,native,candidate,report,stage_state=stage)
 return dict(status='matching real NULL tokenizer entry and return, warning stores and complete next vector-allocation ABI',application_candidate='_read_env_arguments',candidate_code_bytes=338,reached_input=None,non_NULL_path='full source/graph only; genuine append-slot allocator remains pending',initialized_known_bytes=2348,initialized_unmodeled_bytes=4200,native_bss_bytes=2372,CRT_code_credit=0,prefix=prefix,execution_receipts=dict(original=sha(original/'receipt.json'),native=sha(native/'receipt.json')))
