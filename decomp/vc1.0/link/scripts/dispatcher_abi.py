"""Dispatcher's actual formal, owned mutable-name and public FILE observations."""
import json
from pathlib import Path
NAMES=['linker_main','library_main','dumper_main','editor_main','helper_main',None]
FUNCTIONS=[0x17ea0,0x2e030,0x2fee0,0x278a0,0x26fe0,0]
LENGTHS=[4,3,4,4,4,0]
KINDS=[1,2,3,4,0,0]
INITIAL=[b'Linker\0',b'Library Manager\0',b'Dumper\0',b'Editor\0',b'Helper\0',b'\0']

def original(directory):
    return [json.loads(x) for x in (Path(directory)/'events.jsonl').read_text().splitlines()]

def canonical(state,phase):
    tools=[]
    for i,t in enumerate(state['tools']):
        assert t['length']==LENGTHS[i] and t['padding']==0 and t['kind']==KINDS[i]
        tools.append({k:t[k] for k in ['length','padding','kind','bytes']})
    stream=state['stdout_file'];assert len(stream)==8 and stream[4]==1 and stream[5]==stream[7]==0
    assert (stream[0]==0)==(stream[2]==0)
    used=stream[0]-stream[2];assert used==len(bytes.fromhex(state['pending']))
    result=dict(program=state['program'],tools=tools,stdout_file=[used,stream[1],bool(stream[2]),*stream[3:]],pending=state['pending'],errors=state['errors'],ctype=state['ctype'],mb_cur_max=state['mb_cur_max'],argv=state['argv'])
    assert result['mb_cur_max']==1 and len(bytes.fromhex(result['ctype']))==514
    assert result['program']==(b'LINK\0' if phase=='helper' else b'CVTRES\0').hex()
    if state['event'] in ['entry','frontier']:
        if phase=='helper':assert len(state['formal_words'])==2 and state['formal_words'][0]==1 and state['formal_words'][1] and state['argv']==[b'-helper\0'.hex()]
        else:assert state['formal_words']==[] and state['argv']==[]
    return result

def compare(left,right,phase,complete):
    expected=[e for e in left if e['event'] in ['entry','return','frontier']]
    actual=right['events'];assert len(expected)==len(actual)==(2 if complete else 1)
    logical='helper_main' if phase=='helper' else 'driver_usage'
    for l,r in zip(expected,actual):
        assert l['name']==r['name']==logical and l['event']==r['event']
        assert l['eip']==({'helper_main':0x26fe0,'driver_usage':0x27050}[logical] if l['event']!='return' else l['eip'])
        assert canonical(l,phase)==canonical(r,phase),(canonical(l,phase),canonical(r,phase))
        for i,t in enumerate(l['tools']):assert t['run']==FUNCTIONS[i]
        assert r['run_owners']==[True]*6,'actual typed callback ownership'
        if 'writable_names' in r:assert r['writable_names']==[True]*5,'owned writable full-name arrays'
        desired=INITIAL[:]
        if phase=='direct' and l['event']=='return':
            desired=[s[:LENGTHS[i]]+b'\0'+s[LENGTHS[i]+1:] for i,s in enumerate(INITIAL)]
        assert [t['bytes'] for t in l['tools']]==[s.hex() for s in desired]
    if phase=='helper':assert expected[0]['caller']==0x26f40
    if complete:
        assert right['terminal']==[dict(event='exit',code=3)]
        assert left[-1]==dict(event='summary',failed=0,stopped=1,exit=3,stdout_bytes=163 if phase=='helper' else 71,stderr_bytes=0)
        assert actual[0]['actual_caller']==('main' if phase=='helper' else '__wrap_main')
        for entries in [expected,actual]:
            a,z=entries;assert z['esp']==a['esp']+4 and z['eax']==3
            assert all(z[k]==a[k] for k in ['ebx','esi','edi','ebp'])
            assert [t['name_pointer'] for t in a['tools']]==[t['name_pointer'] for t in z['tools']]
    return dict(status='pass',phase=phase,complete=complete,formal_slots=2 if phase=='helper' else 0,observations=len(expected),tool_entries=6,full_name_tails=True,ctype_entries=257,errors_exact=True,nonvolatile_and_cdecl_return=complete,reach='public CLI callback' if phase=='helper' else 'external direct zero-formal caller; ordinary CLI invariant prevents this path')
