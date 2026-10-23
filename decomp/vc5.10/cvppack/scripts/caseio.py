"""Frozen CLI-case inputs and artifacts; no process or original execution."""
import json
from pathlib import Path
import shutil
from state import BINARY,sha


def definition(name):
    assert name in ('help','hello-debug-dll')
    if name=='help':return dict(schema=1,name='help',argv=['/help'],inputs=[],artifacts=[],master=dict(status=1,stdout='cases/help/original/stdout',stderr='cases/help/original/stderr'))
    return json.loads((BINARY/'cases'/name/'input.json').read_text())


def prepare(case,directory):
    for item in case['inputs']:
        assert Path(item['name']).name==item['name']
        source=BINARY/item['source']
        assert source.stat().st_size==item['bytes'] and sha(source)==item['sha256'],'changed frozen case input'
        shutil.copyfile(source,directory/item['name'])


def artifacts(case,work,output):
    result=[]
    for name in case['artifacts']:
        assert Path(name).name==name
        path=work/name
        assert path.is_file(),'missing target artifact'
        captured='artifact-'+name
        shutil.copyfile(path,output/captured)
        result.append(dict(name=captured,bytes=(output/captured).stat().st_size,sha256=sha(output/captured)))
    return result


def master(case):
    value=case['master']
    for kind in ('stdout','stderr'):
        raw=(BINARY/value[kind]).read_bytes()
        expected=value.get(kind+'_sha256')
        if expected:assert sha(BINARY/value[kind])==expected
    return value
