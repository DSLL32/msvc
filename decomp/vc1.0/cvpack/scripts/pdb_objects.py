"""Published PDB object graph: exact allocation ownership and defined ranges."""
import hashlib,json
from pathlib import Path
BINARY=Path(__file__).resolve().parents[1]

def word(raw,offset):return int.from_bytes(raw[offset:offset+4],'little')

def validate(config,pe):
    r=config.get('pdb_object_observations')
    if not r:return None
    path=BINARY/r['evidence'];assert hashlib.sha256(path.read_bytes()).hexdigest()==r['evidence_sha256'],'stale PDB graph proof'
    p=json.loads(path.read_text())
    assert set(p['sources'])=={'src/pdb.c','src/pdb.h'}
    for source,digest in p['sources'].items():assert hashlib.sha256((BINARY/source).read_bytes()).hexdigest()==digest,'PDB graph source changed'
    for a,b,digest in p['instructions']:assert hashlib.sha256(pe.read(a,b-a)).hexdigest()==digest,'PDB allocation instructions changed'
    assert [(a,b)for a,b,_ in p['instructions']]==[(0x2b968,0x2ba39),(0x2b14c,0x2b405)]
    assert p['case']=='hello-debug-dll'and p['parent_size']==116 and p['payload']==4140 and p['block_capacity']==12288
    return p

def capture(address,read):
    raw=read(address,116);payload=word(raw,60);capacity=(payload+0x1fff)&~0xfff
    assert payload==4140 and capacity==12288,'unreviewed PDB block allocation extent'
    count=int.from_bytes(raw[58:60],'little')-0x1000
    assert count==136,'unreviewed PDB offset table allocation extent'
    result={}
    def get(name,pointer,size):
        assert pointer and 0<size<=65536,'invalid PDB owned object extent'
        result[name]=dict(address=pointer,bytes=read(pointer,size).hex())
        return bytes.fromhex(result[name]['bytes'])
    get('offsets',word(raw,64),count*4)
    first=get('type-storage-0',word(raw,72),12)
    get('type-storage-1',word(first,8),12+capacity)
    get('index-storage-0',word(raw,84),12)
    ranges=[(address,address+116)]
    for item in result.values():
        lo=item['address'];hi=lo+len(bytes.fromhex(item['bytes']))
        assert all(hi<=a or b<=lo for a,b in ranges),'PDB allocations overlap'
        ranges.append((lo,hi))
    return result

def compare_node(left,right,old,new,logical,old_names,new_names,consumed):
    master=(BINARY/'cases/hello-debug-dll/input/msvc.pdb').read_bytes()
    assert bytes(left[:64])==bytes(right[:64])==master[:64],'published PDB header differs'
    assert set(old['objects'])==set(new['objects'])=={'offsets','type-storage-0','type-storage-1','index-storage-0'}
    assert consumed and len(consumed['original'])==len(consumed['native'])==1,'missing actual uninitialized-field read proof'
    original_read,native_read=consumed['original'][0],consumed['native'][0]
    assert original_read['address']==old['address'] and native_read['address']==new['address'],'consumed PDB field allocation differs'
    assert word(left,108)==original_read['value']==word(right,108)==native_read['value'],'actual consumed undefined value differs'
    assert original_read['value']==0,'new incremental branch needs its own typed object proof'
    proof={}
    for side,root in (('original',old),('native',new)):
        ranges=[(root['address'],root['address']+116)]
        sizes={'offsets':544,'type-storage-0':12,'type-storage-1':12300,'index-storage-0':12}
        for key,obj in root['objects'].items():
            size=len(bytes.fromhex(obj['bytes']));assert size==sizes[key],'PDB requested allocation extent differs'
            lo=obj['address'];hi=lo+size
            assert lo and all(hi<=a or b<=lo for a,b in ranges),'PDB allocation ownership overlaps'
            ranges.append((lo,hi))
        reads=consumed['reads_'+side]
        assert len(reads)==2 and reads[0]['address']==root['address'] and reads[1]['address']==root['objects']['type-storage-1']['address']+12,'PDB reads do not belong to the published allocation graph'
    for parent,root,names in ((left,old,old_names),(right,new,new_names)):
        objects=root['objects'];storage0=objects['type-storage-0'];storage1=objects['type-storage-1'];index0=objects['index-storage-0']
        assert word(parent,68)==0 and word(parent,72)==storage0['address']and word(parent,76)==word(parent,96)==storage1['address']
        assert word(parent,80)==4140 and word(parent,84)==word(parent,88)==index0['address']and word(parent,92)==0
        assert word(parent,100)==0xffffffff and word(parent,104)==0 and word(parent,112)==1
        for key,item in objects.items():
            body=bytes.fromhex(item['bytes'])
            if key=='offsets':continue
            assert word(body,0)==0,'published PDB block available count differs'
            used=4140 if key=='type-storage-1'else 0
            assert word(body,4)==item['address']+12+used,'PDB block cursor differs from defined payload end'
            assert word(body,8)==(storage1['address']if key=='type-storage-0'else 0),'PDB block next ownership differs'
            if used:assert body[12:12+used]==master[64:]and len(body)==12300,'PDB payload differs or allocation extent is wrong'
            else:assert len(body)==12,'PDB constructor block extent differs'
        table=bytes.fromhex(objects['offsets']['bytes']);payload=bytes.fromhex(storage1['bytes'])[12:4152]
        assert len(table)==544
        offset=0
        for i in range(136):
            assert word(table,i*4)==storage1['address']+12+offset,'PDB type offset table differs'
            assert offset+4<=len(payload),'PDB type record header exceeds defined bytes'
            length=((int.from_bytes(payload[offset+2:offset+4],'little')+2)&0xffff)+2
            assert length>=4 and offset+length<=len(payload),'PDB type record exceeds defined bytes'
            offset+=length
        assert offset==4140,'PDB offset table omits payload records'
    for key,a in old['objects'].items():
        b=new['objects'][key];old_id,new_id=logical(a['address'],old_names),logical(b['address'],new_names)
        assert old_id==new_id,'PDB allocation logical identity differs'
        known=544 if key=='offsets'else 4152 if key=='type-storage-1'else 12
        proof[key]=dict(bytes=len(bytes.fromhex(a['bytes'])),defined_bytes=known,original_raw_sha256=hashlib.sha256(bytes.fromhex(a['bytes'])).hexdigest(),native_raw_sha256=hashlib.sha256(bytes.fromhex(b['bytes'])).hexdigest())
    return dict(objects=proof,consumed_undefined_input=dict(offset=108,value=original_read['value'],classified='observed concrete input, never initialized or synthesized'))
