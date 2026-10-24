"""Actual zero-formal iterator entry/return and borrowed-argument matcher frontier."""
import copy,json,re,struct
from provenance import captured_native_binary,sha
from allocation_state import symbols,call
from vector_state import validate as validate_vectors,snapshot
from scaler_state import semantic
from frontier_contract import validate_original_frontier,validate_native_stop

def validate(original,native,candidate,report):
    rows=[f for f in candidate['functions'] if f['name']=='_next_argument']
    assert len(rows)==1 and rows[0]['chunks']==[[0x12f9c,0x12fbe]]
    assert candidate['argument_cursor_observers']==dict(entry_rva=0x12f9c,return_rva=0x12c45,argument_index=1,returned_bytes_hex='2d696c00')
    receipt=json.loads((original/'receipt.json').read_text())
    if receipt['probe']=='normal':
        result=validate_vectors(original,native,candidate,report)
        return dict(status='normal prefix held at authenticated unsupported public dependency',candidate='_next_argument',candidate_executed=False,prefix=result)
    assert receipt['probe']=='restore'
    events=(original/'events.txt').read_text();validate_original_frontier(events,candidate,'restore');validate_native_stop(native,candidate)
    state=json.loads((native/'state.json').read_text());entries=json.loads((native/'argument-cursor-entries.json').read_text());returns=json.loads((native/'argument-cursor-returns.json').read_text())
    assert len(entries)==len(returns)==1
    entry,end=entries[0],returns[0]
    first=snapshot(original,native,state,entry)
    prefix=validate_vectors(original,native,candidate,report,stage_state=first)
    proof=validate_boundary(original,native,candidate,entry,end,state)
    return dict(status='matching real argument iterator return and complete matcher frontier',candidate='_next_argument',candidate_executed=True,candidate_code_bytes=34,proof=proof,prefix=prefix,execution_receipts=dict(original=sha(original/'receipt.json'),native=sha(native/'receipt.json')))

def validate_boundary(original,native,candidate,entry,end,state,following=False):
    binary=captured_native_binary(native);names=symbols(binary)
    before,after=[list(struct.unpack('<12I',(original/('argument-cursor-'+phase+'-registers.bin')).read_bytes()))for phase in ('entry','return')]
    assert before[1]==0x10612f9c and before[8:]==[0x10612c45,before[2],0x10600000,0]
    assert after[1]==0x10612c45 and after[2]==before[2]+4 and after[4:8]==before[4:8] and after[9:]==before[9:]
    assert before[0]<after[0]
    events=(original/'events.txt').read_text()
    args=re.findall(r'^invoke-arg index=1 address=([0-9a-f]+) bytes=(\d+)$',events,re.M);assert len(args)==1
    returned=int(args[0][0],16);assert int(args[0][1])==4 and after[3]==returned
    assert (original/'argument-cursor-result-000.bin').read_bytes()==b'-il\0'
    source=(original/'relocated-text.bin').read_bytes()
    assert source[0x12c45-0x1000:0x12c4b-0x1000]==bytes.fromhex('5068782b6610')
    assert source[0x12c4b-0x1000]==0xe8 and 0x12c50+struct.unpack_from('<i',source,0x12c4c-0x1000)[0]==0x12fbe
    raw=bytearray((original/'argument-cursor-entry-target-BSS.bin').read_bytes())
    argc,argv=struct.unpack_from('<II',raw,0x55c38-0x50000);assert argc==11
    struct.pack_into('<II',raw,0x55c38-0x50000,10,argv+4)
    assert bytes(raw)==(original/'argument-cursor-return-target-BSS.bin').read_bytes()
    for suffix in ('data','snapshot'):
        assert(original/('argument-cursor-entry-target-'+suffix+'.bin')).read_bytes()==(original/('argument-cursor-return-target-'+suffix+'.bin')).read_bytes()
    assert(original/'argument-cursor-entry-fp.bin').read_bytes()==(original/'argument-cursor-return-fp.bin').read_bytes()
    assert entry['symbol']=='next_argument' and entry['pc']==names['next_argument'][0]
    assert entry['args']=={} and entry['formal_inventory']==[] and entry['words']==[entry['caller']['pc']]
    assert entry['caller']['symbol']=='parse_command_line'
    assert entry['caller']['args']=={k:entry['invocation']['args'][k]for k in ('argc','argv')}
    call(binary,entry['words'][0],entry['pc'])
    assert end['pc']==entry['words'][0] and end['esp']==entry['esp']+4 and end['preserved']==entry['preserved']
    known=entry['compiler_state'];assert known['bss']['command_argc']==11 and known['bss']['command_argv']==entry['invocation']['args']['argv']
    expected=copy.deepcopy(known);expected['bss']['command_argc']=10;expected['bss']['command_argv']+=4
    assert end['compiler_state']==state['compiler_state']==expected
    assert end['eax']==entry['invocation']['argv_pointers'][1] and end['result_bytes']==list(b'-il\0')
    for row in (entry,end):
        assert row['errors']==[0,0,0] and row['invocation']==entry['invocation'] and row['crt_state']==entry['crt_state']
        assert semantic(row['fp'])==semantic(entry['fp'])
    assert state['symbol']=='match_option'
    assert state['pc']==names['match_option'][0] if following else names['match_option'][0]<state['pc']<=sum(names['match_option'])
    assert state['formal_inventory']==[dict(name=k,type=t)for k,t in [('rules','CompilerOptionRule *'),('argument','char *'),('next','char *(*)(void)'),('copy','int')]]
    assert state['args']==dict(rules=names['option_rules'][0],argument=end['eax'],next=names['next_argument'][0],copy=0)
    assert state['caller']['symbol']=='parse_command_line' and state['caller']['args']==entry['caller']['args'];call(binary,state['caller']['pc'],names['match_option'][0])
    assert state['invocation']==entry['invocation'] and semantic(state['command_line_fp'])==semantic(end['fp'])
    for key,value in end['crt_state'].items():assert state[key]==value
    frontier=re.findall(r'^trap seq=(\d+) rva=00012fbe .*stack=([0-9a-f,]+)$',events,re.M);assert len(frontier)==(3 if candidate.get('option_action_observers')else 1)
    seq=int(frontier[0][0]);assert[int(x,16)for x in frontier[0][1].split(',')]==[0x10612c50,0x10662b78,returned,0x10612f9c,0]
    assert(original/('compiler-bss-%03d.bin'%seq)).read_bytes()==bytes(raw)
    for suffix,stem in [('data','compiler-data'),('snapshot','compiler-snapshot')]:assert(original/('%s-%03d.bin'%(stem,seq))).read_bytes()==(original/('argument-cursor-return-target-'+suffix+'.bin')).read_bytes()
    return dict(argc_before=11,argc_after=10,argv_advance_bytes=4,borrowed_result='Invoke argv[1], complete -il NUL',formal_count=0,matcher_formal_count=4,initialized_unknown_bytes_held=4200)
