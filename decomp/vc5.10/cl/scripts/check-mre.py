#!/usr/bin/env python3
"""Negative controls for declarations and the zero-call MRE dependency seam."""
import argparse
import json
from pathlib import Path
import shutil
import tempfile
from mre_imports import PROJECT,ROOT,declarations,verify
from state import project

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--original',type=Path,required=True)
parser.add_argument('--native',type=Path,required=True)
args=parser.parse_args()
count=verify(PROJECT,ROOT)['exports']
assert count==len(declarations()['exports'])
for platform in ('original','native'):
    state=project(platform,getattr(args,platform))
    assert state['fields']['external_mre_calls']==0
    assert state['fields']['globals']['minimal_rebuild']==0
passed=[]
with tempfile.TemporaryDirectory(prefix='mre-controls-',dir='/root/.ai-tmp/msvc2/vc5.10/cl')as directory:
    work=Path(directory)
    for name in ('cases/minimal-c/mre-imports.json','scripts/mspdb50.def'):
        path=work/name;path.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(PROJECT/name,path)
    original=(work/'cases/minimal-c/mre-imports.json').read_text()
    for label,change in [
        ('different provider',lambda p:p.update(provider_sha256='0'*64)),
        ('different export ordinal',lambda p:p['exports'][0].update(ordinal=72)),
        ('wrong external cleanup',lambda p:p['exports'][0].update(argument_bytes=20)),
        ('changed complete wrapper',lambda p:p['exports'][0].update(body_sha256='0'*64)),
        ('wrong qualified IAT',lambda p:p['exports'][0].update(target_iat=0x40a304))]:
        value=json.loads(original);change(value)
        (work/'cases/minimal-c/mre-imports.json').write_text(json.dumps(value))
        try:declarations(work,ROOT)
        except (AssertionError,KeyError,ValueError):passed.append(label)
        else:raise AssertionError('accepted '+label)
    for row in (-2,-1):
        for key,value in [('ordinal',0),('argument_bytes',4),('body_sha256','0'*64)]:
            proof=json.loads(original);proof['exports'][row][key]=value
            (work/'cases/minimal-c/mre-imports.json').write_text(json.dumps(proof))
            try:declarations(work,ROOT)
            except (AssertionError,KeyError,ValueError):passed.append('new import '+str(row)+' '+key)
            else:raise AssertionError('new import ABI mismatch accepted')
    # Clone only raw evidence for mutation; never edit an accepted receipt.
    copied=work/'original';shutil.copytree(args.original,copied)
    (copied/'mre-calls.txt').write_text('1\n')
    import hashlib
    receipt=json.loads((copied/'capture.json').read_text())
    receipt['files']['mre-calls.txt']={'bytes':2,'sha256':hashlib.sha256(b'1\n').hexdigest()}
    (copied/'capture.json').write_text(json.dumps(receipt))
    try:project('original',copied)
    except AssertionError:passed.append('unexpected original MRE call')
    else:raise AssertionError('nonzero MRE invocation accepted')
print('%d qualified stdcall imports and actual zero-call pair pass; %d negative controls pass'%(count,len(passed)))
