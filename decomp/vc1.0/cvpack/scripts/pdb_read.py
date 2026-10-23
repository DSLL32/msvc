"""Exact PDB header and payload reads, observed before their consumers."""
import json,re,hashlib
from state import BINARY

def validate(config,pe):
    r=config.get('pdb_read_observations')
    if not r:return None
    p=BINARY/r['evidence'];assert hashlib.sha256(p.read_bytes()).hexdigest()==r['evidence_sha256'],'stale PDB header read proof'
    proof=json.loads(p.read_text())
    assert proof['call']==0x2af4c and proof['return']==0x2af51 and proof['size']==64
    assert pe.read(proof['call'],5).hex()=='e80f2a0000' and pe.read(proof['return'],3).hex()=='83c40c'
    assert hashlib.sha256((BINARY/'src/pdb.c').read_bytes()).hexdigest()==proof['source_sha256'],'PDB header consumer changed'
    assert proof['payload_call']==0x2b294 and proof['payload_return']==0x2b299 and proof['payload_size']==4140
    assert pe.read(0x2b294,5).hex()=='e8c7260000' and pe.read(0x2b299,3).hex()=='83c40c'
    return proof

def original(events):
    result=[]
    for line in events.splitlines():
        if not line.startswith('pdb-read '):continue
        m=re.fullmatch(r'pdb-read fd (\d+) count (\d+) errno (\d+) last_error (\d+) position (\d+) osfile (\d+) type (\d+) owned (\d+) input (\d+) buffer ([0-9a-f]+) header ([0-9a-f]+)',line)
        assert m,'invalid original PDB header proof'
        keys=['descriptor','returned','errno','last_error','position','osfile','type','owned','input_matches']
        r={key:int(m[i+1])for i,key in enumerate(keys)};r.update(address=int(m[10],16),header=m[11]);result.append(r)
    return result

def compare(config,pe,events,native,required_records=False):
    if not validate(config,pe):assert not native;return []
    old=original(events);assert len(old)==len(native),'PDB header observation count differs'
    master=(BINARY/'cases/hello-debug-dll/input/msvc.pdb').read_bytes()
    if required_records:assert [r['returned']for r in old]==[64,4140],'missing mandatory PDB header/payload read proof'
    for a,b in zip(old,native):
        assert not b.get('error'),b
        assert {key:b[key]for key in a if key!='address'}=={key:value for key,value in a.items()if key!='address'},'PDB read state differs'
        size=a['returned'];assert size in(64,4140) and a['position']==(64 if size==64 else 4204)and a['osfile']==1 and a['type']==1 and a['owned']==a['input_matches']==1
        assert a['header']==(master[:64]if size==64 else master[64:]).hex(),'PDB defined read bytes differ from complete frozen input'
    return old
