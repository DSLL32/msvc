"""A constructor-local TypeServer allocation before global publication."""
import hashlib,json,re
from state import BINARY

def validate(item,pe):
    path=BINARY/item['evidence']
    assert hashlib.sha256(path.read_bytes()).hexdigest()==item['evidence_sha256'],'changed transient allocation proof'
    p=json.loads(path.read_text())
    phase=p.get('phase','constructor')
    expected={'constructor':[[64,96],[100,104]],'header-read':[[0,96],[100,108],[112,116]]}
    assert p['size']==116 and phase in expected and p['defined']==expected[phase],'unreviewed transient initialized range'
    assert p['child_fields']==[72,84] and p['child_size']==12,'unreviewed transient child layout'
    assert set(p['sources'])=={'src/pdb.c','src/pdb.h'}
    for name,digest in p['sources'].items():assert hashlib.sha256((BINARY/name).read_bytes()).hexdigest()==digest,'transient owner source changed'
    for r in p['instructions']:assert hashlib.sha256(pe.read(r['start'],r['end']-r['start'])).hexdigest()==r['sha256'],'transient creation instructions differ'
    expected_instructions=[(0x2b968,0x2ba39)]
    if phase=='header-read':expected_instructions.append((0x2af48,0x2af6d))
    assert [(r['start'],r['end'])for r in p['instructions']]==expected_instructions
    return p

def compare(item,words,parameters,events,pe):
    p=validate(item,pe);value=words[item['slot']];actual=parameters[item['name']]
    phase=p.get('phase','constructor')
    returned,register=(0x2ba39,'esi')if phase=='constructor'else(0x2af6d,'ebx')
    assert words[0]==returned and words[register]==value,'unreviewed transient allocation caller'
    assert not actual['null'] and actual['address']==actual['owner_pointer'],'transient native owner differs'
    assert actual['owner']==dict(function='type_server_open',local='server',size=116),'unreviewed native transient local'
    raw=re.search(r'^input-buffer %08x ([0-9a-f]+)$'%value,events,re.M)
    assert raw,'missing original transient bytes'
    old,new=bytes.fromhex(raw[1]),bytes.fromhex(actual['prebytes'])
    assert len(old)==len(new)==116
    if phase=='header-read':
        from pdb_read import original as original_reads
        reads=original_reads(events);assert reads,'missing completed PDB header write'
        a,b=reads[-1],actual['read_return']
        assert a['address']==value and b['address']==actual['address']and a['returned']==b['returned']==64
        master=(BINARY/'cases/hello-debug-dll/input/msvc.pdb').read_bytes()[:64]
        assert old[:64]==new[:64]==master,'transient PDB header differs'
        assert int.from_bytes(old[100:104],'little')==a['descriptor']and int.from_bytes(new[100:104],'little')==b['descriptor'],'transient PDB descriptor identity differs'
        assert old[104:108]==new[104:108]==b'\0'*4 and old[112:116]==new[112:116]==b'\1\0\0\0','transient reached mode flags differ'
    children={}
    for offset in p['child_fields']:
        address=int.from_bytes(old[offset:offset+4],'little')
        r=re.search(r'^transient-child %08x %d ([0-9a-f]+) ([0-9a-f]+)$'%(value,offset),events,re.M)
        assert r and int(r[1],16)==address,'transient original child ownership differs'
        children[str(offset)]=dict(address=address,bytes=r[2])
    def normalize(raw,base,children):
        assert set(children)=={'72','84'},'unreviewed transient child set'
        raw=bytearray(raw);ranges=[(base,base+116)]
        for offset in p['child_fields']:
            child=children[str(offset)];address=child['address'];body=bytes.fromhex(child['bytes'])
            assert address and len(body)==12 and all(address+12<=a or b<=address for a,b in ranges),'transient child allocations overlap'
            ranges.append((address,address+12))
            assert int.from_bytes(raw[offset:offset+4],'little')==int.from_bytes(raw[offset+4:offset+8],'little')==address,'transient arena head/tail differs'
            assert body[:4]==body[8:12]==b'\0'*4 and int.from_bytes(body[4:8],'little')==address+12,'transient child initialization or self-end cursor differs'
            raw[offset:offset+8]=bytes([offset])*8
        assert raw[64:72]==b'\0'*8 and raw[80:84]==raw[92:96]==b'\0'*4,'transient initialized constructor fields differ'
        if phase=='constructor':assert raw[100:104]==b'\xff'*4,'transient descriptor initial state differs'
        result=bytearray(116)
        for lo,hi in p['defined']:result[lo:hi]=raw[lo:hi]
        return result
    assert normalize(old,value,children)==normalize(new,actual['address'],actual['children']),'transient initialized bytes differ'
    return dict(size=116,defined=p['defined'],children=[72,84],original_address=value,native_address=actual['address'],original_raw_sha256=hashlib.sha256(old).hexdigest(),native_raw_sha256=hashlib.sha256(new).hexdigest())
