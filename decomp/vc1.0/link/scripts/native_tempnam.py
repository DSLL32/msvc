"""Bind the narrow shared tempnam profile to independently captured case input."""
import hashlib
import json
from pathlib import Path

BINARY=Path(__file__).resolve().parents[1]
BASE=BINARY/'cases/resource-identity/tempnam'
OMF_BASE=BINARY/'cases/omf-continuation/tempnam'


def install(metadata,case,environment):
    if 'native_tempnam_input' not in metadata:return None
    record=metadata['native_tempnam_input'];path=(BINARY/record['path']).resolve()
    allowed=OMF_BASE if case in ('omf-continuation-continued-controlled','omf-continuation-use16-controlled') else BASE
    assert path.is_relative_to(allowed.resolve())
    data=path.read_bytes();assert len(data)==record['bytes'] and hashlib.sha256(data).hexdigest()==record['sha256']
    proof=json.loads(data);assert proof['case']==case and proof['status']=='pass'
    assert case in ('resource-identity-languages-controlled','resource-identity-duplicate-controlled','omf-continuation-continued-controlled','omf-continuation-use16-controlled')
    assert proof['original_sha256']==json.loads((BINARY/'functions.json').read_text())['target']['sha256']
    events_path=(path.parent/proof['events']['path']).resolve();assert events_path.is_relative_to(allowed.resolve())
    raw=events_path.read_bytes();assert hashlib.sha256(raw).hexdigest()==proof['events']['sha256']
    events=[json.loads(s) for s in raw.decode().splitlines()]
    observations=[e for e in events if e['event']=='resource_tempnam'];assert len(observations)==2
    before,after=observations;assert before['before']==1 and after['before']==0
    assert before['directory']=='\\' and before['prefix']=='tmp'
    assert before['errno']==before['doserrno']==before['last_error']==0
    canonical='S:\\vc1.0\\link\\'+case
    assert environment['TMP']==environment['TEMP']==canonical
    assert after['result']==canonical+'\\tmp2' and after['errno']==after['doserrno']==after['last_error']==2
    paths=[e for e in events if e['event']=='resource_tempnam_path'];assert len(paths)==2
    assert paths[0]['site']==0x5b321 and paths[0]['path']==canonical and paths[0]['counter']==1 and paths[0]['attributes']==16
    assert paths[1]['site']==0x5b51e and paths[1]['path']==after['result'] and paths[1]['counter']==2
    assert paths[1]['eax']==paths[1]['attributes']==0xffffffff
    assert paths[1]['errno']==paths[1]['doserrno']==paths[1]['last_error']==2
    environment['RECON_TEMPNAM_PROFILE']='libc100'
    return dict(profile='libc100',proof=record['path'],proof_sha256=record['sha256'],observed_TMP=canonical,
                before_counter=1,after_counter=2,errors_after=dict(errno=2,doserrno=2,last_error=2),
                scope='Original TMP-first tempnam with actual empty-directory/counter/access observations; no candidate seed or filename normalization')
