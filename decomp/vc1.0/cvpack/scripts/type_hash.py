"""The reached record hash's three contextual linear lookup transforms."""
import hashlib,json
from state import BINARY


def validate(config,pe):
    ref=config.get('type_hash_observations')
    required=any(r['name']=='hash_type'and r['status']!='stub'for r in config['functions'])
    assert ref or not required,'missing type hash table proof'
    if not ref:return
    path=BINARY/ref['evidence']
    assert hashlib.sha256(path.read_bytes()).hexdigest()==ref['evidence_sha256'],'stale type hash table proof'
    proof=json.loads(path.read_text())
    assert proof['schema']==1
    assert hashlib.sha256((BINARY/proof['source']).read_bytes()).hexdigest()==proof['source_sha256'],'type hash source differs'
    body=proof['original_body']
    assert (body['start'],body['end'])==(0x1c1a0,0x1c4ac)
    assert hashlib.sha256(pe.read(body['start'],body['end']-body['start'])).hexdigest()==body['sha256'],'type hash body differs'
    expected=[('type_hash_low_mix',0x374f8),('type_hash_high_mix',0x375f8),('type_hash_byte',0x376f8)]
    assert [(r['name'],r['original'])for r in proof['tables']]==expected,'missing or unknown type hash transform'
    data={r['name']:r for r in json.loads((BINARY/'data.json').read_text())}
    for table in proof['tables']:
        assert table['size']==256 and len(table['basis'])==8 and all(isinstance(c,int)and 0<=c<256 for c in table['basis']),'unreviewed transform extent/columns'
        item=data[table['name']]
        assert item['original']==table['original']and item['size']==256 and item['source']=='src/types.c','type hash table owner differs'
        raw=pe.read(table['original'],256)
        assert hashlib.sha256(raw).hexdigest()==table['sha256'],'type hash original table differs'
        expanded=[]
        for value in range(256):
            result=0
            for bit,column in enumerate(table['basis']):
                if value&(1<<bit):result^=column
            expanded.append(result)
        assert bytes(expanded)==raw,'type hash basis does not define original table'
    return proof
