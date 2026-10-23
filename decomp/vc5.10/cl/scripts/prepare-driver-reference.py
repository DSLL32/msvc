#!/usr/bin/env python3
"""Explicitly freeze a completed original driver-only case; no execution."""
import argparse
import hashlib
import io
import json
from pathlib import Path
import tarfile
from case_inputs import load
from state import vector,environment,project
from mktemp_state import original as temporary

PROJECT=Path(__file__).resolve().parents[1]
sha=lambda value:hashlib.sha256(value).hexdigest()
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('original',type=Path)
args=parser.parse_args();raw=args.original;capture=json.loads((raw/'capture.json').read_text());case=load(capture['case'])
assert not capture['baseline']and not capture['dependency_response']and capture['controller_error']is None
assert project('original',raw)['fields']['status']==0
config=json.loads((raw/'config.json').read_text());reference=PROJECT/'cases'/case['case']/'reference';reference.mkdir(exist_ok=True)
for name,item in capture['files'].items():
 value=(raw/name).read_bytes();assert item=={'bytes':len(value),'sha256':sha(value)}
trace=(raw/'trace.txt').read_bytes();assert len(trace.splitlines())==len(set(trace.splitlines()))
profile=json.loads((PROJECT/'cases'/case['case']/'entry-profile.json').read_text())
master={'schema':1,**case,'target_sha256':capture['target_sha256'],'reference_status':0,
 'required_accepted':[row['name']for row in config['driver']if row['native_symbol'].encode()in trace.splitlines()],
 'argv':vector(raw/'argv.bin')[1],'environment':environment(vector(raw/'environment.bin')[1]),
 'entry_profile':profile['id'],'program_path':(raw/'pgmptr.bin').read_bytes()[:-1].decode('ascii'),
 'temporary_name_boundary':temporary(raw),'native_resource_provider_sha256':sha((PROJECT/'build/resources.bin').read_bytes()),
 'streams':{name:sha((raw/(name+'.bin')).read_bytes())for name in ('stdout','stderr')},'trace_sha256':sha(trace),
 'capture_sha256':sha((raw/'capture.json').read_bytes())}
for name in ('trace.txt','stdout.bin','stderr.bin'):(reference/name).write_bytes((raw/name).read_bytes())
archive=reference/'original.tar.gz'
with tarfile.open(archive,'w:gz')as tar:
 for name in ['capture.json',*sorted(capture['files'])]:
  value=(raw/name).read_bytes();entry=tarfile.TarInfo('original/'+name);entry.size=len(value);entry.mode=0o600;entry.mtime=0;tar.addfile(entry,io.BytesIO(value))
master['owned_original']={'file':archive.name,'bytes':archive.stat().st_size,'sha256':sha(archive.read_bytes()),'files':len(capture['files'])+1}
(reference/'fixture.json').write_text(json.dumps(master,indent=2)+'\n')
(reference/'manifest.json').write_text(json.dumps({'streams':master['streams'],'status':0,'target_sha256':master['target_sha256'],'source_capture_sha256':master['capture_sha256']},indent=2)+'\n')
print(json.dumps({'case':case['case'],'application_entries':len(trace.splitlines()),'owned_files':len(capture['files'])+1}))
