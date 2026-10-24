"""Offline defined-state/ABI checks for the fixed PCH metadata units."""
import json,re
from pathlib import Path
def layout(text):
    pages=[dict(start=int(start,16),bytes=0xeffc)for _,start,_,bits in re.findall(r'^metadata-page (\d+) start (\w+) bytes0000effc marks (\w+) bits (\d+)$',text,re.M)if int(bits)==0xeffc]
    blocks=[dict(address=int(address,16),data=int(data,16),allocation=int(allocation),used=int(used))for _,address,allocation,data,used,_,_ in re.findall(r'^metadata-arena-block (\d+) address (\w+) allocation (\d+) data (\w+) used (\d+) next (\w+) last (\d+)$',text,re.M)]
    assert pages and blocks
    def owner(address,size):
        if not address:return None
        for index,page in enumerate(pages):
            if page['start']<=address and address+size<=page['start']+page['bytes']:return ['permanent',index,address-page['start']]
        for index,block in enumerate(blocks):
            if block['data']<=address and address+size<=block['data']+block['used']:
                assert all(not(address<p['start']+p['bytes']and address+size>p['start'])for p in pages)
                return ['temporary',index,address-block['data']]
        raise AssertionError(('unowned metadata extent',hex(address),size))
    rows=re.findall(r'^metadata-argument (\d+) address (\w+) type (\w+) next (\w+) expression (\w+) previous (\w+) owned20 outside-all-pages$',text,re.M);assert len(rows)==3
    projected=[];addresses=[]
    for index,address,type,next,expression,previous in rows:
        assert int(index)==len(projected);values=[int(v,16)for v in[address,type,next,expression,previous]];addresses.append(values[0]);projected.append([owner(v,n)for v,n in zip(values,[20,8,20,1,20])])
    for i,row in enumerate(projected):assert row[2]==projected[(i+2)%3][0]and row[4]==(projected[i+1][0]if i<2 else None)
    return dict(pages=pages,blocks=blocks,arguments=projected,addresses=addresses)
def cache(text,when,side):
    m=re.search(r'^metadata-cache '+when+r' reference (\w+) base (\w+) page (\d+)$',text,re.M);assert m
    reference,base,page=int(m[1],16),int(m[2],16),int(m[3]);assert not base or(page<len(side['pages'])and base==side['pages'][page]['start'])
    if when=='before':
        for address in side['addresses']:
            for offset in [4,8,12,16]:assert((address+offset-base)&0xffffffff)>0xeffc
    return dict(reference=reference,base=base,page=page)
def buffers(text):
    rows=re.findall(r'^buffer (\d+) (unopened|\d+)(?: ([0-9a-f]*))?$',text,re.M);assert len(rows)>=7 and len(rows)%7==0
    groups=[]
    for start in range(0,len(rows),7):
        values={}
        for index,(slot,size,data)in enumerate(rows[start:start+7]):
            assert int(slot)==index
            if size=='unopened':values[index]=None
            else:assert 2*int(size)==len(data);values[index]=data
        groups.append(values)
    return groups
def encoded_vector(text,side):
    m=re.search(r'^metadata-encoded-vector before owner (\w+) count(\d+) capacity(\d+) arena0 items (\w+)$',text,re.M);assert m
    address,count,capacity,items=int(m[1],16),int(m[2]),int(m[3]),int(m[4],16);assert count==capacity==6
    def reference(pointer,size):
        rows=[(index,pointer-page['start'])for index,page in enumerate(side['pages'])if page['start']<=pointer and pointer+size<=page['start']+page['bytes']];assert len(rows)==1;return rows[0]
    rows=re.findall(r'^metadata-reference (\d+) original (\w+) encoded (\w+)$',text,re.M);assert len(rows)==count
    values=[]
    for index,pointer,encoded in rows:
        assert int(index)==len(values);pointer=int(pointer,16);encoded=int(encoded,16);page,offset=reference(pointer,1)
        assert encoded==(page<<16)|offset
        actual=re.search(r'^metadata-string defines-'+index+r' (\w+) ',text,re.M);assert actual and int(actual[1],16)==pointer;values.append(encoded)
    return dict(address=address,count=count,capacity=capacity,items=items,owner_reference=reference(address,12),items_reference=reference(items,4*count),values=values)
