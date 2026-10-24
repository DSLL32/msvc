"""Direct vector support contracts over the genuinely parsed forced headers."""
import json
from pathlib import Path
import re

def vector(text,boundary):
    match=re.search(r'^container-vector '+boundary+r' address (\w+) count (\d+) capacity (\d+) arena (-?\d+) items (\w+)$',text,re.M)
    assert match,'missing actual vector observation: '+boundary
    values=re.findall(r'^container-item '+boundary+r' (\d+) address (\w+) name (.+)$',text,re.M)
    assert [int(row[0])for row in values]==[0,1,2]
    return dict(address=int(match[1],16),count=int(match[2]),capacity=int(match[3]),arena=int(match[4]),items=int(match[5],16),values=[int(row[1],16)for row in values],names=[row[2]for row in values])

def buffers(text,config):
    names=[row['name'][1:]for row in config['buffered_streams']]
    return {names[int(index)]:None if count=='unopened'else value for index,count,value in re.findall(r'^buffer (\d+) (unopened|\d+)(?: ([0-9a-f]*))?$',text,re.M)}

def type_state(text,boundary):
    match=re.search(r'^container-type '+boundary+r' address (\w+) flags (\w+) attributes (\w+) related (\w+) depth (\d+)$',text,re.M);assert match
    result={name:int(value,10 if name=='depth'else 16)for name,value in zip(['address','flags','attributes','related','depth'],match.groups())}
    result['shapes']=[dict(address=int(address,16),flags=int(flags,16),attributes=int(attributes,16),related=int(related,16))for index,address,flags,attributes,related in re.findall(r'^container-shape '+boundary+r' (\d+) address (\w+) flags (\w+) attributes (\w+) related (\w+)$',text,re.M)]
    return result

