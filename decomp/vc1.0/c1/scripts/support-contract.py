"""Direct legacy type and segment helpers in real parsed declaration context."""
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

def arguments(plan,state):
    unit=plan['unit']
    if unit<3:return [plan['kind'],state['name_address']]if unit<2 else[plan['kind']]
    return {3:[state['owner']],4:[2,state['owner']],5:[state['type']],6:[state['type'],7],7:[state['name_address'],state['name_address']+5]}[unit]

def result_contract(text,state,before,plan,eax):
    unit=plan['unit'];native_eax=state['after']['registers']['eax']
    if unit<3:
        expected=(7,6,8)[unit];assert eax&0xffff==native_eax&0xffff==expected
        return dict(eax_mask=65535,value=expected)
    depth=[int(v)for v in re.findall(r'^support-type-depth (?:before|after) (\d+)$',text,re.M)]
    assert len(depth)==2 and depth[0]==depth[1]==state['before']['type_description_depth']==state['after']['type_description_depth'],'temporary type-description depth differs'
    if unit in(3,7):
        expected=4 if unit==3 else 1;assert eax==native_eax==expected
        return dict(eax_mask=0xffffffff,value=expected)
    result=state['result'];assert result['address']==native_eax and eax
    if unit==4:
        m=re.search(r'^support-result leaf (\w+) (\d+) (\d+) (\w+) (\w+)$',text,re.M);assert m
        assert int(m[1],16)==eax and int(m[2])==result['kind']==2 and int(m[3])==result['flags']==4
        assert int(m[4],16)==before['type'] and int(m[5],16)==before['owner']
        assert result['type']==state['before']['type']and result['symbol']==state['before']['owner']
        return dict(eax_mask=0xffffffff,owner='constructed leaf',kind=2,flags=4,type='parsed owner type',value='parsed symbol')
    m=re.search(r'^support-result type (.+)$',text,re.M);assert m;words=[int(v,16)for v in m[1].split()];assert len(words)==10 and words[0]==eax
    assert words[1:3]==[result['flags'],result['attributes']]
    if unit==5:
        assert eax!=before['type']and native_eax!=state['before']['type']and words[3]
        assert len(result['shapes'])==2 and result['related']==result['shapes'][0]['address']
        assert [words[4],words[5],words[7],words[8]]==[result['shapes'][0]['flags'],result['shapes'][0]['attributes'],result['shapes'][1]['flags'],result['shapes'][1]['attributes']]
        assert words[6]==before['related']and words[9]==0
        assert result['shapes'][0]['related']==result['shapes'][1]['address']==state['before']['related']and result['shapes'][1]['related']==0
        return dict(eax_mask=0xffffffff,owner='constructed function pointer type',flags=words[1],attributes=words[2],shape_flags=words[4],shape_attributes=words[5],inner_shape='parsed function shape')
    assert unit==6 and eax==before['type']and native_eax==state['before']['type']
    assert words[1:3]==[before['type_flags'],before['type_attributes']]and words[3:]==[0]*7
    assert not result['related']and not result['shapes']
    return dict(eax_mask=0xffffffff,owner='existing interned parsed type',cleared_mask=7)

def literal_state(text,boundary):
    m=re.search(r'^literal-state '+boundary+r' symbol (\w+) token (\w+) head (\w+) type (\w+) flags (\w+) extended (\w+) line (\w+) auxiliary (\w+) index (\d+) value (\w+)$',text,re.M)
    assert m,'missing genuine literal state'
    result={n:int(v,16)for n,v in zip(['symbol','token','head','type','flags','extended','line','auxiliary'],m.groups()[:8])}
    result.update(index=int(m[9]),value=m[10]);return result

