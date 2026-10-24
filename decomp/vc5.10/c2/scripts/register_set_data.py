"""Finite encountered register-set header/free-head ownership, not full table layout."""
import json,re,struct
from state import BINARY,TARGET,sha


def verify(config):
    name='@new_register_set@4'
    if name not in config['accepted']+config.get('trial',[]):return None
    row=next(r for r in config['driver']if r['name']==name)
    raw=(BINARY/row['register_set_model']).read_bytes();assert sha(raw)==row['register_set_model_sha256']
    value=json.loads(raw);assert value['target_sha256']==sha(TARGET.read_bytes())
    for file,digest in value['source_files'].items():assert sha((BINARY/file).read_bytes())==digest,'register-set source/projection guard changed without contextual proof'
    allowed=set(value['source_files'])|{'src/function-records.h','src/function-records.c'}
    tokens=r'\b(?:C2RegisterSet|C2RegisterSetNode|free_register_sets|C2_REGISTER_SET_POOL|new_register_set)\b'
    for path in (BINARY/'src').rglob('*'):
        if path.suffix not in ('.c','.h')or str(path.relative_to(BINARY))in allowed:continue
        text=path.read_text()
        if path.name=='stubs.c':text=re.sub(r'^void __fastcall clear_register_set\(C2RegisterSet \*set\)$','',text,flags=re.M)
        assert not re.search(tokens,text),'unmodeled register-set consumer/index/escape'
    # Existing caller bytes are separately frozen by the block model; enforce its finite calls too.
    calls=re.findall(r'new_register_set\(([^)]*)\)',(BINARY/'src/function-records.c').read_text())
    assert calls==['1','1'],'new caller/pool requires encountered table ownership first'
    stubs=(BINARY/'src/stubs.c').read_text();assert 'new_register_set('not in stubs
    from data_evidence import reader
    assert reader(TARGET)(0x48e124,4)==bytes(4),'original encountered free-head default differs'
    # Canonical object is a two-cell projection, not a claimed original array extent.
    maps={m[1]:int(m[2],16)for m in re.finditer(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',(BINARY/'build/c2.map').read_text(),re.M)}
    address=maps['_free_register_sets']+4
    assert reader(BINARY/'build/c2.exe')(address,4)==bytes(4),'rebuilt encountered free-head default differs'
    return {'original_slot':0x48e124,'rebuilt_slot':address,'credit_bytes':4,'pool':1,'unmodeled':'All other cells; native guard-before-read required. No full table or object extent credit.','model_sha256':sha(raw)}
