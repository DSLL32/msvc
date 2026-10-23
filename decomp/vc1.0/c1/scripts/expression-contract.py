"""Direct expression ABI/owned-memory contracts, independent of fixture execution."""
import re
import struct

def record(text):
    unit=re.search(r'^expr-unit (\d+) mode (\d+) input ([0-9a-f]+)$',text,re.M)
    assert unit,'missing external unit identity'
    memory={}
    for name,address,size,data in re.findall(r'^expr-([a-z0-9-]+) ([0-9a-f]{8}) (\d+) ([0-9a-f]*)$',text,re.M):
        raw=bytes.fromhex(data);assert len(raw)==int(size),(name,size)
        assert name not in memory,'duplicate direct observation: '+name
        memory[name]=dict(address=int(address,16),data=raw)
    return dict(unit=int(unit[1]),mode=int(unit[2]),input=int(unit[3],16),memory=memory,text=text)

def words(data):return list(struct.unpack('<'+'I'*(len(data)//4),data))
def put(raw,offset,value):struct.pack_into('<I',raw,offset,value)
def before(row):
    unit=row['unit'];m=row['memory'];owners=[m['type-before-'+str(i)]['address']for i in range(3)]
    assert row['mode']==(0 if unit==0 else 2 if unit>=4 else 6)
    projections=[]
    for i,pointer in enumerate(owners):
        raw=m['type-before-'+str(i)]['data'];assert len(raw)==16 and pointer
        flags,attributes,shape,a,b=struct.unpack('<HHIII',raw)
        assert (flags,attributes)==[(0x64,0),(0x108,0),(0x41,0x10)][i],('real type owner differs',i,flags,attributes)
        assert a==b==0
        if i==2:
            s=m['shape-before-2'];assert s['address']==shape and struct.unpack('<HHII',s['data'])==(2,1,0,5)
        else:assert not shape
        projections.append(dict(flags=flags,attributes=attributes,shape=None if i<2 else [2,1,0,5]))
    raw=bytearray(80)
    for i in range(3):struct.pack_into('<BBHIIII',raw,i*20,7,3,0,owners[0],0,17+i,0)
    if unit in [2,3]:
        raw[0]=89 if unit==2 else 80;raw[1]=1 if unit==2 else 2
        put(raw,12,row['input']+20);put(raw,16,row['input']+40 if unit==3 else 0)
        put(raw,28,row['input'])
        if unit==3:put(raw,48,row['input'])
    elif unit==4:
        raw[0]=3;raw[1]=5;put(raw,4,owners[2]);put(raw,12,row['input']+72);put(raw,16,5);raw[72:77]=b'iris\0'
    elif unit==5:
        raw[0]=5;put(raw,4,owners[1]);put(raw,12,row['input']+60);struct.pack_into('<IIHH',raw,60,0,0xc0000000,0x3fff,8)
    assert m['input-before']['address']==row['input'] and m['input-before']['data']==raw,'actual owned input bytes differ'
    pool=words(m['pool-before']['data']);arena=words(m['arena-before']['data'])
    assert pool==[0]*5,'context pool2 is no longer the proved initialized-empty state'
    assert arena[0]==0x3ff4 and arena[1], 'missing initialized compiler free-block ownership'
    free=m['free-before'];header=words(free['data']);assert free['address']==arena[1] and 0<header[1]<=arena[0]
    slot=m['slot-before'];assert slot['address']==free['address']+8 and len(slot['data'])==80,'prospective owned free-block data differs'
    return dict(unit=unit,mode=row['mode'],types=projections,pool=pool,block_bytes=arena[0],reused_zero_bytes=header[1],free_has_next=bool(header[0]),input_bytes=80)

def returned(row):
    summary=before(row);m=row['memory'];unit=row['unit'];source=m['input-before']['data'];slot=m['slot-before'];base=slot['address'];owners=[m['type-before-'+str(i)]['address']for i in range(3)]
    assert m['input-after']==m['input-before'],'input object modified'
    for i in range(3):assert m['type-after-'+str(i)]==m['type-before-'+str(i)],'real CType owner modified'
    assert m['shape-after-2']==m['shape-before-2'],'real type shape modified'
    count=2 if unit==2 else 3 if unit==3 else 1
    consumed={0:20,1:20,2:40,3:60,4:28,5:32}[unit]
    expected=bytearray(slot['data']);zero=min(summary['reused_zero_bytes'],80);expected[:zero]=bytes(zero)
    if not unit:
        expected[0]=7;expected[1]=0x8b;put(expected,4,owners[0]);put(expected,12,0)
    else:
        expected[:count*20]=source[:count*20]
        if unit in [2,3]:
            put(expected,12,base+20);put(expected,28,base)
            if unit==3:put(expected,16,base+40);put(expected,48,base)
        elif unit==4:put(expected,12,base+20);expected[20:25]=b'iris\0'
        elif unit==5:put(expected,12,base+20);expected[20:32]=source[60:72]
    assert m['slot-after']['address']==base and m['slot-after']['data']==expected,'allocated output bytes/store-definedness differs'
    nodes=sorted((int(k[5:]),v)for k,v in m.items()if k.startswith('node-'))
    assert len(nodes)==count,'output node count differs'
    for index,node in nodes:
        assert node['address']==base+index*20 and node['data']==expected[index*20:(index+1)*20], 'output graph ownership/parent bytes differ'
    if unit==4:assert m['string-0']['address']==base+20 and m['string-0']['data']==expected[20:28]
    if unit==5:assert m['extended-0']['address']==base+20 and m['extended-0']['data']==source[60:72]
    oldarena=words(m['arena-before']['data']);newarena=words(m['arena-after']['data']);oldfree=words(m['free-before']['data']);pool=words(m['pool-after']['data'])
    assert newarena==[oldarena[0],oldfree[0]],'free block transfer differs'
    assert pool==[oldarena[1],oldarena[1],base+consumed,oldarena[0]-consumed,0],'pool2 allocation ownership/counters differ'
    assert set(k for k in m if k.startswith('node-'))=={'node-'+str(i)for i in range(count)}
    summary.update(nodes=count,allocation_bytes=consumed,output_bytes=consumed,whole_observed_region_bytes=80,zero_helper_written_offsets=None if unit else[0,1,4,5,6,7,12,13,14,15],definedness='Copier copies all20header bytes, rewrites child/parent/payload pointers, copies exact12extended or5string bytes. Reused allocator clears actual old freeblock.bytes (observed), then only shown stores; aligned tails and all other80B bytes remain exactly the independently computed prior/zeroed values. No blanket pointer/padding normalization.')
    text=row['text']
    original=re.search(r'^direct-return eax ([0-9a-f]+) edx ([0-9a-f]+) esp ([0-9a-f]+) expected ([0-9a-f]+) nonvolatile (\d+) fctrl ([0-9a-f]+) before ([0-9a-f]+)$',text,re.M)
    native=re.search(r'^expr-native-return ([0-9a-f]+) ([0-9a-f]+) ([0-9a-f]+) ([0-9a-f]+)$',text,re.M)
    if original:
        start=re.search(r'^direct-start target ([0-9a-f]+) marker ([0-9a-f]+) stack ([0-9a-f]+) count (\d+) args ([0-9a-f]+) ([0-9a-f]+)$',text,re.M)
        assert start and int(start[1],16)==(0x3c420 if not unit else 0x3c430) and int(start[2],16)==0x362c0,'original direct call identity differs'
        assert int(start[4])==(0 if not unit else 2),'original actual argument count differs'
        if unit:assert [int(start[i],16)for i in [5,6]]==[row['input'],row['mode']],'original actual source/mode arguments differ'
        assert int(original[3],16)==int(start[3],16)+(4 if not unit else 12),'original RET cleanup differs'
        assert int(original[1],16)==base and original[3]==original[4]and original[5]=='1'and original[6]==original[7],'original ABI return differs'
        canary=re.search(r'^direct-canary (.*)$',text,re.M);assert canary and canary[1].split()==['a5a5a5a5']*4
    if native:
        assert native[1]==native[2]and native[3]==native[4],'native stack/FPU return differs'
        assert 'expr-stack-guards '+'a5'*16+'\n' in text,'native private stack guard differs'
    assert original or native,'missing actual return ABI evidence'
    summary['ABI']='cdecl noargs/RET0'if unit==0 else'stdcall source+arena/RET8'
    return summary