def validate(original,native,plan,config):
    if plan['unit']>=17:return validate_v3(original,native,plan,config)
    if plan['unit']>=8:return validate_v2(original,native,plan,config)
    text=(original/'events').read_text();state=json.loads((native/'state.json').read_text())
    before=vector(text,'before');actual=state['before']['vector']
    for item in [before,actual]:
        assert (item['count'],item['capacity'],item['arena'])==(3,40,-1)
        assert item['names']==['forced-a.h','forced-b.h','forced-c.h']
        assert item['address']and item['items']and all(item['values'])
    start=re.search(r'^direct-start target (\w+) marker (\w+) stack (\w+) count (\d+) args (\w+) (\w+)$',text,re.M);assert start
    target=next(row for row in config['driver']if row['name']==plan['function'])
    assert int(start[1],16)==target['start']and int(start[4])==plan['arguments']
    unit=plan['unit']
    def arguments(value):return {0:[value['address'],1],1:[value['address'],1],2:[value['address'],value['values'][2]],3:[value['address']],4:[value['address'],value['address']],5:[value['address']],6:[value['values'][0],value['values'][2]]}[unit]
    if unit==7:
        before_type=type_state(text,'before');actual_type=state['before']['type']
        for name in ['flags','attributes','depth']:assert before_type[name]==actual_type[name]
        assert len(before_type['shapes'])==len(actual_type['shapes'])==1
        for name in ['flags','attributes']:assert before_type['shapes'][0][name]==actual_type['shapes'][0][name]
        assert [int(start[i],16)for i in [5,6]]==[2,before_type['address']]
        assert state['arguments']==state['formals']==[2,actual_type['address']]
    else:
        assert [int(start[i],16)for i in range(5,5+plan['arguments'])]==arguments(before)
        assert state['arguments']==state['formals']==arguments(actual)
    returned='direct-return eax 'in text
    first=text[:text.index('direct-return eax ')]if returned else text
    assert buffers(first,config)==state['before']['buffers'],'pending compiler outputs differ before direct call'
    if returned:
        after=vector(text,'after')
        assert after==before and state['after']['vector']==actual
        ret=re.search(r'^direct-return eax (\w+) edx (\w+) esp (\w+) expected (\w+) nonvolatile (\d+) fctrl (\w+) before (\w+)$',text,re.M);assert ret
        if unit<2:
            result=vector(text,'result')
            for source,copy in [(before,result),(actual,state['result'])]:
                assert (copy['count'],copy['capacity'],copy['arena'])==(3,3+plan['capacity_extra'],1)
                assert copy['address']!=source['address']and copy['items']!=source['items']
                assert copy['values']==source['values']and copy['names']==source['names'],'clone must borrow the original initialized item pointers'
            assert int(ret[1],16)==result['address']
        elif unit==6:
            name=re.search(r'^container-name result address (\w+) text (.+)$',text,re.M);assert name
            assert name[2]==state['name_result']==before['names'][0]+before['names'][2]
            assert int(name[1],16)==int(ret[1],16)and int(ret[1],16)not in before['values']and state['after']['registers']['eax']not in actual['values']
        elif unit==7:
            result=type_state(text,'result');native_result=state['type_result']
            assert type_state(text,'after')==before_type and state['after']['type']==actual_type
            assert int(ret[1],16)==result['address']and state['after']['registers']['eax']==native_result['address']
            for name in ['flags','attributes','depth']:assert result[name]==native_result[name]
            for source,copy in [(before_type,result),(actual_type,native_result)]:
                assert copy['address']!=source['address']and copy['flags']==source['flags']and copy['attributes']&7==2
                assert copy['depth']==source['depth']and len(copy['shapes'])==2
                assert copy['related']==copy['shapes'][0]['address']and copy['shapes'][0]['flags']==1
                assert copy['shapes'][0]['related']==source['related']and copy['shapes'][1:]==source['shapes']
            assert result['shapes'][0]['attributes']==native_result['shapes'][0]['attributes']
        elif unit==3:assert int(ret[1],16)==before['values'][0]and state['after']['registers']['eax']==actual['values'][0]
        else:assert int(ret[1],16)==state['after']['registers']['eax']==1
        assert ret[3]==ret[4]and int(ret[3],16)==int(start[3],16)+4+4*plan['arguments']
        assert ret[5]=='1'and ret[6]==ret[7]
        assert re.search(r'^direct-canary (.+)$',text,re.M)[1]=='a5a5a5a5 '*3+'a5a5a5a5'
        assert state['status']=='returned'and state['canary']=='a5'*16
        assert state['after']['registers']['esp']==state['stack']+4+4*plan['arguments']
        for register in ['ebx','esi','edi','ebp','fctrl']:assert state['after']['registers'][register]==state['before']['registers'][register]
        assert buffers(text[text.index('direct-return eax '):],config)==state['after']['buffers']==state['before']['buffers']
    else:
        exception=re.search(r'^exception 80000003 rva (\w+) base (\w+)',text,re.M);assert exception
        reached=int(exception[1],16)+int(exception[2],16)
        expected=target['start']if state['status']=='entry'else next(row['start']for row in config['driver']if row['name']=='_clone_vector_with_extra@12')
        assert reached==expected
        if state['status']=='callee-entry':
            assert state['frontier']['function']=='clone_vector_with_extra'and state['frontier']['argument']==actual['address']
            assert state['frontier']['context']['buffers']==state['before']['buffers']
            slots={int(index):int(value,16)for index,value in re.findall(r'^stack (\d+) (\w+)$',text[text.index('exception 80000003'):],re.M)}
            assert [slots[index]for index in [1,2,3]]==[before['address'],0,1]
            assert state['frontier']['arguments']==[actual['address'],0,1]
        else:assert state['status']=='entry'
    for name in ['stdout','stderr']:assert (original/name).read_bytes()==(native/name).read_bytes()
    from case_paths import canonical_parts
    manifest=json.loads((Path(__file__).resolve().parents[1]/'cases/forced-container-support/manifest.json').read_text())
    receipt=json.loads((native/'receipt.json').read_text());work=Path(receipt['environment']['RECON_DRIVE_S']).joinpath(*canonical_parts(manifest['phases'][plan['phase']]['canonical_working_directory']))
    assert {p.name:p.read_bytes()for p in(original/'work').iterdir()}=={p.name:p.read_bytes()for p in work.iterdir()},'complete direct owned files differ'
    return dict(status='pass',kind='direct vector clone body/ABI/output'if returned else'fresh direct vector helper entry',public_reachability=False)

