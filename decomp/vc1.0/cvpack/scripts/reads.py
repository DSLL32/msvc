"""Read-only caller buffer/return observations, before the consumer executes."""
import hashlib,json
from state import BINARY

def validate(config,pe):
    record=config.get('read_observations')
    if not record:return []
    path=BINARY/record['evidence']
    assert hashlib.sha256(path.read_bytes()).hexdigest()==record['evidence_sha256'],'stale read observation proof'
    proof=json.loads(path.read_text())
    assert hashlib.sha256((BINARY/proof['source']).read_bytes()).hexdigest()==proof['source_sha256'],'changed read consumer source'
    for source,digest in proof.get('additional_sources',{}).items():
        assert hashlib.sha256((BINARY/source).read_bytes()).hexdigest()==digest,'changed additional read consumer source'
    owner=next(r for r in config['functions']if r['name']==proof['owner'])
    callee=next(r for r in config['functions']if r['name']==proof['callee'])
    assert owner['status']!='stub'
    known=set()
    for site in proof['sites']:
        owner=next(r for r in config['functions']if r['name']==site.get('owner',proof['owner']))
        call,returned=site['call'],site['returned']
        assert site.get('maximum_count',4096)==(4746 if call==0x17dc9 else 4096),'unreviewed read site bound'
        assert call not in known and returned not in known;known.update((call,returned))
        assert owner['extent'][0]<=call<returned<owner['extent'][1]
        raw=pe.read(call,5)
        assert raw.hex()==site['call_bytes'] and raw[0]==0xe8 and returned==call+5
        assert returned+int.from_bytes(raw[1:],'little',signed=True)==callee['original']
        assert pe.read(returned,3).hex()==site['return_bytes']=='83c40c'
    return proof['sites']

def original(events):
    result=[]
    for line in events.splitlines():
        if line.startswith('read-entry '):
            _,site,handle,address,count,error,position,fd,osfile,host_position,raw=line.split()
            result.append(dict(site=int(site,16),handle=int(handle),address=int(address,16),count=int(count),entry_errno=int(error),entry_position=int(position),descriptor=int(fd),entry_osfile=int(osfile,16),entry_host_position=int(host_position),prebytes=raw))
        elif line.startswith('read-page-entry '):
            words=line.split()[1:];assert words[::2]==['site','slot','page','flags','position','address'],'invalid read page entry'
            item={key:int(value,16 if key in('site','slot','page','address')else 10)for key,value in zip(words[::2],words[1::2])}
            assert result and result[-1]['site']==item['site']and 'page_entry'not in result[-1]
            result[-1]['page_entry']=item
        elif line.startswith('read-return '):
            _,site,count,error,position,fd,osfile,host_position,raw=line.split()
            assert result and 'returned'not in result[-1]and result[-1]['site']==int(site,16)
            result[-1].update(returned=int(count),errno=int(error),position=int(position),descriptor_return=int(fd),osfile=int(osfile,16),host_position=int(host_position),postbytes=raw)
    return result

def compare(events,native,sites):
    old=original(events)
    assert old and native,'missing observed reads'
    controls=[line.split()for line in events.splitlines()if line.startswith(('read-entry ','read-return ','read-rearm '))]
    cursor=0
    for item in old:
        site=next(r for r in sites if r['call']==item['site'])
        assert controls[cursor][0]=='read-entry';cursor+=1
        assert controls[cursor]==['read-rearm','%08x'%site['call'],'0002a400'];cursor+=1
        if 'returned'in item:
            assert controls[cursor][0]=='read-return';cursor+=1
            assert controls[cursor]==['read-rearm','%08x'%site['returned'],'%08x'%(site['returned']+3)];cursor+=1
    assert cursor==len(controls),'extra read control events'
    assert len(old)==len(native),'read observation count differs'
    receipt=[]
    for a,b in zip(old,native):
        assert ('returned'in a)==('returned'in b) and not b.get('error'),'inconsistent observed read completion'
        assert a['site']in {r['call']for r in sites}
        for field in ('site','handle','count','entry_errno','entry_position','descriptor','entry_osfile','entry_host_position',*(['returned','errno','position','descriptor_return','osfile','host_position']if 'returned'in a else [])):
            assert a[field]==b[field],('read state differs',field,a[field],b[field])
        if 'page_entry'in a or 'page_entry'in b:
            assert 'page_entry'in a and 'page_entry'in b,'missing actual Page entry state'
            for key in('site','flags','position'):assert a['page_entry'][key]==b['page_entry'][key],'Page entry state differs'
            for record in(a,b):
                page=record['page_entry'];assert page['address']==record['address']and page['slot']and page['page'],'Page entry ownership differs'
        assert 0<=a['count']<=next(r for r in sites if r['call']==a['site']).get('maximum_count',4096),'read exceeds site limit'
        if 'returned'not in a:
            assert len(bytes.fromhex(a['prebytes']))==len(bytes.fromhex(b['prebytes']))==a['count']
            receipt.append(dict(site=a['site'],requested=a['count'],completed=False));continue
        count=a['count'];returned=a['returned'];assert 0<=count<=next(r for r in sites if r['call']==a['site']).get('maximum_count',4096) and -1<=returned<=count
        defined=max(returned,0)
        for item in (a,b):
            before,after=bytes.fromhex(item['prebytes']),bytes.fromhex(item['postbytes'])
            assert len(before)==len(after)==count,'observed read buffer extent differs'
            assert before[defined:]==after[defined:],'read altered unwritten tail'
        assert bytes.fromhex(a['postbytes'])[:defined]==bytes.fromhex(b['postbytes'])[:defined],'defined read bytes differ'
        receipt.append(dict(site=a['site'],requested=count,defined=defined,sha256=hashlib.sha256(bytes.fromhex(a['postbytes'])[:defined]).hexdigest(),errno=a['errno'],position=a['position'],host_position=a['host_position']))
    return receipt
