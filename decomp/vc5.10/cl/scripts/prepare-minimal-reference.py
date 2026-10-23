#!/usr/bin/env python3
"""Explicitly freeze a completed fresh original packet; never executes it."""
import argparse
import hashlib
import io
import json
from pathlib import Path
import tarfile
from dependency_response import decode
from mktemp_state import original as temporary
from state import vector,environment

PROJECT=Path(__file__).resolve().parents[1]
sha=lambda b:hashlib.sha256(b).hexdigest()
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('original',type=Path)
args=parser.parse_args();raw=args.original;response=decode(raw);assert response['completed']
reference=PROJECT/'cases/minimal-c/reference';reference.mkdir(exist_ok=True)
capture=json.loads((raw/'capture.json').read_text());config=json.loads((raw/'config.json').read_text())
trace=(raw/'trace.txt').read_bytes();assert len(trace.splitlines())==len(set(trace.splitlines()))
profile=json.loads((PROJECT/'cases/minimal-c/entry-profile.json').read_text())
master={'schema':1,'case':'minimal-c','target_sha256':capture['target_sha256'],
    'capture_sha256':sha((raw/'capture.json').read_bytes()),'reference_status':0,
    'required_accepted':[r['name']for r in config['driver']if r['native_symbol'].encode()in trace.splitlines()],
    'argv':vector(raw/'argv.bin')[1],'environment':environment(vector(raw/'environment.bin')[1]),
    'entry_profile':profile['id'],'program_path':(raw/'pgmptr.bin').read_bytes()[:-1].decode('ascii'),
    'dos_cwd':response['dos_cwd'],'temporary_name_boundary':temporary(raw),
    'native_resource_provider_sha256':sha((PROJECT/'build/resources.bin').read_bytes()),
    'streams':{name:sha((raw/(name+'.bin')).read_bytes())for name in ('stdout','stderr')},
    'trace_sha256':sha(trace),'object_normalization':'COFF TimeDateStamp only: bytes4..7 set to zero',
    'object_sha256':sha((raw/'test.obj').read_bytes()[:4]+b'\0'*4+(raw/'test.obj').read_bytes()[8:])}
for name in ('trace.txt','stdout.bin','stderr.bin','test.obj'):
    (reference/name).write_bytes((raw/name).read_bytes())
archive=reference/'original.tar.gz'
with tarfile.open(archive,'w:gz')as tar:
    for name in ['capture.json',*sorted(capture['files'])]:
        value=(raw/name).read_bytes();entry=tarfile.TarInfo('original/'+name)
        entry.size=len(value);entry.mtime=0;entry.mode=0o600
        tar.addfile(entry,io.BytesIO(value))
master['owned_original']={'file':'original.tar.gz','sha256':sha(archive.read_bytes()),'bytes':archive.stat().st_size,
    'files':len(capture['files'])+1}
(reference/'fixture.json').write_text(json.dumps(master,indent=2)+'\n')
(reference/'manifest.json').write_text(json.dumps({'schema':1,'target_sha256':master['target_sha256'],
    'streams':master['streams'],'status':0,'object_sha256':master['object_sha256'],
    'source_capture_sha256':master['capture_sha256']},indent=2)+'\n')
print(json.dumps({'owned_files':master['owned_original']['files'],'archive_bytes':archive.stat().st_size,
    'application_entries':len(trace.splitlines()),'reference_status':0}))