def permanent_reference(side,address,bytes):
    matches=[(i,address-page['start'])for i,page in enumerate(side['pages'])if page['start']<=address and address+bytes<=page['start']+page['bytes']];assert len(matches)==1;return matches[0]
def source_vector(text,side):
    m=re.search(r'^metadata-vector defines (\w+) count (\d+) capacity (\d+) arena (\d+) items (\w+)$',text,re.M);assert m
    address,count,capacity,arena,items=int(m[1],16),int(m[2]),int(m[3]),int(m[4]),int(m[5],16);assert count==capacity==6 and arena==0;values=[];names=[]
    for i in range(count):
        row=re.search(r'^metadata-string defines-'+str(i)+r' (\w+) (.*)$',text,re.M);assert row
        names.append(row[2]);values.append(permanent_reference(side,int(row[1],16),len(row[2])+1))
    return dict(address=address,items=items,count=count,capacity=capacity,owner_reference=permanent_reference(side,address,12),items_reference=permanent_reference(side,items,4*count),values=values,names=names)
def copy_input(text,side,vector,unit):
    m=re.search(r'^metadata-copy-input cursor (\w+) available (\d+) source (\w+) bytes(\d+) allocation(\d+)$',text,re.M);assert m
    cursor,available,source,bytes,capacity=int(m[1],16),int(m[2]),int(m[3],16),int(m[4]),int(m[5]);assert bytes==24 and capacity==(32 if unit==3 else 10)and source==vector['items']and available>=capacity+16
    return dict(cursor=cursor,available=available,source=source,bytes=bytes,capacity=capacity,cursor_reference=permanent_reference(side,cursor,capacity+16))
def pascal_input(text,side):
    m=re.search(r'^metadata-pascal-input buffer (\w+) allocation256 symbol (\w+) name (\w+) length(\d+) text (.*)$',text,re.M);assert m
    buffer,symbol,name,length,value=int(m[1],16),int(m[2],16),int(m[3],16),int(m[4]),m[5];assert length==len(value)==13 and value=='metadata_sink'
    return dict(buffer=buffer,symbol=symbol,name=name,length=length,text=value,symbol_reference=permanent_reference(side,symbol,16),name_reference=permanent_reference(side,name,length+1))
def qualifier_input(text,side,unit):
    m=re.search(r'^metadata-qualifier-input symbol (\w+) name (\w+) type (\w+) flags(\w+) attributes(\w+) related(\w+)$',text,re.M);assert m
    symbol,name,type,flags,attributes,related=int(m[1],16),m[2],int(m[3],16),int(m[4],16),int(m[5],16),int(m[6],16);assert name==('metadata_qualified'if unit==6 else'metadata_mutable')and related==0 and bool(attributes&0x3000)==(unit==6)
    return dict(symbol=symbol,name=name,type=type,flags=flags,attributes=attributes,related=related,symbol_reference=permanent_reference(side,symbol,16),type_reference=permanent_reference(side,type,8))
def browser_input(text):
    m=re.search(r'^metadata-browser-input stream (\w+) pointer (\w+) base (\w+) count (\d+) flags (\w+) pending(\d+) suppressed0 template-zero24 hook-known-empty1$',text,re.M);assert m
    stream,pointer,base,count,flags,pending=int(m[1],16),int(m[2],16),int(m[3],16),int(m[4]),int(m[5],16),int(m[6]);assert stream and base and pointer-base==pending and 0<=pending<=4094 and count>=2 and flags&2
    return dict(stream=stream,pointer=pointer,base=base,count=count,flags=flags,pending=pending)
