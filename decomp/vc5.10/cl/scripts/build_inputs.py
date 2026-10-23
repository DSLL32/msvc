"""Shared input inventory for this binary's capture and acceptance controllers."""
import hashlib
import os
import tempfile
from pathlib import Path


def inventory(project,root):
    paths=[p for p in (project/'src').glob('*')if p.is_file()]
    paths+=[project/'Makefile',*list((root/'lib/recon/src').glob('*.c')),
        *list((root/'lib/recon/include').glob('*.h')),*list((root/'lib/recon/native').rglob('*.h'))]
    return {str(p):hashlib.sha256(p.read_bytes()).hexdigest()for p in sorted(paths)}

STORE=Path('/root/.ai-tmp/msvc2/vc5.10/cl/build-source-blobs')


def store_blob(data):
    digest=hashlib.sha256(data).hexdigest()
    STORE.mkdir(parents=True,exist_ok=True)
    path=STORE/digest
    if not path.exists():
        descriptor,temporary=tempfile.mkstemp(prefix='source-',dir=STORE)
        try:
            with os.fdopen(descriptor,'wb') as file:file.write(data)
            try:os.link(temporary,path)
            except FileExistsError:pass
        finally:Path(temporary).unlink()
    assert hashlib.sha256(path.read_bytes()).hexdigest()==digest,'immutable build source blob changed'
    return {'path':str(path),'sha256':digest,'bytes':len(data)}


def verify_blob(item):
    path=Path(item['path']);data=path.read_bytes()
    assert len(data)==item['bytes'] and hashlib.sha256(data).hexdigest()==item['sha256'],'compiled source provenance changed'
    return data


def verify_native_build(receipt,expected_binary,project):
    verify_blob(receipt['builder_source'])
    assert receipt['kind']=='linked-native' and receipt['output_sha256']==expected_binary,'native binary/build receipt differs'
    all_sources={}
    for name,item in receipt['sources'].items():
        verify_blob(item);all_sources[name]=item['sha256']
    for object in receipt['objects'].values():
        import json
        entry=json.loads(verify_blob(object['receipt']))
        verify_blob(entry['builder_source'])
        assert entry['kind']=='object' and entry['output_sha256']==object['sha256'],'object provenance differs'
        for name,item in entry['sources'].items():
            verify_blob(item);all_sources[name]=item['sha256']
    # Current reconstructed source must match the compiled source. Shared source
    # may legitimately have advanced; its immutable compiled contents stay bound.
    for path in (project/'src').glob('*'):
        if path.suffix in ('.c','.h'):
            assert all_sources.get(str(path))==hashlib.sha256(path.read_bytes()).hexdigest(),'application source changed since native build'
    command=receipt['command']
    assert all(flag in command for flag in ('-m32','-O0','-g')),'wrong native architecture/debug build'
    return all_sources
