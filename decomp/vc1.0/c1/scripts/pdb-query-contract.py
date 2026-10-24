"""Fixed constructor-owned PDB record interface contracts; no process launch."""
import importlib.util,json,re,struct
from pathlib import Path
spec=importlib.util.spec_from_file_location('pdb_object_contract',Path(__file__).with_name('pdb-object-contract.py'))
base=importlib.util.module_from_spec(spec);spec.loader.exec_module(base)

def begin(text):
    m=re.search(r'^pdb-query-begin unit (\d+) owner (\w+) first (\d+) last (\d+) record (\w+) bytes (\d+) buffer (\w+) length (\w+) allocation (\w+) size65536$',text,re.M);assert m
    return {k:int(v,10 if k in('unit','first','last','bytes')else 16)for k,v in zip(['unit','owner','first','last','record','bytes','buffer','length','allocation'],m.groups())}
def streams(text,boundary=None):
    if boundary is not None:
        token='pdb-query-streams '+boundary+'\n';assert token in text;text=text.split(token,1)[1].split('pdb-query-streams ',1)[0]
    values={int(i):None if size=='unopened'else value for i,size,value in re.findall(r'^buffer (\d+) (unopened|\d+)(?: ([0-9a-f]*))?$',text,re.M)}
    assert set(values)==set(range(7));return values

