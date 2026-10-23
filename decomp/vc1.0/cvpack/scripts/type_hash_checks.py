"""Reject changed basis/ownership/provenance, retaining full table-byte checks."""
import copy,hashlib,json,tempfile
from pathlib import Path
from state import BINARY
from type_hash import validate


def check(config,pe,reject):
    ref=config.get('type_hash_observations')
    if not ref:return
    validate(config,pe)
    proof=json.loads((BINARY/ref['evidence']).read_text())
    changed=copy.deepcopy(config);changed.pop('type_hash_observations')
    reject('type hash proof missing',lambda:validate(changed,pe))
    changed=copy.deepcopy(config);changed['type_hash_observations']['evidence_sha256']='0'*64
    reject('type hash proof stale',lambda:validate(changed,pe))
    scratch=Path.home()/'.ai-tmp/vc1.0-cvpack'
    with tempfile.TemporaryDirectory(prefix='type-hash-proof-',dir=scratch)as folder:
        path=Path(folder)/'proof.json'
        def altered(label,change):
            candidate=copy.deepcopy(proof);change(candidate);path.write_text(json.dumps(candidate))
            current=copy.deepcopy(config);current['type_hash_observations']=dict(evidence=str(path),evidence_sha256=hashlib.sha256(path.read_bytes()).hexdigest())
            reject(label,lambda:validate(current,pe))
        altered('type hash changed basis column',lambda p:p['tables'][0]['basis'].__setitem__(0,2))
        altered('type hash missing transform',lambda p:p['tables'].pop())
        altered('type hash wrong table extent',lambda p:p['tables'][0].__setitem__('size',128))
        altered('type hash changed source binding',lambda p:p.__setitem__('source_sha256','0'*64))
        altered('type hash wrong original body binding',lambda p:p['original_body'].__setitem__('sha256','0'*64))
