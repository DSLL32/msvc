"""Authored CL case inputs; no outputs or original execution at import time."""
import hashlib
import json
from pathlib import Path

PROJECT=Path(__file__).resolve().parents[1]


def load(name):
    plan=json.loads((PROJECT/'cases/plan.json').read_text())
    assert name in plan['authorized_queue'],'case is not authorized'
    themes=[item for item in plan['themes'] if item['id']==name]
    assert len(themes)==1,'unknown/ambiguous case'
    theme=themes[0]
    inputs=[]
    if theme['source']:
        path=PROJECT/'cases'/theme['source']
        inputs.append({'name':path.name,'source':str(path.relative_to(PROJECT)),
            'bytes':path.stat().st_size,'sha256':hashlib.sha256(path.read_bytes()).hexdigest()})
    return {'case':name,'arguments':theme['arguments'],'inputs':inputs}


def copy_inputs(case,directory):
    for item in case['inputs']:
        assert Path(item['name']).name==item['name']
        raw=(PROJECT/item['source']).read_bytes()
        assert len(raw)==item['bytes'] and hashlib.sha256(raw).hexdigest()==item['sha256']
        path=Path(directory)/item['name']
        assert not path.exists(),'case input collides with capture control file'
        path.write_bytes(raw)