def validate_literal(original,native,plan,config):
    text=(original/'events').read_text();state=json.loads((native/'state.json').read_text())
    returned='literal-return esp 'in text
    split=text.index('literal-return esp 'if returned else'exception 80000003')
    names=[r['name'][1:]for r in config['buffered_streams']]
    # The first dump precedes the direct call; the last is the reached writer.
    before_text=text[:text.index('literal-reentry ')]
    before=original_state(before_text,'before',names)
    for key in FIELDS:
        if key in {'owner','name_address','type','related','based_symbol'}:assert bool(before[key])==bool(state['before'][key]),key
        else:assert before[key]==state['before'][key],('literal parsed owner differs',key)
    assert before['buffers']==state['before']['buffers']and state['before']['owner_name']==plan['owner']
    observed=literal_state(text,'before');actual=state['literal_before']
    assert observed['symbol']==before['owner'] and actual['symbol']==state['before']['owner']
    for key in ['token','type','extended','line','auxiliary']:assert bool(observed[key])==bool(actual[key]),key
    assert observed['head']==int.from_bytes(bytes.fromhex(actual['head']),'little')
    assert observed['flags']==actual['flags']and observed['value']==actual['value']and observed['index']==actual['index']
    assert observed['index']>0 and not observed['auxiliary']
    start=re.search(r'^literal-start parent (\w+) marker (\w+) return (\w+) byte (\w+) storage (\w+) stack (\w+) symbol (\w+) token (\w+)$',text,re.M);assert start
    parent=next(r for r in config['driver']if r['name']==plan['function']);marker=next(r for r in config['driver']if r['name']==plan['marker'])
    assert int(start[1],16)==parent['start']and int(start[2],16)==marker['start']and int(start[3],16)==0x3b2d5 and start[4]=='38'
    storage=int(start[5],16);stack=int(start[6],16);assert stack==storage+65536-512
    assert [int(start[7],16),int(start[8],16)]==[observed['symbol'],observed['token']]
    assert state['arguments']==state['formals']==[actual['symbol'],actual['token']]
    reentry=re.findall(r'^literal-reentry marker (\w+) esp (\w+) symbol (\w+) token (\w+) stream (\w+)$',text,re.M);assert len(reentry)==1
    r=[int(v,16)for v in reentry[0]];assert r[0]==marker['start']and storage<=r[1]and r[1]+16<=stack and r[2:]==[observed['symbol'],observed['token'],observed['line']]
    assert state['reentry']['arguments']==[actual['symbol'],actual['token'],actual['line']]
    internal=re.findall(r'^literal-internal-hit pc (\w+) esp (\w+) symbol (\w+) stream (\w+) format-offset (\d+) instruction 381f$',text,re.M)
    steps=re.findall(r'^literal-internal-step pc (\w+) esp (\w+) restored-trap cc$',text,re.M)
    assert len(internal)==len(steps)==(4 if returned else 3),'bounded parent dispatch hits differ'
    for hit,step in zip(internal,steps):
        pc,sp,symbol,stream,offset=[int(v,10 if i==4 else 16)for i,v in enumerate(hit)]
        assert pc==int(start[3],16)and sp==stack-48 and symbol==observed['symbol']and stream==observed['line']and 1<=offset<=4
        assert int(step[0],16)==pc+2 and int(step[1],16)==sp
    writer=re.search(r'^literal-writer entry (\w+) esp (\w+) value (\w+) width (\d+) stream (\w+) bytes (\w+)$',text,re.M);assert writer
    leaf=next(r for r in config['driver']if r['name']==plan['accepts']);width=4 if plan['unit']==8 else 8
    expected='0000a03f'if width==4 else'00000000000004c0'
    assert int(writer[1],16)==leaf['start']and int(writer[4])==state['writer']['width']==width
    assert storage<=int(writer[2],16)and int(writer[2],16)+8<=stack and storage<=int(writer[3],16)and int(writer[3],16)+width<=stack
    assert int(writer[5],16)==observed['line']and writer[6]==state['writer']['bytes']==expected
    assert state['writer']['stream']==actual['line']and state['breakpoints_restored']
    cleanup=re.findall(r'^literal-cleanup (\d+) return (\w+) byte (\w+) marker (\w+) byte cc$',text,re.M)
    assert len(cleanup)==1 and cleanup[0]==('1',start[3],start[4],start[2]),'original breakpoint restoration differs'
    if returned:
        ret=re.search(r'^literal-return esp (\w+) expected (\w+) nonvolatile 1 fctrl (\w+) before (\w+) canary (.+)$',text,re.M)
        assert ret and ret[1]==ret[2]and int(ret[1],16)==stack+12 and ret[3]==ret[4]and ret[5]=='a5a5a5a5 '*3+'a5a5a5a5'
        assert state['status']=='returned'and state['canary']=='a5'*16
        after=original_state(text[split:],'after',names)
        for key in FIELDS:assert after[key]==before[key]and state['after'][key]==state['before'][key],('literal owner changed',key)
        assert after['buffers']==state['after']['buffers'],'complete pending literal bytes differ'
        orig_after=literal_state(text,'after');nat_after=state['literal_after']
        for a,b in [(observed,orig_after),(actual,nat_after)]:
            for key in a:assert b[key]==(a['line']if key=='auxiliary'else a[key]),('literal input/stream changed',key)
    else:
        assert state['status']=='entry'
        exception=re.search(r'^exception 80000003 rva (\w+) base (\w+)',text,re.M);assert exception and int(exception[1],16)+int(exception[2],16)==leaf['start']
        pending={names[int(i)]:None if n=='unopened'else h for i,n,h in re.findall(r'^buffer (\d+) (unopened|\d+)(?: ([0-9a-f]*))?$',text,re.M)}
        assert pending==state['writer_context']['buffers'],'fresh writer prefix differs'
    for name in ['stdout','stderr']:assert (original/name).read_bytes()==(native/name).read_bytes()
    receipt=json.loads((native/'receipt.json').read_text())
    from case_paths import canonical_parts
    work=Path(receipt['environment']['RECON_DRIVE_S']).joinpath(*canonical_parts(json.loads((Path(__file__).resolve().parents[1]/'cases/legacy-type-support/manifest.json').read_text())['phases'][plan['phase']]['canonical_working_directory']))
    assert {p.name:p.read_bytes()for p in (original/'work').iterdir()}=={p.name:p.read_bytes()for p in work.iterdir()},'complete literal owned files differ'
    return dict(status='pass',kind='direct literal body/ABI/output'if returned else'fresh direct literal writer entry',public_reachability=False,writer=plan['accepts'],width=width,value=expected,internal_dispatch_hits=len(internal))