def format_input(text,side):
    setup=re.search(r'^metadata-format-setup slot (\w+) initial-null1 format (\w+) output (\w+) allocation256 original-work (\w+)$',text,re.M);assert setup
    m=re.search(r'^metadata-format-input buffer (\w+) owner16 storage (\w+) block2048 format (\w+) name (\w+) number(\d+) count0 trailer-zero8 original-work-distinct1$',text,re.M);assert m
    buffer,storage,format,name,number=int(m[1],16),int(m[2],16),int(m[3],16),int(m[4],16),int(m[5]);assert buffer and storage and buffer!=int(setup[4],16)and format==int(setup[2],16)and number==6
    assert buffer+16<=storage or storage+2048<=buffer
    return dict(buffer=buffer,storage=storage,format=format,name=name,number=number,name_reference=permanent_reference(side,name,14))
def validate(original,native,plan,entry=False,frontier=None):
    original=Path(original);native=Path(native);ot=(original/'events').read_text();nt=(native/'report').read_text();unit=plan['unit'];assert 0<=unit<=9;assert frontier is None or(unit==3 and entry and frontier=='minimum')
    left,right=layout(ot),layout(nt);assert left['arguments']==right['arguments']
    assert[(b['allocation'],b['used'])for b in left['blocks']]==[(b['allocation'],b['used'])for b in right['blocks']]
    assert len(left['pages'])==len(right['pages'])
    first,last=cache(ot,'before',left),cache(nt,'before',right)
    assert first['reference']==last['reference']==0 and first['base']==last['base']==0
    assert first['page']==65535 and last['page']==0
    assert'metadata-cache-path before lookup-for-all-nonnull-slots'in nt
    vectors=[];copies=[];pascal=[];qualifiers=[];browser=[];formats=[]
    if unit>=1:
        vectors=[source_vector(text,side)for text,side in[(ot,left),(nt,right)]]
        for name in['count','capacity','owner_reference','items_reference','values','names']:assert vectors[0][name]==vectors[1][name]
    if unit in[3,4]:
        copies=[copy_input(text,side,vector,unit)for text,side,vector in[(ot,left,vectors[0]),(nt,right,vectors[1])]]
        for name in['available','bytes','capacity','cursor_reference']:assert copies[0][name]==copies[1][name]
    if unit==5:
        pascal=[pascal_input(text,side)for text,side in[(ot,left),(nt,right)]]
        for name in['length','text','symbol_reference','name_reference']:assert pascal[0][name]==pascal[1][name]
    if unit in[6,7]:
        qualifiers=[qualifier_input(text,side,unit)for text,side in[(ot,left),(nt,right)]]
        for name in['name','flags','attributes','related','symbol_reference','type_reference']:assert qualifiers[0][name]==qualifiers[1][name]
    if unit==8:
        browser=[browser_input(text)for text in[ot,nt]]
        for name in['count','flags','pending']:assert browser[0][name]==browser[1][name]
    if unit==9:
        formats=[format_input(text,side)for text,side in[(ot,left),(nt,right)]];assert formats[0]['name_reference']==formats[1]['name_reference']and formats[0]['number']==formats[1]['number']==vectors[0]['count']
        starts=re.findall(r'^metadata-setup-start stage([4-7]) index\d+ target (\w+) ',ot,re.M)
        expected=[(4,0x29cf0)]if entry else[(4,0x29cf0),(5,0x29eb0),(6,0x29cd0),(7,0x29e00)]
        assert[(int(stage),int(target,16))for stage,target in starts]==expected
        returns=re.findall(r'^metadata-setup-return stage[4-7] index\d+ eax \w+ esp \w+ nonvolatile1 canary-a5x16$',ot,re.M);assert len(returns)==len(expected)
    constructed=[]
    if unit==2:
        constructed=[encoded_vector(text,side)for text,side in[(ot,left),(nt,right)]]
        for name in['count','capacity','owner_reference','items_reference','values']:assert constructed[0][name]==constructed[1][name]
        prepared=[cache(text,'prepared',side)for text,side in[(ot,left),(nt,right)]]
        assert prepared[0]['reference']==prepared[1]['reference']==constructed[0]['values'][-1]and prepared[0]['page']==prepared[1]['page']
        starts=re.findall(r'^metadata-setup-start stage([123]) index\d+ target (\w+) stack \w+ args[12] ',ot,re.M)
        assert[(int(stage),int(target,16))for stage,target in starts]==[(1,0x324c0)]+[(stage,target)for _ in range(6)for stage,target in[(2,0x445f0),(3,0x32780)]]
        returns=re.findall(r'^metadata-setup-return stage[123] index\d+ eax \w+ esp \w+ nonvolatile1 canary-a5x16$',ot,re.M);assert len(returns)==13
    state=json.loads((native/'state.json').read_text())if(native/'state.json').exists()else None
    if entry:
        assert state and state['status']=='entry'and len(state['calls'])==1
        match=re.search(r'^exception 80000003 rva (\w+) base (\w+) first 1$',ot,re.M);assert match and int(match[1],16)+int(match[2],16)==(0x44b80 if frontier else plan['start'])
        slots={int(i):int(v,16)for i,v in re.findall(r'^stack (\d+) (\w+)$',ot[match.end():],re.M)};assert slots[0]==(0x44c1a if frontier else 0x2c220)
        if unit in[3,4]:
            expected=[[c['capacity'],c['bytes']]if frontier else[c['source'],c['bytes'],c['capacity']]for c in copies]
            assert[slots[i+1]for i in range(len(expected[0]))]==expected[0]and state['calls'][0]['frame'][1:]==expected[1]
        elif unit==5:
            assert[slots[1],slots[2]]==[pascal[0]['buffer'],pascal[0]['symbol']]and state['calls'][0]['frame'][1:]==[pascal[1]['buffer'],pascal[1]['symbol']]
        elif unit in[6,7]:assert slots[1]==qualifiers[0]['symbol']and state['calls'][0]['frame'][1:]==[qualifiers[1]['symbol']]
        elif unit==8:assert state['calls'][0]['frame'][1:]==[]
        elif unit==9:
            args=[[f['buffer'],f['format'],f['name'],f['number']]for f in formats]
            assert[slots[i+1]for i in range(4)]==args[0]and state['calls'][0]['frame'][1:]==args[1]
        else:
            expected=[left['addresses'][0],right['addresses'][0]]if unit==0 else[c['address']for c in constructed]if unit==2 else[int(re.search(r'^metadata-vector defines (\w+) ',text,re.M)[1],16)for text in[ot,nt]]
            assert slots[1]==expected[0]and state['calls'][0]['frame'][1]==expected[1]
    else:
        expected='metadata-after nodes-unchanged3 page-marks-unchanged'+str(len(left['pages']))+' no-recursion'
        assert expected in ot and expected in nt
        final=[cache(text,'after',side)for text,side in[(ot,left),(nt,right)]]
        if unit in[3,4,5,6,7,8,9]:
            assert final==[first,last]
            if unit in[3,4]:
                for text,c in zip([ot,nt],copies):
                    copied=min(c['bytes'],c['capacity']);assert'metadata-copy-after returned-owner1 copied'+str(copied)+' source-prefix-equal1 tail-guard-unchanged'+str(c['capacity']+16-copied)+' cursor-advance'+str(c['capacity'])+' available-drop'+str(c['capacity'])in text
                if state:assert len(state['nested_calls'])==1 and'returned_registers'in state['nested_calls'][0]
            elif unit==5:
                for text in[ot,nt]:assert'metadata-pascal-after ax000d prefix-count13 text metadata_sink nul1 tail-a5241'in text
            elif unit in[6,7]:
                for text in[ot,nt]:assert'metadata-qualifier-after eax'+str(int(unit==6))+' type-unchanged8'in text
            elif unit==8:
                for text,before,page_word,observations in[(ot,browser[0],65535,2),(nt,browser[1],0,1)]:
                    for call in[1,2]:assert'metadata-browser-after call'+str(call)+' suppressed1 appended'+('0e'if call==1 else'none')+' pending'+str(before['pending']+1)+' template-unchanged24 aux-browser1'in text
                    raw_after=re.findall(r'^metadata-cache after reference (\w+) base (\w+) page (\d+)$',text,re.M)
                    assert len(raw_after)==observations and all((int(r,16),int(b,16),int(p))==(0,0,page_word)for r,b,p in raw_after)
            else:
                for text in[ot,nt]:
                    assert'metadata-format-result eax15 count15 next-written-prefix15 source metadata_sink:6 original-work-unchanged1'in text
                    assert'metadata-format-after bytes15 read15 text metadata_sink:6 tail-a5241 original-work-unchanged1'in text
        elif unit==0:assert final[0]==final[1]==dict(reference=0xffffffff,base=0,page=65535)
        else:
            for side_index,(text,side,value)in enumerate([(ot,left,final[0]),(nt,right,final[1])]):
                vector=re.search(r'^metadata-vector defines (\w+) count (\d+) ',text,re.M);assert vector
                item=int(re.search(r'^metadata-string defines-'+str(int(vector[2])-1)+r' (\w+) ',text,re.M)[1],16)
                matches=[(i,item-p['start'])for i,p in enumerate(side['pages'])if p['start']<=item<p['start']+p['bytes']];assert len(matches)==1;index,offset=matches[0]
                assert value==dict(reference=(index<<16)|offset,base=side['pages'][index]['start'],page=index)
                after=re.search(r'^metadata-vector-after unit'+str(unit)+r' owner (\w+) items-unchanged(\d+) returned-owner1$',text,re.M);assert after and int(after[1],16)==(constructed[side_index]['address']if unit==2 else int(vector[1],16))and after[2]==vector[2]
            assert final[0]['reference']==final[1]['reference']and final[0]['page']==final[1]['page']
        ret=re.search(r'^direct-return eax \w+ edx \w+ esp (\w+) expected (\w+) nonvolatile 1 fctrl (\w+) before (\w+)$',ot,re.M);assert ret and ret[1]==ret[2]and ret[3]==ret[4]
        assert'direct-canary a5a5a5a5 a5a5a5a5 a5a5a5a5 a5a5a5a5'in ot
        if state:
            assert state['status']=='completed'and len(state['calls'])==(2 if unit==8 else 1)and all('returned_registers'in row for row in state['calls'])
            if unit==8:assert len(state['browser_hook_entries'])==1
        if unit==8:
            returns=re.findall(r'^direct-return eax \w+ edx \w+ esp (\w+) expected (\w+) nonvolatile 1 fctrl (\w+) before (\w+)$',ot,re.M);assert len(returns)==2 and all(r[0]==r[1]and r[2]==r[3]for r in returns)
        assert json.loads((native/'receipt.json').read_text())['exit']==99
    ob,nb=buffers(ot),buffers(nt)
    if unit==8 and not entry:
        assert len(ob)==len(nb)==3 and ob==nb
        assert all(ob[i][j]==ob[0][j]for i in[1,2]for j in range(6))and ob[1][6]==ob[0][6]+'0e'and ob[2][6]==ob[1][6]
    else:
        all_buffers=ob+nb;assert all(v==all_buffers[0]for v in all_buffers)
    assert(original/'stdout').read_bytes()==(native/'stdout').read_bytes()and(original/'stderr').read_bytes()==(native/'stderr').read_bytes()
    work=Path(json.loads((native/'receipt.json').read_text())['work']);assert{p.name:p.read_bytes()for p in(original/'work').iterdir()if p.is_file()}=={p.name:p.read_bytes()for p in work.iterdir()if p.is_file()}
    return dict(status='pass',unit=unit,public_reachability=False,scope='Actual constructor-owned completed argument chain, PCH vectors and parsed symbols; individual entry/cleanup/nonvolatile/pending-output contract.',initial_cache_original=first,initial_cache_native=last,inactive_page_difference='Retained raw65535/0 discrepancy. Walker: every actual nonNULL slot unsigned offset>0xeffc; accepted lookup writes page before consumption. Encoder/decoder setup invoke lookup directly; full poststate strict after lookup. Other fixed methods do not consume the cache and preserve each complete raw state within side.',unwritten_storage='Node padding and allocation tail/guard compared only within side for no mutation. No cross-side uninitialized-tail equality. Pascal result meaningful AX only; void result has no EAX contract.')
