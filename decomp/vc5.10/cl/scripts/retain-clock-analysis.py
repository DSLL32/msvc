#!/usr/bin/env python3
"""Retain baseline-only timer evidence without duplicating repository originals."""
import hashlib
import json
from pathlib import Path
import shutil

PROJECT=Path(__file__).resolve().parents[1];ROOT=PROJECT.parents[2]
SCRATCH=Path('/root/.ai-tmp/msvc2/vc5.10/cl')
sha=lambda data:hashlib.sha256(data).hexdigest()

def retain(source,destination):
    capture=json.loads((source/'capture.json').read_bytes())
    assert capture['case']=='link-timing' and capture['baseline']is True and capture['clock_analysis']is True
    assert not destination.exists(),'historical evidence is immutable'
    originals={sha(p.read_bytes()):p for p in sorted((ROOT/'toolchains/5.10').rglob('*'))if p.is_file()}
    destination.mkdir(parents=True);records={}
    def add(path,name,expected=None):
        raw=path.read_bytes();digest=sha(raw)
        if expected:assert expected=={'bytes':len(raw),'sha256':digest}
        record={'bytes':len(raw),'sha256':digest}
        if digest in originals:record['repository_original']=str(originals[digest].relative_to(ROOT))
        else:
            target=destination/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(raw)
            record['file']=name
        records[str(path)]=record
    add(source/'capture.json','raw/capture.json')
    for name,expected in capture['files'].items():
        relative=Path(name);assert not relative.is_absolute() and '..'not in relative.parts
        add(source/name,'raw/'+name,expected)
    # These two content-addressed source snapshots actually produced this
    # execution. Native/rebuilt/build_dependency_hashes in its general receipt
    # are informational: neither reconstruction ran in baseline analysis.
    for field in ('controller_source','helper_source'):
        binding=capture[field];p=Path(binding['path'])
        assert sha(p.read_bytes())==binding['sha256']
        add(p,'execution-sources/'+field+'.txt')
    for name in ('link-clock-analysis-1-packet.json','clock-consumers.json','clock-packet-controls.json'):
        add(SCRATCH/name,'decoded/'+name)
    manifest={'kind':'Baseline-only original clock/LINK analysis; not a fresh acceptance or native oracle',
        'source_capture':str(source/'capture.json'),'capture_sha256':sha((source/'capture.json').read_bytes()),
        'records':records,'informational_capture_fields':['native_sha256','rebuilt_sha256','build_dependency_hashes'],
        'owned_payload_bytes':sum(row['bytes']for row in records.values()if 'file'in row)}
    (destination/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    return manifest

if __name__=='__main__':
    source=SCRATCH/'link-clock-analysis-1';destination=PROJECT/'cases/link-timing/analysis'
    result=retain(source,destination)
    print(json.dumps({'manifest':str(destination/'manifest.json'),'records':len(result['records']),
        'owned_payload_bytes':result['owned_payload_bytes']}))
