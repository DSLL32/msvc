#!/usr/bin/env python3
"""Native-only regression against frozen original streams and application trace."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import time

BINARY=Path(__file__).resolve().parents[1]

def digest(data):return hashlib.sha256(data).hexdigest()

def main():
    begin=time.monotonic()
    case=BINARY/'cases/unsupported-help-diagnostic'
    manifest=json.loads((case/'case.json').read_text())
    reference=json.loads((case/'trace.json').read_text())
    configuration=json.loads((BINARY/'functions.json').read_text())
    original=BINARY/reference['evidence']
    receipt_bytes=(original/'receipt.json').read_bytes()
    assert digest(receipt_bytes)==reference['receipt_sha256'],'cached original receipt changed'
    receipt=json.loads(receipt_bytes)
    assert receipt['provider_sha256']==manifest['provider']['sha256']and receipt['target_sha256']==reference['target_sha256']
    raw=(original/'events.txt').read_bytes()
    assert digest(raw)==reference['events_sha256'],'cached original trace changed'
    captured_configuration=(original/'configuration.json').read_bytes()
    assert digest(captured_configuration)==reference['configuration_sha256']
    old=json.loads(captured_configuration)
    assert old['target']['sha256']==configuration['target']['sha256']==reference['target_sha256']
    assert re.findall(rb'^exit ([0-9a-f]+)$',raw,re.M)==[b'00000001']and b'\ntrap 'not in raw
    names={r['start']:r['name']for r in old['driver']if r['name']in old['accepted']and r['kind']=='application'}
    expected=[names[int(n,16)]for n in re.findall(rb'^entry ([0-9a-f]+)$',raw,re.M)if int(n,16)in names]
    assert expected and expected==reference['application_trace']
    current={r['name']:r for r in configuration['driver']if r['kind']=='application'}
    assert set(expected)<=current.keys()
    mapping={r['native']:r['name']for r in current.values()}
    assert len(mapping)==len(current),'ambiguous native application symbols'
    environment=case/'environment.bin'
    assert digest(environment.read_bytes())==manifest['environment']['sha256']
    executable=BINARY/'build/native/c2'
    provenance=json.loads((BINARY/'build/native/input-provenance.json').read_text())
    assert provenance['binary_sha256']==digest(executable.read_bytes())
    assert executable.read_bytes()[:6]==b'\x7fELF\x01\x01','fixture executable must be native ELF32'
    expected_streams={}
    for stream in ('stdout','stderr'):
        data=(case/(stream+'.bin')).read_bytes();record=manifest['expected'][stream]
        assert len(data)==record['bytes']and digest(data)==record['sha256']
        expected_streams[stream]=data
    scratch=Path.home()/'.ai-tmp/msvc2/vc5.10/c2';scratch.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='native-fixture-',dir=scratch)as temporary:
        temporary=Path(temporary);work=temporary/'work';work.mkdir()
        filter_file=temporary/'filter';filter_file.write_text(''.join(name+'\n'for name in sorted(mapping)))
        seen=temporary/'seen';seen.write_bytes(b'') # This case began with no accepted application prefix.
        env={k:v for k,v in os.environ.items()if not k.startswith('RECON_TRACE_')and k!='MSC_CMD_FLAGS'}
        env.update(C2_ENVIRONMENT_FILE=str(environment),C2_RESOURCE_FILE=str(BINARY/'build/messages.rmr'),RECON_TRACE_FILTER=str(filter_file),RECON_TRACE_SEEN=str(seen))
        with (temporary/'trace').open('w+b')as trace,tempfile.TemporaryFile()as shared:
            shared.truncate(32+len(mapping))
            env['RECON_TRACE_FD']=str(trace.fileno());env['RECON_TRACE_STATE_FD']=str(shared.fileno())
            setup=time.monotonic()-begin;start=time.monotonic()
            result=subprocess.run([str(executable),*manifest['arguments']],cwd=work,env=env,capture_output=True,pass_fds=(trace.fileno(),shared.fileno()),timeout=5)
            execution=time.monotonic()-start;start=time.monotonic()
            trace.seek(0);observed=[mapping[name]for name in trace.read().decode().splitlines()]
        assert result.returncode==manifest['expected']['status']==1,'native exit status differs'
        assert result.stdout==expected_streams['stdout']and result.stderr==expected_streams['stderr'],'native output differs'
        assert observed==expected,'native application entry trace differs'
        assert not list(work.rglob('*')),'unexpected output artifact'
        checks=time.monotonic()-start
    print(json.dumps({'case':manifest['id'],'status':'PASS','application_entries':len(observed),'stdout_bytes':len(result.stdout),'stderr_bytes':len(result.stderr),'seconds':{'setup':setup,'execution':execution,'checks':checks,'total':time.monotonic()-begin},'transport':'native ELF32 only; frozen original reference'},indent=2))

if __name__=='__main__':main()
