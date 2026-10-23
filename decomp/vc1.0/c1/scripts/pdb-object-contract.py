"""Bounded, store-defined PDB method contracts; raw observations stay intact."""
import importlib.util
from pathlib import Path
import re
import struct

spec=importlib.util.spec_from_file_location('pdb_cleanup_contract',Path(__file__).with_name('pdb-contract.py'))
base=importlib.util.module_from_spec(spec);spec.loader.exec_module(base)


def record(text,unit,native=False):
    memory=base.memory(text)
    rows=[(int(i),int(n),int(p,16)) for i,n,p in re.findall(r'^pdb-allocation (\d+) (\d+) ([0-9a-f]+)$',text,re.M)]
    assert [i for i,n,p in rows]==list(range(len(rows))) and len(rows)>=3
    assert [n for i,n,p in rows[:3]]==[116,12,12]
    for i,n,p in rows:
        assert n and p and p+n<=0xffffffff
        for j,m,q in rows[:i]:assert p+n<=q or q+m<=p,'overlapping allocation owners'
        before=memory['allocation-before-'+str(i)]
        assert before['address']==p and len(before['data'])==min(n,256)
    def owner(pointer,end=False):
        if not pointer:return None
        matches=[(i,pointer-p) for i,n,p in rows if p<=pointer<p+n or end and pointer==p+n]
        assert len(matches)==1,('unknown/ambiguous pointer owner',hex(pointer),matches)
        return matches[0]
    def words(raw):return struct.unpack('<'+'I'*(len(raw)//4),raw)
    def snapshot(label):
        item=memory['object-'+label]
        assert item['address']==rows[0][2] and len(item['data'])==116
        return item['data']
    constructed=snapshot('constructor');result=snapshot('result')
    assert memory['allocation-after-0']['data']==result,'inconsistent owner snapshots'
    fresh=memory['allocation-before-0']['data']
    for a,b in [(0,64),(96,100),(112,116)]:
        assert constructed[a:b]==fresh[a:b],'constructor initialized unwritten bytes'
    assert constructed[64:72]==bytes(8)
    assert words(constructed[100:112])==(0xffffffff,1,1)
    for kind,offset,index in [('records',72,1),('blocks',84,2)]:
        head,tail,count=words(constructed[offset:offset+12])
        assert owner(head)==owner(tail)==(index,0) and count==0
        node=memory['node-'+kind+'-constructor-0']
        assert node['address']==head and words(node['data'])==(0,head+12,0)
    if unit==3:
        assert result[:64]==constructed[:64] and result[96:100]==constructed[96:100]
        header=None;loaded=False;first=last=0;record_bytes=0
    else:
        header=result[:64].hex();loaded=True
        first,last,record_bytes=struct.unpack_from('<HHI',result,56)
        assert 0x1000<=first<=last<=0x1040 and record_bytes<=244
        assert struct.unpack_from('<I',result,44)[0]==0xe0d5c
    assert words(result[104:116])==((0 if unit in (1,3)else 1),1,1)
    handle=struct.unpack_from('<i',result,100)[0]
    assert (handle<0)==(unit in (1,3))
    logical_lists={};payloads={};entry_rows=[]
    for kind,offset in [('records',72),('blocks',84)]:
        head,tail,count=words(result[offset:offset+12])
        assert memory['list-'+kind+'-result']['address']==rows[0][2]+offset
        assert memory['list-'+kind+'-result']['data']==result[offset:offset+12]
        chain=[];pointer=head;seen=set();index=0
        while pointer:
            assert pointer not in seen and index<32
            seen.add(pointer);node=memory['node-'+kind+'-result-'+str(index)]
            assert node['address']==pointer and len(node['data'])==12
            allocation,relative=owner(pointer);assert relative==0
            available,cursor,next_pointer=words(node['data'])
            assert pointer+12<=cursor<=pointer+rows[allocation][1]
            used=cursor-pointer-12
            assert used<=244,'bounded payload proof exceeded'
            after=memory['allocation-after-'+str(allocation)]['data']
            before=memory['allocation-before-'+str(allocation)]['data']
            assert after[:12]==node['data'] and after[12+used:]==before[12+used:],'unwritten allocation tail changed'
            if kind=='records':
                payloads[str(allocation)]=after[12:12+used].hex()
            else:
                assert used%12==0
                for at in range(12,12+used,12):
                    previous,record,index_value=struct.unpack_from('<IIH',after,at)
                    assert after[at+10:at+12]==before[at+10:at+12],'unwritten type-entry padding changed'
                    entry_rows.append((allocation,at,owner(previous),owner(record),index_value))
            chain.append((allocation,available,owner(cursor,end=True),owner(next_pointer)))
            pointer=next_pointer;index+=1
        assert chain and owner(tail)==(chain[-1][0],0)
        logical_lists[kind]=dict(head=owner(head),tail=owner(tail),count=count,nodes=chain)
    if loaded:assert owner(struct.unpack_from('<I',result,96)[0])==logical_lists['records']['tail']
    offset_pointer,index_pointer=words(result[64:72]);offset_values=[];buckets=[]
    if loaded:
        offset_owner=owner(offset_pointer);assert offset_owner[1]==0
        count=last-first;allocation=offset_owner[0]
        after=memory['allocation-after-'+str(allocation)]['data'];before=memory['allocation-before-'+str(allocation)]['data']
        assert len(after)>=count*4 and after[count*4:]==before[count*4:]
        if count:
            item=memory['offsets-result'];assert item['address']==offset_pointer and item['data']==after[:count*4]
            offset_values=[owner(value)for value in words(item['data'])]
        index_owner=owner(index_pointer);assert index_owner[1]==0 and rows[index_owner[0]][1]==16384
        index_data=b''.join(memory['index-result-'+str(at)]['data']for at in range(0,16384,256))
        for at in range(0,16384,256):assert memory['index-result-'+str(at)]['address']==index_pointer+at
        assert memory['allocation-after-'+str(index_owner[0])]['data']==index_data[:256]
        buckets=[(i,owner(value))for i,value in enumerate(words(index_data))if value]
    else:assert not offset_pointer and not index_pointer
    for i,n,p in rows:
        item=memory['allocation-after-'+str(i)]
        assert item['address']==p and len(item['data'])==min(n,256)
    errors={boundary:tuple(int(v,16)for v in values.split())for boundary,values in re.findall(r'^pdb-errors (\S+) (.*)$',text,re.M)}
    assert set(errors)=={'constructor','result'}
    if native:
        returns=[]
        for stage,value,before,after,cw_before,cw_after in re.findall(r'^pdb-native-return (\d+) (\d+) ([0-9a-f]+) ([0-9a-f]+) ([0-9a-f]+) ([0-9a-f]+)$',text,re.M):
            assert before==after and cw_before==cw_after
            returns.append((int(stage),int(value)))
        assert 'pdb-native-stack-guards intact' in text
    else:
        calls=[]
        for tail in re.findall(r'^pdb-call \d+ (.*)$',text,re.M):
            parts=tail.split();assert len(parts)==8
            calls.append(tuple(int(value,10 if i in (3,7)else 16)for i,value in enumerate(parts)))
        returned=[tail.split()for tail in re.findall(r'^pdb-return \d+ (.*)$',text,re.M)]
        stages=[int(v)for v in re.findall(r'^pdb-return (\d+) ',text,re.M)]
        assert len(calls)==len(returned)
        returns=[]
        for call,ret,stage in zip(calls,returned,stages):
            target,self,stack,args,a,b,c,pop=call
            value,edx,esp,preserved,cw,cw_before=[int(v,16)for v in ret]
            assert esp==stack+4+pop and preserved==1 and cw==cw_before
            if stage in (1,4):returns.append((stage,value))
        assert len(re.findall(r'^pdb-canary a5a5a5a5 a5a5a5a5 a5a5a5a5 a5a5a5a5$',text,re.M))==len(calls)
    assert returns==([(1,1),(4,0 if unit==5 else 1)]if unit>=4 else[(1,0 if unit==3 else 1)])
    positions=[int(v)for v in re.findall(r'^pdb-offset (\d+)$',text,re.M)]
    assert positions==([]if unit in (1,3)else[64 if unit==2 else 32 if unit==5 else 224])
    return dict(unit=unit,allocations=[n for i,n,p in rows],header=header,
        type_offsets=offset_values,buckets=buckets,type_entries=entry_rows,record_payloads=payloads,
        lists=logical_lists,loaded_tail=(logical_lists['records']['tail']if loaded else None),
        descriptor='closed'if handle<0 else'owned unit.pdb',position=positions,returns=returns,
        errors=errors,undefined_policy='Constructor header/loaded_tail/cumulative and type-entry padding are raw; only own preservation is checked. Allocation tails are bounded to the observed256B prefix. Original/native FP controls are individually preserved, not claimed equal.')