def owned_state(text,boundary,unit):
    if unit in(8,9):
        m=re.search(r'^container-iterator '+boundary+r' address (\w+) vector (\w+) index (-?\d+)$',text,re.M);assert m
        return dict(address=int(m[1],16),vector=int(m[2],16),index=int(m[3]))
    if unit in(10,11,12):
        if unit==12 and boundary=='after':
            m=re.search(r'^container-released address (\w+)$',text,re.M);assert m;return dict(released=int(m[1],16))
        rows=re.findall(r'^container-bitset '+boundary+r' (\d+) address (\w+) count (\d+) bytes (\w+)$',text,re.M)
        assert [int(row[0])for row in rows]==list(range(2 if unit==11 else 1))
        return [dict(address=int(address,16),count=int(count),bytes=data)for index,address,count,data in rows]
    assert unit==13
    m=re.search(r'^container-flags '+boundary+r' address (\w+) value (\w+) initial (\w+)$',text,re.M);assert m
    return dict(address=int(m[1],16),value=int(m[2],16),initial=int(m[3],16))

def validate_v2(original,native,plan,config):
    unit=plan['unit'];text=(original/'events').read_text();state=json.loads((native/'state.json').read_text())
    before=vector(text,'before');actual=state['before']['vector']
    for item in [before,actual]:
        assert (item['count'],item['capacity'],item['arena'])==(3,40,-1)
        assert item['names']==['forced-a.h','forced-b.h','forced-c.h']
    owned=actual_owned=None
    if unit<=13:
        owned=owned_state(text,'before',unit);actual_owned=state['before']['owned']
        starts=re.findall(r'^container-constructor-start stage (\d+) target (\w+) argument (\d+) stack (\w+)$',text,re.M)
        returns=re.findall(r'^container-constructor-return stage (\d+) address (\w+) esp (\w+) expected (\w+) nonvolatile 1 fctrl (\w+) before (\w+) canary a5x16$',text,re.M)
        constructors=state['constructors'];count=2 if unit==11 else 1
        assert len(starts)==len(returns)==len(constructors)==count
        for index,(start,ret,constructor)in enumerate(zip(starts,returns,constructors)):
            name='allocate_permanent'if unit in(8,9,13)else'new_bitset';argument=4 if unit==13 else 8 if unit<10 else 31
            target=next(row['start']for row in config['driver']if row['name']=='_'+name+'@4')
            assert int(start[0])==int(ret[0])==index+1 and int(start[1],16)==target and int(start[2])==argument
            assert ret[2]==ret[3]and int(ret[2],16)==int(start[3],16)+8 and ret[4]==ret[5]
            assert constructor['function']==name and constructor['argument']==argument
            assert constructor['initialized_bytes']==('00'*argument if name=='allocate_permanent'else'1f00000000000000ff')
            original_address=owned[index]['address']if unit in(10,11,12)else owned['address']
            native_address=actual_owned[index]['address']if unit in(10,11,12)else actual_owned['address']
            assert int(ret[1],16)==original_address and constructor['address']==native_address
        if unit in(8,9):
            assert owned['vector']==before['address']and actual_owned['vector']==actual['address']and owned['index']==actual_owned['index']==1
        elif unit in(10,11,12):
            for index,(left,right)in enumerate(zip(owned,actual_owned)):
                assert left['count']==right['count']==31 and left['bytes']==right['bytes']==('5aa50f40ff'if index==0 else'0ff03320ff')
        else:assert owned['value']==owned['initial']==actual_owned['value']==actual_owned['initial']
    def arguments(value,record):
        if unit in(8,9):return [record['address'],value['values'][2]]if unit==8 else[record['address']]
        if unit in(10,11,12):return [row['address']for row in record]
        if unit==13:return [record['address'],0xf37f9]
        return {14:[1],15:[value['address']],16:[]}[unit]
    start=re.search(r'^direct-start target (\w+) marker (\w+) stack (\w+) count (\d+) args (\w+) (\w+)$',text,re.M);assert start
    target=next(row for row in config['driver']if row['name']==plan['function'])
    assert int(start[1],16)==target['start']and int(start[4])==plan['arguments']
    assert [int(start[i],16)for i in range(5,5+plan['arguments'])]==arguments(before,owned)
    assert state['arguments']==state['formals']==arguments(actual,actual_owned)
    returned='direct-return eax 'in text
    first=text[:text.index('direct-return eax ')]if returned else text
    assert buffers(first,config)==state['before']['buffers']
    if returned:
        after=vector(text,'after');native_after=state['after']['vector']
        for source,changed in [(before,after),(actual,native_after)]:
            expected=dict(source)
            if unit==8:expected.update(values=[source['values'][0],source['values'][2],source['values'][2]],names=['forced-a.h','forced-c.h','forced-c.h'])
            assert changed==expected,'unexpected parsed vector mutation'
        if unit<=13:
            after_owned=owned_state(text,'after',unit);native_owned=state['after']['owned']
            if unit==11:
                for source,changed in [(owned,after_owned),(actual_owned,native_owned)]:
                    assert changed[0]==dict(source[0],bytes='50050c40ff')and changed[1]==source[1]
            elif unit==12:
                assert after_owned==dict(released=owned[0]['address'])and native_owned==dict(released=actual_owned[0]['address'])
            elif unit==13:
                assert after_owned==dict(owned,value=owned['initial']|0xf37f8)and native_owned==dict(actual_owned,value=actual_owned['initial']|0xf37f8)
            else:assert after_owned==owned and native_owned==actual_owned
        ret=re.search(r'^direct-return eax (\w+) edx (\w+) esp (\w+) expected (\w+) nonvolatile (\d+) fctrl (\w+) before (\w+)$',text,re.M);assert ret
        eax=int(ret[1],16);native_eax=state['after']['registers']['eax'];cleanup=0 if plan.get('convention')=='cdecl'else plan['arguments']
        assert ret[3]==ret[4]and int(ret[3],16)==int(start[3],16)+4+4*cleanup and ret[5]=='1'and ret[6]==ret[7]
        assert re.search(r'^direct-canary (.+)$',text,re.M)[1]=='a5a5a5a5 '*3+'a5a5a5a5'
        assert state['status']=='returned'and state['canary']=='a5'*16 and state['after']['registers']['esp']==state['stack']+4+4*cleanup
        for register in ['ebx','esi','edi','ebp','fctrl']:assert state['after']['registers'][register]==state['before']['registers'][register]
        if unit in(9,13):assert eax==native_eax==1
        if unit==10:
            result=owned_state(text,'result',unit)[0];native_result=state['bitset_result']
            assert result==dict(owned[0],address=eax)and native_result==dict(actual_owned[0],address=native_eax)
            assert eax!=owned[0]['address']and native_eax!=actual_owned[0]['address']
        if unit==14:
            m=re.search(r'^container-empty result address (\w+) count (-?\d+) capacity (-?\d+) arena (-?\d+)$',text,re.M);assert m
            assert int(m[1],16)==eax and [int(m[i])for i in [2,3,4]]==[0,0,1]
            assert state['empty_result']==dict(count=0,capacity=0,arena=1)
        if unit==15:assert eax==before['address']&0xfff and native_eax==actual['address']&0xfff
        final_buffers=buffers(text[text.index('direct-return eax '):],config)
        assert final_buffers==state['after']['buffers']
        if unit!=16:assert final_buffers==state['before']['buffers']
        else:assert final_buffers['browser_output']!=state['before']['buffers']['browser_output']
    else:
        m=re.search(r'^exception 80000003 rva (\w+) base (\w+)',text,re.M);assert m
        assert int(m[1],16)+int(m[2],16)==target['start']and state['status']=='entry'
    for name in ['stdout','stderr']:assert(original/name).read_bytes()==(native/name).read_bytes()
    from case_paths import canonical_parts
    phase=json.loads((Path(__file__).resolve().parents[1]/'cases/forced-container-support/manifest.json').read_text())['phases'][plan['phase']]
    receipt=json.loads((native/'receipt.json').read_text());work=Path(receipt['environment']['RECON_DRIVE_S']).joinpath(*canonical_parts(phase['canonical_working_directory']))
    assert {p.name:p.read_bytes()for p in(original/'work').iterdir()}=={p.name:p.read_bytes()for p in work.iterdir()},'complete direct owned files differ'
    return dict(status='pass',kind='direct constructor-owned container body/ABI/output'if returned else'fresh direct constructor-owned entry',public_reachability=False,unit=unit)

