#!/usr/bin/env python3
"""Compile frozen native source bytes and keep immutable binary/source receipts."""
import hashlib
import io
import errno
import shutil
import json
import os
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile
import time
from build_inputs import store_blob

PROJECT=Path(__file__).resolve().parents[1]
ROOT=PROJECT.parents[2]
SCRATCH=Path('/root/.ai-tmp/msvc2/vc5.10/cl')
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
args=sys.argv[1:]
assert args and '-o' in args,'native compiler command with explicit output required'
output=(PROJECT/args[args.index('-o')+1]).resolve()
compile_only='-c' in args
revision=subprocess.check_output(['git','rev-parse',os.environ['RECON_SHARED_REVISION']],cwd=ROOT,text=True).strip()
tree=subprocess.check_output(['git','ls-tree','-r',revision,'lib/recon'],cwd=ROOT,text=True)
shared={}
for line in tree.splitlines():
    metadata,name=line.split('\t',1)
    mode,kind,blob=metadata.split()
    if kind=='blob' and (name.endswith('.h') or name.startswith('lib/recon/src/') and name.endswith('.c')):
        shared[name]=blob
cache=SCRATCH/'shared-source-checkouts'/revision
if not cache.exists():
    temporary=Path(tempfile.mkdtemp(prefix='checkout-',dir=SCRATCH))
    archive=subprocess.check_output(['git','archive',revision,'lib/recon'],cwd=ROOT)
    with tarfile.open(fileobj=io.BytesIO(archive)) as tar:tar.extractall(temporary,filter='data')
    cache.parent.mkdir(parents=True,exist_ok=True)
    try:temporary.rename(cache)
    except OSError as error:
        if error.errno not in (errno.EEXIST,errno.ENOTEMPTY):raise
        shutil.rmtree(temporary)
sources={}
for name,blob in shared.items():
    data=(cache/name).read_bytes()
    assert hashlib.sha1(b'blob '+str(len(data)).encode()+b'\0'+data).hexdigest()==blob,'frozen shared checkout differs from Git object'
    sources[str(ROOT/name)]=store_blob(data)
for path in [PROJECT/'Makefile',*list((PROJECT/'src').glob('*.h'))]:
    sources[str(path)]=store_blob(path.read_bytes())
for argument in args:
    if argument.endswith('.c'):
        path=(PROJECT/argument).resolve()
        if not path.is_relative_to(ROOT/'lib/recon'):sources[str(path)]=store_blob(path.read_bytes())
mirror=Path(tempfile.mkdtemp(prefix='native-source-',dir=SCRATCH))
for path,item in sources.items():
    relative=Path(path).relative_to(ROOT)
    destination=mirror/relative;destination.parent.mkdir(parents=True,exist_ok=True)
    os.link(item['path'],destination)
objects={}
for argument in args:
    if argument.endswith('.o') and (PROJECT/argument).resolve()!=output:
        path=(PROJECT/argument).resolve();record=Path(str(path)+'.build.json')
        value=json.loads(record.read_text())
        assert value['output_sha256']==sha(path) and value['shared_revision']==revision,'input object provenance differs'
        for name,item in value['sources'].items():
            if name in sources:assert sources[name]['sha256']==item['sha256'],'object/header source versions differ'
        objects[str(path)]={'sha256':sha(path),'receipt':store_blob(record.read_bytes())}
actual=[]
for argument in args:
    if argument.endswith('.c'):
        path=(PROJECT/argument).resolve()
        if str(path) not in sources:continue # Added files outside the selected shared release.
        actual.append(str(mirror/path.relative_to(ROOT)))
    elif argument.startswith('-I'):
        path=(PROJECT/argument[2:]).resolve()
        actual.append('-I'+str(mirror/path.relative_to(ROOT)))
    else:actual.append(argument)
actual.append('-fdebug-prefix-map='+str(mirror)+'='+str(ROOT))
started=time.perf_counter();result=subprocess.run(actual,cwd=PROJECT);elapsed=time.perf_counter()-started
if result.returncode:
    if output.exists():output.unlink()
    raise SystemExit(result.returncode)
receipt={'schema':1,'kind':'object' if compile_only else 'linked-native','shared_revision':revision,
    'logical_command':args,'command':actual,'source_tree':str(mirror),'seconds':elapsed,
    'output_sha256':sha(output),'sources':sources,'objects':objects,
    'builder_sha256':sha(Path(__file__)),'builder_source':store_blob(Path(__file__).read_bytes())}
Path(str(output)+'.build.json').write_text(json.dumps(receipt,indent=2)+'\n')
