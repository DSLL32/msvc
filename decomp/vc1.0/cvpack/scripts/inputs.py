"""Immutable ancillary Windows inputs in an isolated native drive namespace."""
import hashlib,json
from pathlib import Path,PureWindowsPath
from state import BINARY,ROOT,load,sha

def specification(record):
    config,_=load();entry=config.get('external_inputs')
    if not entry:return None
    path=BINARY/entry['evidence']
    assert sha(path)==entry['evidence_sha256'],'stale ancillary input proof'
    proof=json.loads(path.read_text())
    if record['name']!=proof['case']:return None
    assert record['inputs'][0]['sha256']==proof['primary_sha256'],'ancillary primary input differs'
    return proof

def frozen(item):
    source=BINARY/item['owned']
    assert source.stat().st_size==item['bytes'] and sha(source)==item['sha256'],'missing/stale frozen ancillary input'
    return source

def namespace(work):return work.with_name(work.name+'-drive-z')

def relative(item):
    path=PureWindowsPath(item['windows'])
    assert path.drive.upper()=='Z:' and path.is_absolute(),'unsupported ancillary input drive'
    assert '..'not in path.parts and '.'not in path.parts,'unbounded ancillary filename'
    return Path(*path.parts[1:])

def prepare(record,work):
    proof=specification(record)
    if not proof:return
    tree=namespace(work);tree.mkdir()
    for item in proof['inputs']:
        target=tree/relative(item);target.parent.mkdir(parents=True,exist_ok=True)
        target.write_bytes(frozen(item).read_bytes())
    verify(record,work)

def verify(record,work):
    proof=specification(record)
    if not proof:return
    expected={str(relative(item)):item for item in proof['inputs']}
    tree=namespace(work)
    files={str(p.relative_to(tree)):p for p in tree.rglob('*')if p.is_file()}
    assert set(files)==set(expected),'ancillary artifact set changed'
    for name,path in files.items():
        item=expected[name];frozen(item)
        assert path.stat().st_size==item['bytes'] and sha(path)==item['sha256'],'ancillary input altered'

def environment(record,work,env):
    if specification(record):
        verify(record,work)
        env['RECON_DRIVE_Z']=str(namespace(work))

def original(record):
    proof=specification(record)
    if not proof:return []
    receipts=[]
    for item in proof['inputs']:
        frozen(item)
        host=Path(item['capture_host_path'])
        assert host.stat().st_size==item['bytes'] and sha(host)==item['sha256'],'original ancillary input differs from owned snapshot'
        receipts.append(dict(windows=item['windows'],bytes=item['bytes'],sha256=item['sha256']))
    return receipts