def typed_state(text,boundary):
    m=re.search(r'^typed-state '+boundary+r' errors (\d+) counter (\d+) override (\d+) parameters (\w+)$',text,re.M);assert m
    return dict(errors=int(m[1]),counter=int(m[2]),override=int(m[3]),parameters=int(m[4],16))

def typed_ir(text,label):
    m=re.search(r'^typed-ir '+label+r' address (\w+) kind (\d+) flags (\d+) type (\w+) typeflags (\w+) value (\w+) right (\w+)$',text,re.M);assert m
    return {key:int(value,10 if key in('kind','flags')else 16)for key,value in zip(['address','kind','flags','type','typeflags','value','right'],m.groups())}

def typed_owners(text):
    m=re.search(r'^typed-owners symbol (\w+) type (\w+) callback (\w+) callbacktype (\w+) shape (\w+) sinktype (\w+)$',text,re.M);assert m
    return dict(zip(['symbol','type','callback','callbacktype','shape','sinktype'],[int(v,16)for v in m.groups()]))

def typed_aggregate(text,boundary):
    m=re.search(r'^typed-aggregate '+boundary+r' context (\w+) tag (\w+) last (\w+) type (\w+) size (\d+) bitfield (\d+) alignment (\d+) used (\d+) flags (\d+) member (\w+)$',text,re.M);assert m
    keys=['context','tag','last','type','size','bitfield','alignment','used','flags','member']
    result={k:int(v,10 if k in('size','bitfield','alignment','used','flags')else 16)for k,v in zip(keys,m.groups())}
    m=re.search(r'^typed-member '+boundary+r' type (\w+) flags (\w+)$',text,re.M);assert m
    result.update(member_type=int(m[1],16),member_flags=int(m[2],16));return result

