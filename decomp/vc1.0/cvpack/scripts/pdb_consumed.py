"""Exact concrete read of the provider's uninitialized incremental member.

Neither run is initialized by the observer. Equality describes the observed
allocation and read only, not a general constructor default.
"""
import hashlib,json,re
from state import BINARY

def validate(config,pe):
    r=config.get('pdb_consumed_observations')
    if not r:return None
    path=BINARY/r['evidence'];assert hashlib.sha256(path.read_bytes()).hexdigest()==r['evidence_sha256'],'stale consumed-field proof'
    p=json.loads(path.read_text())
    assert p['site']==0x2b1a7 and p['offset']==108 and p['size']==116
    assert pe.read(p['site'],4).hex()=='83796c00'
    assert p['native_line']==next(i for i,line in enumerate((BINARY/'src/pdb.c').read_text().splitlines(),1)if line.strip()=='if (server->incremental) {')
    assert set(p['sources'])=={'src/pdb.c','src/pdb.h'},'consumed-field source inventory differs'
    for source,digest in p['sources'].items():assert hashlib.sha256((BINARY/source).read_bytes()).hexdigest()==digest,'consumed-field source changed'
    return p

def compare(config,pe,events,native,required=False):
    proof=validate(config,pe);old=[]
    for line in events.splitlines():
        if not line.startswith('pdb-consumed '):continue
        m=re.fullmatch(r'pdb-consumed address ([0-9a-f]+) value (\d+) site ([0-9a-f]+)',line);assert m,'invalid consumed-value record'
        old.append(dict(address=int(m[1],16),value=int(m[2]),site=int(m[3],16)))
    assert proof or not(old or native),'unreviewed consumed-field observation'
    assert len(old)==len(native)==(1 if required else 0),'missing or extra consumed-field observations'
    for a,b in zip(old,native):
        assert not b.get('error'),b
        assert a['address'] and b['address'] and a['site']==b['site']==proof['site'],'consumed-field identity differs'
        assert a['value']==b['value'],'actual consumed undefined input differs; stop before accepting the consumer'
    return dict(original=old,native=native)
