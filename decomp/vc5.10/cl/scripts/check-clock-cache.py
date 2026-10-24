#!/usr/bin/env python3
"""Focused cached-oracle and actual execution negatives; no original execution."""
import argparse,copy,json,shutil,tempfile,time
from pathlib import Path
import clock_provider as provider

parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('directory',type=Path);args=parser.parse_args()
started=time.perf_counter();passed=[];directory=args.directory.resolve();provider.verify(directory,cached_fixture=True)
def denied(label,path,change,check):
    raw=path.read_bytes()
    try:
        value=json.loads(raw);change(value);path.write_text(json.dumps(value,indent=2)+'\n')
        try:check()
        except(AssertionError,KeyError,ValueError,IndexError,StopIteration):passed.append(label)
        else:raise AssertionError('accepted '+label)
    finally:path.write_bytes(raw)
with tempfile.TemporaryDirectory(prefix='clock-cache-controls-',dir='/root/.ai-tmp/msvc2/vc5.10/cl')as temporary:
    project=Path(temporary);case=project/'cases/link-timing';case.mkdir(parents=True)
    shutil.copytree(provider.PROJECT/'cases/link-timing/reference',case/'reference')
    shutil.copyfile(provider.PROJECT/'cases/link-timing/dependency-packet.json',case/'dependency-packet.json')
    original_project=provider.PROJECT;provider.PROJECT=project
    try:
        provider.cached_packet()
        for label,name,change in[
            ('changed golden packet','dependency-packet.json',lambda v:v['child'].update(path='unknown.exe')),
            ('wrong packet binding','reference/fixture.json',lambda v:v.update(clock_response_sha256='0'*64)),
            ('missing provenance binding','reference/fixture.json',lambda v:v.pop('clock_response_provenance')),
            ('wrong provenance identity','reference/fixture.json',lambda v:v['clock_response_provenance'].update(sha256='0'*64)),
            ('wrong original receipt','reference/fixture.json',lambda v:v.update(capture_sha256='0'*64)),
            ('wrong raw inventory','reference/original-files.json',lambda v:v.update(source_capture_sha256='0'*64)),
            ('baseline receipt','reference/original/capture.json',lambda v:v.update(baseline=True)),
            ('changed decoder provenance','reference/packet-provenance.json',lambda v:v['sources']['clock_packet.py'].update(sha256='0'*64))]:
            denied(label,case/name,change,provider.cached_packet)
        source=case/'reference/packet-sources/clock_packet.py';raw=source.read_bytes()
        try:
            source.write_bytes(raw+b'\n')
            try:provider.cached_packet()
            except AssertionError:passed.append('changed owned decoder bytes')
            else:raise AssertionError('accepted changed owned decoder bytes')
        finally:source.write_bytes(raw)
        source.unlink()
        try:
            try:provider.cached_packet()
            except FileNotFoundError:passed.append('missing owned decoder')
            else:raise AssertionError('accepted missing owned decoder')
        finally:source.write_bytes(raw)
    finally:provider.PROJECT=original_project
for label,change in[
    ('missing child',lambda v:v['requests'].clear()),
    ('extra child',lambda v:v['requests'].append(copy.deepcopy(v['requests'][0]))),
    ('wrong child status',lambda v:v['requests'][0].update(status=2)),
    ('wrong child command',lambda v:v['requests'][0].update(argv=['wrong'])),
    ('wrong child state',lambda v:v['requests'][0]['parent_state_after'].update(last_error=2)),
    ('wrong broker wire',lambda v:v['wire'][0].update(payload_hex='00')),
    ('wrong output effect',lambda v:v['effects'][0]['output'].update(sha256='0'*64)),
    ('wrong response identity',lambda v:v.update(response_sha256='0'*64)),
    ('missing cache identity',lambda v:v.update(cached_fixture=False))]:
    denied(label,directory/'clock-native.json',change,lambda:provider.verify(directory,cached_fixture=True))
for name in ('MSVCRT.DLL','MSPDB50.DLL','test.obj','test.exe'):
    path=directory/name;raw=path.read_bytes()
    try:
        path.write_bytes(bytes([raw[0]^1])+raw[1:])
        try:provider.verify(directory,cached_fixture=True)
        except AssertionError:passed.append('changed actual '+name)
        else:raise AssertionError('accepted changed artifact '+name)
    finally:path.write_bytes(raw)
try:provider.verify(directory)
except AssertionError:passed.append('cached cannot become fresh acceptance')
else:raise AssertionError('cached fixture accepted as fresh')
provider.verify(directory,cached_fixture=True)
print(json.dumps({'passed':True,'negative_controls':len(passed),'seconds':time.perf_counter()-started,'controls':passed}))