def validate(original,native,plan):
    unit=plan['unit'];assert 6<=unit<=14
    original=Path(original);native=Path(native);ot=(original/'events').read_text();nt=(native/'report').read_text();state=json.loads((native/'state.json').read_text())if(native/'state.json').exists()else None
    a=base.record(ot.split('pdb-query-begin',1)[0],0);b=base.record(nt.split('pdb-query-begin',1)[0],0,True)
    assert json.loads(json.dumps(a))==json.loads(json.dumps(b)),'constructor/open state differs'
    left,right=begin(ot),begin(nt);assert left['unit']==right['unit']==unit
    for key in('first','last','bytes'):assert left[key]==right[key]
    assert left['first']<left['last']and 3<=left['bytes']<=244
    for text,query in[(ot,left),(nt,right)]:
        memory=base.base.memory(text);assert memory['object-result']['address']==query['owner']
        offsets=memory['offsets-result'];assert struct.unpack_from('<I',offsets['data'])[0]==query['record']
        record=memory['query-record'];assert record['address']==query['record']and len(record['data'])==query['bytes']+2
        assert(struct.unpack_from('<H',record['data'],2)[0]+2)&65535==query['bytes']
        assert query['allocation']<=query['buffer']and query['buffer']+256<=query['allocation']+65536
        assert query['allocation']<=query['length']and query['length']+4<=query['allocation']+65536
    original_memory=base.base.memory(ot);native_memory=base.base.memory(nt)
    assert original_memory['query-record']['data']==native_memory['query-record']['data']
    def arguments(query):
        return {6:[query['first']],7:[query['first'],query['buffer'],query['length']],8:[],9:[query['owner'],query['first']],10:[query['owner'],query['first'],query['buffer'],query['length']],11:[],12:[query['owner']],13:[query['owner']],14:[query['owner']]}[unit]
    query_text=ot.split('pdb-query-begin',1)[1];returned='pdb-query-value 'in query_text
    expected_count=2 if returned and unit in(7,10)else 1
    calls=re.findall(r'^pdb-call (\d+) (\w+) (\w+) (\w+) (\d+) (\w+) (\w+) (\w+) (\d+)$',query_text,re.M);assert len(calls)==expected_count
    fourth=re.findall(r'^pdb-fourth (\w+)$',query_text,re.M);assert len(fourth)==(expected_count if unit==10 else 0)
    pop={6:4,7:12}.get(unit,0)
    for i,(_,target,self,stack,count,arg_a,arg_b,arg_c,cleanup)in enumerate(calls):
        assert int(target,16)==plan['start']and int(self,16)==(left['owner']if unit in(6,7,8)else 0)
        assert int(count)==len(arguments(left))and int(cleanup)==pop
        args=[int(v,16)for v in(arg_a,arg_b,arg_c)][:int(count)]
        if int(count)==4:args.append(int(fourth[i],16))
        assert args==arguments(left)
        assert left['allocation']<=int(stack,16)and int(stack,16)+48<=left['allocation']+65536
    assert streams(ot,'before'if'pdb-query-streams before\n'in ot else None)==streams(nt,'before')
    if not returned:
        assert state and state['status']=='entry'and len(state['calls'])==1
        exception=re.search(r'^exception 80000003 rva (\w+) base (\w+) first 1$',query_text,re.M);assert exception and int(exception[1],16)+int(exception[2],16)==plan['start']
        registers=re.search(r'^registers eax (\w+) ecx (\w+) edx (\w+) ebx (\w+) esi (\w+) edi (\w+) ebp (\w+) esp (\w+)$',query_text,re.M);assert registers
        assert int(registers[8],16)==int(calls[0][3],16)
        if unit in(6,7,8):assert int(registers[2],16)==left['owner']
        slots={int(i):int(v,16)for i,v in re.findall(r'^stack (\d+) (\w+)$',query_text,re.M)};assert slots[0]==0x17300 and[slots[i+1]for i in range(len(arguments(left)))]==arguments(left)
        call=state['calls'][0];assert call['function']==plan['function'][1:].split('@')[0]
    else:
        returns=re.findall(r'^pdb-return (\d+) (\w+) (\w+) (\w+) (\d+) (\w+) (\w+)$',query_text,re.M);assert len(returns)==expected_count
        native_returns=re.findall(r'^pdb-native-query-return (\d+) (\w+) (\w+) (\w+) (\w+) (\w+)$',nt,re.M);assert len(native_returns)==expected_count
        assert len(re.findall(r'^pdb-canary a5a5a5a5 a5a5a5a5 a5a5a5a5 a5a5a5a5$',query_text,re.M))==expected_count
        if state:assert state['status']=='returned'and len(state['calls'])==expected_count
        for i,(ret,nret)in enumerate(zip(returns,native_returns)):
            stage,eax,edx,esp,preserved,cw,cw_before=ret;nstage,neax,before,after,ncw,ncw_before=nret
            assert int(stage)==int(nstage)==10+i and int(esp,16)==int(calls[i][3],16)+4+pop and preserved=='1'and cw==cw_before
            assert before==after and ncw==ncw_before
            for query,memory,value in[(left,original_memory,int(eax,16)),(right,native_memory,int(neax,16))]:
                raw=memory['object-result']['data'];expected_object=bytearray(raw)
                if unit==8:struct.pack_into('<i',expected_object,100,-1)
                assert memory['query-object-after-'+stage]['address']==query['owner']and memory['query-object-after-'+stage]['data']==bytes(expected_object)
                expected={6:query['record'],7:query['bytes'],8:1,9:query['record']+2,10:query['bytes'],11:0xe0d5c,12:query['first'],13:query['last'],14:(struct.unpack_from('<I',raw,80)[0]+2*(query['first']-query['last']))&0xffffffff}[unit]
                assert(value&65535 if unit in(12,13)else value)==expected
                if unit in(7,10):
                    suffix='full'if i==0 else'short';capacity=query['bytes']+4 if i==0 else 3;count=min(capacity,query['bytes'])
                    assert memory['query-copy-'+suffix]['address']==query['buffer']and memory['query-copy-'+suffix]['data']==memory['query-record']['data'][2:2+count]+b'\xa5'*(256-count)
                    assert memory['query-length-'+suffix]['address']==query['length']and struct.unpack('<I',memory['query-length-'+suffix]['data'])[0]==query['bytes']
            boundary='after-full'if i==0 else'after-short';assert streams(ot,boundary)==streams(nt,boundary)==streams(nt,'before')
            if state:
                call=state['calls'][i];assert call['function']==plan['function'][1:].split('@')[0]
                assert(call['returned_registers']['eax']&65535 if unit in(12,13)else call['returned_registers']['eax'])==int(neax,16)
        if unit in(7,10):
            for text in(ot,nt):assert[int(v)for v in re.findall(r'^pdb-query-copy capacity (\d+) ',text,re.M)]==[left['bytes']+4,3]
        if unit==8:
            def errors(text):
                m=re.search(r'^pdb-errors query-result (.*)$',text,re.M);assert m;return m[1].split()
            assert errors(ot)==errors(nt)
        assert 'pdb-native-stack-guards intact'in nt
    for name in('stdout','stderr'):assert(original/name).read_bytes()==(native/name).read_bytes()
    work=Path(json.loads((native/'receipt.json').read_text())['work'])
    assert{p.name:p.read_bytes()for p in(original/'work').iterdir()}=={p.name:p.read_bytes()for p in work.iterdir()},'complete owned files differ'
    return dict(status='pass',unit=unit,public_reachability=False,kind='actual constructor-owned PDB interface body/ABI/output'if returned else'fresh actual constructor-owned PDB interface entry',loaded_indices=left['last']-left['first'],record_bytes=left['bytes'])
