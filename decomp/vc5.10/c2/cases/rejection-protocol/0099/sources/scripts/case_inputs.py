"""Frozen case data and artifact checks; contains no execution transport."""
import hashlib
import json
from pathlib import Path
import shutil

BINARY=Path(__file__).resolve().parents[1]

def sha(data):return hashlib.sha256(data).hexdigest()

def manifest(config):
    record=config['case']
    directory=BINARY/record.get('directory','cases/unsupported-help-diagnostic')
    assert directory.resolve().parent==(BINARY/'cases').resolve(),'case outside binary cases'
    raw=(directory/'case.json').read_bytes()
    if 'manifest_sha256'in record:assert sha(raw)==record['manifest_sha256'],'case manifest changed'
    value=json.loads(raw);assert value['id']==record['id']
    assert value['arguments']==record['arguments']and value['expected']['status']==record['status']
    assert value['environment']['MSC_CMD_FLAGS']==record['environment']['MSC_CMD_FLAGS']
    environment=(directory/value['environment']['raw_vector']).read_bytes()
    assert sha(environment)==value['environment']['sha256']and environment.endswith(b'\0\0')
    for item in value.get('inputs',[]):
        data=(directory/item['path']).read_bytes()
        assert len(data)==item['bytes']and sha(data)==item['sha256']
        assert Path(item['working_name']).name==item['working_name']
    if 'source'in value:
        source=directory/value['source']['path'];assert sha(source.read_bytes())==value['source']['sha256']
    for stream in ('stdout','stderr'):
        path=directory/('reference/'+stream+'.bin'if record['id']!='unsupported-help-diagnostic'else stream+'.bin')
        data=path.read_bytes();expected=value['expected'][stream]
        assert len(data)==expected['bytes']and sha(data)==expected['sha256']
    for item in value['expected'].get('artifacts',[]):
        data=(directory/item['path']).read_bytes()
        assert len(data)==item['bytes']and sha(data)==item['sha256']
    return directory,value,environment

def materialize(directory,value,work):
    work.mkdir(parents=True,exist_ok=True)
    inputs=[]
    for item in value.get('inputs',[]):
        shutil.copyfile(directory/item['path'],work/item['working_name']);inputs.append(item['working_name'])
    if 'source'in value:
        source=directory/value['source']['path'];shutil.copyfile(source,work/source.name);inputs.append(source.name)
    for item in value['expected'].get('artifacts',[]):
        name=item['output_name'];assert Path(name).name==name
        output=work/name
        if output.exists():assert output.is_file();output.unlink()
    return inputs

def artifacts(work,value,output):
    result=[]
    for number,item in enumerate(value['expected'].get('artifacts',[])):
        path=work/item['output_name'];record={'output_name':item['output_name'],'present':path.exists()}
        if path.exists():
            data=path.read_bytes();name='artifact-%d.bin'%number;(output/name).write_bytes(data)
            record.update(path=name,bytes=len(data),sha256=sha(data))
        result.append(record)
    return result

def normalized(data,item):
    result=bytearray(data)
    for field in item.get('normalize',[]):
        assert item['kind']=='i386-coff-object'and field=={'name':'IMAGE_FILE_HEADER.TimeDateStamp','offset':4,'bytes':4},'unknown normalization'
        assert len(data)>=20 and data[:2]==b'\x4c\x01','normalization needs a real i386 COFF header'
        result[4:8]=bytes(4)
    return bytes(result)
