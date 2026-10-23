"""Three direct legacy segment handlers in real parsed declaration context."""
import json
from pathlib import Path
import re

FIELDS='owner name_address type related kind attributes references type_flags type_attributes shape_flags shape_attributes based_symbol debug_info error_count'.split()
DECIMAL={'kind','references','debug_info','error_count'}
def original_state(text,boundary,names):
    match=re.search(r'^support-state '+boundary+r' (\d+) (.+)$',text,re.M)
    assert match,'missing actual parsed support observation'
    words=match[2].split();assert len(words)==len(FIELDS)
    result={name:int(word,10 if name in DECIMAL else 16)for name,word in zip(FIELDS,words)}
    result['buffers']={names[int(i)]:None if n=='unopened'else h for i,n,h in re.findall(r'^buffer (\d+) (unopened|\d+)(?: ([0-9a-f]*))?$',text,re.M)}
    return result

def validate(original,native,plan,config):
    original=Path(original);native=Path(native)
    text=(original/'events').read_text();state=json.loads((native/'state.json').read_text())
    returned='direct-return eax 'in text
    split=text.index('direct-return eax 'if returned else'exception 80000003')
    names=[r['name'][1:]for r in config['buffered_streams']]
    before=original_state(text[:split],'before',names)
    for key in FIELDS:
        if key in {'owner','name_address','type','related','based_symbol'}:
            assert bool(before[key])==bool(state['before'][key]),('parsed pointer ownership differs',key)
        else:assert before[key]==state['before'][key],('parsed support context differs',key)
    assert before['buffers']==state['before']['buffers'],'pending parsed outputs differ'
    assert state['before']['owner_name']==plan['owner']
    assert before['owner'] and before['name_address'] and before['type'] and not before['based_symbol']
    target=next(r for r in config['driver']if r['name']==plan['function'])
    start=re.search(r'^direct-start target ([0-9a-f]+) marker ([0-9a-f]+) stack ([0-9a-f]+) count (\d+) args ([0-9a-f]+) ([0-9a-f]+)$',text,re.M)
    assert start and int(start[1],16)==target['start'] and int(start[4])==plan['arguments']
    args=[plan['kind'],before['name_address']]if plan['unit']<2 else[plan['kind']]
    assert [int(x,16)for x in start.groups()[4:4+len(args)]]==args
    native_args=[plan['kind'],state['before']['name_address']]if plan['unit']<2 else[plan['kind']]
    assert state['arguments']==state['formals']==native_args
    if not returned:
        exception=re.search(r'^exception 80000003 rva ([0-9a-f]+) base ([0-9a-f]+)',text[split:],re.M)
        assert int(exception[1],16)+int(exception[2],16)==target['start']
        slots={int(i):int(v,16)for i,v in re.findall(r'^stack (\d+) ([0-9a-f]+)$',text[split:],re.M)}
        assert [slots[i+1]for i in range(len(args))]==args
        assert state['status']=='entry'
        return dict(status='pass',kind='fresh direct entry',public_reachability=False)
    assert state['status']=='returned'
    after=original_state(text[split:],'after',names)
    for a,b in ((before,after),(state['before'],state['after'])):
        for key in FIELDS:
            expected=a[key]
            if key=='based_symbol'and plan['unit']<2:expected=a['owner']
            if key=='references'and plan['unit']<2:expected=(expected+a['debug_info'])&0xffff
            assert b[key]==expected,('direct support post-state differs',key,b[key],expected)
        assert b['buffers']==a['buffers'],'support helper changed pending IR'
    ret=re.search(r'^direct-return eax ([0-9a-f]+) edx ([0-9a-f]+) esp ([0-9a-f]+) expected ([0-9a-f]+) nonvolatile (\d+) fctrl ([0-9a-f]+) before ([0-9a-f]+)$',text,re.M)
    assert ret and ret[3]==ret[4] and ret[5]=='1' and ret[6]==ret[7]
    assert re.search(r'^direct-canary (.+)$',text,re.M)[1]=='a5a5a5a5 '*3+'a5a5a5a5'
    assert state['after']['registers']['esp']==state['stack']+4+4*len(args)
    assert state['canary']=='a5'*16
    for r in ('ebx','esi','edi','ebp','fctrl'):
        assert state['after']['registers'][r]==state['before']['registers'][r],('native nonvolatile differs',r)
    expected=(7,6,8)[plan['unit']]
    assert int(ret[1],16)&0xffff==state['after']['registers']['eax']&0xffff==expected
    for name in ('stdout','stderr'):
        assert (original/name).read_bytes()==(native/name).read_bytes()
    receipt=json.loads((native/'receipt.json').read_text())
    work=Path(receipt['environment']['RECON_DRIVE_S'])/'vc1.0/c1/legacy-type-support'
    assert {p.name:p.read_bytes()for p in (original/'work').iterdir()}=={p.name:p.read_bytes()for p in work.iterdir()},'complete owned files differ'
    return dict(status='pass',kind='direct body/ABI/state',public_reachability=False,defined_result=dict(eax_mask=65535,value=expected),original_before=before,original_after=after)
