#!/usr/bin/env python3
"""Keep the failed UTC0 native observation alongside its later valid proof."""
import hashlib,json
from pathlib import Path

PROJECT=Path(__file__).resolve().parents[1];ROOT=PROJECT.parents[2]
sha=lambda raw:hashlib.sha256(raw).hexdigest()
source=Path('/root/.ai-tmp/msvc2/vc5.10/cl/start-timer-1-native')
destination=PROJECT/'cases/link-timing/failures/utc0'
assert not destination.exists()
capture=json.loads((source/'capture.json').read_bytes())
assert capture['case']=='link-timing'and capture['controller_error']is not None
proof=PROJECT/'cases/link-timing/milestones/start-timer/manifest.json'
assert any(row['sha256']==capture['native_sha256']for row in json.loads(proof.read_bytes())['records'].values())
originals={sha(p.read_bytes()):p for p in sorted((ROOT/'toolchains/5.10').rglob('*'))if p.is_file()}
destination.mkdir(parents=True);records={}
for name in['capture.json',*capture['files']]:
    relative=Path(name);assert not relative.is_absolute()and'..'not in relative.parts
    raw=(source/name).read_bytes();digest=sha(raw)
    if name!='capture.json':assert capture['files'][name]=={'bytes':len(raw),'sha256':digest}
    row={'bytes':len(raw),'sha256':digest}
    if digest in originals:row['repository_original']=str(originals[digest].relative_to(ROOT))
    else:
        target=destination/'raw'/relative;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(raw)
        row['file']='raw/'+name
    records[str(source/name)]=row
manifest={'kind':'Rejected native UTC0 observation; never acceptance',
    'records':records,'compiled_binary_proof':{'path':str(proof.relative_to(PROJECT)),'sha256':sha(proof.read_bytes())},
    'reason':'Actual _ftime entry errno0/DOS2/Last0 returned errno2/DOS2/Last0; timestamps/FP matched. Later host-only UTC input matched without API or target-state reset.',
    'owned_payload_bytes':sum(row['bytes']for row in records.values()if'file'in row)}
(destination/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(json.dumps({'records':len(records),'owned_payload_bytes':manifest['owned_payload_bytes']}))