def validate_v3(original,native,plan,config):
    # Extend semantic checks before accepting each subsequent unit.
    unit=plan['unit'];assert 17<=unit<=30
    text=(original/'events').read_text();state=json.loads((native/'state.json').read_text())
    source=vector(text,'before');actual=state['before']['vector']
    for value in(source,actual):assert(value['count'],value['capacity'],value['arena'],value['names'])==(3,40,-1,['forced-a.h','forced-b.h','forced-c.h'])
    aggregate=27<=unit<=29
    owners=native_owners={}
    if aggregate:
        aggregate_before=typed_aggregate(text,'before');native_aggregate=state['before']['typed']['aggregate']
        for key in('size','bitfield','alignment','used','flags','member_flags'):assert aggregate_before[key]==native_aggregate[key]
        assert re.search(r'^typed-name aggregate address \w+ text ContainerItem$',text,re.M)
    else:
        owners=typed_owners(text);native_owners=state['before']['typed']['owners'];assert all(owners.values())and all(native_owners.values())
        if unit in(24,25):
            for key,label in [('name','function'),('detail','callback')]:
                m=re.search(r'^typed-name '+label+r' address (\w+) text (.+)$',text,re.M);assert m;owners[key]=int(m[1],16)
            if unit==24:
                m=re.search(r'^typed-output stream (\w+)$',text,re.M);assert m;owners['stream']=int(m[1],16)

    constructors=[]
    if unit in(18,22,23):
        count=2 if unit==18 else 1
        starts=re.findall(r'^container-constructor-start stage (\d+) target (\w+) argument (\d+) stack (\w+)$',text,re.M)
        returns=re.findall(r'^container-constructor-return stage (\d+) address (\w+) esp (\w+) expected (\w+) nonvolatile 1 fctrl (\w+) before (\w+) canary a5x16$',text,re.M)
        assert len(starts)==len(returns)==len(state['constructors'])==count
        for index,(start,ret,constructor)in enumerate(zip(starts,returns,state['constructors'])):
            unsigned=(unit==23 or unit==18 and index==1);argument=11 if unsigned else 37;name='make_unsigned_integer_constant'if unsigned else'make_integer_constant'
            target=next(r['start']for r in config['driver']if r['name']=='_'+name+'@4')
            assert int(start[0])==int(ret[0])==index+1 and int(start[1],16)==target and int(start[2])==argument
            assert ret[2]==ret[3]and int(ret[2],16)==int(start[3],16)+8 and ret[4]==ret[5]
            value=typed_ir(text,'constructor'+str(index+1));assert value['address']==int(ret[1],16)
            assert constructor['function']==name and constructor['argument']==argument and constructor['address']==constructor['ir']['address']
            for ir in(value,constructor['ir']):assert{key:ir[key]for key in('kind','flags','typeflags','value','right')}==dict(kind=7,flags=3,typeflags=0x4064 if unsigned else 0x64,value=argument,right=0)
            constructors.append(value)
    def arguments(owned,created,context):
        if unit==24:return[owned['symbol'],owned['stream']]
        if unit==25:return[248,owned['symbol'],0,owned['name'],owned['detail'],owned['sinktype'],owned['callbacktype']]
        if unit in(26,27,28):return[]
        if unit==29:return[context['member_type']]
        if unit==30:return[owned['symbol']]
        return [37]if unit==17 else[80,created[0]['address'],created[1]['address']]if unit==18 else[owned['shape']]if unit in(19,21)else[owned['type']]if unit==20 else[owned['sinktype'],created[0]['address']]
    start=re.search(r'^direct-start target (\w+) marker (\w+) stack (\w+) count (\d+) args (\w+) (\w+)$',text,re.M);assert start
    target=next(row for row in config['driver']if row['name']==plan['function'])
    assert int(start[1],16)==target['start']and int(start[4])==plan['arguments']
    original_args=[int(start[i],16)for i in range(5,5+min(2,plan['arguments']))]
    if plan['arguments']>=3:
        third=re.search(r'^direct-third (\w+)$',text,re.M);assert third;original_args.append(int(third[1],16))
    if plan['arguments']>3:
        rest=re.findall(r'^direct-argument (\d+) (\w+)$',text,re.M);assert [int(i)for i,v in rest]==list(range(4,plan['arguments']+1));original_args.extend(int(v,16)for i,v in rest)
    assert original_args==arguments(owners,constructors,aggregate_before if aggregate else None)
    native_created=[c['ir']for c in state.get('constructors',[])]
    assert state['arguments']==state['formals']==arguments(native_owners,native_created,native_aggregate if aggregate else None)
    before=typed_state(text,'before');native_before={k:state['before']['typed'][k]for k in before};assert before==native_before
    if unit in(19,21):
        before_type=type_state(text,'before');native_type=state['before']['typed']['type']
        assert len(before_type['shapes'])==len(native_type['shapes'])==2
        assert before_type['address']==owners['callbacktype']and native_type['address']==native_owners['callbacktype']
        for left,right in zip(before_type['shapes'],native_type['shapes']):
            for key in('flags','attributes'):assert left[key]==right[key]
        for key in('flags','attributes','depth'):assert before_type[key]==native_type[key]
    returned='direct-return eax 'in text
    assert buffers(text.split('direct-return eax ')[0],config)==state['before']['buffers']
    if returned:
        assert vector(text,'after')==source and state['after']['vector']==actual
        expected=dict(before)
        if unit==25:expected['errors']+=1
        if unit==26:expected['counter']=(expected['counter']-1)&0xffffffff
        assert typed_state(text,'after')==expected and{k:state['after']['typed'][k]for k in before}==expected
        ret=re.search(r'^direct-return eax (\w+) edx (\w+) esp (\w+) expected (\w+) nonvolatile (\d+) fctrl (\w+) before (\w+)$',text,re.M);assert ret
        cleanup=0 if plan.get('convention')=='cdecl'else plan['arguments']
        assert ret[3]==ret[4]and int(ret[3],16)==int(start[3],16)+4+4*cleanup and ret[5]=='1'and ret[6]==ret[7]
        assert re.search(r'^direct-canary (.+)$',text,re.M)[1]=='a5a5a5a5 '*3+'a5a5a5a5'
        assert state['status']=='returned'and state['canary']=='a5'*16 and state['after']['registers']['esp']==state['stack']+4+4*cleanup
        for register in ['ebx','esi','edi','ebp','fctrl']:assert state['after']['registers'][register]==state['before']['registers'][register]
        eax=int(ret[1],16);native_eax=state['after']['registers']['eax']
        if unit in(17,18):
            result=typed_ir(text,'result');native_result=state['typed_result']
            assert result['address']==eax and native_result['address']==native_eax
            for key in('kind','flags','typeflags'):assert result[key]==native_result[key]
            if unit==17:
                for value in(result,native_result):assert{key:value[key]for key in('kind','flags','typeflags','value','right')}==dict(kind=7,flags=3,typeflags=0x4064,value=37,right=0)
            else:
                for value,created in[(result,constructors),(native_result,native_created)]:assert value['kind']==80 and value['value']==created[0]['address']and value['right']==created[1]['address']and value['type']==created[1]['type']
        elif unit in(19,21):
            assert type_state(text,'after')==before_type and state['after']['typed']['type']==native_type
            assert eax==(before_type['shapes'][-1]['address']if unit==19 else owners['shape'])
            assert native_eax==(native_type['shapes'][-1]['address']if unit==19 else native_owners['shape'])
        elif unit==20:assert eax==owners['type']and native_eax==native_owners['type']
        elif unit in(22,23,30):
            assert eax==native_eax
            if unit==23:assert eax in(0,1)
            if unit==30:assert eax==1
        elif unit==26:assert eax==native_eax==before['counter']
        elif aggregate:
            after_aggregate=typed_aggregate(text,'after');native_after=state['after']['typed']['aggregate']
            for initial,changed in[(aggregate_before,after_aggregate),(native_aggregate,native_after)]:
                expected_context=dict(initial)
                if unit==29:
                    assert initial['bitfield']==0 and initial['used']==0 and initial['member_flags']&15==2 and initial['size']==2 and initial['alignment']==2
                    expected_context['size']+=2
                assert changed==expected_context
            if unit==27:assert eax==aggregate_before['tag']and native_eax==native_aggregate['tag']
            elif unit==28:
                m=re.search(r'^typed-name result address (\w+) text (.+)$',text,re.M);assert m
                assert int(m[1],16)==eax and m[2]==state['typed_name_result']=='ContainerItem'
            else:assert eax==native_eax==2
        # The prototype and explicit diagnostic intentionally change output.
        final_buffers=buffers(text[text.index('direct-return eax '):],config)
        assert final_buffers==state['after']['buffers']
        if unit not in(24,25):assert final_buffers==state['before']['buffers']
    else:
        m=re.search(r'^exception 80000003 rva (\w+) base (\w+)',text,re.M);assert m
        assert int(m[1],16)+int(m[2],16)==target['start']and state['status']=='entry'
    for name in ['stdout','stderr']:assert(original/name).read_bytes()==(native/name).read_bytes()
    from case_paths import canonical_parts
    phase=json.loads((Path(__file__).resolve().parents[1]/'cases/forced-container-support/manifest.json').read_text())['phases'][plan['phase']]
    receipt=json.loads((native/'receipt.json').read_text());work=Path(receipt['environment']['RECON_DRIVE_S']).joinpath(*canonical_parts(phase['canonical_working_directory']))
    assert{p.name:p.read_bytes()for p in(original/'work').iterdir()}=={p.name:p.read_bytes()for p in work.iterdir()},'complete direct owned files differ'
    return dict(status='pass',kind='direct typed helper body/ABI/output'if returned else'fresh direct typed helper entry',public_reachability=False,unit=unit)