def validate_support_v3(original,native,plan,config):
    assert plan['unit']in(10,11,12)
    text=(original/'events').read_text();state=json.loads((native/'state.json').read_text());names=[r['name'][1:]for r in config['buffered_streams']]
    split=text.index('direct-return eax ');before=original_state(text[:split],'before',names);after=original_state(text[split:],'after',names)
    for key in FIELDS:
        if key in {'owner','name_address','type','related','based_symbol'}:assert bool(before[key])==bool(state['before'][key]),key
        else:assert before[key]==state['before'][key],key
        assert after[key]==before[key]and state['after'][key]==state['before'][key],('v3 parsed owner changed',key)
    if plan['unit']<12:assert before['kind']==13 and before['type_flags']==0x802 and state['before']['owner_name']==plan['owner']=='SUPPORT_DATA'
    else:assert before['kind']==7 and state['before']['owner_name']==plan['owner']=='memcpy'
    depths=[int(v)for v in re.findall(r'^support-type-depth (?:before|after) (\d+)$',text,re.M)]
    assert len(depths)==2 and depths[0]==depths[1]==state['before']['type_description_depth']==state['after']['type_description_depth']
    assert before['buffers']==after['buffers']==state['before']['buffers']==state['after']['buffers']
    start=re.search(r'^direct-start target (\w+) marker (\w+) stack (\w+) count (\d+) args (\w+) (\w+)$',text,re.M);assert start
    target=next(r for r in config['driver']if r['name']==plan['function']);assert int(start[1],16)==target['start']and int(start[4])==plan['arguments'] and int(start[5],16)==(172 if plan['unit']==12 else before['owner'])
    assert state['arguments']==state['formals']and state['arguments'][0]==(172 if plan['unit']==12 else state['before']['owner'])
    ret=re.search(r'^direct-return eax (\w+) edx (\w+) esp (\w+) expected (\w+) nonvolatile (\d+) fctrl (\w+) before (\w+)$',text,re.M);assert ret and ret[3]==ret[4] and int(ret[3],16)==int(start[3],16)+4+4*plan['arguments'] and ret[5]=='1'and ret[6]==ret[7]
    assert re.search(r'^direct-canary (.+)$',text,re.M)[1]=='a5a5a5a5 '*3+'a5a5a5a5'
    assert state['status']=='returned'and state['canary']=='a5'*16 and state['after']['registers']['esp']==state['stack']+4+4*plan['arguments']
    for register in ['ebx','esi','edi','ebp','fctrl']:assert state['after']['registers'][register]==state['before']['registers'][register]
    if plan['unit']==12:
        observations=re.findall(r'^support-intrinsic (before|after) Oi (\d+) interned (\w+) next (\d+)$',text,re.M)
        assert len(observations)==2 and [r[0]for r in observations]==['before','after']
        a=state['before']['intrinsic'];b=state['after']['intrinsic']
        assert int(observations[0][1])==int(observations[1][1])==a['option_Oi']==b['option_Oi']==0
        assert int(observations[0][3])==int(observations[1][3])==a['next_symbol_index']==b['next_symbol_index']
        assert bool(int(observations[0][2],16))==bool(a['interned'])
        assert int(observations[1][2],16)==before['name_address']and b['interned']==state['before']['name_address']
        assert int(ret[1],16)==before['owner']and state['after']['registers']['eax']==state['before']['owner']
        for name in ['stdout','stderr']:assert(original/name).read_bytes()==(native/name).read_bytes()
        receipt=json.loads((native/'receipt.json').read_text());from case_paths import canonical_parts
        manifest=json.loads((Path(__file__).resolve().parents[1]/'cases/legacy-type-support/manifest.json').read_text());work=Path(receipt['environment']['RECON_DRIVE_S']).joinpath(*canonical_parts(manifest['phases'][plan['phase']]['canonical_working_directory']))
        assert {p.name:p.read_bytes()for p in (original/'work').iterdir()}=={p.name:p.read_bytes()for p in work.iterdir()}
        return dict(status='pass',kind='direct existing intrinsic fallback owner',public_reachability=False,operation=172,returned='actual parsed memcpy symbol',constructor_branch='Reconstructed statically and graph-audited; this direct phase exercises the existing declaration branch.',public_gate='type_requires_intrinsic_fallback returns false in original; public producer remains dormant.')
    if plan['unit']==11:
        observations=re.findall(r'^support-fixup (before|after) buffer (\w+) allocation4096 head (\w+) tail (\w+)$',text,re.M)
        assert len(observations)==2 and [r[0]for r in observations]==['before','after']
        buffer=int(observations[0][1],16);head=int(observations[1][2],16);tail=int(observations[1][3],16)
        assert buffer and int(observations[1][1],16)==buffer and observations[0][2:]==('00000000','00000000')and head==tail and head
        original_arguments=[before['owner'],buffer+8,4]
        assert [int(start[5],16),int(start[6],16),int(re.search(r'^direct-third (\w+)$',text,re.M)[1],16)]==original_arguments
        a=state['before']['fixup'];b=state['after']['fixup']
        assert a['buffer']and a['head']==a['tail']==0 and a['allocation_bytes']==b['allocation_bytes']==4096 and b['buffer']==a['buffer']and b['head']==b['tail']and b['head']
        assert state['arguments']==[state['before']['owner'],a['buffer']+8,4]
        result=re.search(r'^support-fixup-result symbol (\w+) offset (\d+) bytes (\d+) next (\w+)$',text,re.M);assert result
        assert [int(result[1],16),int(result[2]),int(result[3]),int(result[4],16)]==[before['owner'],8,4,0]
        assert state['fixup_result']==dict(symbol=state['before']['owner'],offset=8,bytes=4,next=0)
        for name in ['stdout','stderr']:assert(original/name).read_bytes()==(native/name).read_bytes()
        receipt=json.loads((native/'receipt.json').read_text());from case_paths import canonical_parts
        manifest=json.loads((Path(__file__).resolve().parents[1]/'cases/legacy-type-support/manifest.json').read_text());work=Path(receipt['environment']['RECON_DRIVE_S']).joinpath(*canonical_parts(manifest['phases'][plan['phase']]['canonical_working_directory']))
        assert {p.name:p.read_bytes()for p in (original/'work').iterdir()}=={p.name:p.read_bytes()for p in work.iterdir()}
        return dict(status='pass',kind='direct owned legacy debug fixup',public_reachability=False,offset=8,bytes=4,allocation_bytes=4096,scope='Actual parsed segment and checked_malloc4096 workspace; one real arena1 node, RET12, no record/global seeding or EAX claim for void helper.')
    observations=re.findall(r'^support-segment (before|after) next (\d+) association (\w+)$',text,re.M);assert len(observations)==2 and [x[0]for x in observations]==['before','after']
    old_index=int(observations[0][1]);new_index=int(observations[1][1]);associated=int(observations[1][2],16)
    assert not int(observations[0][2],16)and associated and new_index==old_index+1
    assert state['before']['segment']==dict(next_symbol_index=old_index,association=0)
    assert state['after']['segment']['next_symbol_index']==new_index and state['after']['segment']['association']
    result=re.search(r'^support-segment-result kind (\d+) flags (\d+) name (\S+) type (\w+) index (\d+) attributes (\w+) references (\d+) storage (\w+) global-owner (\w+)$',text,re.M);assert result
    actual=state['segment_result'];assert [int(result[1]),int(result[2]),result[3],int(result[5]),int(result[6],16),int(result[7]),result[8]]==[actual['kind'],actual['flags'],actual['name'],actual['index'],actual['attributes'],actual['references'],bytes(actual['storage']).hex()]==[9,33,'__seg__SUPPORT_DATA',old_index,0x8004,1,'428000']
    assert int(result[4],16)==before['type']and actual['type']==state['before']['type']
    assert int(result[9],16)==associated and actual['global_owner']==state['after']['segment']['association']
    for name in ['stdout','stderr']:assert(original/name).read_bytes()==(native/name).read_bytes()
    receipt=json.loads((native/'receipt.json').read_text());from case_paths import canonical_parts
    manifest=json.loads((Path(__file__).resolve().parents[1]/'cases/legacy-type-support/manifest.json').read_text());work=Path(receipt['environment']['RECON_DRIVE_S']).joinpath(*canonical_parts(manifest['phases'][plan['phase']]['canonical_working_directory']))
    assert {p.name:p.read_bytes()for p in (original/'work').iterdir()}=={p.name:p.read_bytes()for p in work.iterdir()}
    return dict(status='pass',kind='direct segment materialization/owned interned name',public_reachability=False,created_name=actual['name'],index=old_index,defined_result='Actual zeroed permanent allocation, parsed segment type and global-scope ownership; no EAX claim for void parent.')

def validate(original,native,plan,config):
    original=Path(original);native=Path(native)
    if plan['unit']>=10:return validate_support_v3(original,native,plan,config)
    if plan['unit']>=8:return validate_literal(original,native,plan,config)
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
    args=arguments(plan,before)
    assert [int(x,16)for x in start.groups()[4:4+len(args)]]==args
    native_args=arguments(plan,state['before'])
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
    defined=result_contract(text,state,before,plan,int(ret[1],16))
    for name in ('stdout','stderr'):
        assert (original/name).read_bytes()==(native/name).read_bytes()
    receipt=json.loads((native/'receipt.json').read_text())
    work=Path(receipt['environment']['RECON_DRIVE_S'])/'vc1.0/c1/legacy-type-support'
    assert {p.name:p.read_bytes()for p in (original/'work').iterdir()}=={p.name:p.read_bytes()for p in work.iterdir()},'complete owned files differ'
    return dict(status='pass',kind='direct body/ABI/state',public_reachability=False,defined_result=defined,original_before=before,original_after=after)
