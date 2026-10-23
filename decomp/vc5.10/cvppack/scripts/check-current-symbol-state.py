#!/usr/bin/env python3
"""Owned active symbol arguments and unaccepted tail-extent controls, native only."""
import copy
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import time
from audit import graph_driver
from binary import PE
from state import BINARY, ROOT, sha

start=time.perf_counter()
pair=BINARY/'cases/hello-debug-dll/evidence/0077-rewrite-type-name-symbol-tail-proof'
config=json.loads((pair/'original/configuration.json').read_text())
data=json.loads((pair/'original/data.json').read_text())
events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py')
decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()}
views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in [('original','events-view-*.bin'),('native','native-view-*.bin')]}
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
baseline=compare(native);assert baseline['parameters']['record']==dict(root='type_data',offset=602,bytes='1700031014100000107461675250434f4c454d455353414745')
rejected=[]
def reject(name,mutate,reason,raw=events):
    state=copy.deepcopy(native);mutate(state)
    try:compare(state,raw)
    except (AssertionError,KeyError) as error:
        assert reason in str(error),('wrong rejection guard',name,str(error))
        rejected.append(dict(name=name,diagnostic=str(error)))
    else:raise AssertionError('accepted '+name)
for field,value in [('root','line_data'),('offset',603),('bytes','1700031015100000107461675250434f4c454d455353414745')]:
    reject('wrong-record-'+field,lambda s,f=field,v=value:s['parameters']['record'].update({f:v}),'typed ABI arguments differ')
reject('wrong-callback-identity',lambda s:s['parameters']['key'].update(function='public_record_key'),'typed ABI arguments differ')
reject('wrong-record-pointee-width',lambda s:s['observed_argument_types'][0].update(pointee_width=4),'actual frontier pointee extent differs')
original=decoder.decode(events)
def original_register(index,value):
    lines=events.splitlines();i=max(i for i,line in enumerate(lines) if line.startswith('registers '))
    fields=lines[i].split();fields[index+1]='%08x'%value;lines[i]=' '.join(fields)
    return '\n'.join(lines)+'\n'
def original_memory(address,offset,raw):
    match=list(re.finditer(r'^memory %08x ([0-9a-f]+)$'%address,events,re.M));assert match
    begin,end=match[-1].span(1);payload=bytearray.fromhex(match[-1][1]);payload[offset:offset+len(raw)]=raw
    return events[:begin]+payload.hex()+events[end:]
root_row=next(row for row in data if row['name']=='type_data')
root=int.from_bytes(original['memory'][root_row['original']],'little')
reject('original-record-not-active-cursor',lambda s:None,'symbol record does not alias active owned input cursor',original_register(2,root+603))
reject('original-record-length-exceeds-capacity',lambda s:None,'symbol record exceeds observed input capacity',original_memory(root,602,b'\xff\xff'))
reject('original-record-too-short',lambda s:None,'symbol record exceeds observed input capacity',original_memory(root,602,b'\x00\x00'))
pe=PE(ROOT/config['target']['path']);driver=graph_driver(config['functions'],config,pe)
tail=next(row for row in driver if row['name']=='@insert_global_symbol@8')
assert tail['end']==0x40d5f2 and tail['accepted'] is False and tail['evidence'] is None and tail['pending_extent'] is False
proof_path=BINARY/next(row for row in config['functions'] if row['name']=='insert_global_symbol')['abi_tail_extent']['evidence']
for name,mutate in [('wrong-target',lambda p:p.update(target_sha256='0'*64)),('wrong-code',lambda p:p.update(code_sha256='0'*64)),('missing-return',lambda p:p['returns'].pop()),('wrong-return-cleanup',lambda p:p['returns'][0].update(cleanup=4)),('undeclared-cold-chunk',lambda p:p.update(code_chunks=[[0x40d600,0x40d601]])),('truncated-primary',lambda p:p['primary'].__setitem__(1,0x40d431))]:
    with tempfile.TemporaryDirectory(prefix='cvpack-tail-proof-',dir=Path.home()/'.ai-tmp') as temporary:
        changed=copy.deepcopy(config);proof=json.loads(proof_path.read_text());mutate(proof)
        path=Path(temporary)/'proof.json';path.write_text(json.dumps(proof))
        row=next(row for row in changed['functions'] if row['name']=='insert_global_symbol')
        row['abi_tail_extent']=dict(evidence=str(path),sha256=sha(path))
        try:graph_driver(changed['functions'],changed,pe)
        except AssertionError as error:rejected.append(dict(name='tail-'+name,diagnostic=str(error)))
        else:raise AssertionError('accepted '+name)
actual=[]
for field in ['pointee-width','omitted-formal']:
    with tempfile.TemporaryDirectory(prefix='cvpack-symbol-abi-',dir=Path.home()/'.ai-tmp') as temporary:
        directory=Path(temporary);changed=copy.deepcopy(config)
        row=next(r for r in changed['functions'] if r['name']=='insert_global_symbol')
        if field=='pointee-width':row['parameters'][0]['native_pointee_width']=4;reason='actual pointer formal pointee extent differs'
        else:row['parameters'].pop();reason='incomplete frontier ABI contract'
        bad=directory/'configuration.json';bad.write_text(json.dumps(changed));output=directory/'native'
        result=subprocess.run([sys.executable,str(BINARY/'scripts/native-debug.py'),'--original',str(pair/'original'),'--config',str(bad),'--content-store',str(BINARY/'build/source-content'),'--output',str(output)],cwd=BINARY,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=15)
        diagnostic='\n'.join((output/file).read_text() for file in ('gdb-stdout','gdb-stderr'))
        assert result.returncode and reason in diagnostic,diagnostic
        actual.append(dict(name=field,diagnostic=diagnostic,bad_config_sha256=sha(bad),observer_sha256=sha(output/'gdb-state.py')))
report=dict(schema=1,native_only=True,count=len(rejected)+len(actual),rejected=rejected,actual_gdb_rejections=actual,parameters=baseline['parameters'],extent_only=tail,original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),checker_sha256=sha(Path(__file__)),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),audit_wrapper_sha256=sha(BINARY/'scripts/audit.py'),seconds=time.perf_counter()-start)
(pair/'current-symbol-state-checks.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(dict(rejected=report['count'],seconds=report['seconds'])))
