"""Exact reached type resolver calls: caller local, defined flag and result owner."""
import hashlib,json
from state import BINARY

def required(config,case):
    return case=='hello-debug-dll'and any(r['name']=='get_type_record'and r['status']!='stub'for r in config['functions'])

def validate(config,pe):
    ref=config.get('resolver_return_observations')
    assert ref or not required(config,'hello-debug-dll'),'missing resolver return proof'
    if not ref:return
    path=BINARY/ref['evidence'];assert hashlib.sha256(path.read_bytes()).hexdigest()==ref['evidence_sha256'],'stale resolver return proof'
    proof=json.loads(path.read_text());assert proof['types']==[126,124,117,120]and proof['caller_types']==[4218,4210,4144,4125]
    assert proof['returned']==0x265c5 and proof['esp_offset']==32 and proof['size']==2 and proof['owner']=='compact_new_type'and proof['local']=='unused'
    for region in proof['instructions']:assert hashlib.sha256(pe.read(region['start'],region['end']-region['start'])).hexdigest()==region['sha256'],'resolver output instruction proof differs'
    for source,digest in proof['sources'].items():assert hashlib.sha256((BINARY/source).read_bytes()).hexdigest()==digest,'resolver output source binding differs'
    lines=(BINARY/'src/types.c').read_text().splitlines();assert lines[proof['native_line']-1].strip()=='entry = get_type_record((unsigned short)(entry->forward_index-0x1000), &unused);','resolver native caller differs'
    return proof

def parse(events):
    entries=[];returns=[]
    for line in events.splitlines():
        parts=line.split()
        if parts and parts[0]=='resolver-entry':
            assert len(parts)==7
            sequence,esp,type,flag,returned,before=parts[1:];entries.append(dict(sequence=int(sequence),esp=int(esp,16),type=int(type,16),flag=int(flag,16),returned=int(returned,16),before=int(before,16).to_bytes(2,'little').hex()))
        elif parts and parts[0]=='resolver-return':
            assert len(parts)==8
            sequence,esp,result,flag,value,relative,record=parts[1:];returns.append(dict(sequence=int(sequence),esp=int(esp,16),result=int(result,16),flag=int(flag,16),value=int(value,16).to_bytes(2,'little').hex(),relative=int(relative),record=record))
    return dict(entries=entries,returns=returns)

def compare(config,pe,events,native,old_walks,new_walks,case):
    proof=validate(config,pe)
    if not native:
        assert not required(config,case),'missing native resolver returns'
        return
    assert proof and not native.get('error'),'resolver observer failed'
    original=parse(events);count=len(proof['types']);assert len(original['entries'])==len(original['returns'])==len(native['entries'])==len(native['returns'])==count,'missing/extra resolver calls or returns'
    assert events.splitlines().count('resolver-rearm 0001e700 0001e703')==count and events.splitlines().count('resolver-rearm 000265c5 000265c8')==count,'resolver per-call rearm missing'
    result=[]
    for i,(old,new,a,b)in enumerate(zip(original['entries'],native['entries'],original['returns'],native['returns'])):
        expected=proof['types'][i];parent=proof['caller_types'][i]
        assert old['sequence']==new['sequence']==a['sequence']==b['sequence']==i+1 and old['type']&0xffff==new['type']==expected,'resolver actual argument/inventory differs'
        assert old['returned']==proof['returned']and old['flag']==old['esp']+proof['esp_offset']and a['esp']==old['esp']+4 and a['flag']==old['flag'],'resolver original caller/output local differs'
        assert new['flag']==new['local_address']==b['flag']and(new['caller'],new['caller_line'],new['local'],new['size'],new['parent_type'])==(proof['owner'],proof['native_line'],proof['local'],2,parent),'resolver native typed caller/local differs'
        assert len(bytes.fromhex(old['before']))==len(bytes.fromhex(new['before']))==2,'resolver raw entry bytes missing'
        assert a['value']==b['value']=='0000'and a['relative']==b['relative']==expected,'resolver defined output/result differs'
        for entry,returned,walks in((old,a,old_walks),(new,b,new_walks)):
            table=walks['local_type_records'][0];raw=bytes.fromhex(table['bytes']);offset=expected*28
            assert returned['result']==table['address']+offset and returned['record']==raw[offset:offset+28].hex(),'resolver returned record differs from actual owned typed record'
            if entry is new:assert entry['parent_record']==table['address']+(parent-0x1000)*28,'resolver native caller parent identity differs'
        result.append(dict(type=expected,parent_type=parent,flag='0000',returned_type=0x1000+expected,original_before=old['before'],native_before=new['before']))
    return result
